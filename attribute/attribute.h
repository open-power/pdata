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

#ifndef __ATTRIBUTE_H
#define __ATTRIBUTE_H

#include <stdbool.h>
#include <inttypes.h>

#define ATTR_MAX_LEN	72
#define TARGET_MAX_LEN	32

#define	NUM_CHIPLETS	64

enum attr_type {
	ATTR_TYPE_UNKNOWN = 0,
	ATTR_TYPE_UINT8   = 1,
	ATTR_TYPE_UINT16  = 2,
	ATTR_TYPE_UINT32  = 3,
	ATTR_TYPE_UINT64  = 4,
	ATTR_TYPE_INT8    = 5,
	ATTR_TYPE_INT16   = 6,
	ATTR_TYPE_INT32   = 7,
	ATTR_TYPE_INT64   = 8,
};

struct attr_enum {
	char *key;
	uint64_t value;
};

struct attr {
	char name[ATTR_MAX_LEN];
	enum attr_type type;
	int data_size;
	int dim_count;
	int *dim;
	int size;
	int enum_count;
	struct attr_enum *aenum;
	uint8_t *value;
};

struct attr_list {
	int count;
	struct attr *attr;
};

struct target {
	char name[TARGET_MAX_LEN];
	int id_count;
	int *id;
};

struct target_list {
	int count;
	struct target *target;
};

struct attr_info {
	struct attr_list alist;
	struct target_list tlist;
};

#endif /* __ATTRIBUTE_H */
