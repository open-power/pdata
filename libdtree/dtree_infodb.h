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

#ifndef __DTREE_INFODB_H__
#define __DTREE_INFODB_H__

#include <stdbool.h>

#include "dtree.h"

#define DTREE_TARGET_MAX_LEN	32

struct dtree_attr_list {
	int count;
	struct dtree_attr *attr;
};

struct dtree_target {
	char name[DTREE_TARGET_MAX_LEN];
	int id_count;
	int *id;
};

struct dtree_target_list {
	int count;
	struct dtree_target *target;
};

struct dtree_infodb {
	struct dtree_attr_list alist;
	struct dtree_target_list tlist;
};

bool dtree_infodb_load(const char *filename, struct dtree_infodb *infodb);
struct dtree_attr *dtree_infodb_attr(struct dtree_infodb *infodb, const char *name);

#endif /* __DTREE_INFODB_H__ */
