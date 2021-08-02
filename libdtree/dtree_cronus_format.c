/* Copyright 2021 IBM Corp.
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
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>

#include "dtree.h"
#include "dtree_attr.h"
#include "dtree_cronus.h"

struct {
	enum dtree_attr_type type;
	char *label;
} cronus_attr_type_map[] = {
	{ DTREE_ATTR_TYPE_UINT8,   "u8"  },
	{ DTREE_ATTR_TYPE_UINT16,  "u16" },
	{ DTREE_ATTR_TYPE_UINT32,  "u32" },
	{ DTREE_ATTR_TYPE_UINT64,  "u64" },
	{ DTREE_ATTR_TYPE_INT8,    "s8"  },
	{ DTREE_ATTR_TYPE_INT16,   "s16" },
	{ DTREE_ATTR_TYPE_INT32,   "s32" },
	{ DTREE_ATTR_TYPE_INT64,   "s64" },
	{ DTREE_ATTR_TYPE_STRING,  "str" },
	{ DTREE_ATTR_TYPE_COMPLEX, "cpx" },
	{ DTREE_ATTR_TYPE_UNKNOWN,  NULL },
};

static enum dtree_attr_type attr_type_from_cronus_string(char *str)
{
	int i;

	for (i=0; cronus_attr_type_map[i].label; i++) {
		if (strcmp(str, cronus_attr_type_map[i].label) == 0)
			return cronus_attr_type_map[i].type;
	}

	return DTREE_ATTR_TYPE_UNKNOWN;
}

static const char *attr_type_to_cronus_string(uint8_t type)
{
	int i;

	for (i=0; cronus_attr_type_map[i].label; i++) {
		if (type == cronus_attr_type_map[i].type)
			return cronus_attr_type_map[i].label;
	}

	return "<NULL>";
}

static void cronus_print_single_num(FILE *fp, uint8_t *ptr, int elem_size)
{
	if (elem_size == 1) {
		uint8_t val = *ptr;
		fprintf(fp, "0x%02x", val);
	} else if (elem_size == 2) {
		uint16_t val = *(uint16_t *)ptr;
		fprintf(fp, "0x%04x", val);
	} else if (elem_size == 4) {
		uint32_t val = *(uint32_t *)ptr;
		fprintf(fp, "0x%08x", val);
	} else if (elem_size == 8) {
		uint64_t val = *(uint64_t *)ptr;
		fprintf(fp, "0x%016" PRIx64, val);
	} else {
		assert(0);
	}
}

static void cronus_print_single(FILE *fp, const struct dtree_attr *attr, uint8_t *ptr)
{
	int count = 1, i;

	switch (attr->type) {
	case DTREE_ATTR_TYPE_UINT8:
	case DTREE_ATTR_TYPE_UINT16:
	case DTREE_ATTR_TYPE_UINT32:
	case DTREE_ATTR_TYPE_UINT64:
	case DTREE_ATTR_TYPE_INT8:
	case DTREE_ATTR_TYPE_INT16:
	case DTREE_ATTR_TYPE_INT32:
	case DTREE_ATTR_TYPE_INT64:
		break;

	default:
		assert(0);
	}

	if (!ptr) {
		ptr = attr->value;
		count = attr->count;
	}

	for (i=0; i<count; i++) {
		cronus_print_single_num(fp, ptr, attr->elem_size);
		ptr += attr->elem_size;

		if (i < count-1)
			fprintf(fp, " ");
	}
}

static bool cronus_print_enum(FILE *fp, const struct dtree_attr *attr, uint8_t *ptr)
{
	uint64_t val = 0;
	int count = 1, i, j;

	if (attr->enum_count == 0)
		return false;

	if (!ptr) {
		ptr = attr->value;
		count = attr->count;
	}

	for (i=0; i<count; i++) {
		bool found = false;

		if (attr->elem_size == 1) {
			val = *ptr;
		} else if (attr->elem_size == 2) {
			val = *(uint16_t *)ptr;
		} else if (attr->elem_size == 4) {
			val = *(uint32_t *)ptr;
		} else if (attr->elem_size == 8) {
			val = *(uint64_t *)ptr;
		} else {
			assert(0);
		}

		for (j=0; j<attr->enum_count; j++) {
			struct dtree_attr_enum *ae = &attr->aenum[j];

			if (ae->value == val) {
				fprintf(fp, "%s", ae->key);
				found = true;
				break;
			}
		}
		if (!found) {
			fprintf(fp, "UNKNOWN_ENUM (");
			cronus_print_single_num(fp, ptr, attr->elem_size);
			fprintf(fp, ")");
		}

		ptr += attr->elem_size;

		if (i < count-1)
			fprintf(fp, " ");
	}

	return true;
}

void cronus_print_string(FILE *fp, const struct dtree_attr *attr, uint8_t *ptr)
{
	int count = 1, i;

	if (!ptr) {
		ptr = attr->value;
		count = attr->count;
	}

	for (i=0; i<count; i++) {
		fprintf(fp, "\"%.*s\"", attr->elem_size, (char *)ptr);
		ptr += attr->elem_size;

		if (i < count-1)
			fprintf(fp, " ");
	}
}

void cronus_print_complex(FILE *fp, const struct dtree_attr *attr, uint8_t *ptr)
{
	int count = 1, i, j;

	if (!ptr) {
		ptr = attr->value;
		count = attr->count;
	}

	for (i=0; i<count; i++) {
		size_t n = strlen(attr->spec);

		for (j=0; j<n; j++) {
			int elem_size = attr->spec[j] - '0';

			cronus_print_single_num(fp, ptr, elem_size);
			ptr += elem_size;

			if (j < n-1)
				fprintf(fp, " ");
		}

		if (i < count-1)
			fprintf(fp, " ");
	}
}

static void cronus_print_node(FILE *fp, const char *node)
{
	fprintf(fp, "target = %s\n", node);
}

static void cronus_print_data_type(FILE *fp, const struct dtree_attr *attr)
{
	fprintf(fp, "%s", attr_type_to_cronus_string(attr->type));
	if (attr->enum_count > 0) {
		fprintf(fp, "e");
	}
}

static void cronus_print_value(FILE *fp, const struct dtree_attr *attr, uint8_t *value)
{
	if (attr->type == DTREE_ATTR_TYPE_COMPLEX) {
		cronus_print_complex(fp, attr, value);
	} else if (attr->type == DTREE_ATTR_TYPE_STRING) {
		cronus_print_string(fp, attr, value);
	} else {
		if (!cronus_print_enum(fp, attr, value))
			cronus_print_single(fp, attr, value);
	}
}

static void cronus_print_scalar(FILE *fp, const struct dtree_attr *attr)
{
	fprintf(fp, "%s", attr->name);
	fprintf(fp, "    ");
	cronus_print_data_type(fp, attr);
	fprintf(fp, "    ");
	cronus_print_value(fp, attr, attr->value);
	fprintf(fp, "\n");
}

static void cronus_print_array1(FILE *fp, const struct dtree_attr *attr)
{
	uint8_t *ptr = attr->value;
	int i;

	for (i=0; i<attr->dim[0]; i++) {
		fprintf(fp, "%s", attr->name);
		fprintf(fp, "[%d]", i);
		fprintf(fp, " ");
		cronus_print_data_type(fp, attr);
		fprintf(fp, "[%d]", attr->dim[0]);
		fprintf(fp, " ");
		cronus_print_value(fp, attr, ptr);
		ptr += attr->elem_size;
		fprintf(fp, "\n");
	}
}

static void cronus_print_array2(FILE *fp, const struct dtree_attr *attr)
{
	uint8_t *ptr = attr->value;
	int i, j;

	for (i=0; i<attr->dim[0]; i++) {
	for (j=0; j<attr->dim[1]; j++) {
		fprintf(fp, "%s", attr->name);
		fprintf(fp, "[%d][%d]", i, j);
		fprintf(fp, " ");
		cronus_print_data_type(fp, attr);
		fprintf(fp, "[%d][%d]", attr->dim[0], attr->dim[1]);
		fprintf(fp, " ");
		cronus_print_value(fp, attr, ptr);
		ptr += attr->elem_size;
		fprintf(fp, "\n");
	}
	}
}

static void cronus_print_array3(FILE *fp, const struct dtree_attr *attr)
{
	uint8_t *ptr = attr->value;
	int i, j, k;

	for (i=0; i<attr->dim[0]; i++) {
	for (j=0; j<attr->dim[1]; j++) {
	for (k=0; k<attr->dim[2]; k++) {
		fprintf(fp, "%s", attr->name);
		fprintf(fp, "[%d][%d][%d]", i, j, k);
		fprintf(fp, " ");
		cronus_print_data_type(fp, attr);
		fprintf(fp, "[%d][%d][%d]", attr->dim[0], attr->dim[1], attr->dim[2]);
		fprintf(fp, " ");
		cronus_print_value(fp, attr, ptr);
		ptr += attr->elem_size;
		fprintf(fp, "\n");
	}
	}
	}
}

void dtree_cronus_print_node(const char *target, FILE *fp)
{
	cronus_print_node(fp, target);
}

void dtree_cronus_print_attr(const struct dtree_attr *attr, FILE *fp)
{
	if (attr->type == DTREE_ATTR_TYPE_UNKNOWN)
		return;

	switch (attr->dim_count) {
	case 0:
		cronus_print_scalar(fp, attr);
		break;

	case 1:
		cronus_print_array1(fp, attr);
		break;

	case 2:
		cronus_print_array2(fp, attr);
		break;

	case 3:
		cronus_print_array3(fp, attr);
		break;
	}
}

static int cronus_parse_target(void *ctx, char *line)
{
	struct dtm_node *node, *root;
	char *tok, *ptr;

	dtree_import_set_node(NULL, ctx);

	tok = strtok(line, "=");
	if (!tok)
		return -1;

	tok = strtok(NULL, "=");
	if (!tok)
		return -1;

	ptr = tok;
	while (*ptr == ' ')
		ptr++;

	root = dtree_import_root(ctx);
	node = dtree_from_cronus_target(root, ptr);
	if (!node)
		return -1;

	dtree_import_set_node(node, ctx);
	return 0;
}

static int cronus_parse_attr(void *ctx, char *line)
{
	struct dtm_node *node;
	struct dtree_attr *attr;
	enum dtree_attr_type type;
	char *attr_name, *data_type;
	char *tok, *saveptr = NULL;
	uint8_t *ptr;
	int dim_count, count, index;
	int idx[3] = { -1, -1, -1 };
	int dim[3] = { -1, -1, -1 };
	int i, ret;
	bool is_enum = false;

	node = dtree_import_node(ctx);
	if (!node)
		return -1;

	/* attribute name */
	tok = strtok_r(line, " ", &saveptr);
	if (!tok)
		return -1;

	attr_name = strtok(tok, "[");
	if (!attr_name)
		return -1;

	/* Skip properties that don't begin with "ATTR" */
	if (strlen(attr_name) < 4 || strncmp(attr_name, "ATTR", 4))
		return 0;

	/* Skip attributes that are not in device tree */
	ret = dtree_import_attr(attr_name, ctx, &attr);
	if (ret != 0)
		return 0;

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
		return -1;

	data_type = strtok(tok, "[");
	if (!data_type)
		return -1;

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

	dim_count = 0;
	count = 1;
	for (i=0; i<3; i++) {
		if (dim[i] != -1) {
			dim_count += 1;
			count *= dim[i];
		} else {
			break;
		}
	}

	/* validate against device tree attribute */
	type = attr_type_from_cronus_string(data_type);
	if (type != attr->type)
		return -1;

	if (dim_count != attr->dim_count)
		return -1;
	for (i=0; i<dim_count; i++) {
		if (dim[i] != attr->dim[i])
			return -1;
		if (idx[i] < 0 || idx[i] >= dim[i])
			return -1;
	}

	index = 0;
	if (attr->dim_count == 1) {
		index = idx[0];
	} else if (attr->dim_count == 2) {
		index = idx[0] * attr->dim[1] +
			idx[1];
	} else if (attr->dim_count == 3) {
		index = idx[0] * attr->dim[1] * attr->dim[2] +
			idx[1] * attr->dim[2] +
			idx[2];
	}

	ptr = attr->value + index * attr->elem_size;
	ret = -1;

	/* attribute value */
	if (attr->type == DTREE_ATTR_TYPE_COMPLEX) {
		uint64_t val;
		int data_size;

		for (i=0; i<strlen(attr->spec); i++) {
			tok = strtok_r(NULL, " ", &saveptr);
			if (!tok)
				return -1;

			val = strtoull(tok, NULL, 0);
			data_size = attr->spec[i] - '0';
			dtree_attr_set_num(ptr, data_size, val);
			ptr += data_size;
		}
	} else if (attr->type == DTREE_ATTR_TYPE_STRING) {
		size_t n;

		tok = strtok_r(NULL, " ", &saveptr);
		if (!tok)
			return -1;

		if (tok[0] != '"')
			return -1;

		n = strlen(tok);
		if (tok[n-1] != '"')
			return -1;

		tok[n-1] = '\0';
		dtree_attr_set_string(attr, ptr, &tok[1]);
	} else {
		tok = strtok_r(NULL, " ", &saveptr);
		if (!tok)
			return -1;

		if (is_enum) {
			if (!dtree_attr_set_enum(attr, ptr, tok))
				return -1;
		} else {
			dtree_attr_set_value(attr, ptr, tok);
		}
	}

	return dtree_import_attr_update(ctx);
}

int dtree_cronus_parse(void *ctx, char *buf)
{
	int ret;

	/* Skip empty lines */
	if (strlen(buf) < 6)
		return 0;

	if (strncmp(buf, "target", 6) == 0) {
		ret = cronus_parse_target(ctx, buf);
	} else {
		ret = cronus_parse_attr(ctx, buf);
	}

	return ret;
}
