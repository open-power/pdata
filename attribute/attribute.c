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

	dfile = dtm_file_open(dtb);
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

static int do_dump_print_node(struct dtm_node *node, void *priv)
{
	struct dtm_node *match_node = (struct dtm_node *)priv;
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
	uint8_t *ptr = attr->value;
	int i;

	for (i=0; i<attr->size; i++) {
		switch (attr->type) {
		case ATTR_TYPE_UINT8:
			printf(" 0x%02x", *ptr);
			break;

		case ATTR_TYPE_UINT16:
			printf(" 0x%04x", *(uint16_t *)ptr);
			break;

		case ATTR_TYPE_UINT32:
			printf(" 0x%08x", *(uint32_t *)ptr);
			break;

		case ATTR_TYPE_UINT64:
			printf(" 0x%" PRIx64, *(uint64_t *)ptr);
			break;

		case ATTR_TYPE_INT8:
			printf(" %d", *(int8_t *)ptr);
			break;

		case ATTR_TYPE_INT16:
			printf(" %d", *(int16_t *)ptr);
			break;

		case ATTR_TYPE_INT32:
			printf(" %d", *(int32_t *)ptr);
			break;

		case ATTR_TYPE_INT64:
			printf(" %"PRId64, *(int64_t *)ptr);
			break;

		default:
			break;
		}

		ptr += attr->data_size;
	}
}

static int do_dump_print_prop(struct dtm_node *node, struct dtm_property *prop, void *priv)
{
	const char *name, *value;
	int value_len;
	struct attr attr = {
		.type = ATTR_TYPE_UNKNOWN,
	};

	name = dtm_prop_name(prop);
	if (strncmp(name, "ATTR", 4) != 0)
		return 0;

	value = dtm_prop_value(prop, &value_len);
	attr_decode(&attr, (uint8_t *)value, value_len);

	if (!attr.defined && !priv)
		goto skip;

	printf("  %s: %s", name, attr_type_to_string(attr.type));
	if (!attr.defined) {
		printf(" UNDEFINED");
	} else {
		if (attr.size <= 4) {
			do_dump_value(&attr);
		} else {
			printf(" [%d]", attr.size);
		}
	}
	printf("\n");

skip:
	if (attr.dim)
		free(attr.dim);

	if (attr.value)
		free(attr.value);

	return 0;
}

static int do_dump(const char *dtb, const char *target)
{
	struct dtm_file *dfile;
	struct dtm_node *root, *node = NULL;
	int ret;

	dfile = dtm_file_open(dtb);
	if (!dfile)
		return 1;

	root = dtm_file_read(dfile);
	dtm_file_close(dfile);
	if (!root)
		return 1;

	if (target) {
		if (target[0] == '/') {
			node = dtm_find_node_by_path(root, target);
		} else {
			node = from_cronus_target(root, target);
		}
		if (!node) {
			fprintf(stderr, "Failed to translate target %s\n", target);
			return 2;
		}

		ret = dtm_traverse(node, true, do_dump_print_node, do_dump_print_prop, node);
	} else {
		ret = dtm_traverse(root, true, do_dump_print_node, do_dump_print_prop, NULL);
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

void do_export_data_type(struct attr *attr)
{
	switch (attr->type) {
	case ATTR_TYPE_UINT8:
		printf("u8");
		break;

	case ATTR_TYPE_UINT16:
		printf("u16");
		break;

	case ATTR_TYPE_UINT32:
		printf("u32");
		break;

	case ATTR_TYPE_UINT64:
		printf("u64");
		break;

	case ATTR_TYPE_INT8:
		printf("s8");
		break;

	case ATTR_TYPE_INT16:
		printf("s16");
		break;

	case ATTR_TYPE_INT32:
		printf("s32");
		break;

	case ATTR_TYPE_INT64:
		printf("s64");
		break;

	default:
		printf("UNKNWON");
		break;
	}

	if (attr->enum_count > 0) {
		printf("e");
	}
}

int do_export_value_string(struct attr *attr, uint8_t *value)
{
	uint64_t val = 0;
	int used = 0;

	if (attr->type == ATTR_TYPE_UINT8 || attr->type == ATTR_TYPE_INT8) {
		val = *(uint8_t *)value;
		used = 1;
	} else if (attr->type == ATTR_TYPE_UINT16 || attr->type == ATTR_TYPE_INT16) {
		val = *(uint16_t *)value;
		used = 2;
	} else if (attr->type == ATTR_TYPE_UINT32 || attr->type == ATTR_TYPE_INT32) {
		val = *(uint32_t *)value;
		used = 4;
	} else if (attr->type == ATTR_TYPE_UINT64 || attr->type == ATTR_TYPE_INT64) {
		val = *(uint64_t *)value;
		used = 8;
	}

	if (attr->enum_count > 0) {
		int i;

		for (i=0; i<attr->enum_count; i++) {
			if (attr->aenum[i].value == val) {
				printf("%s", attr->aenum[i].key);
				return used;
			}
		}
		printf("UNKNOWN_ENUM");
		return used;
	}

	if (used == 1) {
		printf("0x%02x", (uint8_t)val);
	} else if (used == 2) {
		printf("0x%04x", (uint16_t)val);
	} else if (used == 4) {
		printf("0x%08x", (uint32_t)val);
	} else if (used == 8) {
		printf("0x%016llx", (unsigned long long)val);
	} else {
		printf("ERROR%d", used);
	}

	return used;
}

static void do_export_prop_scalar(struct attr *attr, struct attr *value)
{
	printf("%s", attr->name);
	printf("    ");
	do_export_data_type(attr);
	printf("    ");
	do_export_value_string(attr, value->value);
	printf("\n");
}

static void do_export_prop_array1(struct attr *attr, struct attr *value)
{
	int offset = 0, n;
	int i;

	for (i=0; i<attr->dim[0]; i++) {
		printf("%s", attr->name);
		printf("[%d]", i);
		printf(" ");
		do_export_data_type(attr);
		printf("[%d]", attr->dim[0]);
		printf(" ");
		n = do_export_value_string(attr, value->value + offset);
		offset += n;
		printf("\n");
	}
}

static void do_export_prop_array2(struct attr *attr, struct attr *value)
{
	int offset = 0, n;
	int i, j;

	for (i=0; i<attr->dim[0]; i++) {
		for (j=0; j<attr->dim[1]; j++) {
			printf("%s", attr->name);
			printf("[%d][%d]", i, j);
			printf(" ");
			do_export_data_type(attr);
			printf("[%d][%d]", attr->dim[0], attr->dim[1]);
			printf(" ");
			n = do_export_value_string(attr, value->value + offset);
			offset += n;
			printf("\n");
		}
	}
}

static void do_export_prop_array3(struct attr *attr, struct attr *value)
{
	int offset = 0, n;
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
				n = do_export_value_string(attr, value->value + offset);
				offset += n;
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

	buf = dtm_prop_value(prop, &buflen);
	attr_decode(&value, buf, buflen);

	if (!value.defined)
		goto skip;

	attr = attr_db_attr(state->ainfo, name);
	if (!attr)
		goto skip;

	assert(attr->type == value.type);
	assert(attr->data_size == value.data_size);
	assert(attr->dim_count == value.dim_count);
	assert(attr->size == value.size);

	switch (attr->dim_count) {
	case 0:
		do_export_prop_scalar(attr, &value);
		break;

	case 1:
		do_export_prop_array1(attr, &value);
		break;

	case 2:
		do_export_prop_array2(attr, &value);
		break;

	case 3:
		do_export_prop_array3(attr, &value);
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

	dfile = dtm_file_open(dtb);
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
	struct attr attr;
	char *attr_name, *data_type, *value_str;
	char *tok, *saveptr = NULL;
	const uint8_t *cbuf;
	uint8_t *buf;
	int buflen;
	unsigned long long value;
	int idx[3] = { 0, 0, 0 };
	int dim[3] = { -1, -1, -1 };
	int i, index;
	bool is_enum = false;

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
		is_enum = true;
	}

	for (i=0; i<3; i++) {
		tok = strtok(NULL, "[");
		if (!tok)
			break;

		assert(tok[strlen(tok)-1] == ']');
		tok[strlen(tok)-1] = '\0';
		dim[i] = atoi(tok);
	}

	/* attribute value */
	value_str = strtok_r(NULL, " ", &saveptr);
	if (!value_str)
		return false;

	prop = dtm_node_get_property(state->node, attr_name);
	if (!prop) {
		fprintf(stderr, "  %s: attribute not defined\n", attr_name);
		return false;
	}

	cbuf = dtm_prop_value(prop, &buflen);
	if (!buf) {
		fprintf(stderr, "  %s: failed to read value\n", attr_name);
		return false;
	}

	attr_decode(&attr, cbuf, buflen);

	for (i=0; i<attr.dim_count; i++) {
		if (dim[i] != attr.dim[i]) {
			int j;

			fprintf(stderr, "  %s: dim mismatch ", attr_name);
			for (j=0; j<attr.dim_count; j++)
				fprintf(stderr, "[%d]", dim[j]);
			fprintf(stderr, " != ");
			for (j=0; j<attr.dim_count; j++)
				fprintf(stderr, "[%d]", attr.dim[j]);
			fprintf(stderr, "\n");
			assert(dim[i] == attr.dim[i]);
		}
	}
	for (i=0; i<attr.dim_count; i++) {
		if (idx[i] >= attr.dim[i]) {
			int j;

			fprintf(stderr, "  %s: index overflow ", attr_name);
			for (j=0; j<attr.dim_count; j++)
				fprintf(stderr, "[%d]", idx[j]);
			fprintf(stderr, " > ");
			for (j=0; j<attr.dim_count; j++)
				fprintf(stderr, "[%d]", attr.dim[j]);
			fprintf(stderr, "\n");
			assert(idx[i] < attr.dim[i]);
		}
	}

	index = 0;
	if (attr.dim_count == 1) {
		index = idx[0];
	} else if (attr.dim_count == 2) {
		index = idx[0] * attr.dim[0] + idx[1];
	} else if (attr.dim_count == 3) {
		index = idx[0] * attr.dim[0] + idx[1] * attr.dim[1] + idx[2];
	}

	if (is_enum) {
		struct attr *attr_def;
		bool found = false;

		attr_def = attr_db_attr(state->ainfo, attr_name);
		assert(attr_def);

		for (i=0; i<attr_def->enum_count; i++) {
			if (!strcmp(attr_def->aenum[i].key, value_str)) {
				value = attr_def->aenum[i].value;
				found = true;
				break;
			}
		}
		assert(found);
	} else {
		value = strtoull(value_str, NULL, 0);
	}

	if (attr.data_size == 1) {
		uint8_t *ptr = (uint8_t *)attr.value;
		uint8_t val = value;
		ptr[index] =  val;

	} else if (attr.data_size == 2) {
		uint16_t *ptr = (uint16_t *)attr.value;
		uint16_t val = value;
		ptr[index] = htobe16(val);

	} else if (attr.data_size == 4) {
		uint32_t *ptr = (uint32_t *)attr.value;
		uint32_t val = value;
		ptr[index] = htobe32(val);

	} else if (attr.data_size == 8) {
		uint64_t *ptr = (uint64_t *)attr.value;
		uint64_t val = value;
		ptr[index] = htobe64(val);
	}

	attr_encode(&attr, &buf, &buflen);
	dtm_prop_set_value(prop, buf, buflen);
	free(buf);

	state->count++;
	if (state->count < attr.size)
		return true;

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
	char old[strlen(dtb)+12];
	int ret;

	dfile = dtm_file_open(dtb);
	if (!dfile)
		return 1;

	root = dtm_file_read(dfile);
	dtm_file_close(dfile);
	if (!root)
		return 1;

	if (!attr_db_load(infodb, &ainfo))
		return 2;

	state = (struct do_import_state) {
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

	sprintf(old, "%s.old.import", dtb);
	ret = rename(dtb, old);
	if (ret != 0)
		fprintf(stderr, "Failed to rename %s to %s\n", dtb, old);

	dfile = dtm_file_create(dtb);
	if (!dfile)
		return 4;

	if (!dtm_file_write(dfile, root))
		return 5;

	dtm_file_close(dfile);

	return 0;
}

static int do_translate(const char *dtb, const char *target)
{
	struct dtm_file *dfile;
	struct dtm_node *root, *node;
	char *path;

	dfile = dtm_file_open(dtb);
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
		}

		path = dtm_node_path(node);
		assert(path);
	}

	printf("%s ---> %s\n", target, path);
	free(path);

	return 0;
}

static void usage(const char *prog)
{
	fprintf(stderr, "Usage: %s create <dtb> <infodb> <out-dtb>\n", prog);
	fprintf(stderr, "       %s dump <dtb> [<target>]\n", prog);
	fprintf(stderr, "       %s export <dtb> <infodb>\n", prog);
	fprintf(stderr, "       %s import <dtb> <infodb> <attr-dump>\n", prog);
	fprintf(stderr, "       %s translate <dtb> <target>\n", prog);
	exit(1);
}

int main(int argc, const char **argv)
{
	int ret;

	if (argc < 2)
		usage(argv[0]);

	if (strcmp(argv[1], "create") == 0) {
		if (argc != 5)
			usage(argv[0]);

		ret = do_create(argv[2], argv[3], argv[4]);

	} else if (strcmp(argv[1], "dump") == 0) {
		if (argc != 3 && argc != 4)
			usage(argv[0]);

		if (argc == 3)
			ret = do_dump(argv[2], NULL);
		else
			ret = do_dump(argv[2], argv[3]);

	} else if (strcmp(argv[1], "export") == 0) {
		if (argc != 4)
			usage(argv[0]);

		ret = do_export(argv[2], argv[3]);

	} else if (strcmp(argv[1], "import") == 0) {
		if (argc != 5)
			usage(argv[0]);

		ret = do_import(argv[2], argv[3], argv[4]);

	} else if (strcmp(argv[1], "translate") == 0) {
		if (argc != 4)
			usage(argv[0]);

		ret = do_translate(argv[2], argv[3]);

	} else {
		usage(argv[0]);
	}

	return ret;
}
