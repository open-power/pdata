# Generate CEC device tree for new POWER processor

pdata provides infrastructure (tools) to generate cec device tree and metadata files (attributes_info.H and attributes_info.db) but, it will expect few files with the required information so, please refer below steps for preparing those
files to get the cec device tree and metadata files for the new POWER processor.
  
  1. Create new power processor version specific folder [here](../data).
  
     **Note:** Folder name should be processor version i.e p9|p10|p11.
     
  2. Prepare the required hwp's and non-hwp targets list to filter.
  
     **Note:**
     - File name should be `filter_TargetsList.lsv`.
     - Required target name should be same as mrw target id.
     - Please refer sample [here](../data/p10/filter_TargetsList.lsv).
  
  3. Prepare the required attributes list to filter.
  
     **Note:**
     - File name should be `filter_AttributesList.lsv`.
     - All attribute should have respective prefix based on attribute usage i.e. hwp or non-hwp attribute.
     - Please refer sample [here](../data/p10/filter_AttributesList.lsv).
  
  4. Prepare the list of required hwp's targets **type**  list and its corresponding mrw target **id** in the new file.
     This is mainly used to convert hwp target type into mrw target id to make it unique across all steps.
     
     **Note**:
     - File name should be `FAPITargetsNameMapList.lsv`.
     - Please refer sample [here](../data/p10/FAPITargetsNameMapList.lsv).
 
  
  5. Prepare the required targets (hwp and non-hwp) list with mrw target **id** value and its corresponding
     pdbg **compatible property** value.
  
     **Note:**
     - File name should be 'pdbg_compatible_propMapping.lsv`.
     - Please refer sample [here](../data/p10/pdbg_compatible_propMapping.lsv).
     
  6. Prepare the required targets (hwp's and non-hwp targets) definition and its non-hwp attributes list. The hwp's target
     the definition will use to map all hwp's attributes which are present in the required hwp's attributes xml file into respective
     targets so, please make sure all required hwp's targets definition information is added or not and also few non-hwp
     attributes are common to more than one targets so, please make sure those attributes are added to the respective target
     and added that target id as a parent into required child targets.
   
     **Note:**
     - File name should be `target_types_obmc.xml`.
     - target definition should be as mrw target xml format.
     - Please refer sample [here](../data/p10/target_types_obmc.xml).
     
  7. Prepare the required hwp's attributes xml filename list to process from the pub-ekb repository to get the required attributes.
  
     **Note:**
     - File name should be `reqEkbAttrsXmlFileList.lsv`.
     - Please refer sample [here](../data/p10/reqEkbAttrsXmlFileList.lsv).
     - Also, same filename list needs to add into op-build ekb package by using `EKB_HWP_ATTRS_XML_FILES` and openBMC ekb
       recipe by using `REQ_ATTRS_XMLS` variable.
      
       p10 References for example:
       - [op-build ekb package](https://github.com/open-power/op-build/blob/master-p10/openpower/package/ekb/ekb.mk).
       - [openBMC ekb recipe](https://github.com/openbmc/openbmc/blob/master/meta-openpower/recipes-bsp/ekb/ekb.inc)
      
  
  8. Prepare the required non-hwp attributes definition.
  
     **Note:**
     - File name should be `attribute_types_obmc.xml`.
     - attribute definition should be same as mrw attribute xml format.
     - Please refer sample [here](../data/p10/attribute_types_obmc.xml).

  9. Prepare the hardware procedure attribute list to overwrite with the platform specific defalut value.
    
     **Note:**
     - File name should be `bmc_customized_ekb_attrs.xml`.
     - Please refer sample [here](../data/p10/bmc_customized_ekb_attrs.xml).


  10. Prepare the attributes list to preserve.
    
      **Note:**
      - File name should be `preserved_attrs_list` to preserve attributes value during the service pack update (aka, code update).
      - File name should be `reinit_devtree_attrs_list` to reinit attributes value during the cold boot.
      - Please refer samples : [preserved_attrs_list](../data/p10/preserved_attrs_list) and [reinit_devtree_attrs_list](../data/p10/reinit_devtree_attrs_list).
     
  11. Once prepared all above required files then use configure option i.e `CHIP` to pass new processor version to build device tree and meta-data. Please follow [build steps](../README.md#build-info-and-details) to get require files.
     
      **Note:** For op-build and openBMC build, need to use `CHIP` configuration parameter to pass expected processor version.
