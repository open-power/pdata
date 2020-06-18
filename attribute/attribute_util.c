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
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "attribute.h"
#include "attribute_util.h"

struct {
	enum attr_type type;
	char *label;
} attr_type_map[] = {
	{ ATTR_TYPE_UINT8,  "uint8" },
	{ ATTR_TYPE_UINT16, "uint16" },
	{ ATTR_TYPE_UINT32, "uint32" },
	{ ATTR_TYPE_UINT64, "uint64" },
	{ ATTR_TYPE_INT8,   "int8" },
	{ ATTR_TYPE_INT16,  "int16" },
	{ ATTR_TYPE_INT32,  "int32" },
	{ ATTR_TYPE_INT64,  "int64" },
	{ ATTR_TYPE_UNKNOWN, NULL },
};

enum attr_type attr_type_from_string(char *str)
{
	int i;

	for (i=0; attr_type_map[i].label; i++) {
		if (strcmp(str, attr_type_map[i].label) == 0)
			return attr_type_map[i].type;
	}

	return ATTR_TYPE_UNKNOWN;
}

const char *attr_type_to_string(uint8_t type)
{
	int i;

	for (i=0; attr_type_map[i].label; i++) {
		if (type == attr_type_map[i].type)
			return attr_type_map[i].label;
	}

	return "<NULL>";
}

int attr_type_size(enum attr_type type)
{
	int size = 0;

	if (type == ATTR_TYPE_UINT8 || type == ATTR_TYPE_INT8) {
		size = 1;
	} else if (type == ATTR_TYPE_UINT16 || type == ATTR_TYPE_INT16) {
		size = 2;
	} else if (type == ATTR_TYPE_UINT32 || type == ATTR_TYPE_INT32) {
		size = 4;
	} else if (type == ATTR_TYPE_UINT64 || type == ATTR_TYPE_INT64) {
		size = 8;
	} else {
		assert(0);
	}

	return size;
}

void attr_copy(struct attr *src, struct attr *dst)
{
	assert(dst);

	*dst = (struct attr) {
		.type = src->type,
		.data_size = src->data_size,
		.dim_count = src->dim_count,
		.size = src->size,
		.enum_count = src->enum_count,
		.aenum = src->aenum,
	};

	strncpy(dst->name, src->name, ATTR_MAX_LEN);
	if (dst->dim_count > 0) {
		int i;

		dst->dim = malloc(dst->dim_count * sizeof(int));
		assert(dst->dim);

		for (i=0; i<src->dim_count; i++)
			dst->dim[i] = src->dim[i];
	}

	dst->value = malloc(dst->data_size * dst->size);
	assert(dst->value);
	memcpy(dst->value, src->value, dst->data_size * dst->size);
}

void attr_set_value_num(uint8_t *ptr, int data_size, uint64_t val)
{
	if (data_size == 1) {
		uint8_t value = val & 0xff;
		memcpy(ptr, &value, 1);
	} else if (data_size == 2) {
		uint16_t value = val & 0xffff;
		memcpy(ptr, &value, 2);
	} else if (data_size == 4) {
		uint32_t value = val & 0xffffffff;
		memcpy(ptr, &value, 4);
	} else if (data_size == 8) {
		uint64_t value = val;
		memcpy(ptr, &value, 8);
	} else {
		assert(0);
	}
}

void attr_set_value(struct attr *attr, uint8_t *ptr, const char *tok)
{
	uint64_t val;

	switch (attr->type) {
	case ATTR_TYPE_UINT8:
	case ATTR_TYPE_UINT16:
	case ATTR_TYPE_UINT32:
	case ATTR_TYPE_UINT64:
	case ATTR_TYPE_INT8:
	case ATTR_TYPE_INT16:
	case ATTR_TYPE_INT32:
	case ATTR_TYPE_INT64:
		val = strtoull(tok, NULL, 0);
		attr_set_value_num(ptr, attr->data_size, val);
		break;

	default:
		assert(0);
	}
}

bool attr_set_enum_value(struct attr *attr, uint8_t *ptr, const char *tok)
{
	int i;

	for (i=0; i<attr->enum_count; i++) {
		struct attr_enum *ae = &attr->aenum[i];

		if (strcmp(ae->key, tok) == 0) {
			attr_set_value_num(ptr, attr->data_size, ae->value);
			return true;
		}
	}

	return false;
}

void attr_print_value_num(uint8_t *ptr, int data_size)
{
	if (data_size == 1) {
		uint8_t val = *ptr;
		printf("0x%02x", val);
	} else if (data_size == 2) {
		uint16_t val = *(uint16_t *)ptr;
		printf("0x%04x", val);
	} else if (data_size == 4) {
		uint32_t val = *(uint32_t *)ptr;
		printf("0x%08x", val);
	} else if (data_size == 8) {
		uint64_t val = *(uint64_t *)ptr;
		printf("0x%016" PRIx64, val);
	} else {
		assert(0);
	}
}

void attr_print_value(struct attr *attr, uint8_t *ptr)
{
	int count = 1, i;

	switch (attr->type) {
	case ATTR_TYPE_UINT8:
	case ATTR_TYPE_UINT16:
	case ATTR_TYPE_UINT32:
	case ATTR_TYPE_UINT64:
	case ATTR_TYPE_INT8:
	case ATTR_TYPE_INT16:
	case ATTR_TYPE_INT32:
	case ATTR_TYPE_INT64:
		break;

	default:
		assert(0);
	}

	if (!ptr) {
		ptr = attr->value;
		count = attr->size;
	}

	for (i=0; i<count; i++) {
		attr_print_value_num(ptr, attr->data_size);
		ptr += attr->data_size;

		if (i < count-1)
			printf(" ");
	}
}

bool attr_print_enum_value(struct attr *attr, uint8_t *ptr)
{
	uint64_t val = 0;
	int count = 1, i, j;

	if (attr->enum_count == 0)
		return false;

	if (!ptr) {
		ptr = attr->value;
		count = attr->size;
	}

	for (i=0; i<count; i++) {
		bool found = false;

		if (attr->data_size == 1) {
			val = *ptr;
		} else if (attr->data_size == 2) {
			val = *(uint16_t *)ptr;
		} else if (attr->data_size == 4) {
			val = *(uint32_t *)ptr;
		} else if (attr->data_size == 8) {
			val = *(uint64_t *)ptr;
		} else {
			assert(0);
		}
		ptr += attr->data_size;

		for (j=0; j<attr->enum_count; j++) {
			struct attr_enum *ae = &attr->aenum[j];

			if (ae->value == val) {
				printf("%s", ae->key);
				found = true;
				break;
			}
		}
		if (!found)
			printf("UNKNOWN_ENUM");

		if (i < count-1)
			printf(" ");
	}

	return true;
}
