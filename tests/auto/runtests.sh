#!/bin/sh

CDPATH=''
BASEDIR=$(cd "$(dirname "$0")" && pwd)
status=0

case $BASEDIR in
/opt/*)
    for testcase in "$BASEDIR"/tst_*; do
        [ -x "$testcase" ] || continue
        "$testcase"
        result=$?
        printf 'RET CODE: %s\n' "$result"
        [ "$result" -eq 0 ] || status=$result
    done
    ;;
*)
    LD_LIBRARY_PATH="$BASEDIR/../../../qtmozembed/objdir-mer-dbg/src${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    export LD_LIBRARY_PATH
    export LOW_MEMORY_DISABLED=1

    find "$BASEDIR" -name '*.gcda' -exec rm -f {} +

    for casedir in "$BASEDIR"/tst_*; do
        [ -d "$casedir" ] || continue
        casename=$(basename "$casedir")
        (cd "$casedir" && "./$casename")
        result=$?
        printf 'RET CODE: %s\n' "$result"
        [ "$result" -eq 0 ] || status=$result
    done
    ;;
esac

exit "$status"
