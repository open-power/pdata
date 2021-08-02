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
#include <assert.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

#include "libdtm/dtm.h"

#include "dtree.h"
#include "dtree_dump.h"

struct {
	enum dtree_attr_type type;
	char *label;
} dump_attr_type_map[] = {
	{ DTREE_ATTR_TYPE_UINT8,   "uint8"  },
	{ DTREE_ATTR_TYPE_UINT16,  "uint16" },
	{ DTREE_ATTR_TYPE_UINT32,  "uint32" },
	{ DTREE_ATTR_TYPE_UINT64,  "uint64" },
	{ DTREE_ATTR_TYPE_INT8,    "int8"  },
	{ DTREE_ATTR_TYPE_INT16,   "int16" },
	{ DTREE_ATTR_TYPE_INT32,   "int32" },
	{ DTREE_ATTR_TYPE_INT64,   "int64" },
	{ DTREE_ATTR_TYPE_STRING,  "string" },
	{ DTREE_ATTR_TYPE_COMPLEX, "complex" },
	{ DTREE_ATTR_TYPE_UNKNOWN, "" },
};

static const char *attr_type_to_dump_string(uint8_t type)
{
	int i;

	for (i=0; dump_attr_type_map[i].label; i++) {
		if (type == dump_attr_type_map[i].type)
			return dump_attr_type_map[i].label;
	}

	return "invalid";
}

static void dump_print_value_num(uint8_t *ptr, int data_size, FILE *fp)
{
	if (data_size == 1) {
		fprintf(fp, "0x%02x", *ptr);
	} else if (data_size == 2) {
		fprintf(fp, "0x%04x", *(uint16_t *)ptr);
	} else if (data_size == 4) {
		fprintf(fp, "0x%08x", *(uint32_t *)ptr);
	} else if (data_size == 8) {
		fprintf(fp, "0x%016" PRIx64, *(uint64_t *)ptr);
	} else {
		assert(0);
	}
}

static void dump_print_value(const struct dtree_attr *attr, FILE *fp)
{
	uint8_t *ptr;
	int i;

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

	ptr = attr->value;

	for (i=0; i<attr->count; i++) {
		dump_print_value_num(ptr, attr->elem_size, fp);
		ptr += attr->elem_size;

		if (i < attr->count-1)
			fprintf(fp, " ");
	}
}

static bool dump_print_enum(const struct dtree_attr *attr, FILE *fp)
{
	uint8_t *ptr;
	uint64_t val = 0;
	int i, j;

	if (attr->enum_count == 0)
		return false;

	ptr = attr->value;

	for (i=0; i<attr->count; i++) {
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
			dump_print_value_num(ptr, attr->elem_size, fp);
			fprintf(fp, ")");
		}

		if (i < attr->count-1)
			fprintf(fp, " ");

		ptr += attr->elem_size;
	}

	return true;
}

static void dump_print_string(const struct dtree_attr *attr, FILE *fp)
{
	uint8_t *ptr;
	int i;

	ptr = attr->value;

	for (i=0; i<attr->count; i++) {
		fprintf(fp, "\"%.*s\"", attr->elem_size, (char *)ptr);
		ptr += attr->elem_size;

		if (i < attr->count-1)
			fprintf(fp, " ");
	}
}

static void dump_print_complex(const struct dtree_attr *attr, FILE *fp)
{
	uint8_t *ptr;
	int i, j;

	ptr = attr->value;

	for (i=0; i<attr->count; i++) {
		size_t n = strlen(attr->spec);

		for (j=0; j<n; j++) {
			int data_size = attr->spec[j] - '0';

			dump_print_value_num(ptr, data_size, fp);
			ptr += data_size;

			if (j < n-1)
				fprintf(fp, " ");
		}

		if (i < attr->count-1)
			fprintf(fp, " ");
	}
}

void dtree_dump_print_node(const struct dtm_node *node, FILE *fp)
{
	char *path;

	path = dtm_node_path(node);
	assert(path);
	fprintf(fp, "%s\n", path);
	free(path);
}

void dtree_dump_print_attr_name(const struct dtree_attr *attr, FILE *fp)
{
	fprintf(fp, "  %s: %s ", attr->name, attr_type_to_dump_string(attr->type));
}

void dtree_dump_print_attr(const struct dtree_attr *attr, FILE *fp)
{
	if (attr->type == DTREE_ATTR_TYPE_UNKNOWN) {
		fprintf(fp, "**UNKNOWN**");
	} else if (attr->type == DTREE_ATTR_TYPE_COMPLEX) {
		dump_print_complex(attr, fp);
	} else if (attr->type == DTREE_ATTR_TYPE_STRING) {
		dump_print_string(attr, fp);
	} else {
		if (!dump_print_enum(attr, fp))
			dump_print_value(attr, fp);
	}
}
