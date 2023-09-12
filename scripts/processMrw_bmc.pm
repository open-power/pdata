package processMrw_bmc;

use Data::Dumper;

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
            # The DIMM target that needs to be picked up has a new type
            # as per the new HB changes to support DDR5. They will be
            # unit-ddr* type and not lcard-dimm*. While parsing we get all
            # of them, so skip the unit-ddr*, get the location code from
            # the target with lcard type and set the location code for its
            # child targets which are again unit-ddr*
            my $target_type = $targetObj->getTargetType($target);
            if (($target_type eq "unit-ddr") ||
                ($target_type eq "unit-ddr4-jedec") ||
                ($target_type eq "unit-ddr5-jedec"))
               {
                   next;
               }

            my $location_code = getLocationCode($targetObj, $target);
            # needed for Bonnell which has older DIMM type
            $targetObj->setAttribute($target, "LOCATION_CODE", $location_code);

            foreach my $child (@{ $targetObj->getTargetChildren($target) })
            {
                my $child_target_type = $targetObj->getTargetType($child);
                if (($child_target_type eq "unit-ddr") ||
                    ($child_target_type eq "unit-ddr4-jedec") ||
                    ($child_target_type eq "unit-ddr5-jedec"))
                {
                     $targetObj->setAttribute($child, "LOCATION_CODE", $location_code);
                }
             }
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
        elsif ($type eq "OSCREFCLK" and ($targetObj->getTargetType($target) eq "chip-SI5332") or
                                        ($targetObj->getTargetType($target) eq "chip-Si5332LD"))
        {
            $oscrefclk++;
            $osrefclk_instance_per_node++;
            # Hardcoding the sys instance as "0" since we will have only one system
            $targetObj->{targeting}{SYS}[0]{NODES}[$node]{OSCREFCLK}[$osrefclk_instance_per_node]{KEY}= $target;

            my $oscrefclk_phys = $node_phys . "/oscrefclk-$osrefclk_instance_per_node";
            $targetObj->setAttribute($target, "PHYS_PATH", $oscrefclk_phys);

            # OSCREFCLK is Non-FAPI target
            $targetObj->setAttribute($target,"FAPI_NAME", "NA");

            # Add I2C_PORT, I2C_ADDRESS, and I2C_PARENT_PHYS_PATH attributes
            # based on the I2C destination connection of the retrieved target.
            # These attributes helps to add the retrieved target under
            # the respective connected (via I2C) destination target in the
            # device tree to perform the i2c read and write operation
            # on the OSCREFCLK target.
            # For example, bmc-0 -> i2c-0 -> oscrefclk-0.
            my $destConn = $targetObj->findDestConnections($target, "I2C", "");
            my @destConnList = @{$destConn->{CONN}};
            my $numConnections = scalar @destConnList;

            if ($numConnections != 1)
            {
                die "Incorrect number of OSCREFCLK I2C bus connection. Expected 1 and Found $numConnections";
            }

            my $i2c_port = $targetObj->getAttribute($destConnList[0]{SOURCE}, "I2C_PORT");
            $targetObj->setAttribute($target, "I2C_PORT", $i2c_port);

            my $i2c_addr = $targetObj->getAttribute($destConnList[0]{DEST}, "I2C_ADDRESS");
            $targetObj->setAttribute($target, "I2C_ADDRESS", $i2c_addr);

            # Found the connected BMC card from the source i2c path
            my $src_parent = $destConnList[0]{SOURCE_PARENT};
            while($targetObj->getType($src_parent) ne "BMC")
            {
                $src_parent = $targetObj->getTargetParent($src_parent);
            }
            if ($src_parent eq "")
            {
                die "Could not find the connected BMC from the source i2c bus connection: $destConnList[0]{SOURCE}\n";
            }
            my $src_parent_phys_path = $targetObj->getAttribute($src_parent, "PHYS_PATH");
            $targetObj->setAttribute($target, "I2C_PARENT_PHYS_PATH", $src_parent_phys_path);
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
