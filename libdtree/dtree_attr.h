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

#ifndef __DTREE_ATTR_H__
#define __DTREE_ATTR_H__

#include <stdbool.h>

#include "dtree.h"

enum dtree_attr_type dtree_attr_type_from_string(const char *str);
const char *dtree_attr_type_to_string(uint8_t type);
int dtree_attr_type_size(enum dtree_attr_type type);
int dtree_attr_spec_size(const char *spec);

void dtree_attr_set_num(uint8_t *ptr, int data_size, uint64_t val);
void dtree_attr_set_value(struct dtree_attr *attr, uint8_t *ptr, const char *tok);
bool dtree_attr_set_enum(struct dtree_attr *attr, uint8_t *ptr, const char *tok);
void dtree_attr_set_string(struct dtree_attr *attr, uint8_t *ptr, const char *tok);

void dtree_attr_copy(const struct dtree_attr *src, struct dtree_attr *dst);
void dtree_attr_free(struct dtree_attr *attr);

void dtree_attr_encode(const struct dtree_attr *attr, uint8_t **out, int *outlen);
void dtree_attr_decode(struct dtree_attr *attr, const uint8_t *buf, int buflen);

#endif /* _DTREE_ATTR_H__ */
