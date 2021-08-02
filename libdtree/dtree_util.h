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

#ifndef __DTREE_UTIL_H__
#define __DTREE_UTIL_H__

const char *dtree_to_fapi_class(const char *dtree_class);
const char *cronus_to_dtree_class(const char *cronus_class);
const char *dtree_to_cronus_class(const char *dtree_class);
char *dtree_name_to_class(const char *name);

#endif /* __DTREE_UTIL_H__ */
