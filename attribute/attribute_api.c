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
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "attribute/attribute.h"
#include "attribute/attribute_format.h"
#include "attribute/attribute_api.h"

int attr_read(struct pdbg_target *target, const char *name,
	      uint8_t *value, size_t value_len)
{
	struct attr attr;
	const uint8_t *cbuf;
	size_t buflen;

	cbuf = pdbg_target_property(target, name, &buflen);
	if (!cbuf)
		return ENOENT;

	attr_decode(&attr, cbuf, buflen);

	if ((unsigned int)attr.size != value_len) {
		free(attr.value);
		return EMSGSIZE;
	}

	if (!attr.defined) {
		free(attr.value);
		return EINVAL;
	}

	memcpy(value, attr.value, value_len);
	free(attr.value);

	return 0;
}

int attr_write(struct pdbg_target *target, const char *name,
	       uint8_t *value, size_t value_len)
{
	struct attr attr;
	const uint8_t *cbuf;
	uint8_t *buf;
	size_t buflen;
	int len;
	bool ok;

	cbuf = pdbg_target_property(target, name, &buflen);
	if (!cbuf)
		return ENOENT;

	attr_decode(&attr, cbuf, buflen);
	free(attr.value);

	if ((unsigned int)attr.size != value_len)
		return EMSGSIZE;

	attr.value = value;
	attr.defined = true;

	attr_encode(&attr, &buf, &len);
	buflen = len;

	ok = pdbg_target_set_property(target, name, buf, buflen);
	free(buf);
	if (!ok)
		return EIO;

	return 0;
}
