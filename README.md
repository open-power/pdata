# pdata

pdata provides tools and libraries to manage the power system device tree. Also, it contains infrastructure to generate power
system device tree and meta-data based on given processor-specific targets and attributes details (the attribute data will be
picked from given system mrw xml).

## Build Info and Details 

### attributes tool

  The attributes tool is used to manage power system device tree. Please refer
  [here](./docs/pdata_deliverables_usage.md#attributes-tool) to get more details.
  
  ***To build:***
  ```
  autoreconf -i
  ./configure CHIP=p10
  make
  ```
  - Use the `CHIP` configuration option to build with the required target processor.
  
### Auto-generate files

Tools for auto-generate the meta-data and power system device tree are written in Perl and those tools are
dependent with following standard Perl modules so, please install in your build machine.
    
**Dependent Perl modules list:**
    
  - XML::LibXML
  - XML::Simple
  - Getopt::Long
  - File::Basename
  - Class::Struct
  - Digest::MD5
  - Data::Dumper
  - Math::BigInt
  - Carp
  - List::Util
  - XML::Parser

### Meta-Data:

  Meta-Data are required to read or write attributes value based on given target from power system cec device tree.
  So, The below meta-data files are automatically generated by pdata.
  
  - **attributes_info.db** - Please refer [here](./docs/pdata_deliverables_usage.md#attributes_infodb) to get more details.
    
  - **attributes_info.H** - Please refer [here](./docs/pdata_deliverables_usage.md#attributes_infoh) to get more details.

  ***To build:***
  ```
  autoreconf -i
  ./configure --enable-gen_attrsinfo CHIP=p10
  EKB=/path/to/pub-ekb make
  ```
  - Use the `CHIP` configuration option to build with the required target processor.
  
  - Use the `EKB` environment variable to pass cloned pub-ekb repository path to get hwp's attributes xml files.
  
### POWER system cec device tree:

  The POWER system cec device tree is used to initialize and boot the POWER server.
  Please refer [here](./docs/pdata_deliverables_usage.md#power-system-cec-device-tree) to get more details.

  ***To build:***
  ```
  autoreconf -i
  ./configure --enable-gen-dynamicdt CHIP=p10
  EKB=/path/to/pub-ekb SYSTEMS_MRW_XML=/path/to/rainier-xml/Rainier-2U-MRW.xml make
  ```
  - Use the `CHIP` configuration option to build with the required target processor.
  
  - Use the `EKB` environment variable to pass cloned pub-ekb repository path to get hwp's attributes xml files.
  
  - Use the `SYSTEMS_MRW_XML` environment variable to pass cloned system mrw xml file path to get system-specific attributes data.

    - The pdata tool will support to generate multiple power system device tree which is all built based on same
      IBM POWER processor. So, `SYSTEMS_MRW_XML` can be used to pass multiple systems mrw xml.
      
      E.g.: `SYSTEMS_MRW_XML="/path/to/rainier-xml/Rainier-2U-MRW.xml /path/to/rainier-xml/Rainier-4U-MRW.xml"`

### Other References:
  - To add a new ***attribute*** into meta-data and power system device tree, please refer
    [here](./docs/add_new_attribute.md#add-new-attribute).

  - To add new ***target*** into meta-data and power system device tree, please refer
    [here](./docs/add_new_target.md#add-new-target).

  - To add a new ***POWER processor*** specific meta-data and power system device tree, please refer
    [here](./docs/new_power_proc_cec_devtree.md#generate-cec-device-tree-for-new-power-processor).

### Support not in place:
- Currently, pdata won't support adding new ***system*** specific attribute or target so, if required then
  need to add support in tools.

- Currently the meta-data and power system device tree generation are based on ***filter list*** 
  ([attributs](./data/p10/filter_AttributesList.lsv) and [targets](./data/p10/filter_TargetsList.lsv)) so, need to add support 
  in tools for ***non-filter*** based generation.

