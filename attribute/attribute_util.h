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

#ifndef __ATTRIBUTE_UTIL_H
#define __ATTRIBUTE_UTIL_H

#include <attribute/attribute.h>

enum attr_type attr_type_from_string(char *str);
const char *attr_type_to_string(uint8_t type);
enum attr_type attr_type_from_short_string(char *str);
const char *attr_type_to_short_string(uint8_t type);
int attr_type_size(enum attr_type type);
void attr_copy(struct attr *src, struct attr *dst);

void attr_set_value_num(uint8_t *ptr, int data_size, uint64_t val);
void attr_set_value(struct attr *attr, uint8_t *ptr, const char *tok);
bool attr_set_enum_value(struct attr *attr, uint8_t *ptr, const char *tok);
void attr_set_string_value(struct attr *attr, uint8_t *ptr, const char *tok);

void attr_print_value_num(uint8_t *ptr, int data_size);
void attr_print_value(struct attr *attr, uint8_t *ptr);
bool attr_print_enum_value(struct attr *attr, uint8_t *ptr);
void attr_print_string_value(struct attr *attr, uint8_t *ptr);
void attr_print_complex_value(struct attr *attr, uint8_t *ptr);

#endif /* __ATTRIBUTE_UTIL_H */
