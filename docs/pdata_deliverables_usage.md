# pdata deliverables usage

pdata provides below tools and libraries to consume by application or end-user for use power system
cec (central electronics complex) device tree to init and boot POWER server.

## attributes tool

The attributes tool is used to manage the power system cec device tree by using the below subcommands.

- **read**: Used to read attribute for a given target (target can be Cronus target format or device tree target (node) path).

- **write**: Used to write given attribute value into power system cec device tree based on a given target.

- **translate**: Used to translate given Cronus target name into device tree target path and vice versa.

- **export**: Used to export the entire device tree to get all targets and attributes information that is present
in the device tree.

- **import**: Used to update multiple attributes value into device tree by modifying the exported device tree data.

**Note:** 
- This tool will expect attributes info-db (meta-data) i.e `attributes_info.db` and device tree.
- `PDBG_DTB` environment variable or `<dtb>` option can be use to pass device tree file path.
- `PDATA_INFODB` environment variable or `<infodb>` option can be use to pass attributes info db (database about attributes).

## Meta Data

### attributes_info.db
  
  The attributes_info.db file is used to maintain meta-data (data type, the array of dimension, enum fields, and specification
  of complex type attributes) about attributes and its associated targets list.
  For more details, please refer [here](../INFODB).
    
  The generated attributes infodb can be consumed by the `attributes` tool through command line parameter `<infodb>` or
  the environment variable `PDATA_INFODB` to manage the power system device tree for reading |write attributes.
    
### attributes_info.H
  
  The attributes_info.H header file is used to maintain meta-data (data type, the array of dimension, enum fields and
  specification of complex type attributes) about attributes and it will consume by application.
    
  Meta-data are required to read|write attributes for required targets from the power system device tree and
  those meta-data are defined in many XML's
  ([hwp's XML's](https://github.com/open-power/pub-ekb/tree/main-p10/chips/p10/procedures/xml/attribute_info) and
  [platform xml's](../data/p10)) files so, the pdata tool will generate this header file
  with those attributes meta-data by consuming those xml's files and additionally it will contain two macro's
  `DT_GET_PROP` and `DT_SET_PROP`, it will be expanded with meta-data to call `libdt-api` get|setProperty API's based on
  given attribute name. Also, it will contain attribute data type based on xml's data so, the application can declare buffer
  easily without worrying actual type.
    
### libdt-api

  It used to read|write target attribute from power system device tree.
  This library is just a wrapper to call attributes get|set api's from [libpdbg](https://github.com/open-power/pdbg) because,
  libpdbg providing different set of attribute get|set api's (pdbg_target_set|get_attribute, 
  pdbg_target_set|get_attribute_packed and pdbg_set|get_target_property). So, this library just will give single
  get|setProperty api for use attributes from power system device tree and internally it will call respective libpdbg 
  attribute get|set api based on given attribute data type.
  
  Mainly, this library API is used in macro's i.e. `DT_GET_PROP` and `DT_SET_PROP` from `attributes_info.H` to abstract
  attributes usage without worrying about meta-data for application.
  
  E.g.: **foo.cpp**
  ```    
  #include "attributes_info.H"
    
  // attribute name "ATTR_PHYS_DEV_PATH
    
  ATTR_PHYS_DEV_PATH_Type physStringPath;
    
  // target - require target to read attribute and
  // the target can be picked from the power system device tree by using the respective pdbg API
  // below one is just dummy uninitialized pdbg target object
  struct pdbg_target target;
    
  // To read attribute
  if (DT_GET_PROP(ATTR_PHYS_DEV_PATH, target, physStringPath))
  {
    // failure
  }
    
  // Update physStringPath buffer with required value
   
  // To write attribute
  if (DT_SET_PROP(ATTR_PHYS_DEV_PATH, target, physStringPath))
  {
    // failure
  }
  ```
  Simply, you can imagine abstracting the attributes used with macros without worrying about meta-data on the application side.
    
## POWER System CEC device tree

  The POWER system device tree is used between various software components (openBMC apps and Hostboot) to initialize and
  boot the POWER server.
  
  The generated POWER system device tree will contain the required CEC (Central Electronics Complex) targets (HW unit) details 
  in the form of standard [device tree](https://www.devicetree.org/) but, the address translation will be based on `index` 
  property which is available for all targets (node) due to hwp's address translation. Also, node name and compatible property will be based on [pdbg](https://github.com/open-power/pdbg) expectation because using libpdbg as backend layer for use power system cec device tree.

