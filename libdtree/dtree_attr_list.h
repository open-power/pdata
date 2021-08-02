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

#ifndef __DTREE_ATTR_LIST_H__
#define __DTREE_ATTR_LIST_H__

#include <stdbool.h>

struct name_list {
	char **name;
	int count;
	int size;
};

bool dtree_attr_list_parse(const char *attrdb, struct name_list *alist);
bool dtree_attr_list_exists(const struct name_list *alist, const char *attr);

#endif /* __DTREE_ATTR_LIST_H__ */
