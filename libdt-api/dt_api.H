// SPDX-License-Identifier: Apache-2.0
#ifndef __DT_API_H
#define __DT_API_H

/**
 * @file dt_api.H
 * @author Ramesh Iyyar
 * @brief device tree property read and write definitions for use by application
 *
 * libdt_api defines a api for read and write attributes which is stored in
 * device tree as property under node which is called as target.
 *
 * The defined dt api are based on libpdbg library. So, Before using dt-api
 * pdbg library should be initialized by application.
 *
 */

extern "C" {
#include <libpdbg.h>
}

#include <string>

/* DT get and set property api under fapi2 namespace,
*  because HWPs attributes read/write are tightly coupled
*  with fapi2 namespace
*/
namespace fapi2
{
    /**
     * @brief This api is used to call pdbg get API based on
     *        attribute type name
     *
     * @param[in] target pointer to attributes pdbg target
     * @param[in] attrId Attribute name to get value from device tree
     * @param[out] val Attribute pointer to store readed values
     * @param[in] eleCount No of elements of attribute type
     * @param[in] attrSize size of attribute type
     * @param[in] attrTypeName Attribute type name
     * @param[in] attrSpec Specification of attributes i.e bytes count for endianess
     * @return 0 on success. 1 on failure
     *
     */
    uint32_t getProperty( struct pdbg_target* target, const std::string &attrId, void *val, uint32_t eleCount, size_t attrSize, const std::string &attrTypeName, const std::string &attrSpec );

    /**
     * @brief This api is used to call pdbg set API based on
     *        attribute type name
     *
     * @param[in] target pointer to attributes pdbg target
     * @param[in] attrId Attribute name to set value into device tree
     * @param[in] val Attribute pointer to write values into device tree
     * @param[in] eleCount No of elements of attribute type
     * @param[in] attrSize size of attribute type
     * @param[in] attrTypeName Attribute type name
     * @param[in] attrSpec Specification of attributes i.e bytes count for endianess
     * @return 0 on success. 1 on failure
     *
     */
    uint32_t setProperty( struct pdbg_target *target, const std::string &attrId, void *val, uint32_t eleCount, size_t attrSize, const std::string &attrTypeName, const std::string &attrSpec );

} // end of fapi2 namespace

#endif
