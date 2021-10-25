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

#ifndef __ATTRIBUTE_TARGET_H
#define __ATTRIBUTE_TARGET_H

#include "libdtm/dtm.h"

const char *dtree_to_fapi_class(const char *dtree_class);
const char *cronus_to_dtree_class(const char *cronus_class);
const char *dtree_to_cronus_class(const char *dtree_class);

struct dtm_node *from_cronus_target(struct dtm_node *root, const char *name);
char *to_cronus_target(struct dtm_node *root, struct dtm_node *node);

#endif /* __ATTRIBUTE_TARGET_H */
