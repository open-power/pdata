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
#include <stdarg.h>

#include <libfdt.h>

struct fdt_error_map {
	int code;
	const char *msg;
};

#define FDT_ERROR(x)	{ x, #x }

static struct fdt_error_map fdt_error_map[FDT_ERR_MAX] = {
	FDT_ERROR(FDT_ERR_NOTFOUND),
	FDT_ERROR(FDT_ERR_EXISTS),
	FDT_ERROR(FDT_ERR_NOSPACE),
	FDT_ERROR(FDT_ERR_BADOFFSET),
	FDT_ERROR(FDT_ERR_BADPATH),
	FDT_ERROR(FDT_ERR_BADPHANDLE),
	FDT_ERROR(FDT_ERR_BADSTATE),
	FDT_ERROR(FDT_ERR_TRUNCATED),
	FDT_ERROR(FDT_ERR_BADMAGIC),
	FDT_ERROR(FDT_ERR_BADVERSION),
	FDT_ERROR(FDT_ERR_BADSTRUCTURE),
	FDT_ERROR(FDT_ERR_BADLAYOUT),
	FDT_ERROR(FDT_ERR_INTERNAL),
	FDT_ERROR(FDT_ERR_BADNCELLS),
	FDT_ERROR(FDT_ERR_BADVALUE),
	FDT_ERROR(FDT_ERR_BADOVERLAY),
	FDT_ERROR(FDT_ERR_NOPHANDLES),
};

void fdt_error(int ret, const char *fmt, ...)
{
	va_list ap;

	ret = (ret < 0) ? -ret : ret;
	if (ret <= FDT_ERR_MAX)
		fprintf(stderr, "%s: ", fdt_error_map[ret-1].msg);
	else
		fprintf(stderr, "FDT_UNKNOWN_ERR: ");

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}
