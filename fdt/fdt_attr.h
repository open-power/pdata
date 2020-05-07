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

#ifndef __FDT_ATTR_H__
#define __FDT_ATTR_H__

/**
 * @file fdt_attr.h
 * @brief FDT based attribute api
 *
 * libfdt_attr defines attribute handling API where attributes are stored as
 * properties in device trees.  Attributes values have specific data
 * structure.  Two types of data structures are supported:
 *
 * 1. simple (arrays of 1, 2, 4, or 8 byte integers)
 * 2. complex (packed stream of 1, 2, 4, or 8 byte integers)
 *
 * The write APIs will ensure that data is stored in consistent endian in the
 * device tree.  The read APIs will ensure that data is retrieved in host
 * endian format.
 */

/**
 * @brief Read simple value of given attribute from device tree
 *
 * The attribute value is considered as an array of 1, 2, 4 or 8 byte integer.
 *
 * @param[in] fdt Flatted device tree pointer
 * @param[in] path Full path of the node in device tree
 * @param[in] name Name of the attribute
 * @param[in] data_size Size of an element in the array
 * @param[in] count Number of elements in the array
 * @param[out] value Value of the attribute
 * @return 0 on success, errno on failure
 *    ENOENT - If the node or attribute does not exist
 *    ENOMEM - If memory allocaton fails
 *    EINVAL - If the data_size and/or count are invalid
 *    EIO    - If there is error reading attribute
 */
int fdt_attr_read(void *fdt, const char *path, const char *name,
		  uint32_t data_size, uint32_t count, uint8_t *value);

/**
 * @brief Write simple value of given attribute to device tree
 *
 * The attribute value is considered as an array of 1, 2, 4 or 8 byte integer.
 *
 * @param[in] fdt Flatted device tree pointer
 * @param[in] path Full path of the node in device tree
 * @param[in] name Name of the attribute
 * @param[in] data_size Size of an element in the array
 * @param[in] count Number of elements in the array
 * @param[out] value Value of the attribute
 * @return 0 on success, errno on failure
 *    ENOENT - If the node or attribute does not exist
 *    ENOMEM - If memory allocaton fails
 *    EINVAL - If the data_size and/or count are invalid
 *    EIO    - If there is error writing attribute
 */
int fdt_attr_write(void *fdt, const char *path, const char *name,
		   uint32_t data_size, uint32_t count, uint8_t *value);

/**
 * @brief Read complex value of given attribute from device tree
 *
 * The attribute value is considered as a packed stream of 1, 2, 4 or 8 byte
 * integers.  The specification describes how the integers are packed.
 *
 * Example:
 *    A stream of uint32_t, uint8_t, uint16_t, uint32_t is specified as
 *    "4124".
 *
 * If the pattern is repeated (e.g. array of structures), then count is set to
 * the repetition count (array size).
 *
 * @param[in] fdt Flatted device tree pointer
 * @param[in] path Full path of the node in device tree
 * @param[in] name Name of the attribute
 * @param[in] spec Specification of packed integers
 * @param[in] count Repetition count
 * @param[out] value Value of the attribute
 * @return 0 on success, errno on failure
 *    ENOENT - If the node or attribute does not exist
 *    ENOMEM - If memory allocaton fails
 *    EINVAL - If the specification is invalid
 *    EIO    - If there is error reading attribute
 */
int fdt_attr_read_packed(void *fdt, const char *path, const char *name,
			 const char *spec, uint32_t count, uint8_t *value);

/**
 * @brief Write complex value of given attribute to device tree
 *
 * The attribute value is considered as a packed stream of 1, 2, 4 or 8 byte
 * integers.  The specification describes how the integers are packed.
 *
 * Example:
 *    A stream of uint32_t, uint8_t, uint16_t, uint32_t is specified as
 *    "4124".
 *
 * If the pattern is repeated (e.g. array of structures), then count is set to
 * the repetition count (array size).
 *
 * @param[in] fdt Flatted device tree pointer
 * @param[in] path Full path of the node in device tree
 * @param[in] name Name of the attribute
 * @param[in] spec Specification of packed integers
 * @param[in] count Repetition count
 * @param[out] value Value of the attribute
 * @return 0 on success, errno on failure
 *    ENOENT - If the node or attribute does not exist
 *    ENOMEM - If memory allocaton fails
 *    EINVAL - If the specification is invalid
 *    EIO    - If there is error writing attribute
 */
int fdt_attr_write_packed(void *fdt, const char *path, const char *name,
			  const char *spec, uint32_t count, uint8_t *value);

#endif /* __FDT_ATTR_H__ */

