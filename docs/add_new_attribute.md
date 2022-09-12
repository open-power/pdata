# Add a new attribute

The power system device tree will maintain two types of attributes (HWP's (Hardware Procedure) and Non-HWP's attribute)
for each target based on needs to init and boot POWER server. So, Please refer respective sub-section for steps.

- To add ***hwp*** (hardware procedure) attribute, please refer
  [here](./add_new_attribute.md#hwps-attribute).
 
- To add ***non-hwp*** attribute, please refer
  [here](./add_new_attribute.md#non-hwps-attribute).
  
- To **preserve** the attribute value, please refer
  [here](./add_new_attribute.md#preserve-attribute).
  
**Note:** The steps will point to many files and a few will be POWER10 specific so, please refer and update the respective files
based on your requirement if you are looking for another POWER processor version.

### HWP's Attribute:

  The attribute which is consumed by [pub-ekb](https://github.com/open-power/pub-ekb/tree/main-p10) hardware procedure to init
  hardware units are called the hwp attribute.
  
  1. If the required new attribute defined in any of existing pub-ekb
     [hwp's attributes xml's list](../data/p10/reqEkbAttrsXmlFileList.lsv)
     then just adding that new attribute name into
     [attribute filter list](../data/p10/filter_AttributesList.lsv)
     is sufficient. So, Please refer format for hwp attribute in attribute filter list to add i.e. prefix should be
     `FA:` along with attribute name (e.g. `FA:ATTR_NAME`).
    
     **E.g.:** `FA:ATTR_NAME` in new line.
    
     ***Tips to check:***
     ```
     # Clone pub-ekb repository
     git clone https://github.com/open-power/pub-ekb/
     
     cd pub-ekb
     
     # Checkout required branch i.e. git checkout branch_name
     
     find . -name "*.xml" | xargs grep -l "<id>ATTR_CP_REFCLOCK_SELECT"
     O/P:  ./chips/p10/procedures/xml/attribute_info/p10_clock_attributes.xml
     
     # Just check above xml file path is present in hwp's attributes XML's list
     ```
  2. If the xml file path is not present in 
     [hwp's attributes xml's list](../data/p10/reqEkbAttrsXmlFileList.lsv)
     then follow below steps.
    
     1. Add the xml file path in
        [hwp's attributes xml's list](../data/p10/reqEkbAttrsXmlFileList.lsv)
        to consider required attributes xml files from given pub-ekb repo path during build time.
 
     2. Add the xml file path into `EKB_HWP_ATTRS_XML_FILES` variable which is present in
        [op-build ekb package](https://github.com/open-power/op-build/blob/master-p10/openpower/package/ekb/ekb.mk)
        to install required attributes xml files into staging directory instead of all files to consume by pdata for add
        new attribute into power system device tree during op-build.
    
     3. Add the xml file path into the `REQ_ATTRS_XMLS` variable which is present in
        [openbmc ekb recipe](https://github.com/openbmc/openbmc/blob/master/meta-openpower/recipes-bsp/ekb/ekb.inc)
        to install required attributes xml files into staging directory instead of all files to consume by pdata for add
        new attribute into metadata files (attributes_info.H and attributes_info.db) during openbmc build.
    
     4. Add the attribute name into
        [attribute filter list](../data/p10/filter_AttributesList.lsv)
        to get only required attributes from hwp's attributes xml file. Please refer format in attribute filter list to add
        i.e. prefix should be `FA:` along with attribute name (e.g. `FA:ATTR_NAME`).
        
  3. If the default value of attribute wants to change into the platform specific then follow below steps.

     1. Add the HWP attribute with default value in the [custom ekb attributes file](../data/p10/bmc_customized_ekb_attrs.xml).

     E.g.:
     ```
         <attribute>
             <id>ATTR_NAME</id>
             <default>value</default>
         </attribute>
     ```
     
     **Note:** The attribute must be defined in any of the pub-ekb [attributes xml files](https://github.com/open-power/pub-ekb/tree/main-p10/chips/p10/procedures/xml/attribute_info).
    
  4. To preserve the attribute value, please refer [here](./add_new_attribute.md#preserve-attribute).
    
### Non-HWP's Attribute:

The attribute which is consumed by the platform (openBMC apps and openPOWER Hostboot) to init and boot the POWER server is
called a non-hwp attribute.

  1. Add attribute definition (information about the type and other fields) based on needs in
     [attributes definition xml file](../data/p10/attribute_types_obmc.xml).
     
     **Note**: attribute definition ***should be in MRW format***. Please refer 
     [here](https://github.com/open-power/common-mrw-xml/blob/master/attribute_types_mrw.xml).
     
  2. Add attribute information for required target based on needs in
     [target definition xml file](../data/p10/target_types_obmc.xml).
    
     1. If the target is already present then add attribute information alone in xml as like other attributes
        (default value also can give).
     
     2. If the required attribute is want to add to more than one target then add attribute information into all require targets. Targets attributes are added based on ***inheritance approach*** (each target will have
        `parent` tag) so, adding attribute into parent target will apply to all its child targets.
     
     3. If the target is not present then add it as a new target (please refer to the format of the existing target definition in xml file).
        Also, please check whether that target is present in the required files by looking
        [here](./add_new_target.md#add-new-target).
       
  3. Add the attribute name into
     [attribute filter list](../data/p10/filter_AttributesList.lsv)
     to get only required attributes from attributes xml file. Please refer format in attribute filter list to add i.e.
     prefix should be `CA:` along with attribute name (e.g. `CA:ATTR_NAME`).

  4. To preserve the attribute value, please refer [here](./add_new_attribute.md#preserve-attribute).

### Preserve Attribute:

The service processor platform may be need to preserve the device tree attributes value in the different scenarios based on the needs.

  1. Add the attribute name in the [preserved attributes list](../data/p10/preserved_attrs_list) if wants to preserve during the service package update (aka, code update).
   
  2. Add the attribute name in the [reinit attributes list](../data/p10/reinit_devtree_attrs_list) if wants to reinit the attribute value during the cold boot based on the previous boot attribute value.
