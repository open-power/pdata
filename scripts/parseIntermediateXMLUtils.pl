#!/usr/bin/env perl
# SPDX-License-Identifier: Apache-2.0

####################################################################
#                                                                  #
# This tool is used add common utility function to parse           #
# generated intermediate xml                                       #
#                                                                  #
####################################################################

use File::Basename;
my $currDir = dirname($0);
require "$currDir/utils.pl";

my $verbose = "NA";
sub initVerbose
{
    $verbose = $_[0];
}

# Preparing structure type for MRW attribute definition
# This can used for ekb xml if ekb xml converted into MRW format
use Class::Struct;

struct ComplexTypeField => {
    name        => '$',
    type        => '$',
    default     => '$',
    bits        => '$',
};

struct NativeType => {
    name        => '$',
    default     => '$',
};

struct EnumDefinition => {
    default             => '$',
    enumeratorList      => '@',
};

struct SimpleType => {
    DataType            => '$',
    default             => '$',
    arrayDimension      => '$',
    subType           => '$',
    enumId              => '$',
    stringSize          => '$',
    enumDefinition      => 'EnumDefinition',
};

struct AttributeDefinition => {
    datatype                    => '$',
    global                      => '$',
    hasStringConversion         => '$',
    hbOnly                      => '$',
    hwpfToAttrMap               => '@',
    ignoreEkb                   => '$',
    mrwRequired                 => '$',
    no_export                   => '$',
    persistency                 => '$',
    range                       => '@',
    readable                    => '$',
    virtual                     => '$',
    writeable                   => '$',
    listOfComplexTypeFields     => '@',
    nativeType                  => 'NativeType',
    simpleType                  => 'SimpleType',
};

sub isVerboseReq
{
    my $verboseLevel = $_[0];

    my $isReq = 0;
    if ( ( index($verbose, $verboseLevel ) > -1) or ( index($verbose, 'A') > -1 ) )
    {
        $isReq = 1;
    }

    return ( $isReq )
}

sub getAttrsDef
{
    my $inXMLFile = $_[0];
    my $filterFile = $_[1];

    my %attributeDefList;

    my $inXMLData = XML::LibXML->load_xml(location => $inXMLFile);

    if ( $filterFile ne "" )
    {
        my %reqAttrsList = getReqAllAttrsFilterList($filterFile);
        foreach my $rAttr ( sort ( keys %reqAttrsList))
        {
            my $attrDefPath = '/attributes/attribute/id[text()=\''.$rAttr.'\']/ancestor::attribute';

            foreach my $attrDefData ( $inXMLData->findnodes($attrDefPath) )
            {
                my @ret = parseAttributeDefinition($attrDefData, $inXMLData);
                $attributeDefList{$ret[0]} = $ret[1];
            }
        }
    }
    else
    {
        my $attrDefPath = '/attributes/attribute';
        for my $attrDefData ( $inXMLData->findnodes($attrDefPath) )
        {
            my @ret = parseAttributeDefinition($attrDefData, $inXMLData);
            $attributeDefList{$ret[0]} = $ret[1];
        }
    }

    return (%attributeDefList)
}

sub parseAttributeDefinition
{
    my $attrDef = $_[0];
    my $inXMLData = $_[1];

    my $attrID = $attrDef->findvalue('id');
    if ( $attrID eq "" )
    {
        print "CRITICAL: AttributeDef ID missing in xml\n" if isVerboseReq('C');
        last;
    }

    if ( exists $attributeDefList{$attrID} )
    {
        print "WARNING: AttributeDef ID: $attrID is already exists\n" if isVerboseReq('W');
        last;
    }

    my $attributeDefinition = AttributeDefinition->new(simpleType=> new SimpleType());
    my $global =  $attrDef->findvalue('global');
    if( $global ne "" )
    {
        $attributeDefinition->global($global);
    }

    my $hasStringConversion = $attrDef->findvalue('hasStringConversion');
    if( $hasStringConversion ne "")
    {
        $attributeDefinition->hasStringConversion($hasStringConversion);
    }

    my $hbOnly = $attrDef->findvalue('hbOnly');
    if( $hbOnly ne "" )
    {
        $attributeDefinition->hbOnly($hbOnly);
    }

    if( $attrDef->exists('hwpfToAttrMap') )
    {
        my $id = $attrDef->findvalue('hwpfToAttrMap/id');
        my $macro = $attrDef->findvalue('hwpfToAttrMap/macro');
        $attributeDefinition->hwpfToAttrMap($id, $macro);
    }
    my $ignoreEkb = $attrDef->findvalue('ignoreEkb');
    if( $ignoreEkb ne "")
    {
        $attributeDefinition->ignoreEkb($ignoreEkb);
    }

    my $mrwRequired = $attrDef->findvalue('mrwRequired');
    if ( $mrwRequired ne "")
    {
        $attributeDefinition->mrwRequired($mrwRequired);
    }

    my $no_export = $attrDef->findvalue('no_export');
    if ( $no_export ne "" )
    {
        $attributeDefinition->no_export($no_export);
    }

    my $persistency = $attrDef->findvalue('persistency');
    if ( $persistency ne "")
    {
        $attributeDefinition->persistency($persistency);
    }

    if ( $attrDef->exists('range') )
    {
        my $max = $attrDef->findvalue('range/max');
        my $min = $attrDef->findvalue('range/min');
        $attributeDefinition->range($max, $min);
    }

    my $virtual = $attrDef->findvalue('virtual');
    if ( $virtual ne "" )
    {
        $attributeDefinition->virtual($virtual);
    }

    if ( $attrDef->findnodes('readable')->size > 0)
    {
        $attributeDefinition->readable(1);
    }

    if ( $attrDef->findnodes('writeable')->size > 0)
    {
        $attributeDefinition->writeable(1);
    }

    if( $attrDef->exists('nativeType') )
    {
        $attributeDefinition->datatype("nativeType");

        my $nativeType = NativeType->new();
        $nativeType->name($attrDef->findvalue('nativeType/name'));
        $nativeType->default($attrDef->findvalue('nativeType/default'));

        $attributeDefinition->nativeType($nativeType);
    }
    elsif ( $attrDef->exists('complexType') )
    {
        $attributeDefinition->datatype("complexType");
        foreach my $field ( $attrDef->findnodes('complexType/field'))
        {
            my $ComplexTypeField = ComplexTypeField->new();
            $ComplexTypeField->name($field->findvalue('name'));
            $ComplexTypeField->type($field->findvalue('type'));
            $ComplexTypeField->default($field->findvalue('default'));
            $ComplexTypeField->bits($field->findvalue('bits'));

            push(@{$attributeDefinition->listOfComplexTypeFields}, $ComplexTypeField);
        }
    }
    elsif ( $attrDef->exists('simpleType') )
    {
        $attributeDefinition->datatype("simpleType");
        my $SimpleType = SimpleType->new();

        if( $attrDef->exists('simpleType/Target_t') )
        {
            $SimpleType->default($attrDef->findvalue('simpleType/Target_t/default'));
            $SimpleType->DataType("Target_t");
        }
        elsif( $attrDef->exists('simpleType/hbmutex'))
        {
            $SimpleType->default($attrDef->findvalue('simpleType/hbmutex/default'));
            $SimpleType->DataType("hbmutex");
        }
        elsif( $attrDef->exists('simpleType/hbrecursivemutex'))
        {
            $SimpleType->DataType("hbrecursivemutex");
        }
        else
        {
            my $type = "";
            if( $attrDef->exists('simpleType/array') )
            {
                $type = "array";
                $SimpleType->arrayDimension($attrDef->findvalue('simpleType/array'));
                my $strsize = $SimpleType->arrayDimension;
                $SimpleType->DataType("array");
            }
            elsif( $attrDef->exists('simpleType/enumeration') )
            {
                $type = "enum";
                $SimpleType->default($attrDef->findvalue('simpleType/enumeration/default'));
                $SimpleType->enumId($attrDef->findvalue('simpleType/enumeration/id'));
                $SimpleType->DataType($type);

                my $enumDefPath = '/attributes/enumerationType/id[text()=\''.$attrID.'\']/ancestor::enumerationType';
                my $enumDefData = $inXMLData->find($enumDefPath);
                if ( $enumDefData->size() > 0 )
                {
                    $SimpleType->enumDefinition(parseEnumerationTypes($enumDefData));
                }
            }

            my $primitivePath = "simpleType/int8_t | simpleType/int16_t | simpleType/int32_t | simpleType/int64_t | ";
            $primitivePath .= "simpleType/uint8_t | simpleType/uint16_t | simpleType/uint32_t | simpleType/uint64_t | ";
            $primitivePath .= "simpleType/string";

            my $primitiveType;
            foreach my $primitiveTypeTag ($attrDef->findnodes($primitivePath))
            {
                $primitiveType = $primitiveTypeTag->nodeName;
                $SimpleType->default($primitiveTypeTag->findvalue('default'));
            }

            # If attribute type is array then considering primitiveType as array value type
            if( $type eq "array" )
            {
                if ($primitiveType ne "")
                {
                    # Checking if array have enum type then enum have primitive type as well so cancat enum plus primitive type
                    if( $attrDef->exists('simpleType/enumeration') )
                    {
                        $primitiveType = "enum_".$primitiveType if $attrDef->findvalue('simpleType/enumeration') ne "";
                        $SimpleType->default($attrDef->findvalue('simpleType/enumeration/default'));
                        $SimpleType->enumId($attrDef->findvalue('simpleType/enumeration/id'));

                        my $enumDefPath = '/attributes/enumerationType/id[text()=\''.$attrID.'\']/ancestor::enumerationType';
                        my $enumDefData = $inXMLData->find($enumDefPath);
                        if ( $enumDefData->size() > 0 )
                        {
                            $SimpleType->enumDefinition(parseEnumerationTypes($enumDefData));
                        }
                    }
                    $SimpleType->subType($primitiveType);
                    $SimpleType->stringSize($attrDef->findvalue('simpleType/string/sizeInclNull')) if $primitiveType eq "string";;
                }
                else
                {
                    print "CRITICAL: array value data type is not found for attribute $attrID\n" if isVerboseReq('C');
                    last;
                }
            }
            elsif ( $type eq "enum" )
            {
                if ($primitiveType ne "")
                {
                    $SimpleType->subType($primitiveType);
                }
                else
                {
                    print "CRITICAL: array value data type is not found for attribute $attrID\n" if isVerboseReq('C');
                    last;
                }
            }
            else
            {
                if( $primitiveType ne "" )
                {
                    $SimpleType->DataType($primitiveType);
                    $SimpleType->stringSize($attrDef->findvalue('simpleType/string/sizeInclNull')) if $primitiveType eq "string";;
                }
                else
                {
                    print "CRITICAL: Subtype is not found for simpleType of attribute $attrID\n" if isVerboseReq('C');
                    last;
                }
            }
        }

        if( $SimpleType->default eq "")
        {
            $SimpleType->default($attrDef->findvalue('simpleType/default'));
        }
        $attributeDefinition->simpleType($SimpleType);
    }
    else
    {
        print "CRITICAL: Supported Datatype is not found for attribute $attrID\n" if isVerboseReq('C');
        last;
    }

    print "DEBUG: Attribute: $attrID\n" if isVerboseReq('D');

    return ($attrID, $attributeDefinition);
}

sub parseEnumerationTypes
{
    my $enumDefData = $_[0];

    foreach my $enumAttrData ($enumDefData->get_nodelist)
    {
        my $enumAttrID = $enumAttrData->findvalue('id');
        if ( $enumAttrID eq "" )
        {
            print "CRITICAL: Enumeration attribute id is missing in xml\n" if isVerboseReq('C');
            next;
        }

        if ( exists $enumAttrList{$enumAttrID} )
        {
            print "WARNING: Enumeration attribute id: $enumAttrID is already exists\n" if isVerboseReq('W');
            next;
        }

        my $defaultValue = $enumAttrData->findvalue('default');

        my $enumDef = EnumDefinition->new();
        $enumDef->default($defaultValue);
        foreach my $enumerator ($enumAttrData->findnodes('enumerator'))
        {
            my $enumName = $enumerator->findvalue('name');
            my $enumVal = $enumerator->findvalue('value');

            if ( $enumName eq "")
            {
                print "ERROR: enumName is missing for enumID: $enumAttrID in xml\n" if isVerboseReq('E');
                next;
            }

            if ( $enumVal eq "")
            {
                print "CRITICAL: enumVal is missing for enumName: $enumName to enumID: $enumAttrID in xml\n" if isVerboseReq('C');
                next;
            }
            my @enumPair = ( $enumName, $enumVal );
            my @enumeratorList = \@{$enumDef->enumeratorList};
            push (@enumeratorList, \@enumPair);
        }

        return ($enumDef);
    }
}

# need to return 1 for other modules to include this
1;
