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

#ifndef __DTREE_H__
#define __DTREE_H__

#include <stdint.h>

struct dtm_node;

#define DTREE_ATTR_MAX_LEN	72

enum dtree_attr_type {
	DTREE_ATTR_TYPE_UNKNOWN = 0,
	DTREE_ATTR_TYPE_UINT8   = 1,
	DTREE_ATTR_TYPE_UINT16  = 2,
	DTREE_ATTR_TYPE_UINT32  = 3,
	DTREE_ATTR_TYPE_UINT64  = 4,
	DTREE_ATTR_TYPE_INT8    = 5,
	DTREE_ATTR_TYPE_INT16   = 6,
	DTREE_ATTR_TYPE_INT32   = 7,
	DTREE_ATTR_TYPE_INT64   = 8,
	DTREE_ATTR_TYPE_STRING  = 9,
	DTREE_ATTR_TYPE_COMPLEX = 10,
};

struct dtree_attr_enum {
	char *key;
	uint64_t value;
};

struct dtree_attr {
	char name[DTREE_ATTR_MAX_LEN];
	enum dtree_attr_type type;
	int elem_size;
	int dim_count;
	int *dim;
	int count;
	int enum_count;
	struct dtree_attr_enum *aenum;
	char *spec;
	uint8_t *value;
};

/**
 * @brief Callback for each node during export
 *
 * @param[in] root  Device tree root
 * @param[in] node  Device tree node
 * @param[in] priv  Private data for export
 * @return 0 to continue export, non-zero value will stop export
 */
typedef int (*dtree_export_node_fn)(struct dtm_node *root, struct dtm_node *node, void *priv);

/**
 * @brief Callback for each attribute during export
 *
 * @param[in] node  Device tree attribute
 * @param[in] priv  Private data for export
 * @return 0 to continue export, non-zero value will stop export
 */
typedef int (*dtree_export_attr_fn)(const struct dtree_attr *attr, void *priv);

/**
 * @brief Callback for parsing import data
 *
 * @param[in] ctx  Import context
 * @param[in] priv  Private data for import
 * @return 0 on successful import, non-zero on error
 */
typedef int (*dtree_import_parse_fn)(void *ctx, void *priv);

/**
 * @brief Create a device tree with attributes
 *
 * This function reads attribute and target information from attribute
 * information database (infodb), and adds attributes to matching targets in
 * specified device tree.
 *
 * @param[in] dtb_path  Path to binary device tree
 * @param[in] infodb_path  Path to attribute information database
 * @param[in] dtb_out_path  Path to generated binary device tree
 * @return 0 if all nodes and attributes are created, -1 on failure
 */
int dtree_create(const char *dtb_path,
		 const char *infodb_path,
		 const char *dtb_out_path);


/**
 * @brief Export attributes from a device tree
 *
 * This function traverses all the nodes of the tree in depth-first fashion.
 * It takes two callbacks:
 *
 *   node_fn - for each node
 *   attr_fn - for each attribute
 *
 * If any of the export callbacks returns non-zero value, then the export
 * is aborted and export returns with that return value.
 *
 * @param[in] dtb_path  Path to binary device tree
 * @param[in] infodb_path  Path to attribute information database
 * @param[in] attrdb_path  Path to list of attributes to export
 * @param[in] node_fn  Callback function called for each node
 * @param[in] attr_fn  Callback function called for each attribute
 * @param[in] priv  Private data for callbacks
 * @return 0 if all nodes and attributes are exported, non-zero otherwise
 */
int dtree_export(const char *dtb_path,
		 const char *infodb_path,
		 const char *attrdb_path,
		 dtree_export_node_fn node_fn,
		 dtree_export_attr_fn attr_fn,
		 void *priv);


/**
 * @brief Import attributes to a device tree
 *
 * This function traverses all the nodes of the tree in depth-first fashion.
 * It takes two callbacks:
 *
 *   node_fn - for each node
 *   attr_fn - for each attribute
 *
 * If any of the export callbacks returns non-zero value, then the export
 * is aborted and export returns with that return value.
 *
 * @param[in] dtb_path  Path to binary device tree
 * @param[in] infodb_path  Path to attribute information database
 * @param[in] parse_fn  Callback function called to parse import data
 * @param[in] priv  Private data for callback
 * @return 0 if all nodes and attributes are exported, non-zero otherwise
 */
int dtree_import(const char *dtb_path,
		 const char *infodb_path,
		 dtree_import_parse_fn parse_fn,
		 void *priv);

/**
 * @brief Get device tree root from import context
 *
 * @param[in] ctx  Import context
 * @return device tree root
 */
void *dtree_import_root(void *ctx);

/**
 * @brief Set the current node during import
 *
 * This function sets the current node for importing attributes.
 *
 * @param[in] node_path  Device tree path to a node
 * @param[in] ctx  Import context
 * @return 0 for valid node, -1 otherwise
 */
int dtree_import_set_node(struct dtm_node *node, void *ctx);

/**
 * @brief Get the current node from import context
 *
 * @param[in] ctx  Import context
 * @return the current node
 */
struct dtm_node *dtree_import_node(void *ctx);

/**
 * @brief Get attribute definition from import context
 *
 * This requires the current node has been set.
 *
 * @param[in] ctx  Import context
 * @param[in] attr_name  Name of attribute
 * @param[out] attr  Attribute information
 * @return 0 on success, -1 otherwise
 */
int dtree_import_attr(const char *attr_name, void *ctx, struct dtree_attr **attr);

/**
 * @brief Update the attribute value
 *
 * This function updates the attribute value.
 *
 * @param[in] attr  Attribute data
 * @param[in] ctx  Import context
 * @return 0 on successful import, -1 otherwise
 */
int dtree_import_attr_update(void *ctx);

/**
 * @brief Export device tree is cronus dump format
 *
 * @param[in] dtb_path  Device tree path
 * @param[in] infodb_path  Attribute metadata database path
 * @param[in] attrdb_path  File with a list of attributes to export
 * @param[in] fp_export  File pointer for export
 * @return 0 on success, -1 on error
 */
int dtree_cronus_export(const char *dtb_path,
			const char *infodb_path,
			const char *attrdb_path,
			FILE *fp_export);

/**
 * @brief Import device tree from cronus dump format
 *
 * @param[out] dtb_path  Device tree path
 * @param[in]  infodb_path  Attribute metadata database path
 * @param[in]  fp_import  Flie pointer for import
 * @return 0 on success, -1 on error
 */
int dtree_cronus_import(const char *dtb_path,
			const char *infodb_path,
			FILE *fp_import);

#endif /* __DTREE_H__ */

