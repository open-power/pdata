/* Copyright 2020 IBM Corp.
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

#ifndef __ATTRIBUTE_API_H__
#define __ATTRIBUTE_API_H__

#include <libpdbg.h>

int attr_read(struct pdbg_target *target, const char *name,
	      uint8_t *value, size_t value_len);
int attr_write(struct pdbg_target *target, const char *name,
	       uint8_t *value, size_t value_len);

#endif /* ATTRIBUTE_API_H__ */
