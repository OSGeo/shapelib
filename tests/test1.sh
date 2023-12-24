#!/bin/bash

set -eu

readonly EXPECT="${1:-$SCRIPTDIR/expect1.out}"
readonly EG_DATA="$2"

{
echo -------------------------------------------------------------------------
echo Test 1: dump anno.shp
echo -------------------------------------------------------------------------
"${SHPDUMP:-./shpdump}" "$EG_DATA/anno.shp" | head -250

echo -------------------------------------------------------------------------
echo Test 2: dump brklinz.shp
echo -------------------------------------------------------------------------
"${SHPDUMP:-./shpdump}" "$EG_DATA/brklinz.shp" | head -500

echo -------------------------------------------------------------------------
echo Test 3: dump polygon.shp
echo -------------------------------------------------------------------------
"${SHPDUMP:-./shpdump}" "$EG_DATA/polygon.shp" | head -500

echo -------------------------------------------------------------------------
echo Test 4: dump pline.dbf - uses new F field type
echo -------------------------------------------------------------------------
"${DBFDUMP:-./dbfdump}" -m -h "$EG_DATA/pline.dbf" | head -50

echo -------------------------------------------------------------------------
echo Test 5: NULL Shapes.
echo -------------------------------------------------------------------------
"${SHPDUMP:-./shpdump}" "$EG_DATA/csah.dbf" | head -150
} > s1.out


if result=$(diff "$EXPECT" "s1.out"); then
	echo "******* Stream 1 Succeeded *********"
	exit 0
else
	echo "******* Stream 1 Failed *********"
	echo "$result"
	exit 1
fi
