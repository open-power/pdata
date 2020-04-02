#!/bin/sh
# SPDX-License-Identifier: Apache-2.0

echo "<attributes>" 
cat $* | grep -v "<attributes>" | grep -v "</attributes>" | grep -v "<?xml"
echo "</attributes>" 
