#!/usr/bin/env perl
# SPDX-License-Identifier: Apache-2.0

####################################################################
#                                                                  #
# This tool is used add common utility function for common task    #
#                                                                  #
####################################################################

sub getEKBAttrsFilterList
{
    my $filterFile = $_[0];

    open my $fd, $filterFile or die "Could not open $filterFile: \"$!\"";

    my %reqAttrsList;
    while ( my $line = <$fd> )
    {
        # skip comment line
        if ( $line =~ m/#/ )
        {
            next;
        }
        chomp $line;
        if ( ( substr $line, 0, 3 ) eq "FA:" )
        {
            my $attrID = substr $line, 3;
            $reqAttrsList{$attrID} = undef;
        }
    }

    close $fd;

    return (%reqAttrsList);
}

sub getReqCustomAttrsList
{
    my $filterFile = $_[0];

    open my $fd, $filterFile or die "Could not open $filterFile: \"$!\"";

    my %reqCustomAttrsList;
    while ( my $line = <$fd> )
    {
        # skip comment line
        if ( $line =~ m/#/ )
        {
            next;
        }
        chomp $line;
        if ( ( substr $line, 0, 3 ) eq "CA:" )
        {
            my $attrID = substr $line, 3;
            $reqCustomAttrsList{$attrID} = undef;
        }
    }

    close $fd;

    return (%reqCustomAttrsList);
}

sub getRequiredTgts
{
    my $filterFile = $_[0];

    open my $fd, $filterFile or die "Could not open $filterFile: \"$!\"";

    my %reqTgtsList;
    while ( my $line = <$fd> )
    {
        # skip comment line
        if ( $line =~ m/#/ )
        {
            next;
        }
        chomp $line;
        $reqTgtsList{$line} = undef;
    }

    close $fd;

    return (%reqTgtsList);
}

sub getReqAllAttrsFilterList
{
    my $filterFile = $_[0];

    open my $fd, $filterFile or die "Could not open $filterFile: \"$!\"";

    my %reqAttrsList;
    while ( my $line = <$fd> )
    {
        # skip comment line
        if ( $line =~ m/#/ )
        {
            next;
        }
        chomp $line;
        if ( ( substr $line, 0, 3 ) eq "FA:" )
        {
            # Skipping "FA:ATTR_" because target xml wont contain "ATTR_" as
            # prefix with attribute id
            my $attrID = substr $line, 8;
            $reqAttrsList{$attrID} = undef;
        }
        elsif ( ( substr $line, 0, 3 ) eq "CA:" )
        {
            my $attrID = substr $line, 3;
            $reqAttrsList{$attrID} = undef;
        }
    }

    close $fd;

    return (%reqAttrsList);
}

# need to return 1 for other modules to include this
1;
