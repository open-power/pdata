#!/usr/bin/env perl

#################################################################################
#
#   * This perl script generate device tree structure from intermediate xml
#     which is contain MRW, FAPI and Non-FAPI targets/attributes.
#   * This perl script also support to prepare dts with
#     required targets and attributes.
#
#################################################################################

use File::Basename;
use XML::LibXML;
use Class::Struct;
use Getopt::Long;
use strict;

my $currDir = dirname($0);
my $topSrcDir = dirname($currDir);
require "$currDir/parseIntermediateXMLUtils.pl";

# Global variables list begin

# To mention this tool name if unsupported attribute data type present in xml
my $tool = basename($0);
# To store commandline arguments
my $inXMLFile;
my $dtsFilename;
my $filterAttrsFile;
my $filterTgtsFile;
my $pdbgMapFile;
my $help;
my $myVerbose;

# To store xml data
my %attributeDefList;
my %fapiTargetList;
my %mrwTargetList;
my %pdbgCompPropMapList;
# Used for dts (device tree structure) generation
my %headOfDTree;
my $dtsFHandle;

my $attrPrefix = "ATTR_";

struct Node => {
    nodeName        => '$',
    childNodes      => '%',
    # DTS Property
    compatible      => '$',
    index           => '$',
    attributeList   => '%',
};

# Process commandline options

GetOptions( "inXML:s"           => \$inXMLFile,
            "outDTS:s"          => \$dtsFilename,
            "filterAttrsFile:s" => \$filterAttrsFile,
            "filterTgtsFile:s"  => \$filterTgtsFile,
            "pdbgMapFile:s"   => \$pdbgMapFile,
            "help"            => \$help,
            "verbose=s",      => \$myVerbose
          );

if ( $help )
{
    printUsage();
    exit 0;
}
if ( $inXMLFile eq "" )
{
    print "--inXML is required with xml file which should be in MRW format xml with mrw, fapia and non-fapi attributes to generate dts file\n";
    exit 1;
}
if ( $dtsFilename eq "")
{
    print "--outDTS is required with dts file name to store generated targets with attributes in dts format\n";
    exit 1;
}
if ( ($dtsFilename ne "") and !($dtsFilename =~ m/\.dts/ ))
{
    print "The given dts file name \"$dtsFilename\" extension is not .dts\n";
    exit 1;
}
if ( $pdbgMapFile eq "")
{
    print "--pdbgMapFile is required with mrw target type name with respective pdbg compatible property name to store into dts\n";
    exit 1;
}

# Start fomr main function
main();

###############################
#      Subroutine Begin       #
###############################

sub printUsage
{
    print "
Description:

    *   This tool will generate device tree structure from intermediate xml
        which is contain MRW, FAPI and Non-FAPI targets/attributes.
    *   This tool also support to prepare dts with required
        targets and attributes.\n" if $help;

    print "
Usage of $tool:

    -i|--inXML          : [M] :  Used to give intermediate xml name which contain MRW and FAPI
                                 Targets with associated attributes.
                                 E.g.: --inXML <xml_file.xml>

    -o|--outDTS         : [M] :  Used to give device tree structure file to stored generated dts                                 data.
                                 E.g.: --outDTS <dts_file.dts>

    -p|--pdbgFile       : [M] :  Used to pass pdbg compatible property map file name
                                 to map mrw target type in compatible proberty field
                                 E.g.: --pdbgFile <pdbg_compPropList.lsv>

    -f|--filterAttrsFile: [O] :  Used to give required attributes list in lsv file.
                                 E.g. : --filterAttrsFile <systemName_FilterAttrsList.lsv>

    -f|--filterTgtsFile : [O] :  Used to give required targets list in lsv file.
                                 E.g. : --filterTgtsFile <systemName_FilterTgtsList.lsv>

    -v|--verbose        : [O] :  Use to print debug information
                                 To print different level of log use following format
                                 -v|--verbose A,C,E,W,I
                                 A - All, C - CRITICAL, E - ERROR, W - WARNING, I - INFO

    -h|--help           : [O] :  To print the tool usage

";
}

sub main
{
    initVerbose($myVerbose);
    init();
    mergeFAPITargetAttrsIntoMRWIfFound();
    prepareDeviceTreeHierarchy();
    createDTSFile();
}

sub init
{

    my @ret = getTargetsData($inXMLFile, $filterTgtsFile);
    %mrwTargetList = %{$ret[0]};
    %fapiTargetList = %{$ret[1]};
    %attributeDefList = getAttrsDef( $inXMLFile, $filterAttrsFile );
    my $mrwSize = keys %mrwTargetList;
    my $fapiSize = keys %fapiTargetList;
    my $attrsSize = keys %attributeDefList;
    print "INFO: mrwTgtSize: $mrwSize fapiTgtSize: $fapiSize attrsSize: $attrsSize\n" if isVerboseReq('I');
    %pdbgCompPropMapList = getRequiredTgtsPdbgCompPropMapList ( $pdbgMapFile );
    my $pdbgSize = keys %pdbgCompPropMapList;
}

sub mergeFAPITargetAttrsIntoMRWIfFound
{
    foreach my $FAPITargetID ( keys %fapiTargetList )
    {
        my $found = 0;
        foreach my $MRWTargetID ( keys %mrwTargetList )
        {
            if($FAPITargetID eq $mrwTargetList{$MRWTargetID} -> MRWTarget::targetType)
            {
                $found = 1;
                my $mrwTarget = $mrwTargetList{$MRWTargetID};
                my %updMRWTgtAttrsList = %{$mrwTarget->targetAttrList};
                my %fapiTgtAttrsList = %{$fapiTargetList{$FAPITargetID} -> FAPITarget::targetAttrList};

                foreach my $fapiAttr ( sort ( keys %fapiTgtAttrsList) )
                {
                    if( exists $updMRWTgtAttrsList{$fapiAttr} )
                    {
                        if ( $updMRWTgtAttrsList{$fapiAttr}-> AttributeData::valueDataType eq "")
                        {
                            $updMRWTgtAttrsList{$fapiAttr} = $fapiTgtAttrsList{$fapiAttr};
                        }
                    }
                    else
                    {
                        $updMRWTgtAttrsList{$fapiAttr} = $fapiTgtAttrsList{$fapiAttr};
                    }
                }
                $mrwTarget->targetAttrList(\%updMRWTgtAttrsList);
            }
        }

        if($found eq 1)
        {
            print "INFO: FAPI target: $FAPITargetID merged with MRW target. Hence removing element from FAPI targets list\n" if isVerboseReq('I');
        }
    }
}

sub prepareDeviceTreeHierarchy
{
    my %nonPervTgtsList;
    my %omiPervPath;

    # Prepare for MRW targets
    foreach my $MRWTargetID ( sort (keys %mrwTargetList))
    {
        # Ignoring if dimm, ocmb and memport targets because those targets should come under omi and omi is pervasive target
        # but ignoring targets are not pervasive targets. Those targets device tree hierarchy will prepare based on
        # omi pervasive path which is prepared in this for loop
        if ( index( $mrwTargetList{$MRWTargetID}->targetType, "dimm") != -1 or
             index( $mrwTargetList{$MRWTargetID}->targetType, "chip-ocmb") != -1 or
             index( $mrwTargetList{$MRWTargetID}->targetType, "unit-mem_port") != -1 or
             index( $mrwTargetList{$MRWTargetID}->targetType, "chip-vreg-generic") != -1
           )
        {
            $nonPervTgtsList{$MRWTargetID} = $mrwTargetList{$MRWTargetID};
            next;
        }

        if ( !exists ${$mrwTargetList{$MRWTargetID}->targetAttrList}{'PHYS_PATH'} )
        {
            print "CRITICAL: \"PHYS_PATH\" attribute is not found for MRW Target : \"$MRWTargetID\". So Ignoring\n" if isVerboseReq('C');
            next;
        }
        my $physPath = ${$mrwTargetList{$MRWTargetID}->targetAttrList}{'PHYS_PATH'}->value;

        my $finalPath = $physPath;
        my $pervasivePath;
        # If parvasive path found for a particular target then considering that path should beto rearrange.
        # Because FAPI targets are modelled based on pervasive targets
        if ( exists ${$mrwTargetList{$MRWTargetID}->targetAttrList}{'PARENT_PERVASIVE'} )
        {
            # E.g. :
            # $pervasivePath = "physical:sys-0/node-0/proc-0/perv-32";
            # $physPath = "physical:sys-0/node-0/proc-0/eq-0/fc-0/core-0";
            # $finalPath => physical:sys-0/node-0/proc-0/perv-32/eq-0/fc-0/core-0

            $pervasivePath = ${$mrwTargetList{$MRWTargetID}->targetAttrList}{'PARENT_PERVASIVE'}->value;
            $finalPath = $pervasivePath.substr($physPath, rindex($pervasivePath, "/"), length($physPath) - rindex($pervasivePath, "/") );
        }

        # Adding pervasive path for "unit-fc-*" targets based on CHIPLET_ID attribute,
        # beause to avoid dupilcate device tree hierarchy creation.
        # TODO: Need to revisit for EX targets too if required some other system
        # TODO: Index hard code value (7/6) for proc path need to revisit, it will mess if proc sequence id more than one digit
        if( index($mrwTargetList{$MRWTargetID}->targetType, "unit-fc-") != -1 )
        {
            if ( !exists ${$mrwTargetList{$MRWTargetID}->targetAttrList}{'CHIPLET_ID'} )
            {
                print "CRITICAL: \"CHIPLET_ID\" attribute is not found for MRW Target : \"$MRWTargetID\". Ignoring otherwise perv path will be duplicate" if isVerboseReq('C');
                next;
            }
            my $chipletID = ${$mrwTargetList{$MRWTargetID}->targetAttrList}{'CHIPLET_ID'}->value;
            $finalPath = substr($physPath, 0, index($physPath, 'proc') + 7)."perv-".hex($chipletID).substr($physPath, index($physPath, 'proc') + 6);
        }

        # If target path is contain proc then add pib target after proc target path
        # for scom address translation as per pdbg expectation
        # TODO: Index hard code value (7/6) for proc path need to revisit, it will mess if proc sequence id more than one digit
        if( ( index($finalPath, "proc") != -1 )  and ( index($mrwTargetList{$MRWTargetID}->targetType, "chip-processor") == -1 ) )
        {
            $finalPath = substr($finalPath, 0, index($finalPath, 'proc') + 7)."pib".substr($finalPath, index($finalPath, 'proc') + 6, length($finalPath) - ( index($finalPath, 'proc') + 6) ) ;
        }
        # Getting path value alone by ignoring "physical:"
        my @hash = split(/\//, substr($finalPath, index($finalPath, ':') + 1));
        processTargetPath($MRWTargetID, \@hash);
        # Create omi pervasive path list for use to dimm. ocmb and memport targets
        $omiPervPath{$MRWTargetID} = $finalPath if index( $mrwTargetList{$MRWTargetID}->targetType, "unit-omi") != -1;

        # Add FSI target manually under proc target if MRW target is chip-processor.
        addFSITarget($MRWTargetID, $finalPath) if index($mrwTargetList{$MRWTargetID}->targetType, "chip-processor") != -1;
    }

    # Prepare device tree hierarchy for non pervasive targets which need to come under proc/.../omi target hierarchy
    prepareDeviceTreeHierarchyForNonPervTgts(\%nonPervTgtsList, \%omiPervPath);
}

sub addFSITarget
{
    my $procTgtId = $_[0];
    my $procPath = $_[1];

    my $fsiPath = $procPath."/fsi";
    my $fsiTgtId = $procTgtId."fsi";
    my @hash = split(/\//, substr($fsiPath, index($fsiPath, ':') + 1));
    my $fsiTgt = MRWTarget->new();
    $fsiTgt->targetType("unit-fsi");
    $fsiTgt->targetAttrList({});
    $mrwTargetList{$fsiTgtId} = $fsiTgt;
    processTargetPath($fsiTgtId, \@hash);
}

sub prepareDeviceTreeHierarchyForNonPervTgts
{
    my %nonPervTgtsList = %{$_[0]};
    my %omiTgtPervPathList = %{$_[1]};

    foreach my $tgtID ( sort (keys %nonPervTgtsList))
    {
        my %targetAttrList = %{$nonPervTgtsList{$tgtID}->targetAttrList};
        if ( !exists $targetAttrList{"AFFINITY_PATH"} )
        {
            print "CRITICAL: \"AFFINITY_PATH\" attribute is not found for MRW Target : \"$tgtID\". So Ignoring to add not pervasive
            under proc target\n" if isVerboseReq('C');
            next;
        }

        my $tgtPath = $targetAttrList{'AFFINITY_PATH'}->value;
        # First Ignoring affinity: from path then getting till omi path to covert as omi id for getting omi perv path
        my $omiTgtId = substr( substr($tgtPath, index($tgtPath, ':') + 1), 0, index($tgtPath, 'omi-') - 4);
        $omiTgtId =~ s/[-\/]//g;

        if ( !exists $omiTgtPervPathList{$omiTgtId} )
        {
            print "CRITICAL: \" OMI target id \"$omiTgtId\" is not found to get OMI parv path\n" if isVerboseReq('C');
            next;
        }

        my $omiTgtPervPath = $omiTgtPervPathList{$omiTgtId};

        # Getting non pervasive target path alone from AFFINITY_PATH
        my $nonPervTgtPath = substr($tgtPath, index($tgtPath, 'omi-') + 5, length($tgtPath) - ( index($tgtPath, 'omi-') + 5) );
        my $nonPervTgtPathWithOMITgtPath = $omiTgtPervPath.$nonPervTgtPath;

        # Getting path value alone by ignoring "physical:"
        my @hash = split(/\//, substr($nonPervTgtPathWithOMITgtPath, index($nonPervTgtPathWithOMITgtPath, ':') + 1));
        processTargetPath($tgtID, \@hash);
    }
}

sub processTargetPath
{
    my $TargetID = $_[0];
    my @splittedPath = @{$_[1]};
    # If Any intermediate node is not found from given path then create empty node and continue
    my $refDTreeNodesList = \%headOfDTree;
    my $lastNode;
    foreach my $key ( @splittedPath )
    {
        if ( !exists ( $refDTreeNodesList -> { $key } ) )
        {
            $refDTreeNodesList -> { $key } = createNode($key);
        }
        $lastNode = $refDTreeNodesList -> {$key};
        $refDTreeNodesList = \%{ $refDTreeNodesList -> { $key } -> Node::childNodes } ;
    }
    # Assign node member values for the last node based on last key(path name) in thn given path
    $lastNode->compatible($mrwTargetList{$TargetID} -> MRWTarget::targetType);
    $lastNode->attributeList($mrwTargetList{$TargetID} -> MRWTarget::targetAttrList);
}

sub createNode
{
	my $node = Node->new();
	$node->nodeName($_[0]);
    $node->index(substr($node->nodeName, index($node->nodeName, '-') + 1));
    $node->compatible("");
    $node->attributeList({});
	$node->childNodes({});
	return ($node);
}

sub createDTSFile
{
	open $dtsFHandle, '>', $dtsFilename or die "Could not open $dtsFilename: \"$!\"";

	# Add DTS version and staring brace
	print {$dtsFHandle} "/dts-v1/;\n\n/ {\n";

	# Add all nodes into DTS file
    addNodesIntoDTSFile(\%headOfDTree);

	# Add end brace to DTS
	print {$dtsFHandle} "};";

    close $dtsFHandle;
    my $baseDtsFileName = basename($dtsFilename);
    print "\nDevice tree structure file \"$baseDtsFileName\" is created successfully...\n";
}

sub addNodesIntoDTSFile
{
	my $nodes = $_[0];
    my $dontAddNode = 0;
    my $addCompProp = 1;

    foreach my $key ( sort (keys %{$nodes}))
    {
        # Not adding sys and node target as per pdbg expectaion
        if ( $key =~ m/sys/ or $key =~ m/node/ )
        {
            $dontAddNode = 1;
        }

        # Not adding compatible property for sys and fsi as per pdbg expectaion
        if ( $key =~ m/sys/ or $key =~ m/fsi/ )
        {
            $addCompProp = 0;
        }

        my $nodeName = $nodes -> {$key} -> Node::nodeName;

        # Removing "-" in node name as per pdbg expectation
        $nodeName =~ s/-//g;

        print {$dtsFHandle} "\n$nodeName {\n" if $dontAddNode ne 1;
        my $index = $nodes -> {$key} -> Node::index;

        addTargetDataIntoDTSFile($nodes -> {$key} -> Node::compatible, $nodes -> {$key} -> Node::index, \%{$nodes -> {$key} -> Node::attributeList}, $dontAddNode, $addCompProp);
        addNodesIntoDTSFile($nodes -> {$key} -> Node::childNodes);
        print {$dtsFHandle} "};\n" if $dontAddNode ne 1;
    }
}

sub addTargetDataIntoDTSFile
{
    my $compatible = $_[0];
	my $nodeIndex = $_[1];
    my %attributeList = %{$_[2]};
    my $indexReqToAdd = $_[3];
    my $compPropReqAdd = $_[4];

    if ( $compPropReqAdd eq "1" )
    {
        # Add "compatible" property
        my $pdbgCompPropertyVal;
        if ( exists $pdbgCompPropMapList{$compatible} )
        {
            $pdbgCompPropertyVal = $pdbgCompPropMapList{$compatible};
            # Not adding double quotes if present, to support list of compatible property
            $pdbgCompPropertyVal = "\"".$pdbgCompPropertyVal."\"" if !( $pdbgCompPropertyVal =~ m/"/);
        }
        else # If not found using mrw target type name only
        {
            $pdbgCompPropertyVal = "\"".$compatible."\"";
        }

        print {$dtsFHandle} "compatible = $pdbgCompPropertyVal;\n" if $pdbgCompPropertyVal ne "\"\""; # not adding empty
    }
    # Add index
	my $nodeIndexHex = sprintf("<0x%02x>", $nodeIndex);
	print {$dtsFHandle} "index = $nodeIndexHex;\n" if $indexReqToAdd ne 1;

    # TODO Need to revisit adding same attributes into list of targets
    $attributeList{"HWAS_STATE"} = AttributeData->new() if ( (!exists $attributeList{"HWAS_STATE"}) and ($indexReqToAdd ne 1) );

    # Adding PHYS_DEV_PATH and PHYS_BIN_PATH attributes value by using PHYS_PATH attribute.
    # because, PHYS_PATH type is class which is not supported in device tree.
    if ( $compatible ne "unit-fsi" and $compatible ne "")
    {
        my $physicalPathVal = $attributeList{"PHYS_PATH"}->value;

        $attributeList{"PHYS_DEV_PATH"} = AttributeData->new();
        $attributeList{"PHYS_DEV_PATH"}->value($physicalPathVal);

        $attributeList{"PHYS_BIN_PATH"} = AttributeData->new();
        $attributeList{"PHYS_BIN_PATH"}->value(getBinaryFormatForPhysPath($physicalPathVal));
    }
    # Attributes
    foreach my $AttrID ( sort ( keys %attributeList ) )
    {
        my $attrType = getAttributeType($AttrID);
        if ( $attrType eq "")
        {
            next;
        }

        if ($attrType eq "simpleType")
        {
            my $attrVal = $attributeList{$AttrID} -> AttributeData::value;

            # TODO Need to revisit setting CHIP_UNIT_POS value from FAPI_POS
            # Added beacuse HWPs expecting CHIP_UNIT_POS value and its should come MRW
            # So once added in MRW need to remove
            $attrVal = $attributeList{"FAPI_POS"} -> AttributeData::value if $AttrID eq "CHIP_UNIT_POS";
            # Getting default value from attribute definition if not value is defined
            my $simpleType = $attributeDefList{$AttrID}-> AttributeDefinition::simpleType;
            my $simpleTypeDefault = $simpleType->default;
            $attrVal = $simpleTypeDefault if $attrVal eq "";

            my $isToolAddedDefVal = 0;
            if ( $attrVal eq "" )
            {
                $attrVal = 0;
                $isToolAddedDefVal = 1;
            }

            if( $simpleType->DataType eq "array")
            {
                my $eleCnt = 1;
                my @dims = split(/,/, $simpleType->arrayDimension);
                foreach my $dim (@dims)
                {
                    $eleCnt *= $dim;
                }

                my @arrayValues;
                if ( $simpleType->subType eq "string" )
                {
                    (@arrayValues) = $attrVal =~ m/"(.*?)"/g;
                }
                elsif( index($simpleType->subType, "enum_") != -1)
                {
                    # Resetting value as empty if tool is initialized to get enum value from enum definition
                    $attrVal = "" if $isToolAddedDefVal eq 1;
                    my $enumNames = $attrVal if $attrVal ne "";

                    # Getting default value from enum definition if value is not defined
                    my $enumDef = $simpleType->enumDefinition;
                    $enumNames = $enumDef->default if $enumNames eq "";

                    @arrayValues = split(/,/,$enumNames);
                    foreach my $enumName (@arrayValues)
                    {
                        # Getting enum value from enum name
                        my %tmpenumeratorList = %{$enumDef->enumeratorList};
                        my $enumVal = $tmpenumeratorList{$enumName};

                        # TODO need to remove setting value by tool if xml have values
                        $enumVal = 0 if $enumVal eq "";
                        # Replacing enum name with value
                        $enumName = $enumVal;
                    }
                }
                else
                {
                    @arrayValues = split(/,/,$attrVal);
                }

                # Adding 0 for non available value for array element count
                my $arrayValuesSize = @arrayValues;
                for ( my $i = $arrayValuesSize; $i < $eleCnt; $i++)
                {
                    push(@arrayValues, 0)
                }
                setDTSFormatValueForSimpleAttr($AttrID, $simpleType->DataType."_".$simpleType->subType, \@arrayValues, $eleCnt)
            }
            elsif ( $simpleType->DataType eq "int8_t" or $simpleType->DataType eq "int16_t" or $simpleType->DataType eq "int32_t"  or $simpleType->DataType eq "int64_t" or 
                $simpleType->DataType eq "uint8_t" or $simpleType->DataType eq "uint16_t" or $simpleType->DataType eq "uint32_t"  or $simpleType->DataType eq "uint64_t")
            {
                my @arrayValues;
                push(@arrayValues, $attrVal);
                setDTSFormatValueForSimpleAttr($AttrID, $simpleType->DataType, \@arrayValues, 1)
            }
            elsif ( $simpleType->DataType eq "enum")
            {
                # Resetting value as empty if tool is initialized to get enum value from enum definition
                $attrVal = "" if $isToolAddedDefVal eq 1;

                my $enumName = $attrVal if $attrVal ne "";

                # Getting default value from enum definition if value is not defined
                my $enumDef = $simpleType->enumDefinition;
                $enumName = $enumDef->default if $enumName eq "";

                my $enumVal;
                if ($enumName ne "0")
                {
                    # Getting enum value from enum name
                    $enumVal = getEnumVal(\@{$enumDef->enumeratorList}, $enumName);

                    # Setting first enum value as default value,
                    $enumVal = getEnumVal(\@{$enumDef->enumeratorList}, "") if $enumVal eq "";
                }
                else
                {
                    # FAPI (ekb) Enum attribute will may have initToZero tag
                    # so, default value will be 0
                    $enumVal = $enumName;
                }

                my @arrayValues;
                push(@arrayValues, $enumVal);
                setDTSFormatValueForSimpleAttr($AttrID, $simpleType->DataType."_".$simpleType->subType, \@arrayValues, 1)
            }
            elsif ( $simpleType->DataType eq "string")
            {
                my $property = $attrVal eq "" ? $AttrID : "$AttrID = \"$attrVal\"";
                my @arrayValues;
                push(@arrayValues, $attrVal);
                setDTSFormatValueForSimpleAttr($AttrID, $simpleType->DataType, \@arrayValues, 1)
            }
        }
        elsif ( $attrType eq "complexType" )
        {
            my $attrValue = $attributeList{$AttrID} -> AttributeData::value;

            if ($attrValue eq "")
            {
                my @ret = getSpecAndDefValForComplexTypeAttr(\@{$attributeDefList{$AttrID}->complexType->listOfComplexTypeFields}, $attributeDefList{$AttrID}->complexType->arrayDimension);
                $attrValue = @ret[1];
            }
            my $dtsFormatedVal = "[".$attrValue." ]";
            print {$dtsFHandle} "$attrPrefix$AttrID = $dtsFormatedVal;\n";
        }
    }
}

sub getAttributeType
{
    my $attrID = $_[0];
    my $attrType;

    if( exists $attributeDefList{$attrID})
    {
        $attrType = $attributeDefList{$attrID}-> AttributeDefinition::datatype;
    }
    else
    {
        print "ERROR: Attribute: $attrID is not found in attribute definition\n" if isVerboseReq('E');
    }

    return ($attrType);
}

sub setDTSFormatValueForSimpleAttr
{
    my $attrName = $_[0];
    my $type = $_[1];
    my @values = @{$_[2]};
    my $eleCnt = $_[3];

    # Getting actual type from input type value if type contains array or enum
    # E.g : enum_uint32_t => uint32 or array_enum_uint32_t  => uint32
    if ( $type =~ m/array/ or $type =~ m/enum/ )
    {
        $type = substr($type, 0, rindex($type, "_"));
        $type = substr($type, rindex($type, "_") + 1);
    }

    my $dtsFormatedVal;
    my $begFormatSym;
    my $valSize = @values;
    foreach my $pValue (@values)
    {
        if( $type =~ m/int8/ )
        {
            if ( $pValue =~ m/0x/ )
            {
                $pValue =~ s/0x//g;
                $pValue = "0".$pValue if length($pValue) == 1;
            }
            else
            {
                $pValue = sprintf("%02X", $pValue);
            }

            $begFormatSym = "[" if $begFormatSym eq "";
            $dtsFormatedVal .= $pValue;
            $dtsFormatedVal .= " " if $valSize > 1;
        }
        elsif( $type =~ m/int16/ )
        {
            if ( $pValue =~ m/0x/ )
            {
                $pValue =~ s/0x//g;
                $pValue = "0".$pValue if length($pValue) == 1;
            }
            else
            {
                $pValue = sprintf("%02X", $pValue);
            }

            my $fByte = ( $pValue >> 8 ) & 0x00FF;
            my $sByte = $pValue & 0x00FF;

            my $tmp = sprintf("%02d %02d", $fByte, $sByte);
            $begFormatSym = "[" if $begFormatSym eq "";
            $dtsFormatedVal .= $tmp;
            $dtsFormatedVal .= " " if $valSize > 1;
        }
        elsif( $type =~ m/int32/ )
        {
            $begFormatSym = "<" if $begFormatSym eq "";
            $dtsFormatedVal .= "$pValue";
            $dtsFormatedVal .= " " if $valSize > 1;
        }
        elsif( $type =~ m/int64/ )
        {
            $begFormatSym = "<" if $begFormatSym eq "";
            $dtsFormatedVal .= "$pValue";
            $dtsFormatedVal .= " " if $valSize > 1;
        }
        elsif( $type =~ m/string/ )
        {
           $dtsFormatedVal .= "\"$pValue\"";
           $dtsFormatedVal .= ", " if $valSize > 1;
        }
        else
        {
            # Unsupported type given
            return "";
        }
    }
    $dtsFormatedVal = "[ ".$dtsFormatedVal if $begFormatSym eq "[";
    $dtsFormatedVal .= " ]" if $begFormatSym eq "[";
    $dtsFormatedVal = "<".$dtsFormatedVal if $begFormatSym eq "<";
    $dtsFormatedVal .= ">" if $begFormatSym eq "<";
    print {$dtsFHandle} "$attrPrefix$attrName = $dtsFormatedVal;\n";
}

sub getEnumVal
{
    my @enumeratorList = @{$_[0]};
    my $enumName = $_[1];

    foreach my $enumPair ( @enumeratorList )
    {
        return $enumPair->[1] if $enumName eq $enumPair->[0];
    }
    # Return first enum value if not found for given enum or enum name is empty
    return $enumeratorList[0][1];
}

sub getBinaryFormatForPhysPath
{
    my $physPath = $_[0];

    my ($pathType, $path) = split(/:/, $physPath);
    my @pathElements = split(/\//, $path);
    my $pathElementsSize = @pathElements;

    # Reducing one from configured array size and dividing by 2 to get path element size
    my $configpathElementsSize = ($attributeDefList{"PHYS_BIN_PATH"}->simpleType->arrayDimension - 1) / 2;

    if ( $configpathElementsSize < $pathElementsSize )
    {
        print "CRITICAL: The max path element size of PHYS_BIN_PATH is $configpathElementsSize but the given path element size in PHYS_DEV_PATH attribute value[$physPath] is $pathElementsSize\n" if isVerboseReq('C');
        return "";
    }

    if ( $pathType eq "physical" )
    {
        $pathType = 2;
    }
    else
    {
        print "CRITICAL: Invalid path type[$pathType] in given path [$physPath]\n" if isVerboseReq('C');
        return "";
    }

    # Device tree is big endian format. so, storing path type and size of path elements values
    # as one byte in big endian format.
    my $pathtype_size = (0xF0 & ($pathType << 4)) + (0x0F & $pathElementsSize);

    my $convertedBinaryFormat = $pathtype_size.",";

    foreach my $pathEle ( @pathElements )
    {
        # splitting path element into type and instance. e.g: sys-0 => type = sys and instance = 0
        my @pathEleFields = split(/-/, $pathEle);
        my $found = 0;
        foreach my $tgtTypeEnum ( @{$attributeDefList{"TYPE"}->simpleType->enumDefinition->enumeratorList} )
        {
            if ( uc($pathEleFields[0]) eq $tgtTypeEnum->[0] )
            {
                $found = 1;
                # storing target type and its instance value as binary format.
                # e.g.: sys-0 => 00 00 => first "00" is enum value of sys and second "00" is hex value of 0
                $convertedBinaryFormat .= $tgtTypeEnum->[1].",".$pathEleFields[1].",";
            }
        }

        if ( $found eq 0 )
        {
            print "CRITICAL: The given path element type[$pathEleFields[0]] for path[$physPath] is not found in TYPE attibute enum list.\n" if isVerboseReq('C');
            return "";
        }
    }

    # Adding zero if the path elements size which is found in given PHYS_DEV_PATH
    # is less than the config size
    if ( $configpathElementsSize > $pathElementsSize )
    {
        for (my $i = $pathElementsSize; $i < $configpathElementsSize; $i++)
        {
            # for target type field
            $convertedBinaryFormat .= "0".",";
            # for target instance field
            $convertedBinaryFormat .= "0".",";
        }
    }

    return $convertedBinaryFormat;
}
###############################
#      Subroutine End         #
###############################
