#!/bin/sh
# SPDX-License-Identifier: Apache-2.0

# This shell script is used to generate either attribute header or DTS file
# for a given system.
#
# External repos:
#
#   <system>-xml.git (e.g. rainier-xml.git)
#   ekb.git
#
# Environmental variables:
#
#  SYSTEM_NAME    - name of the system (e.g. rainier)
#  TARGET_PROC    - target processor (e.g. p10)
#  SYSTEM_MRW_XML - system specific MRW xml (e.g. Raininer-2U-MRW.xml)
#  EKB            - path to target specific EKB
#

set -e

usage ()
{
    echo "Usage: $0 header|dts|infodb <filename>"
    exit 1
}

check_var ()
{
    var="$1"
    eval value=\$${var}
    if [ -z "$value" ] ; then
        echo "$var is not defined"
        exit 1
    fi
}

debug ()
{
    if [ -n "${DEBUG}" ] ; then
        echo $@
    fi
}

# Ensure required environment variables are defined
check_var SYSTEM_NAME
check_var TARGET_PROC
check_var SYSTEM_MRW_XML
check_var EKB

if [ $# -ne 2 ] ; then
    usage
fi

if [ "$1" != "header" -a "$1" != "dts" -a "$1" != "infodb" ] ; then
    usage
fi

filetype="$1"
outfile="$2"

out_dir=$(dirname "$outfile")
tmp_dir="$out_dir/tmp"
rm -rf "$tmp_dir"
mkdir -p "$tmp_dir"

script_dir=$(dirname "$0")
src_dir=$(dirname "$script_dir")
data_dir="$src_dir/data/$TARGET_PROC"

filter_attr="$data_dir/filter_AttributesList.lsv"
filter_target="$data_dir/filter_TargetsList.lsv"

# Used to add local path into perl lib enviroment varible
# for look up the local perl modules
export PERL5LIB="$script_dir"

# Step 1:
#      Processing system xml to get all required targets and attributes
#      for device tree hierarchy and platform specific system
#      targets processing

debug "Step 1: Processing system xml " $(basename "$SYSTEM_MRW_XML")
"$script_dir/processMrw.pl" \
    -x "$SYSTEM_MRW_XML" \
    -b bmc \
    -o "$tmp_dir/${SYSTEM_NAME}_bmc_mrw.xml"

# Step 2:
#      Filtering system mrw xml which is generated from step1
#      by using system specific filter file

debug "Step 2: Filtering processed system xml " $(basename "$SYSTEM_MRW_XML")
"$script_dir/filterXML.pl" \
    --inXML "$tmp_dir/${SYSTEM_NAME}_bmc_mrw.xml" \
    --outXML "$tmp_dir/${SYSTEM_NAME}_bmc_mrw_filtered.xml" \
    --filterAttrsFile "$filter_attr" \
    --filterTgtsFile "$filter_target" \
    --filterType systemXML

# Step 3:
#      Converting required ekb attributes xml into mrw attributes and targets
#      type xml format, because ekb and mrw attributes xml files are
#      different format, so it will be difficult to process xml's to
#      generate device tree blob file. So splitting ekb xml

debug "Step 3: Splitting EKB HWPs required attributes xml into MRW Targets and Attributes type xml"
"$script_dir/processEkbAttrsXML.pl" \
    --inEkbAttrsXMLPath ${EKB} \
    --inEkbAttrsXMLFilesList "$data_dir/reqEkbAttrsXmlFileList.lsv" \
    --filterAttrsFile "$filter_attr" \
    --fapiTgtNameMap "$data_dir/FAPITargetsNameMapList.lsv" \
    --outEkbAttrsXML "$tmp_dir/attribute_types_ekb.xml" \
    --outEkbTgtsXML "$tmp_dir/target_types_ekb.xml"

# Step 4:
#      Merging FAPI and Non-FAPI attributes type xml into single
#      xml file for making process simple

debug "Step 4: Merging FAPI and Non-FAPI attributes definition..."
"$script_dir/mergeMRWFormatXml.sh" \
    "$tmp_dir/attribute_types_ekb.xml" \
    "$data_dir/attribute_types_obmc.xml" \
    > "$tmp_dir/attribute_types_final.xml"

# Step 5:
#      Mapping custom targets parent attributes into platform xml file
#      for it child targets

debug "Step 5: Mapping custom targets parent attributes into platform xml file"
"$script_dir/mapTgtsAttrs.pl" \
    --fromTgtXml "$data_dir/target_types_obmc.xml" \
    --filterTgtList "$filter_target" \
    --outXml "$tmp_dir/target_types_obmc_withItsParentAttrs.xml" \
    --tgtXMLType customTgt

# Step 6:
#      Mapping generated ekb targets attributes into platform xml file

debug "Step 6: Mapping FAPI Targets along with attributes into bmc plat file"
"$script_dir/mapTgtsAttrs.pl" \
    --fromTgtXml "$tmp_dir/target_types_ekb.xml" \
    --toTgtXml "$tmp_dir/target_types_obmc_withItsParentAttrs.xml" \
    --outXml "$tmp_dir/target_types_final.xml" \
    --tgtXMLType ekbTgt

# Step 7:
#      Merging all following generated xml files into single for
#      creating intermediate xml file
#            - System xml
#            - target_types_final.xml
#            - attribute_types_final.xml

debug "Step 7: Getting intermediate xml by merging system, target and attributes types xml files"
"$script_dir/mergeMRWFormatXml.sh" \
    "$tmp_dir/${SYSTEM_NAME}_bmc_mrw_filtered.xml" \
    "$tmp_dir/target_types_final.xml" \
    "$tmp_dir/attribute_types_final.xml" \
    > "$tmp_dir/intermediate.xml"

if [ "$filetype" = "header" ] ; then

# Step 8: Generating header file from intermediate xml which is contains
#         MRW, FAPI and Non-FAPI targets/attributes.
#
#         The generated header file will contains meta data i.e
#          - data type: to declare attribute variable
#          - macros: to set/get attributes value from device tree
#          - spec: for api purpose to take care endianess
#          - element count: attribute value element count

    debug "Step 8: Generating header file"
    "$script_dir/genAttrsHeaderFile.pl" \
        --inXML "$tmp_dir/attribute_types_final.xml" \
        --outAHFile "$outfile"

elif [ "$filetype" = "infodb" ] ; then

# Step 8: Generating attributes info database which is contains
#         MRW, FAPI and Non-FAPI targets/attributes and generated info db
#         can use for attribute tool to read|write attributes by using
#         power system device tree.

    debug "Step 8: Generating info db file"
    "$script_dir/genAttrsInfoDB.pl" \
        --inXML "$tmp_dir/intermediate.xml" \
        --outInfoDB "$outfile"

else

# Step 8: Generating device tree structure (dts) file to generate DTB
#
#         The generated dts file will contain targets and its attributes as a
#         device tree format, so dtc compiler can use to get dtb file

    debug "Step 8: Generating device tree structure file"
    "$script_dir/genDTS.pl" \
        --inXML "$tmp_dir/intermediate.xml" \
        --pdbgMapFile "$data_dir/pdbg_compatible_propMapping.lsv" \
        --outDTS "$outfile"
fi
