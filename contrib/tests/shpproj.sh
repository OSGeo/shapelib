#!/bin/bash

readonly SCRIPTDIR=$(dirname "${BASH_SOURCE[0]}")
readonly TOP_BUILDDIR="$SCRIPTDIR/../.."

"${SHPCREATE:-$TOP_BUILDDIR/shpcreate}" "test" point

"${SHPADD:-$TOP_BUILDDIR/shpadd}" "test" -83.54949956              34.992401
"${SHPADD:-$TOP_BUILDDIR/shpadd}" "test" -83.52162155              34.99276748
"${SHPADD:-$TOP_BUILDDIR/shpadd}" "test" -84.01681518              34.67275985
"${SHPADD:-$TOP_BUILDDIR/shpadd}" "test" -84.15596023              34.64862437
"${SHPADD:-$TOP_BUILDDIR/shpadd}" "test" -83.61951463              34.54927047

"${DBFCREATE:-$TOP_BUILDDIR/dbfcreate}" "test" -s fd 30
"${DBFADD:-$TOP_BUILDDIR/dbfadd}" "test" "1"
"${DBFADD:-$TOP_BUILDDIR/dbfadd}" "test" "2"
"${DBFADD:-$TOP_BUILDDIR/dbfadd}" "test" "3"
"${DBFADD:-$TOP_BUILDDIR/dbfadd}" "test" "4"
"${DBFADD:-$TOP_BUILDDIR/dbfadd}" "test" "5"

"${SHPDUMP:-$TOP_BUILDDIR/shpdump}" -precision 8 "test" > "test.out"


if result=$(diff "$SCRIPTDIR/expect.out" "test.out"); then
	echo "******* Test Succeeded *********"
	exit 0
else
	echo "******* Test Failed *********"
	echo "$result"
	exit 1
fi
