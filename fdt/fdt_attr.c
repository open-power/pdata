/* Copyright 2020 IBM Corp.
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
#include <errno.h>

#include <libfdt.h>

#include "fdt_prop.h"

int fdt_attr_read(void *fdt, const char *path, const char *name,
		  uint32_t data_size, uint32_t count, uint8_t *value)
{
	uint8_t *buf;
	int buflen, ret, i;

	if (data_size > 8 || count > 1000000)
		return EINVAL;

	buflen = data_size * count;
	buf = malloc(buflen);
	if (!buf)
		return ENOMEM;

	ret = fdt_prop_read(fdt, path, name, buf, &buflen);
	if (ret != 0) {
		free(buf);
		return ret;
	}

	if (data_size == 1) {
		memcpy(value, buf, buflen);

	} else if (data_size == 2) {
		uint16_t *b = (uint16_t *)buf;
		uint16_t *v = (uint16_t *)value;

		for (i=0; i<count; i++)
			v[i] = be16toh(b[i]);

	} else if (data_size == 4) {
		uint32_t *b = (uint32_t *)buf;
		uint32_t *v = (uint32_t *)value;

		for (i=0; i<count; i++)
			v[i] = be32toh(b[i]);

	} else if (data_size == 8) {
		uint64_t *b = (uint64_t *)buf;
		uint64_t *v = (uint64_t *)value;

		for (i=0; i<count; i++)
			v[i] = be64toh(b[i]);

	} else {
		free(buf);
		return EINVAL;
	}

	free(buf);
	return 0;
}

int fdt_attr_write(void *fdt, const char *path, const char *name,
		   uint32_t data_size, uint32_t count, uint8_t *value)
{
	uint8_t *buf;
	int buflen, ret, i;

	if (data_size > 8 || count > 1000000)
		return EINVAL;

	buflen = data_size * count;
	buf = malloc(buflen);
	if (!buf)
		return ENOMEM;

	if (data_size == 1) {
		memcpy(buf, value, buflen);

	} else if (data_size == 2) {
		uint16_t *b = (uint16_t *)buf;
		uint16_t *v = (uint16_t *)value;

		for (i=0; i<count; i++)
			b[i] = htobe16(v[i]);

	} else if (data_size == 4) {
		uint32_t *b = (uint32_t *)buf;
		uint32_t *v = (uint32_t *)value;

		for (i=0; i<count; i++)
			b[i] = htobe32(v[i]);

	} else if (data_size == 8) {
		uint64_t *b = (uint64_t *)buf;
		uint64_t *v = (uint64_t *)value;

		for (i=0; i<count; i++)
			b[i] = htobe64(v[i]);

	} else {
		free(buf);
		return EINVAL;
	}

	ret = fdt_prop_write(fdt, path, name, buf, buflen);
	if (ret != 0) {
		free(buf);
		return ret;
	}

	free(buf);
	return 0;
}

static int spec_size(const char *spec)
{
	int size = 0, i;

	for (i=0; i<strlen(spec); i++) {
		char ch = spec[i];

		if (ch == '1')
			size += 1;
		else if (ch == '2')
			size += 2;
		else if (ch == '4')
			size += 4;
		else if (ch == '8')
			size += 8;
		else
			return -1;
	}

	return size;
}

int fdt_attr_read_packed(void *fdt, const char *path, const char *name,
			 const char *spec, uint8_t *value)
{
	uint8_t *buf;
	int buflen, ret, pos, i;

	if (!spec || spec[0] == '\0')
		return EINVAL;

	buflen = spec_size(spec);
	if (buflen <= 0)
		return EINVAL;

	buf = malloc(buflen);
	if (!buf)
		return ENOMEM;

	ret = fdt_prop_read(fdt, path, name, buf, &buflen);
	if (ret != 0) {
		free(buf);
		return ret;
	}

	pos = 0;
	for (i=0; i<strlen(spec); i++) {
		char ch = spec[i];

		if (ch == '1') {
			uint8_t *b = buf + pos;
			uint8_t *v = value + pos;

			*v = *b;
			pos += 1;

		} else if (ch == '2') {
			uint16_t *b = (uint16_t *)(buf + pos);
			uint16_t *v = (uint16_t *)(value + pos);

			*v = be16toh(*b);
			pos += 2;

		} else if (ch == '4') {
			uint32_t *b = (uint32_t *)(buf + pos);
			uint32_t *v = (uint32_t *)(value + pos);

			*v = be32toh(*b);
			pos += 4;

		} else if (ch == '8') {
			uint64_t *b = (uint64_t *)(buf + pos);
			uint64_t *v = (uint64_t *)(value + pos);

			*v = be64toh(*b);
			pos += 8;
		}
	}

	free(buf);
	return 0;
}

int fdt_attr_write_packed(void *fdt, const char *path, const char *name,
			  const char *spec, uint8_t *value)
{
	uint8_t *buf;
	int buflen, ret, pos, i;

	if (!spec || spec[0] == '\0')
		return EINVAL;

	buflen = spec_size(spec);
	if (buflen <= 0)
		return EINVAL;

	buf = malloc(buflen);
	if (!buf)
		return ENOMEM;

	pos = 0;
	for (i=0; i<strlen(spec); i++) {
		char ch = spec[i];

		if (ch == '1') {
			uint8_t *b = buf + pos;
			uint8_t *v = value + pos;

			*b = *v;
			pos += 1;

		} else if (ch == '2') {
			uint16_t *b = (uint16_t *)(buf + pos);
			uint16_t *v = (uint16_t *)(value + pos);

			*b = htobe16(*v);
			pos += 2;

		} else if (ch == '4') {
			uint32_t *b = (uint32_t *)(buf + pos);
			uint32_t *v = (uint32_t *)(value + pos);

			*b = htobe32(*v);
			pos += 4;

		} else if (ch == '8') {
			uint64_t *b = (uint64_t *)(buf + pos);
			uint64_t *v = (uint64_t *)(value + pos);

			*b = htobe64(*v);
			pos += 8;
		}
	}

	ret = fdt_prop_write(fdt, path, name, buf, buflen);
	if (ret != 0) {
		free(buf);
		return ret;
	}

	free(buf);
	return 0;
}
