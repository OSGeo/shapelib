#!/bin/sh

#
#	Use example programs to create a very simple dataset that
#	should display in ARCView II.
#

set -eu

readonly SCRIPTDIR=$(dirname "$0")
readonly EXPECT="${1:-$SCRIPTDIR/expect3.out}"
readonly TESTDIR=`mktemp -d -p "$(pwd)"`

# check if tmp dir was created
if [[ ! "$TESTDIR" || ! -d "$TESTDIR" ]]; then
  echo "Could not create temp dir"
  exit 1
fi

{
"${SHPCREATE:-./shpcreate}" "${TESTDIR:-.}"/test polygon
"${DBFCREATE:-./dbfcreate}" "${TESTDIR:-.}"/test.dbf -s Description 30 -n TestInt 6 0 -n TestDouble 16 5 -d TestDate -l TestBool

"${SHPADD:-./shpadd}" "${TESTDIR:-.}"/test 0 0 100 0 100 100 0 100 0 0 + 20 20 20 30 30 30 20 20
"${DBFADD:-./dbfadd}" "${TESTDIR:-.}"/test.dbf "Square with triangle missing" 1.4 2.5 20241102 T

"${SHPADD:-./shpadd}" "${TESTDIR:-.}"/test 150 150 160 150 180 170 150 150
"${DBFADD:-./dbfadd}" "${TESTDIR:-.}"/test.dbf "Smaller triangle" 100 1000.25 20241102 F

"${SHPADD:-./shpadd}" "${TESTDIR:-.}"/test 150 150 160 150 180 170 150 150
"${DBFADD:-./dbfadd}" "${TESTDIR:-.}"/test.dbf "" "" "" "" ""

"${SHPDUMP:-./shpdump}" "${TESTDIR:-.}"/test.shp
"${DBFDUMP:-./dbfdump}" "${TESTDIR:-.}"/test.dbf
} > s3.out


supports_strip_trailing_cr() {
	diff --help 2>/dev/null | grep -q -- '--strip-trailing-cr'
}

run_diff() {
	if supports_strip_trailing_cr; then
		diff --strip-trailing-cr "$EXPECT" "s3.out"
	else
		diff "$EXPECT" "s3.out"
	fi
}

if result=$(run_diff); then
	echo "******* Stream 3 Succeeded *********"
	exit 0
else
	echo "******* Stream 3 Failed *********"
	echo "$result"
	exit 1
fi
