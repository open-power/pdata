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

#ifndef __DTM_H__
#define __DTM_H__

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Abstract data type representing FDT on disk and in memory
 */
struct dtm_file;

/**
 * @brief Abstract data type representing device tree node
 */
struct dtm_node;

/**
 * @brief Abstract data type representing a list of device tree nodes
 */
struct dtm_nodelist;

/**
 * @brief Abstract data type representing device tree node property
 */
struct dtm_property;

/**
 * @brief Callback for each node during travese
 *
 * @param[in] node  Device tree node
 * @param[in] priv  Private data for traverse
 * @return 0 to continue traverse, non-zero value will stop traverse
 */
typedef int (*dtm_traverse_node_fn)(struct dtm_node *node, void *priv);

/**
 * @brief Callback for each node property during travese
 *
 * @param[in] node  Device tree node
 * @param[in] prop  Device tree node property
 * @param[in] priv  Private data for traverse
 * @return 0 to continue traverse, non-zero value will stop traverse
 */
typedef int (*dtm_traverse_prop_fn)(struct dtm_node *node, struct dtm_property *prop, void *priv);

/**
 * @brief Open FDT file for reading
 *
 * @param[in] filename  Filename of FDT blob
 * @param[in] do_write  Whether file needs to modified
 * @return pointer to dtm_file structure or NULL on failure
 */
struct dtm_file *dtm_file_open(const char *filename, bool do_write);

/**
 * @brief Open FDT file for writing
 *
 * @param[in] filename  Filename of FDT blob
 * @return pointer to dtm_file structure or NULL on failure
 */
struct dtm_file *dtm_file_create(const char *filename);

/**
 * @brief Close FDT file
 *
 * If the file is opened for reading, this will close the file and free all
 * the memory associated it.
 *
 * If the file is opened for writing, this will write the FDT data to file and
 * free all the memory associated with it.
 *
 * @param[in] dfile  Pointer to dtm_file structure
 * @return 0 on success, errno on failure
 */
int dtm_file_close(struct dtm_file *dfile);

/**
 * @brief Read FDT blob into a tree structure
 *
 * @param[in] dfile  dtm_file for FDT file opened for read
 * @return  Root node of the tree structure, NULL on failure
 */
struct dtm_node *dtm_file_read(struct dtm_file *dfile);

/**
 * @brief Write FDT blob from a tree structure
 *
 * @param[in] dfile  dtm_file for FDT file opened for write
 * @param[in] root  Root node of the tree structure
 * @return true on success, false on failure
 */
bool dtm_file_write(struct dtm_file *dfile, struct dtm_node *root);

/**
 * @brief Update node information in FDT file
 *
 * @param[in] dfile  dtm_file for FDT file opened for write
 * @param[in] node   Node to be updated
 * @param[in] name   Name of property to be updated, NULL for all
 * @return true on success, false on failure
 */
bool dtm_file_update_node(struct dtm_file *dfile, struct dtm_node *node, const char *name);

/**
 * @brief Create a new tree with root node
 *
 * @return root node on success, NULL on failure
 */
struct dtm_node *dtm_tree_new(void);

/**
 * @brief Free the constructed device tree
 *
 * @param[in] root  Root of the device tree
 */
void dtm_tree_free(struct dtm_node *node);

/**
 * @brief Copy a device tree
 *
 * @param[in] root  Root of the device tree
 * @return root node of copied tree, NULL on failure
 */
struct dtm_node *dtm_tree_copy(struct dtm_node *root);

/**
 * @brief Get name of a node
 *
 * @param[in] node  Node of a device tree
 * @return name of the node
 */
const char *dtm_node_name(struct dtm_node *node);

/**
 * @brief Add a property to a node
 *
 * @param[in] node  A node
 * @param[in] name  Name of the property
 * @param[in] value  Value of the property
 * @param[in] valuelen  Length (in bytes) of the value of the property
 * @return 0 on success, -1 on failure
 */
int dtm_node_add_property(struct dtm_node *node, const char *name, void *value, int valuelen);

/**
 * @brief Get a property from a node by name
 *
 * @param[in] node  A node
 * @param[in] name  Name of the property
 * @return property if found, NULL if not found
 */
struct dtm_property *dtm_node_get_property(struct dtm_node *node, const char *name);

/**
 * @brief Get the device tree path of a node
 *
 * @param[in] node  A node
 * @return device tree path of the node (as allocated string), NULL on error
 */
char *dtm_node_path(struct dtm_node *node);

/**
 * @brief Get the index of a node
 *
 * If the specified node does not have index property, then get the index of
 * the parent, continuing till the root.
 *
 * If none of the nodes till the root have index property defined, then it
 * results in failure.
 *
 * @param[in] node  A node
 * @return index of the node, -1 on failure
 */
int dtm_node_index(struct dtm_node *node);

/**
 * @brief Get the parent of a node
 *
 * @param[in] node  A node
 * @return parent node, or NULL if node is the root node
 */
struct dtm_node *dtm_node_parent(struct dtm_node *node);

/**
 * @brief Get the name of the property
 *
 * @param[in] prop  A property
 * @return name of the property
 */
const char *dtm_prop_name(struct dtm_property *prop);

/**
 * @brief Get the value of the property
 *
 * @param[in] prop  A property
 * @param[out] value_len  Length of the property
 * @return pointer to the value of the property
 */
const void *dtm_prop_value(struct dtm_property *prop, int *value_len);

/**
 * @brief Set the value of the property
 *
 * @param[in] prop  A property
 * @param[in] value  Value of the property
 * @param[in] value_len  Length of the value
 * @return 0 on success, -1 on failure
 */
int dtm_prop_set_value(struct dtm_property *prop, uint8_t *value, int value_len);

/**
 * @brief Get the value of the integer property
 *
 * @param[in] prop  A property
 * @return value of the integer property
 */
uint32_t dtm_prop_value_u32(struct dtm_property *prop);

/**
 * @brief Traverse a device tree
 *
 * This traverses all the nodes of the tree in depth-first fashion.  It takes
 * two callbacks:
 *
 *   node_fn - for each traversed node
 *   prop_fn - for each traverse node property (can be NULL)
 *
 * If any of the traverse function returns non-zero value, then the traverse
 * function returns with that return value.
 *
 * @param[in] root  Root of the device tree
 * @param[in] do_all  Whether to traverse all nodes or only enabled nodes
 * @param[in] node_fn  Callback function called for each node
 * @param[in] prop_fn  Callback function called for each node
 * @param[in] priv  Private data for callbacks
 * @return 0 if all nodes and properties are traversed, non-zero otherwise
 */
int dtm_traverse(struct dtm_node *root,
		 bool do_all,
		 dtm_traverse_node_fn node_fn,
		 dtm_traverse_prop_fn prop_fn,
		 void *priv);

/**
 * @brief Traverse a device tree
 *
 * This traverses all the nodes of the tree in breadth-first fashion.  It takes
 * two callbacks:
 *
 *   node_fn - for each traversed node
 *   prop_fn - for each traverse node property (can be NULL)
 *
 * If any of the traverse function returns non-zero value, then the traverse
 * function returns with that return value.
 *
 * @param[in] root  Root of the device tree
 * @param[in] do_all  Whether to traverse all nodes or only enabled nodes
 * @param[in] node_fn  Callback function called for each node
 * @param[in] prop_fn  Callback function called for each node
 * @param[in] priv  Private data for callbacks
 * @return 0 if all nodes and properties are traversed, non-zero otherwise
 */
int dtm_traverse_bfs(struct dtm_node *root,
		     bool do_all,
		     dtm_traverse_node_fn node_fn,
		     dtm_traverse_prop_fn prop_fn,
		     void *priv);

struct dtm_node *dtm_find_node_by_name(struct dtm_node *root, const char *name);
struct dtm_node *dtm_find_node_by_compatible(struct dtm_node *root, const char *compatible);
struct dtm_node *dtm_find_node_by_path(struct dtm_node *root, const char *path);

struct dtm_node *dtm_tree_rearrange(struct dtm_node *root,
				    struct dtm_nodelist *nlist);

#endif /* __DTM_H__ */
