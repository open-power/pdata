#!/bin/sh

set -e

DIR=$(dirname "$0")
DIR=$(cd "$DIR"; pwd)

ATTRIBUTES="./attributes"
DTS="$DIR/test.dts"
INFODB="$DIR/test_info.db"

DTB="./test.dtb"
DTB1="./test1.dtb"
DUMP="./attr_dump"
DUMP2="./attr_dump2"

if [ ! -x "$ATTRIBUTES" ] ; then
	echo "attributes tool not found in the current directory, exiting"
	exit 1
fi

echo "Create bare dtb"
dtc -I dts -O dtb $DTS > $DTB

echo "Populate dtb with attributes"
$ATTRIBUTES create $DTB $INFODB $DTB1

echo "Modify attributes for root"
$ATTRIBUTES write $DTB1 $INFODB / ATTR_TEST1 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24
$ATTRIBUTES write $DTB1 $INFODB / ATTR_TEST2 0x1111 0x2222 0x3333 0x4444 0x5555 0x6666
$ATTRIBUTES write $DTB1 $INFODB / ATTR_TEST3 key2 key2 key1 key1
$ATTRIBUTES write $DTB1 $INFODB / ATTR_TEST4 0x1234567812345678

echo "Modify attributes for proc0"
$ATTRIBUTES write $DTB1 $INFODB /proc0 ATTR_TEST5 processor1
$ATTRIBUTES write $DTB1 $INFODB /proc0 ATTR_TEST6 0xdeadbeef 1 0xdead 0x11223344 0x55 0x6677

echo "Modify attributes for proc1"
$ATTRIBUTES write $DTB1 $INFODB /proc1 ATTR_TEST5 processor2
$ATTRIBUTES write $DTB1 $INFODB /proc1 ATTR_TEST6 0xdeadbeef 1 0xbeef 0x12345678 0x12 0x1234

echo "Export dtb"
$ATTRIBUTES export $DTB1 $INFODB > $DUMP

echo "Re-create with default attributes"
$ATTRIBUTES create $DTB $INFODB $DTB1

echo "Import dtb"
$ATTRIBUTES import $DTB1 $INFODB $DUMP

echo "Reexport dtb"
$ATTRIBUTES export $DTB1 $INFODB > $DUMP2

echo "Check for export diff"
diff $DUMP $DUMP2

echo "Read attributes for /"
$ATTRIBUTES read $DTB1 $INFODB / ATTR_TEST1
$ATTRIBUTES read $DTB1 $INFODB / ATTR_TEST2
$ATTRIBUTES read $DTB1 $INFODB / ATTR_TEST3
$ATTRIBUTES read $DTB1 $INFODB / ATTR_TEST4

echo "Read attributes for /proc0"
$ATTRIBUTES read $DTB1 $INFODB /proc0 ATTR_TEST5
$ATTRIBUTES read $DTB1 $INFODB /proc0 ATTR_TEST6

echo "Read attributes for /proc1"
$ATTRIBUTES read $DTB1 $INFODB /proc1 ATTR_TEST5
$ATTRIBUTES read $DTB1 $INFODB /proc1 ATTR_TEST6
