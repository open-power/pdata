// SPDX-License-Identifier: Apache-2.0
#include <cassert>
#include <iostream>

#include "dt_api.H"

namespace fapi2
{
    uint32_t getProperty( struct pdbg_target* target, const std::string &attrId, void *val, uint32_t eleCount, size_t attrSize, const std::string &attrTypeName, const std::string &attrSpec )
    {
        // Attribute name may contain namespace, so ignoring NS if present
        // beacuse the device tree will only keep attribute name
        size_t pos = attrId.find("::");
        pos = pos == std::string::npos ? 0 : pos + 2;
        const std::string attrIdWONS(attrId, pos);

        /* NULL targets use pdbg_dt_root */
        if (!target) {
            /* TODO: This should never happen but we've only got a partial
             * implementation of targetting so far */
            std::cerr << "NULL target reading attribute: " << attrId << ". So, using pdbg_dt_root for the moment" << std::endl;
            target = pdbg_target_root();
        }

        const char* path = pdbg_target_path(target);
        assert(path);

        if ( attrTypeName == "struct" )
        {
            if (!pdbg_target_get_attribute_packed(target, attrIdWONS.c_str(), attrSpec.c_str(), val))
            {
                std::cerr << "pdbg_target_get_attribute_packed failed" << std::endl;
                return 1; // FAPI2_RC_INVALID_ATTR_GET
            }
        }
        else
        {
            if (!pdbg_target_get_attribute(target, attrIdWONS.c_str(), std::stoi(attrSpec), eleCount, val))
            {
                std::cerr << "pdbg_target_get_attribute failed" << std::endl;
                return 1; // FAPI2_RC_INVALID_ATTR_GET
            }
        }
        return 0; // FAPI2_RC_SUCCESS
    }

    uint32_t setProperty( struct pdbg_target *target, const std::string &attrId, void *val, uint32_t eleCount, size_t attrSize, const std::string &attrTypeName, const std::string &attrSpec )
    {
        // Attribute name may contain namespace, so ignoring NS if present
        // beacuse the device tree will only keep attribute name
        size_t pos = attrId.find("::");
        pos = pos == std::string::npos ? 0 : pos + 2;
        const std::string attrIdWONS(attrId, pos);

        /* NULL targets use pdbg_dt_root */
        if (!target) {
            /* TODO: This should never happen but we've only got a partial
             * implementation of targetting so far */
            std::cerr << "NULL target reading attribute: " << attrId << ". So, using pdbg_dt_root for the moment" << std::endl;
            target = pdbg_target_root();
        }

        if ( attrTypeName == "struct" )
        {
            if (!pdbg_target_set_attribute_packed(target, attrIdWONS.c_str(), attrSpec.c_str(), val))
            {
                std::cerr << "pdbg_target_set_attribute_packed failed" << std::endl;
                return 1; // FAPI2_RC_INVALID_ATTR_GET
            }
        }
        else
        {
            if (!pdbg_target_set_attribute(target, attrIdWONS.c_str(), std::stoi(attrSpec), eleCount, val))
            {
                std::cerr << "pdbg_target_set_attribute failed" << std::endl;
                return 1; // FAPI2_RC_INVALID_ATTR_GET
            }
        }

        return 0; // FAPI2_RC_SUCCESS
    }
} // end of fapi2 namespace
