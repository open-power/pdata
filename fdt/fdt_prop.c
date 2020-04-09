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
#include <errno.h>

#include <libfdt.h>

#include "fdt_prop.h"


int fdt_prop_read(void *fdt, const char *path, const char *name,
		  uint8_t *value, int *value_len)
{
	int nodeoffset;
	const void *buf;
	int buflen;

	nodeoffset = fdt_path_offset(fdt, path);
	if (nodeoffset < 0)
		return ENOENT;

	buf = fdt_getprop(fdt, nodeoffset, name, &buflen);
	if (!buf)
		return ENOENT;

	if (*value_len < buflen)
		return EINVAL;

	memcpy(value, buf, buflen);
	*value_len = buflen;
	return 0;
}

int fdt_prop_write(void *fdt, const char *path, const char *name,
		   const uint8_t *value, int value_len)
{
	int nodeoffset, ret;

	nodeoffset = fdt_path_offset(fdt, path);
	if (nodeoffset < 0)
		return ENOENT;

	ret = fdt_setprop_inplace(fdt, nodeoffset, name, value, value_len);
	if (ret != 0)
		return EIO;

	return 0;
}
