#!/bin/sh

readonly SCRIPTDIR=$(dirname "$0")
readonly EXPECT="${1:-$SCRIPTDIR/expect2.out}"

for i in 0 1 2 3 4 5 6 7 8 9 10 11 12 13; do
  echo -----------------------------------------------------------------------
  echo Test 2/$i
  echo -----------------------------------------------------------------------

  "${SHPTEST:-./shptest}" $i
  "${SHPDUMP:-./shpdump}" test${i}.shp
done > "s2.out"


supports_strip_trailing_cr() {
	diff --help 2>/dev/null | grep -q -- '--strip-trailing-cr'
}

run_diff() {
	if supports_strip_trailing_cr; then
		diff --strip-trailing-cr "$EXPECT" "s2.out"
	else
		diff "$EXPECT" "s2.out"
	fi
}

if result=$(run_diff); then
	echo "******* Stream 2 Succeeded *********"
	exit 0
else
	echo "******* Stream 2 Failed *********"
	echo "$result"
	exit 1
fi
