#!/bin/bash

#
#	Use example programs to create a very simple dataset that
#	should display in ARCView II.
#

set -eu

readonly SCRIPTDIR=$(dirname "${BASH_SOURCE[0]}")
readonly EXPECT="${1:-$SCRIPTDIR/expect3.out}"

{
"${SHPCREATE:-./shpcreate}" test polygon
"${DBFCREATE:-./dbfcreate}" test.dbf -s Description 30 -n TestInt 6 0 -n TestDouble 16 5

"${SHPADD:-./shpadd}" test 0 0 100 0 100 100 0 100 0 0 + 20 20 20 30 30 30 20 20
"${DBFADD:-./dbfadd}" test.dbf "Square with triangle missing" 1.4 2.5

"${SHPADD:-./shpadd}" test 150 150 160 150 180 170 150 150
"${DBFADD:-./dbfadd}" test.dbf "Smaller triangle" 100 1000.25

"${SHPADD:-./shpadd}" test 150 150 160 150 180 170 150 150
"${DBFADD:-./dbfadd}" test.dbf "" "" ""

"${SHPDUMP:-./shpdump}" test.shp
"${DBFDUMP:-./dbfdump}" test.dbf
} > s3.out

if result=$(diff "$EXPECT" "s3.out"); then
	echo "******* Stream 3 Succeeded *********"
	exit 0
else
	echo "******* Stream 3 Failed *********"
	echo "$result"
	exit 1
fi
