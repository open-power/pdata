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
#include <assert.h>

#include "attribute.h"
#include "attribute_format.h"

static int rand_max(int max)
{
	return 1 + random() % max;
}

static uint8_t random_u8(void)
{
	uint8_t value = random() & 0xff;
	return value;
}

static uint8_t random_u16(void)
{
	uint16_t value = random() & 0xffff;
	return value;
}

static uint8_t random_u32(void)
{
	uint32_t value = random() & 0xffffffff;
	return value;
}

static uint8_t random_u64(void)
{
	uint64_t value = random_u32();
	value = (value << 32) | random_u32();
	return value;
}

static int8_t random_s8(void)
{
	int8_t value = INT8_MIN + random_u8();
	return value;
}

static int16_t random_s16(void)
{
	int16_t value = INT16_MIN + random_u16();
	return value;
}

static int32_t random_s32(void)
{
	int32_t value = INT32_MIN + random_u32();
	return value;
}

static int64_t random_s64(void)
{
	int64_t value = INT64_MIN + random_u64();
	return value;
}

static void test_scalar(enum attr_type type, int data_size, uint8_t *value, int encode_len)
{
	struct attr attr, attr2;
	uint8_t *buf;
	int buflen;

	attr = (struct attr) {
		.name = "dummy",
		.type = type,
		.data_size = data_size,
		.dim_count = 0,
		.size = 1,
		.enum_count = 0,
		.defined = 1,
		.value = value,
	};

	attr2 = (struct attr) {
		.name = "dummy",
	};

	attr_encode(&attr, &buf, &buflen);
	assert(buf);
	assert(buflen == encode_len);

	attr_decode(&attr2, buf, buflen);
	assert(attr2.type == attr.type);
	assert(attr2.data_size == attr.data_size);
	assert(attr2.dim_count == attr.dim_count);
	assert(attr2.size == attr.size);
	assert(!memcmp(attr2.value, attr.value, data_size));

	free(buf);
}

static void test_array(int dim_count, int *dim, uint8_t *value, int encode_len)
{
	struct attr attr, attr2;
	uint8_t *buf;
	int buflen;
	int size = 1, i;

	for (i=0; i<dim_count; i++)
		size = size * dim[i];

	attr = (struct attr) {
		.name = "dummy",
		.type = ATTR_TYPE_UINT32,
		.data_size = 4,
		.dim_count = dim_count,
		.dim = dim,
		.size = size,
		.enum_count = 0,
		.defined = 1,
		.value = value,
	};

	attr2 = (struct attr) {
		.name = "dummy",
	};

	attr_encode(&attr, &buf, &buflen);
	assert(buf);
	assert(buflen == encode_len);

	attr_decode(&attr2, buf, buflen);
	assert(attr2.type == attr.type);
	assert(attr2.data_size == attr.data_size);
	assert(attr2.dim_count == attr.dim_count);
	for (i=0; i<dim_count; i++)
		assert(attr2.dim[i] == attr.dim[i]);
	assert(attr2.size == attr.size);
	assert(!memcmp(attr2.value, attr.value, attr.data_size * size));

	free(buf);
}

static void test1(void)
{
	uint64_t u64 = random_u64();
	int64_t s64 = random_s64();
	uint32_t u32 = random_u32();
	int32_t s32 = random_s32();
	uint16_t u16 = random_u16();
	int16_t s16 = random_s16();
	uint8_t u8 = random_u8();
	int8_t s8 = random_s8();

	test_scalar(ATTR_TYPE_UINT8, 1, (uint8_t *)&u8, 5);
	test_scalar(ATTR_TYPE_INT8, 1, (uint8_t *)&s8, 5);

	test_scalar(ATTR_TYPE_UINT16, 2, (uint8_t *)&u16, 6);
	test_scalar(ATTR_TYPE_INT16, 2, (uint8_t *)&s16, 6);

	test_scalar(ATTR_TYPE_UINT32, 4, (uint8_t *)&u32, 8);
	test_scalar(ATTR_TYPE_INT32, 4, (uint8_t *)&s32, 8);

	test_scalar(ATTR_TYPE_UINT64, 8, (uint8_t *)&u64, 12);
	test_scalar(ATTR_TYPE_INT64, 8, (uint8_t *)&s64, 12);
}

static void test2(void)
{
	uint32_t *value;
	int dim1[1];
	int dim2[2];
	int dim3[3];
	int size, i;

	dim1[0] = rand_max(100000);
	size = dim1[0];

	value = malloc(size * 4);
	assert(value);

	for (i=0; i<size; i++)
		value[i] = random_u32();

	test_array(1, dim1, (uint8_t *)value, 8 + 4*size);
	free(value);

	dim2[0] = rand_max(100);
	dim2[1] = rand_max(100);
	size = dim2[0] * dim2[1];

	value = malloc(size * 4);
	assert(value);

	for (i=0; i<size; i++)
		value[i] = random_u32();

	test_array(2, dim2, (uint8_t *)value, 12 + 4*size);
	free(value);

	dim3[0] = rand_max(100);
	dim3[1] = rand_max(100);
	dim3[2] = rand_max(100);
	size = dim3[0] * dim3[1] * dim3[2];

	value = malloc(size * 4);
	assert(value);

	for (i=0; i<size; i++)
		value[i] = random_u32();

	test_array(3, dim3, (uint8_t *)value, 16 + 4*size);
	free(value);
}

int main(void)
{
	int i;

	for (i=0; i<10000; i++)
		test1();

	for (i=0; i<100; i++)
		test2();

	return 0;
}
