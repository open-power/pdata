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

/**
 * @file attribute_api.h
 * @author Amitay Isaacs
 * @brief Attribute API to read/write attributes from device tree
 *
 * The attribute api provides accessing attributes stored in device tree as
 * properties.  The attributes are encoded to store the meta-data along with
 * the actual value of the attribute.  This allows correctly handling data
 * endian.
 *
 * The attribute api is based on libpdbg library.
 *
 */
#include <libpdbg.h>

/**
 * @brief Read an attribute from device tree
 *
 * The calling program needs to know the actual size of the attribute value.
 * If there is a mismatch between the expected size and the actual stored
 * size, then error is returned.  The value of attribute is copied to the
 * provided buffer.
 *
 * @param[in] target  Pdbg target identifying device tree node
 * @param[in] name   Name of the attribute
 * @param[out] value  Buffer for value of the attribute
 * @param[in] value_len  Length of the value of the attribute
 * @return 0 on success, errno on failure
 *
 * ENOENT   if attribute is not found
 * EMSGSIZE if the value_len does not match the length of value stored
 * EINVAL   if the attribute value is not initialized
 *
 */
int attr_read(struct pdbg_target *target, const char *name,
	      uint8_t *value, size_t value_len);

/**
 * @brief Write an attribute to device tree
 *
 * If there is a mismatch between the value size and the actual stored
 * size, then error is returned.
 *
 * @param[in] target  Pdbg target identifying device tree node
 * @param[in] name   Name of the attribute
 * @param[in] value  Value of the attribute
 * @param[in] value_len  Length of the value of the attribute
 * @return 0 on success, errno on failure
 *
 * ENOENT   if attribute is not found
 * EMSGSIZE if the value_len does not match the length of value stored
 * EIO      if there was an error writing to device tree
 */
int attr_write(struct pdbg_target *target, const char *name,
	       uint8_t *value, size_t value_len);

#endif /* ATTRIBUTE_API_H__ */
