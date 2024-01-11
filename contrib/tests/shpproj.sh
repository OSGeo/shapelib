#!/bin/bash

readonly SCRIPTDIR=$(dirname "${BASH_SOURCE[0]}")

"${SHPCREATE:-$top_builddir/shpcreate}" "test" point

"${SHPADD:-$top_builddir/shpadd}" "test" -83.54949956              34.992401
"${SHPADD:-$top_builddir/shpadd}" "test" -83.52162155              34.99276748
"${SHPADD:-$top_builddir/shpadd}" "test" -84.01681518              34.67275985
"${SHPADD:-$top_builddir/shpadd}" "test" -84.15596023              34.64862437
"${SHPADD:-$top_builddir/shpadd}" "test" -83.61951463              34.54927047

"${DBFCREATE:-$top_builddir/dbfcreate}" "test" -s fd 30
"${DBFADD:-$top_builddir/dbfadd}" "test" "1"
"${DBFADD:-$top_builddir/dbfadd}" "test" "2"
"${DBFADD:-$top_builddir/dbfadd}" "test" "3"
"${DBFADD:-$top_builddir/dbfadd}" "test" "4"
"${DBFADD:-$top_builddir/dbfadd}" "test" "5"

"${SHPDUMP:-$top_builddir/shpdump}" -precision 8 "test" > "test.out"


if result=$(diff "$SCRIPTDIR/expect.out" "test.out"); then
	echo "******* Test Succeeded *********"
	exit 0
else
	echo "******* Test Failed *********"
	echo "$result"
	exit 1
fi
