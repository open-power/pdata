package processMrw_bmc;

sub return_plugins
{
    %::hwsvmrw_plugins = (
        bmc_proc=>\&process_bmc_proc,
    );
}

sub loadBMC
{
    my $targetObj = shift;
    buildBMCAffinity($targetObj);
}

sub buildBMCAffinity
{
    my $targetObj = shift;

    foreach my $target (sort keys %{ $targetObj->{data}->{TARGETS} })
    {
        my $type = $targetObj->getType($target);

        if ($type eq "" || $type eq "NA")
        {
            next;
        }
        elsif ($type eq "PROC")
        {
            my $location_code = getLocationCode($targetObj, $target);
            $targetObj->setAttribute($target, "LOCATION_CODE", $location_code);
        }
        elsif ($type eq "DIMM")
        {
            my $location_code = getLocationCode($targetObj, $target);
            $targetObj->setAttribute($target, "LOCATION_CODE", $location_code);
        }
        elsif ($type eq "OCMB_CHIP")
        {
            my $location_code = getLocationCode($targetObj, $target);
            $targetObj->setAttribute($target, "LOCATION_CODE", $location_code);
        }
        elsif ($type eq "BMC")
        {
            my $location_code = getLocationCode($targetObj, $target);
            $targetObj->setAttribute($target, "LOCATION_CODE", $location_code);
        }
    }
}

sub process_bmc_proc
{
    return;
}

sub getLocationCode
{
    my $targetObj = shift;
    my $tempTarget = shift;

    my @locationCodeArray = "";
    my $arrayIndex = 0;
    my $locationCode = "Ufcs";
    my $locationCodeType = '';
    my $tempLocationCode = '';

    if($targetObj->getTargetParent($tempTarget) eq '')
    {
        $locationCode = 'Ufcs';
    }
    else
    {
        my $finish = 1;
        do
        {
            if(!defined $tempTarget)
            {
                $finish = 0;
            }
            else
            {
                if(!$targetObj->isBadAttribute($tempTarget, "LOCATION_CODE"))
                {
                    $tempLocationCode = $targetObj->getAttribute($tempTarget,
                        "LOCATION_CODE");
                }
                else
                {
                    $tempLocationCode = '';
                }

                if(!$targetObj->isBadAttribute($tempTarget,
                        "LOCATION_CODE_TYPE"))
                {
                    $locationCodeType = $targetObj->getAttribute($tempTarget,
                        "LOCATION_CODE_TYPE");
                }
                else
                {
                    $locationCodeType = '';
                }

                if($locationCodeType eq '' || $locationCodeType eq 'ASSEMBLY' ||
                    $tempLocationCode eq '')
                {
                    $tempTarget = $targetObj->getTargetParent($tempTarget);
                }
                elsif($locationCodeType eq 'RELATIVE')
                {
                    $locationCodeArray[$arrayIndex++] = $tempLocationCode;
                    $tempTarget = $targetObj->getTargetParent($tempTarget);
                }
                elsif($locationCodeType eq 'ABSOLUTE')
                {
                    $locationCodeArray[$arrayIndex++] = $tempLocationCode;
                    $finish = 0;
                }
            }
        }while($finish);
    }

    for(my $i = $arrayIndex; $i > 0; $i--)
    {
        $locationCode = $locationCode."-".$locationCodeArray[$i-1];
    }

    return $locationCode;
}

# The end of the perl module
1;
