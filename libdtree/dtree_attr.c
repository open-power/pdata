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
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "dtree.h"
#include "dtree_attr.h"

struct {
	enum dtree_attr_type type;
	char *label;
} attr_type_map[] = {
	{ DTREE_ATTR_TYPE_UINT8,  "uint8" },
	{ DTREE_ATTR_TYPE_UINT16, "uint16" },
	{ DTREE_ATTR_TYPE_UINT32, "uint32" },
	{ DTREE_ATTR_TYPE_UINT64, "uint64" },
	{ DTREE_ATTR_TYPE_INT8,   "int8" },
	{ DTREE_ATTR_TYPE_INT16,  "int16" },
	{ DTREE_ATTR_TYPE_INT32,  "int32" },
	{ DTREE_ATTR_TYPE_INT64,  "int64" },
	{ DTREE_ATTR_TYPE_STRING, "str" },
	{ DTREE_ATTR_TYPE_COMPLEX, "complex" },
	{ DTREE_ATTR_TYPE_UNKNOWN, NULL },
};

enum dtree_attr_type dtree_attr_type_from_string(const char *str)
{
	int i;

	for (i=0; attr_type_map[i].label; i++) {
		if (strcmp(str, attr_type_map[i].label) == 0)
			return attr_type_map[i].type;
	}

	return DTREE_ATTR_TYPE_UNKNOWN;
}

const char *dtree_attr_type_to_string(uint8_t type)
{
	int i;

	for (i=0; attr_type_map[i].label; i++) {
		if (type == attr_type_map[i].type)
			return attr_type_map[i].label;
	}

	return "<NULL>";
}

int dtree_attr_type_size(enum dtree_attr_type type)
{
	int size = 0;

	switch (type) {
	case DTREE_ATTR_TYPE_UINT8:
	case DTREE_ATTR_TYPE_INT8:
		size = 1;
		break;

	case DTREE_ATTR_TYPE_UINT16:
	case DTREE_ATTR_TYPE_INT16:
		size = 2;
		break;

	case DTREE_ATTR_TYPE_UINT32:
	case DTREE_ATTR_TYPE_INT32:
		size = 4;
		break;

	case DTREE_ATTR_TYPE_UINT64:
	case DTREE_ATTR_TYPE_INT64:
		size = 8;
		break;

	default:
		assert(0);
	}

	return size;
}

int dtree_attr_spec_size(const char *spec)
{
	int size = 0, i;

	for (i=0; i<strlen(spec); i++) {
		int data = spec[i] - '0';
		size += data;
	}

	return size;
}

void dtree_attr_set_num(uint8_t *ptr, int elem_size, uint64_t val)
{
	if (elem_size == 1) {
		uint8_t value = val & 0xff;
		memcpy(ptr, &value, 1);
	} else if (elem_size == 2) {
		uint16_t value = val & 0xffff;
		memcpy(ptr, &value, 2);
	} else if (elem_size == 4) {
		uint32_t value = val & 0xffffffff;
		memcpy(ptr, &value, 4);
	} else if (elem_size == 8) {
		uint64_t value = val;
		memcpy(ptr, &value, 8);
	} else {
		assert(0);
	}
}

void dtree_attr_set_value(struct dtree_attr *attr, uint8_t *ptr, const char *tok)
{
	uint64_t val;

	switch (attr->type) {
	case DTREE_ATTR_TYPE_UINT8:
	case DTREE_ATTR_TYPE_UINT16:
	case DTREE_ATTR_TYPE_UINT32:
	case DTREE_ATTR_TYPE_UINT64:
	case DTREE_ATTR_TYPE_INT8:
	case DTREE_ATTR_TYPE_INT16:
	case DTREE_ATTR_TYPE_INT32:
	case DTREE_ATTR_TYPE_INT64:
		val = strtoull(tok, NULL, 0);
		dtree_attr_set_num(ptr, attr->elem_size, val);
		break;

	default:
		assert(0);
	}
}

bool dtree_attr_set_enum(struct dtree_attr *attr, uint8_t *ptr, const char *tok)
{
	int i;

	for (i=0; i<attr->enum_count; i++) {
		struct dtree_attr_enum *ae = &attr->aenum[i];

		if (strcmp(ae->key, tok) == 0) {
			dtree_attr_set_num(ptr, attr->elem_size, ae->value);
			return true;
		}
	}

	return false;
}

void dtree_attr_set_string(struct dtree_attr *attr, uint8_t *ptr, const char *tok)
{
	strncpy((char *)ptr, tok, attr->elem_size);
}

void dtree_attr_copy(const struct dtree_attr *src, struct dtree_attr *dst)
{
	assert(dst);

	*dst = (struct dtree_attr) {
		.type = src->type,
		.elem_size = src->elem_size,
		.dim_count = src->dim_count,
		.count = src->count,
		.enum_count = src->enum_count,
		.aenum = src->aenum,
	};

	strncpy(dst->name, src->name, DTREE_ATTR_MAX_LEN);
	if (dst->dim_count > 0) {
		int i;

		dst->dim = malloc(dst->dim_count * sizeof(int));
		assert(dst->dim);

		for (i=0; i<src->dim_count; i++)
			dst->dim[i] = src->dim[i];
	}

	if (src->spec) {
		dst->spec = strdup(src->spec);
		assert(dst->spec);
	}

	dst->value = malloc(dst->elem_size * dst->count);
	assert(dst->value);
	memcpy(dst->value, src->value, dst->elem_size * dst->count);
}

void dtree_attr_free(struct dtree_attr *attr)
{
	assert(attr);

	if (attr->dim)
		free(attr->dim);
	/*
	if (attr->aenum)
		free(attr->aenum);
	*/
	if (attr->spec)
		free(attr->spec);
	if (attr->value)
		free(attr->value);
}

void dtree_attr_encode(const struct dtree_attr *attr, uint8_t **out, int *outlen)
{
	uint8_t *buf;
	uint32_t buflen;
	int i, j;

	buflen = attr->count * attr->elem_size;
	buf = malloc(buflen);
	assert(buf);

	if (attr->type == DTREE_ATTR_TYPE_COMPLEX) {
		uint8_t *b = buf;
		uint8_t *v = attr->value;
		char ch;

		for (i=0; i<attr->count; i++) {
			for (j=0; j<strlen(attr->spec); j++) {
				ch = attr->spec[j];

				if (ch == '1') {
					*b = *v;
					b += 1;
					v += 1;
				} else if (ch == '2') {
					*(uint16_t *)b = htobe16(*(uint16_t *)v);
					b += 2;
					v += 2;
				} else if (ch == '4') {
					*(uint32_t *)b = htobe32(*(uint32_t *)v);
					b += 4;
					v += 4;
				} else if (ch == '8') {
					*(uint64_t *)b = htobe64(*(uint64_t *)v);
					b += 8;
					v += 8;
				}
			}
		}

	} else if (attr->type == DTREE_ATTR_TYPE_STRING) {
		memcpy(buf, attr->value, buflen);
	} else {
		if (attr->elem_size == 1) {
			memcpy(buf, attr->value, attr->count * attr->elem_size);

		} else if (attr->elem_size == 2) {
			uint16_t *b = (uint16_t *)buf;
			uint16_t *v = (uint16_t *)attr->value;

			for (i=0; i<attr->count; i++)
				b[i] = htobe16(v[i]);

		} else if (attr->elem_size == 4) {
			uint32_t *b = (uint32_t *)buf;
			uint32_t *v = (uint32_t *)attr->value;

			for (i=0; i<attr->count; i++)
				b[i] = htobe32(v[i]);

		} else if (attr->elem_size == 8) {
			uint64_t *b = (uint64_t *)buf;
			uint64_t *v = (uint64_t *)attr->value;

			for (i=0; i<attr->count; i++)
				b[i] = htobe64(v[i]);
		}
	}

	*out = buf;
	*outlen = buflen;
}

void dtree_attr_decode(struct dtree_attr *attr, const uint8_t *buf, int buflen)
{
	int i, j;

	assert(buflen == attr->count * attr->elem_size);

	if (attr->value)
		free(attr->value);

	attr->value = malloc(attr->count * attr->elem_size);
	assert(attr->value);

	if (attr->type == DTREE_ATTR_TYPE_COMPLEX) {
		const uint8_t *b = buf;
		uint8_t *v = attr->value;
		char ch;

		for (i=0; i<attr->count; i++) {
			for (j=0; j<strlen(attr->spec); j++) {
				ch = attr->spec[j];

				if (ch == '1') {
					*v = *b;
					b += 1;
					v += 1;
				} else if (ch == '2') {
					*(uint16_t *)v = be16toh(*(uint16_t *)b);
					b += 2;
					v += 2;
				} else if (ch == '4') {
					*(uint32_t *)v = be32toh(*(uint32_t *)b);
					b += 4;
					v += 4;
				} else if (ch == '8') {
					*(uint64_t *)v = be64toh(*(uint64_t *)b);
					b += 8;
					v += 8;
				}
			}
		}

	} else if (attr->type == DTREE_ATTR_TYPE_STRING) {
		memcpy(attr->value, buf, buflen);
	} else {
		if (attr->elem_size == 1) {
			memcpy(attr->value, buf, buflen);

		} else if (attr->elem_size == 2) {
			uint16_t *b = (uint16_t *)buf;
			uint16_t *v = (uint16_t *)attr->value;

			for (i=0; i<attr->count; i++)
				v[i] = be16toh(b[i]);

		} else if (attr->elem_size == 4) {
			uint32_t *b = (uint32_t *)buf;
			uint32_t *v = (uint32_t *)attr->value;

			for (i=0; i<attr->count; i++)
				v[i] = be32toh(b[i]);

		} else if (attr->elem_size == 8) {
			uint64_t *b = (uint64_t *)buf;
			uint64_t *v = (uint64_t *)attr->value;

			for (i=0; i<attr->count; i++)
				v[i] = htobe64(b[i]);
		}
	}
}
