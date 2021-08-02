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

#ifndef __DTREE_DUMP_H__
#define __DTREE_DUMP_H__

#include <stdio.h>

void dtree_dump_print_node(const struct dtm_node *node, FILE *fp);
void dtree_dump_print_attr_name(const struct dtree_attr *attr, FILE *fp);
void dtree_dump_print_attr(const struct dtree_attr *attr, FILE *fp);

#endif /* __DTREE_DUMP_H__ */
