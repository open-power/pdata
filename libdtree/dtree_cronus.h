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

#ifndef __DTREE_CRONUS_H__
#define __DTREE_CRONUS_H__

struct dtm_node *dtree_from_cronus_target(struct dtm_node *root, const char *name);
char *dtree_to_cronus_target(const struct dtm_node *root, struct dtm_node *node);

void dtree_cronus_print_node(const char *target, FILE *fp);
void dtree_cronus_print_attr(const struct dtree_attr *attr, FILE *fp);

int dtree_cronus_parse(void *ctx, char *buf);

#endif /* __DTREE_CRONUS_H__ */
