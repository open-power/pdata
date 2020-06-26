/* Copyright 2019 IBM Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * 	http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h>

#include "libdtm/dtm.h"

#include "attribute.h"
#include "attribute_db.h"
#include "attribute_format.h"
#include "attribute_target.h"
#include "attribute_util.h"

static char *node_name_to_class(const char *name)
{
	char *tmp, *tok;
	size_t i, n;

	if (name[0] == '\0')
		return strdup("root");

	tmp = strdup(name);
	assert(tmp);

	tok = strtok(tmp, "@");
	assert(tok);

	n = strlen(tok);
	for (i = n-1; i >= 0; i--) {
		if (isdigit(tok[i]))
			tok[i] = '\0';
		else
			break;
	}

	return tok;
}

static int do_create_add_prop(struct dtm_node *node, void *priv)
{
	struct attr_info *ainfo = (struct attr_info *)priv;
	struct target *target = NULL;
	const char *name, *fapi_target;
	char *cname;
	int ret, i;

	name = dtm_node_name(node);
	cname = node_name_to_class(name);
	fapi_target = dtree_to_fapi_class(cname);
	free(cname);

	if (!fapi_target)
		return 0;

	for (i=0; i<ainfo->tlist.count; i++) {
		if (strcmp(fapi_target, ainfo->tlist.target[i].name) == 0) {
			target = &ainfo->tlist.target[i];
			break;
		}
	}

	if (!target)
		return 0;

	for (i=0; i<target->id_count; i++) {
		struct attr *attr;
		uint8_t *buf = NULL;
		int buflen;
		int id = target->id[i];

		assert(id < ainfo->alist.count);
		attr = &ainfo->alist.attr[id];

		attr_encode(attr, &buf, &buflen);
		assert(buf && buflen > 0);

		ret = dtm_node_add_property(node, attr->name, buf, buflen);
		if (ret != 0)
			return ret;

		free(buf);
	}

	return 0;
}

static int do_create(const char *dtb, const char *infodb, const char *out_dtb)
{
	struct dtm_file *dfile;
	struct dtm_node *root;
	struct attr_info ainfo;
	int ret;

	dfile = dtm_file_open(dtb, false);
	if (!dfile)
		return 1;

	root = dtm_file_read(dfile);
	dtm_file_close(dfile);
	if (!root)
		return 1;

	if (!attr_db_load(infodb, &ainfo))
		return 2;

	ret = dtm_traverse(root, true, do_create_add_prop, NULL, &ainfo);
	if (ret != 0)
		return 3;

	dfile = dtm_file_create(out_dtb);
	if (!dfile)
		return 4;

	if (!dtm_file_write(dfile, root))
		return 5;

	dtm_file_close(dfile);

	return 0;
}

struct do_dump_state {
	struct attr_info *ainfo;
	struct dtm_node *node;
};

static int do_dump_print_node(struct dtm_node *node, void *priv)
{
	struct do_dump_state *state = (struct do_dump_state *)priv;
	struct dtm_node *match_node = state->node;
	char *path;

	/* Single node */
	if (match_node && match_node != node)
		return 99;

	path = dtm_node_path(node);
	assert(path);
	printf("%s\n", path);
	free(path);

	return 0;
}

static void do_dump_value(struct attr *attr)
{
	if (attr->type == ATTR_TYPE_COMPLEX) {
		attr_print_complex_value(attr, NULL);
	} else if (attr->type == ATTR_TYPE_STRING) {
		attr_print_string_value(attr, NULL);
	} else {
		if (!attr_print_enum_value(attr, NULL))
			attr_print_value(attr, NULL);
	}
}

static int do_dump_print_prop(struct dtm_node *node, struct dtm_property *prop, void *priv)
{
	struct do_dump_state *state = (struct do_dump_state *)priv;
	const char *name;
	const void *buf;
	int buflen;
	struct attr *attr, value;

	value = (struct attr) {
		.type = ATTR_TYPE_UNKNOWN,
	};

	name = dtm_prop_name(prop);
	if (strncmp(name, "ATTR", 4) != 0)
		return 0;

	attr = attr_db_attr(state->ainfo, name);
	if (!attr)
		goto skip;

	attr_copy(attr, &value);

	buf = dtm_prop_value(prop, &buflen);
	attr_decode(&value, (uint8_t *)buf, buflen);

	printf("  %s: %s", name, attr_type_to_string(value.type));
	if (value.size <= 4) {
		do_dump_value(&value);
	} else {
		printf(" [%d]", value.size);
	}
	printf("\n");

skip:
	if (value.dim)
		free(value.dim);

	if (value.value)
		free(value.value);

	return 0;
}

static int do_dump(const char *dtb, const char *infodb, const char *target)
{
	struct dtm_file *dfile;
	struct dtm_node *root, *node = NULL;
	struct attr_info ainfo;
	struct do_dump_state state;
	int ret;

	dfile = dtm_file_open(dtb, false);
	if (!dfile)
		return 1;

	root = dtm_file_read(dfile);
	dtm_file_close(dfile);
	if (!root)
		return 1;

	if (!attr_db_load(infodb, &ainfo))
		return 2;

	state = (struct do_dump_state) {
		.ainfo = &ainfo,
	};

	if (target) {
		if (target[0] == '/') {
			node = dtm_find_node_by_path(root, target);
		} else {
			node = from_cronus_target(root, target);
		}
		if (!node) {
			fprintf(stderr, "Failed to translate target %s\n", target);
			return 3;
		}

		state.node = node;

		ret = dtm_traverse(node, true, do_dump_print_node, do_dump_print_prop, &state);
	} else {
		ret = dtm_traverse(root, true, do_dump_print_node, do_dump_print_prop, &state);
	}

	if (ret == 99)
		ret = 0;

	return ret;
}

struct do_export_state {
	struct attr_info *ainfo;
	struct dtm_node *root;
	struct dtm_node *node;
};

static int do_export_node(struct dtm_node *node, void *priv)
{
	struct do_export_state *state = (struct do_export_state *)priv;
	char *name;

	name = to_cronus_target(state->root, node);
	if (name) {
		state->node = node;
		printf("target = %s\n", name);
		free(name);
	} else {
		state->node = NULL;
	}

	return 0;
}

static void do_export_data_type(struct attr *attr)
{
	printf("%s", attr_type_to_short_string(attr->type));
	if (attr->enum_count > 0) {
		printf("e");
	}
}

static void do_export_value_string(struct attr *attr, uint8_t *value)
{
	if (attr->type == ATTR_TYPE_COMPLEX) {
		attr_print_complex_value(attr, value);
	} else if (attr->type == ATTR_TYPE_STRING) {
		attr_print_string_value(attr, value);
	} else {
		if (!attr_print_enum_value(attr, value))
			attr_print_value(attr, value);
	}
}

static void do_export_prop_scalar(struct attr *attr)
{
	printf("%s", attr->name);
	printf("    ");
	do_export_data_type(attr);
	printf("    ");
	do_export_value_string(attr, attr->value);
	printf("\n");
}

static void do_export_prop_array1(struct attr *attr)
{
	uint8_t *ptr = attr->value;
	int i;

	for (i=0; i<attr->dim[0]; i++) {
		printf("%s", attr->name);
		printf("[%d]", i);
		printf(" ");
		do_export_data_type(attr);
		printf("[%d]", attr->dim[0]);
		printf(" ");
		do_export_value_string(attr, ptr);
		ptr += attr->data_size;
		printf("\n");
	}
}

static void do_export_prop_array2(struct attr *attr)
{
	uint8_t *ptr = attr->value;
	int i, j;

	for (i=0; i<attr->dim[0]; i++) {
		for (j=0; j<attr->dim[1]; j++) {
			printf("%s", attr->name);
			printf("[%d][%d]", i, j);
			printf(" ");
			do_export_data_type(attr);
			printf("[%d][%d]", attr->dim[0], attr->dim[1]);
			printf(" ");
			do_export_value_string(attr, ptr);
			ptr += attr->data_size;
			printf("\n");
		}
	}
}

static void do_export_prop_array3(struct attr *attr)
{
	uint8_t *ptr = attr->value;
	int i, j, k;

	for (i=0; i<attr->dim[0]; i++) {
		for (j=0; j<attr->dim[1]; j++) {
			for (k=0; k<attr->dim[2]; k++) {
				printf("%s", attr->name);
				printf("[%d][%d][%d]", i, j, k);
				printf(" ");
				do_export_data_type(attr);
				printf("[%d][%d][%d]", attr->dim[0], attr->dim[1], attr->dim[2]);
				printf(" ");
				do_export_value_string(attr, ptr);
				ptr += attr->data_size;
				printf("\n");
			}
		}
	}
}

static int do_export_prop(struct dtm_node *node, struct dtm_property *prop, void *priv)
{
	struct do_export_state *state = (struct do_export_state *)priv;
	struct attr *attr, value;
	const char *name;
	const uint8_t *buf;
	int buflen;

	if (!state->node)
		return 0;

	name = dtm_prop_name(prop);
	if (strncmp(name, "ATTR", 4) != 0)
		return 0;

	attr = attr_db_attr(state->ainfo, name);
	if (!attr)
		goto skip;

	attr_copy(attr, &value);

	buf = dtm_prop_value(prop, &buflen);
	attr_decode(&value, buf, buflen);

	switch (value.dim_count) {
	case 0:
		do_export_prop_scalar(&value);
		break;

	case 1:
		do_export_prop_array1(&value);
		break;

	case 2:
		do_export_prop_array2(&value);
		break;

	case 3:
		do_export_prop_array3(&value);
		break;

	default:
		fprintf(stderr, "Unsupported array size\n");
		assert(0);
	}

skip:
	if (value.dim)
		free(value.dim);

	if (value.value)
		free(value.value);

	return 0;
}

static int do_export(const char *dtb, const char *infodb)
{
	struct do_export_state state;
	struct dtm_file *dfile;
	struct dtm_node *root;
	struct attr_info ainfo;
	int ret;

	dfile = dtm_file_open(dtb, false);
	if (!dfile)
		return 1;

	root = dtm_file_read(dfile);
	dtm_file_close(dfile);
	if (!root)
		return 1;

	if (!attr_db_load(infodb, &ainfo))
		return 2;

	state = (struct do_export_state) {
		.ainfo = &ainfo,
		.root = root,
	};

	ret = dtm_traverse(root, true, do_export_node, do_export_prop, &state);
	if (ret != 0)
		return 3;

	return 0;
}

struct do_import_state {
	struct dtm_file *dfile;
	struct attr_info *ainfo;
	struct dtm_node *root;

	struct dtm_node *node;
	int count;
};

static bool do_import_parse_target(struct do_import_state *state, char *line)
{
	struct dtm_node *node;
	char *tok, *ptr, *path;

	state->node = NULL;

	tok = strtok(line, "=");
	if (!tok)
		return false;

	tok = strtok(NULL, "=");
	if (!tok)
		return false;

	ptr = tok;
	while (*ptr == ' ')
		ptr++;

	node = from_cronus_target(state->root, ptr);
	if (!node) {
		fprintf(stderr, "%s --> Failed to translate\n", ptr);
		return false;
	}

	state->node = node;

	path = dtm_node_path(node);
	assert(path);

	fprintf(stderr, "%s --> %s\n", ptr, path);
	free(path);

	return true;
}

static bool do_import_parse_attr(struct do_import_state *state, char *line)
{
	struct dtm_property *prop;
	struct attr *attr, value;
	char *attr_name, *data_type;
	char *tok, *saveptr = NULL;
	const uint8_t *cbuf;
	uint8_t *buf, *ptr;
	int buflen;
	int idx[3] = { 0, 0, 0 };
	int dim[3] = { -1, -1, -1 };
	int i, index;

	if (!state->node)
		return true;

	/* attribute name */
	tok = strtok_r(line, " ", &saveptr);
	if (!tok)
		return false;

	attr_name = strtok(tok, "[");
	if (!attr_name)
		return false;

	if (strlen(attr_name) < 4 || strncmp(attr_name, "ATTR", 4))
		return true;

	for (i=0; i<3; i++) {
		tok = strtok(NULL, "[");
		if (!tok)
			break;

		assert(tok[strlen(tok)-1] == ']');
		tok[strlen(tok)-1] = '\0';
		idx[i] = atoi(tok);
	}

	/* attribute data type */
	tok = strtok_r(NULL, " ", &saveptr);
	if (!tok)
		return false;

	data_type = strtok(tok, "[");
	if (!data_type)
		return false;

	if (data_type[strlen(data_type)-1] == 'e') {
		data_type[strlen(data_type)-1] = '\0';
	}

	for (i=0; i<3; i++) {
		tok = strtok(NULL, "[");
		if (!tok)
			break;

		assert(tok[strlen(tok)-1] == ']');
		tok[strlen(tok)-1] = '\0';
		dim[i] = atoi(tok);
	}

	prop = dtm_node_get_property(state->node, attr_name);
	if (!prop) {
		fprintf(stderr, "  %s: No such attribute\n", attr_name);
		return false;
	}

	cbuf = dtm_prop_value(prop, &buflen);
	if (!cbuf) {
		fprintf(stderr, "  %s: failed to read value\n", attr_name);
		return false;
	}

	attr = attr_db_attr(state->ainfo, attr_name);
	if (!attr) {
		fprintf(stderr, "  %s: attribute unknown\n", attr_name);
		return false;
	}

	if (attr_type_from_short_string(data_type) != attr->type) {
		fprintf(stderr, "  %s: type mismatch\n", attr_name);
		return false;
	}

	for (i=0; i<attr->dim_count; i++) {
		if (dim[i] != attr->dim[i]) {
			int j;

			fprintf(stderr, "  %s: dim mismatch ", attr_name);
			for (j=0; j<attr->dim_count; j++)
				fprintf(stderr, "[%d]", dim[j]);
			fprintf(stderr, " != ");
			for (j=0; j<attr->dim_count; j++)
				fprintf(stderr, "[%d]", attr->dim[j]);
			fprintf(stderr, "\n");
			assert(dim[i] == attr->dim[i]);
		}
	}
	for (i=0; i<attr->dim_count; i++) {
		if (idx[i] >= attr->dim[i]) {
			int j;

			fprintf(stderr, "  %s: index overflow ", attr_name);
			for (j=0; j<attr->dim_count; j++)
				fprintf(stderr, "[%d]", idx[j]);
			fprintf(stderr, " > ");
			for (j=0; j<attr->dim_count; j++)
				fprintf(stderr, "[%d]", attr->dim[j]);
			fprintf(stderr, "\n");
			assert(idx[i] < attr->dim[i]);
		}
	}

	index = 0;
	if (attr->dim_count == 1) {
		index = idx[0];
	} else if (attr->dim_count == 2) {
		index = idx[0] * attr->dim[1] + idx[1];
	} else if (attr->dim_count == 3) {
		index = idx[0] * attr->dim[1] * attr->dim[2] + idx[1] * attr->dim[2] + idx[2];
	}

	attr_copy(attr, &value);
	attr_decode(&value, cbuf, buflen);

	ptr = value.value + index * value.data_size;

	/* attribute value */
	if (attr->type == ATTR_TYPE_COMPLEX) {
		uint64_t val;
		int data_size;

		for (i=0; i<strlen(attr->spec); i++) {
			tok = strtok_r(NULL, " ", &saveptr);
			if (!tok)
				return false;

			val = strtoull(tok, NULL, 0);
			data_size = attr->spec[i] - '0';
			attr_set_value_num(ptr, data_size, val);
			ptr += data_size;
		}
	} else if (attr->type == ATTR_TYPE_STRING) {
		size_t n;

		tok = strtok_r(NULL, " ", &saveptr);
		if (!tok)
			return false;

		if (tok[0] != '"')
			return false;

		n = strlen(tok);
		if (tok[n-1] != '"')
			return false;

		tok[n-1] = '\0';

		attr_set_string_value(attr, ptr, &tok[1]);
	} else {
		tok = strtok_r(NULL, " ", &saveptr);
		if (!tok)
			return false;

		if (!attr_set_enum_value(attr, ptr, tok))
			attr_set_value(attr, ptr, tok);
	}

	attr_encode(&value, &buf, &buflen);
	dtm_prop_set_value(prop, buf, buflen);
	free(buf);

	state->count++;
	if (state->count < value.size)
		return true;

	if (!dtm_file_update_node(state->dfile, state->node, attr_name)) {
		fprintf(stderr, "  %s: import failed\n", attr_name);
		return false;
	}

	fprintf(stderr, "  %s: imported\n", attr_name);

	state->count = 0;

	return true;
}

static bool do_import_parse(struct do_import_state *state, char *line)
{
	char str[128];

	/* Skip empty lines */
	if (strlen(line) < 6)
		return true;

	strcpy(str, line);

	if (!strncmp(line, "target", 6)) {
		if (!do_import_parse_target(state, line))
			return true;
	} else {
		if (!do_import_parse_attr(state, line))
			return true;
	}

	return true;
}

static int do_import(const char *dtb, const char *infodb, const char *dump_file)
{
	struct do_import_state state;
	struct dtm_file *dfile;
	struct dtm_node *root;
	struct attr_info ainfo;
	FILE *fp;
	char line[1024], *ptr;

	dfile = dtm_file_open(dtb, true);
	if (!dfile)
		return 1;

	root = dtm_file_read(dfile);
	if (!root)
		return 1;

	if (!attr_db_load(infodb, &ainfo))
		return 2;

	state = (struct do_import_state) {
		.dfile = dfile,
		.ainfo = &ainfo,
		.root = root,
	};

	fp = fopen(dump_file, "r");
	if (!fp) {
		fprintf(stderr, "Failed to open cronus dump %s\n", dump_file);
		return 3;
	}

	while ((ptr = fgets(line, sizeof(line), fp))) {
		assert(ptr[strlen(ptr)-1] == '\n');
		ptr[strlen(ptr)-1] = '\0';

		if (!do_import_parse(&state, line))
			break;
	}
	fclose(fp);

	dtm_file_close(dfile);
	return 0;
}

static int do_read(const char *dtb, const char *infodb, const char *target, const char *name)
{
	struct dtm_file *dfile;
	struct dtm_node *root, *node;
	struct dtm_property *prop;
	const void *val;
	struct attr_info ainfo;
	struct attr *attr, value;
	int len, i;

	dfile = dtm_file_open(dtb, false);
	if (!dfile)
		return 1;

	root = dtm_file_read(dfile);
	dtm_file_close(dfile);
	if (!root)
		return 1;

	if (!attr_db_load(infodb, &ainfo))
		return 2;

	if (target[0] == '/') {
		node = dtm_find_node_by_path(root, target);
		if (!node) {
			fprintf(stderr, "No such target %s\n", target);
			return 3;
		}
	} else {
		node = from_cronus_target(root, target);
		if (!node) {
			fprintf(stderr, "Failed to translate %s\n", target);
			return 3;
		}
	}

	attr = attr_db_attr(&ainfo, name);
	if (!attr) {
		fprintf(stderr, "Attribute %s not defined\n", name);
		return 4;
	}

	prop = dtm_node_get_property(node, name);
	if (!prop) {
		fprintf(stderr, "No such attribute %s\n", name);
		return 4;
	}

	attr_copy(attr, &value);

	val = dtm_prop_value(prop, &len);
	attr_decode(&value, (const uint8_t *)val, len);

	printf("%s", name);
	if (value.dim_count > 0) {
		printf("<");
		printf("%d", value.dim[0]);
		for (i=1; i<value.dim_count; i++)
			printf(",%d", value.dim[i]);
		printf(">");
	}

	printf(" = ");

	if (attr->type == ATTR_TYPE_COMPLEX) {
		attr_print_complex_value(&value, NULL);
	} else if (attr->type == ATTR_TYPE_STRING) {
		attr_print_string_value(&value, NULL);
	} else {
		if (!attr_print_enum_value(&value, NULL))
			attr_print_value(&value, NULL);
	}

	printf("\n");

	return 0;
}

static int do_translate(const char *dtb, const char *target)
{
	struct dtm_file *dfile;
	struct dtm_node *root, *node;
	char *path;

	dfile = dtm_file_open(dtb, false);
	if (!dfile)
		return 1;

	root = dtm_file_read(dfile);
	dtm_file_close(dfile);
	if (!root)
		return 1;

	if (target[0] == '/') {
		node = dtm_find_node_by_path(root, target);
		if (!node) {
			fprintf(stderr, "No such target %s\n", target);
			return 2;
		}

		path = to_cronus_target(root, node);
		if (!path) {
			fprintf(stderr, "Failed to translate %s\n", target);
			return 2;
		}
	} else {
		node = from_cronus_target(root, target);
		if (!node) {
			fprintf(stderr, "Failed to translate %s\n", target);
			return 2;
		}

		path = dtm_node_path(node);
		assert(path);
	}

	printf("%s ---> %s\n", target, path);
	free(path);

	return 0;
}

static int do_write(const char *dtb, const char *infodb, const char *target,
		    const char *name, const char **argv, int argc)
{
	struct dtm_file *dfile;
	struct dtm_node *root, *node;
	struct dtm_property *prop;
	const void *val;
	uint8_t *buf, *ptr;
	struct attr_info ainfo;
	struct attr *attr, value;
	int len, count, i;

	dfile = dtm_file_open(dtb, true);
	if (!dfile)
		return 1;

	root = dtm_file_read(dfile);
	if (!root)
		return 1;

	if (!attr_db_load(infodb, &ainfo))
		return 2;

	if (target[0] == '/') {
		node = dtm_find_node_by_path(root, target);
		if (!node) {
			fprintf(stderr, "No such target %s\n", target);
			return 3;
		}
	} else {
		node = from_cronus_target(root, target);
		if (!node) {
			fprintf(stderr, "Failed to translate %s\n", target);
			return 3;
		}
	}

	attr = attr_db_attr(&ainfo, name);
	if (!attr) {
		fprintf(stderr, "Attribute %s not defined\n", name);
		return 3;
	}

	prop = dtm_node_get_property(node, name);
	if (!prop) {
		fprintf(stderr, "No such attribute %s\n", name);
		return 3;
	}

	attr_copy(attr, &value);

	val = dtm_prop_value(prop, &len);
	attr_decode(&value, (const uint8_t *)val, len);

	count = attr->size;
	if (attr->type == ATTR_TYPE_COMPLEX) {
		count *= strlen(attr->spec);
	}

	if (argc != count) {
		fprintf(stderr, "Insufficient values %d, expected %d\n", argc, count);
		return 4;
	}

	ptr = value.value;
	for (i=0; i<value.size; i++) {
		if (attr->type == ATTR_TYPE_COMPLEX) {
			uint64_t val;
			int data_size, j;

			count = i * strlen(attr->spec);

			for (j=0; j<strlen(attr->spec); j++) {
				val = strtoull(argv[count+j], NULL, 0);
				data_size = attr->spec[j] - '0';
				attr_set_value_num(ptr, data_size, val);
				ptr += data_size;
			}
		} else if (attr->type == ATTR_TYPE_STRING) {
			attr_set_string_value(attr, ptr, argv[i]);
			ptr += attr->data_size;
		} else {
			if (!attr_set_enum_value(attr, ptr, argv[i]))
				attr_set_value(attr, ptr, argv[i]);
			ptr += attr->data_size;
		}
	}

	attr_encode(&value, &buf, &len);
	dtm_prop_set_value(prop, buf, len);
	free(buf);

	if (!dtm_file_update_node(dfile, node, name)) {
		fprintf(stderr, "Failed to update attribute %s\n", name);
		return 5;
	}

	dtm_file_close(dfile);
	return 0;
}

static void bmc_usage(const char *prog)
{
	fprintf(stderr, "Usage: %s export\n", prog);
	fprintf(stderr, "       %s import <attr-dump>\n", prog);
	fprintf(stderr, "       %s read <target> <attribute>\n", prog);
	fprintf(stderr, "       %s write <target> <attribute> <value>\n", prog);
	fprintf(stderr, "       %s translate <target>\n", prog);
	exit(1);
}

static int bmc_main(const char *dtb, const char *infodb, int argc, const char **argv)
{
	int ret = -1;

	if (argc < 2)
		bmc_usage(argv[0]);

	if (strcmp(argv[1], "export") == 0) {
		if (argc != 2)
			bmc_usage(argv[0]);

		ret = do_export(dtb, infodb);

	} else if (strcmp(argv[1], "import") == 0) {
		if (argc != 3)
			bmc_usage(argv[0]);

		ret = do_import(dtb, infodb, argv[2]);

	} else if (strcmp(argv[1], "read") == 0) {
		if (argc != 4)
			bmc_usage(argv[0]);

		ret = do_read(dtb, infodb, argv[2], argv[3]);

	} else if (strcmp(argv[1], "translate") == 0) {
		if (argc != 3)
			bmc_usage(argv[0]);

		ret = do_translate(dtb, argv[2]);

	} else if (strcmp(argv[1], "write") == 0) {
		if (argc < 5)
			bmc_usage(argv[0]);

		ret = do_write(dtb, infodb, argv[2], argv[3], &argv[4], argc-4);

	} else {
		bmc_usage(argv[0]);
	}

	return ret;
}

static void usage(const char *prog)
{
	fprintf(stderr, "Usage: %s create <dtb> <infodb> <out-dtb>\n", prog);
	fprintf(stderr, "       %s dump <dtb> <infodb> [<target>]\n", prog);
	fprintf(stderr, "       %s export <dtb> <infodb>\n", prog);
	fprintf(stderr, "       %s import <dtb> <infodb> <attr-dump>\n", prog);
	fprintf(stderr, "       %s read <dtb> <infodb> <target> <attriute>\n", prog);
	fprintf(stderr, "       %s translate <dtb> <target>\n", prog);
	fprintf(stderr, "       %s write <dtb> <infodb> <target> <attriute> <value>\n", prog);
	exit(1);
}

int main(int argc, const char **argv)
{
	const char *dtb, *infodb;
	int ret = -1;

	dtb = getenv("PDBG_DTB");
	infodb = getenv("PDATA_INFODB");
	if (dtb && infodb) {
		return bmc_main(dtb, infodb, argc, argv);
	}

	if (argc < 2)
		usage(argv[0]);

	if (strcmp(argv[1], "create") == 0) {
		if (argc != 5)
			usage(argv[0]);

		ret = do_create(argv[2], argv[3], argv[4]);

	} else if (strcmp(argv[1], "dump") == 0) {
		if (argc != 4 && argc != 5)
			usage(argv[0]);

		if (argc == 4)
			ret = do_dump(argv[2], argv[3], NULL);
		else
			ret = do_dump(argv[2], argv[3], argv[4]);

	} else if (strcmp(argv[1], "export") == 0) {
		if (argc != 4)
			usage(argv[0]);

		ret = do_export(argv[2], argv[3]);

	} else if (strcmp(argv[1], "import") == 0) {
		if (argc != 5)
			usage(argv[0]);

		ret = do_import(argv[2], argv[3], argv[4]);

	} else if (strcmp(argv[1], "read") == 0) {
		if (argc != 6)
			usage(argv[0]);

		ret = do_read(argv[2], argv[3], argv[4], argv[5]);

	} else if (strcmp(argv[1], "translate") == 0) {
		if (argc != 4)
			usage(argv[0]);

		ret = do_translate(argv[2], argv[3]);

	} else if (strcmp(argv[1], "write") == 0) {
		if (argc < 7)
			usage(argv[0]);

		ret = do_write(argv[2], argv[3], argv[4], argv[5], &argv[6], argc-6);

	} else {
		usage(argv[0]);
	}

	return ret;
}
