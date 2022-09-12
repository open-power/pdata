# Add new target
 
The power system device tree will maintain two types of targets (HWP's (Hardware Procedure) and Non-HWP's target) to represent
hardware unit as target (node in device tree) based on needs to init and boot the POWER server. So, please refer respective
subsection for steps.

- To add ***hwp*** target, please refer
  [here](./add_new_target.md#hwps-target).
    
- To add ***non-hwp*** target, please refer
  [here](./add_new_target.md#non-hwps-target).
  
- To add target in the pdbg side, please refer
  [here](./add_new_target.md#pdbg-target).
  
**Note:** The steps will point to many files and a few will be POWER10 specific so, please refer and update the respective files
based on your requirement if you are looking for another POWER processor version.

### HWP's Target

  The target which is consumed by [pub-ekb](https://github.com/open-power/pub-ekb/tree/main-p10) hardware procedure to init hardware units is
  called hwp target.
  
  1. Add corresponding mrw target **id** for required hwp's target **type** in
     [hwp target name map list](../data/p10/FAPITargetsNameMapList.lsv).
     
     Please refer [here](https://github.com/open-power/common-mrw-xml) to get mrw target id value.
     ```
     # Format:- FAPITargetType:MRWTargetID
     
     TARGET_TYPE_SYSTEM:sys-sys-power10
     ```
  2. Add the corresponding pdbg **compatible property** for the required mrw target id in
     [pdbg compatible property list](../data/p10/pdbg_compatible_propMapping.lsv).
     
     Please refer [here](https://github.com/open-power/pdbg/blob/master/p10.dts.m4) to get pdbg compatible property value.
     
     If the pdbg compatible property value is not found then, please refer [here](./add_new_target.md#pdbg-target).

     ```
     # Format : MRWTargetID:PdbgCompatiblePropValue
     
     unit-core-power10:ibm,power10-core
     ```
     **Note:** The pdbg compatible property map file will support more than one compatible property for the same mrw target id.
     
     E.g.: `chip-processor-power10:"ibm,power-proc", "ibm,power10-proc"`
   
  3. Add mrw target id into
     [target filter list](../data/p10/filter_TargetsList.lsv) in new line.

  4. Add target name into [class_map](../libdtree/dtree_util.c) to read by using attribute tool.
  
### Non-HWP's Target

The target which is consumed by the platform (openBMC apps and openPOWER Hostboot) to init and boot the POWER server is called
non-hwp target.

  1. Add target definition in
     [target definition xml file](../data/p10/target_types_obmc.xml).
  
     **Note**:
     - target ***id*** should be same as mrw target id. Please refer
       [here](https://github.com/open-power/common-mrw-xml) to get mrw target id value.
     - target should have ***parent*** tag.
     - target also can have non-hwps attributes information (please refer
       [here](./add_new_attribute.md#non-hwps-attribute) to add if required).
     
       E.g.:
       ```
       <targetType>
          <attribute>
              <id>DISABLE_SECURITY</id>
          </attribute>
          <id>sys-sys-power10</id>
          <parent>sys</parent>
       </targetType>
       ```  
  
  2. Add corresponding pdbg **compatible property** for mrw target id in
     [pdbg compatible property list](../data/p10/pdbg_compatible_propMapping.lsv).
     
     Please refer [here](https://github.com/open-power/pdbg/blob/master/p10.dts.m4) to get pdbg compatible property value.
     
     If the pdbg compatible property value is not found then, please refer [here](./add_new_target.md#pdbg-target).
     
     ```
     # Format : MRWTargetID:PdbgCompatiblePropValue
     
     unit-core-power10:ibm,power10-core
     ```
     **Note:** The pdbg compatible property map file will support more than one compatible property for the same mrw target id.
     
     E.g.: `chip-processor-power10:"ibm,power-proc", "ibm,power10-proc"`
     
  3. Add mrw target id into
     [target filter list](../data/p10/filter_TargetsList.lsv) in new line.

  4. Add target name into [class_map](../libdtree/dtree_util.c) to read by using attribute tool.
 
 ### PDBG Target

The pdbg target need to be define in the [pdbg](https://github.com/open-power/pdbg) to use a new target in the device tree.

  - Add a hardware unit in the pdbg side. please refer the [sample](https://github.com/open-power/pdbg/commit/4784f86f62e39c6c31a59d0f0ae96e9a80b6e085).
  - Add a device tree node for the new target in the respective pdbg dts file. please refer the [sample](https://github.com/open-power/pdbg/commit/ae97f20eb8370dbd904345deae207626ee76c008).
  
  **Note:**
  - More [samples](https://github.com/open-power/pdbg/commits?author=rameshiyyar).
  - These samples does not include the translation code to add in the hardware unit, that helps to perfom the scom operation on the new target.Please check any of the existing scomable targets code in the pdbg.
