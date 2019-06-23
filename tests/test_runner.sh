#!/bin/bash

# Script to run the regression tests

set -e

cd -P "$(dirname "${BASH_SOURCE[0]}")"  # cd to the test directory

[[ "$1" = "--verbose" ]] && verbose=y

sic=../src/sic
tempfile=$(mktemp)
status=0

if [[ ! -x "$sic" ]]; then
    echo "Can't find sic executable!"
    exit 1
fi

for f in `ls -1 *.sictest`; do
    echo -n "$f  "
    if $sic "$f" > $tempfile; then
        echo "PASSED!"
    else
        echo "FAILED!"
        cat $tempfile
        status=1
        break
    fi

    if [[ -n "$verbose" ]]; then
        cat $tempfile
    fi
done

rm $tempfile
exit $status

