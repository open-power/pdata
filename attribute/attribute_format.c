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
#include <math.h>
#include <assert.h>

#include "attribute.h"
#include "attribute_db.h"
#include "attribute_format.h"
#include "attribute_util.h"

struct {
	enum attr_type type;
	char *label;
	uint8_t size;
} attr_type_size_map[] = {
	{ ATTR_TYPE_UINT8,  "u8", 0x01 },
	{ ATTR_TYPE_UINT16, "u16", 0x02 },
	{ ATTR_TYPE_UINT32, "u32", 0x04 },
	{ ATTR_TYPE_UINT64, "u64", 0x08 },
	{ ATTR_TYPE_INT8,   "s8", 0x11 },
	{ ATTR_TYPE_INT16,  "s16", 0x12 },
	{ ATTR_TYPE_INT32,  "s32", 0x14 },
	{ ATTR_TYPE_INT64,  "s64", 0x18 },
	{ ATTR_TYPE_UNKNOWN, NULL, 0x00 },
};

static int attr_type_to_size(enum attr_type type)
{
	int i;

	for (i=0; attr_type_size_map[i].label; i++) {
		if (type == attr_type_size_map[i].type)
			return attr_type_size_map[i].size;
	}

	fprintf(stderr, "Invalid attribute type %d\n", type);
	assert(0);
}

static enum attr_type attr_type_from_size(uint8_t size)
{
	int i;

	for (i=0; attr_type_size_map[i].label; i++) {
		if (size == attr_type_size_map[i].size)
			return attr_type_size_map[i].type;
	}

	fprintf(stderr, "Invalid attribute size %02x\n", size);
	assert(0);
}

/*
 * Attribute encoding
 *
 *     byte0        byte1        byte2       byte3
 * +------------+------------+------------+------------+
 * |    0x00    |  data_size |   defined  | array_dim  |
 * +------------+------------+-------------------------+
 * |   dimension 1 size  (optional if array_dim == 0)  |
 * +---------------------------------------------------+
 * |   dimension 2 size  (optional if array_dim == 0)  |
 * +---------------------------------------------------+
 * |   dimension 3 size  (optional if array_dim == 0)  |
 * +---------------------------------------------------+
 * |                                                   |
 * |            Value buffer                           |
 * |      (size = data_size * number_of_values)        |
 * |      (number_of_values = dim1 * dim2 * dim3)      |
 * +---------------------------------------------------+
 */

void attr_encode(struct attr *attr, uint8_t **out, int *outlen)
{
	uint8_t *buf;
	uint32_t header, dim, buflen, offset = 0;
	uint8_t size, flag;
	int i;

	buflen = (1 + attr->dim_count) * 4 + attr->size * attr->data_size;
	buf = calloc(1, buflen);
	assert(buf);

	/* header word */
	header = 0;

	size = attr_type_to_size(attr->type);
	header |= size;
	header <<= 8;

	flag = attr->defined ? 1 : 0;
	header |= flag;
	header <<= 8;

	header |= attr->dim_count;
	*(uint32_t *)(buf + offset) = htobe32(header);
	offset += 4;

	/* dim words */
	for (i=0; i<attr->dim_count; i++) {
		dim = attr->dim[i];
		*(uint32_t *)(buf + offset) = htobe32(dim);
		offset += 4;
	}

	/* data bytes */
	memcpy(buf + offset, attr->value, attr->size * attr->data_size);

	*out = buf;
	*outlen = buflen;
}

void attr_decode(struct attr *attr, const uint8_t *buf, int buflen)
{
	uint32_t header, offset = 0;
	uint8_t size, flag;

	/* header word */
	assert(buflen >= 4);
	header = be32toh(*(uint32_t *)(buf + offset));
	offset += 4;

	attr->dim_count = header & 0xff;
	header >>= 8;

	flag = header & 0xff;
	attr->defined = (flag == 1);
	header >>= 8;

	size = header & 0xff;
	attr->type = attr_type_from_size(size);
	attr->data_size = size & 0x0f;

	attr->size = 1;
	if (attr->dim_count > 0) {
		int i;

		attr->dim = malloc(attr->dim_count * sizeof(int));
		assert(attr->dim);

		for (i=0; i<attr->dim_count; i++) {
			assert(buflen - offset >= 4);
			attr->dim[i] = be32toh(*(uint32_t *)(buf + offset));
			offset += 4;

			attr->size *= attr->dim[i];
		}
	} else {
		attr->dim = 0;
	}

	attr->value = malloc(attr->size * attr->data_size);
	assert(attr->value);

	assert(buflen - offset == attr->size * attr->data_size);
	memcpy(attr->value, buf+offset, buflen-offset);
}
