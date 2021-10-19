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

    my $node = -1;
    my $sys_phys = "";
    my $node_phys = "";
    my $oscrefclk = -1;
    my $osrefclk_instance_per_node = -1;

    foreach my $target (sort keys %{ $targetObj->{data}->{TARGETS} })
    {
        my $type = $targetObj->getType($target);

        if ($type eq "" || $type eq "NA")
        {
            next;
        }
        elsif ($type eq "SYS")
        {
            $sys_phys = $targetObj->getAttribute($target, "PHYS_PATH");
            $sys_phys = substr($sys_phys, 9);
        }
        elsif ($type eq "NODE")
        {
            $node++;
            $node_phys = "physical:".$sys_phys."/node-$node";
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
        elsif ($type eq "TPM")
        {
            my $location_code = getLocationCode($targetObj, $target);
            $targetObj->setAttribute($target, "LOCATION_CODE", $location_code);
        }
        elsif ($type eq "OSCREFCLK" and $targetObj->getTargetType($target) eq "chip-SI5332")
        {
            $oscrefclk++;
            $osrefclk_instance_per_node++;
            # Hardcoding the sys instance as "0" since we will have only one system
            $targetObj->{targeting}{SYS}[0]{NODES}[$node]{OSCREFCLK}[$osrefclk_instance_per_node]{KEY}= $target;

            my $oscrefclk_phys = $node_phys . "/oscrefclk-$osrefclk_instance_per_node";
            $targetObj->setAttribute($target, "PHYS_PATH", $oscrefclk_phys);

            # OSCREFCLK is Non-FAPI target
            $targetObj->setAttribute($target,"FAPI_NAME", "NA");
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
