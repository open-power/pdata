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
#include <endian.h>

#include "attribute.h"
#include "attribute_db.h"
#include "attribute_format.h"
#include "attribute_util.h"

void attr_encode(struct attr *attr, uint8_t **out, int *outlen)
{
	uint8_t *buf;
	uint32_t buflen;
	int i, j;

	buflen = attr->size * attr->data_size;
	buf = calloc(1, buflen);
	assert(buf);

	if (attr->type == ATTR_TYPE_COMPLEX) {
		uint8_t *b = buf;
		uint8_t *v = attr->value;
		char ch;

		for (i=0; i<attr->size; i++) {
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

	} else if (attr->type == ATTR_TYPE_STRING) {
		memcpy(buf, attr->value, buflen);
	} else {
		if (attr->data_size == 1) {
			memcpy(buf, attr->value, attr->size * attr->data_size);

		} else if (attr->data_size == 2) {
			uint16_t *b = (uint16_t *)buf;
			uint16_t *v = (uint16_t *)attr->value;

			for (i=0; i<attr->size; i++)
				b[i] = htobe16(v[i]);

		} else if (attr->data_size == 4) {
			uint32_t *b = (uint32_t *)buf;
			uint32_t *v = (uint32_t *)attr->value;

			for (i=0; i<attr->size; i++)
				b[i] = htobe32(v[i]);

		} else if (attr->data_size == 8) {
			uint64_t *b = (uint64_t *)buf;
			uint64_t *v = (uint64_t *)attr->value;

			for (i=0; i<attr->size; i++)
				b[i] = htobe64(v[i]);
		}
	}

	*out = buf;
	*outlen = buflen;
}

void attr_decode(struct attr *attr, const uint8_t *buf, int buflen)
{
	int i, j;

	assert(buflen == attr->size * attr->data_size);

	if (attr->value)
		free(attr->value);

	attr->value = malloc(attr->size * attr->data_size);
	assert(attr->value);

	if (attr->type == ATTR_TYPE_COMPLEX) {
		const uint8_t *b = buf;
		uint8_t *v = attr->value;
		char ch;

		for (i=0; i<attr->size; i++) {
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

	} else if (attr->type == ATTR_TYPE_STRING) {
		memcpy(attr->value, buf, buflen);
	} else {
		if (attr->data_size == 1) {
			memcpy(attr->value, buf, buflen);

		} else if (attr->data_size == 2) {
			uint16_t *b = (uint16_t *)buf;
			uint16_t *v = (uint16_t *)attr->value;

			for (i=0; i<attr->size; i++)
				v[i] = be16toh(b[i]);

		} else if (attr->data_size == 4) {
			uint32_t *b = (uint32_t *)buf;
			uint32_t *v = (uint32_t *)attr->value;

			for (i=0; i<attr->size; i++)
				v[i] = be32toh(b[i]);

		} else if (attr->data_size == 8) {
			uint64_t *b = (uint64_t *)buf;
			uint64_t *v = (uint64_t *)attr->value;

			for (i=0; i<attr->size; i++)
				v[i] = htobe64(b[i]);
		}
	}
}
