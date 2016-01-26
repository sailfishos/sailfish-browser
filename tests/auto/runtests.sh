#!/bin/sh

BASEDIR=$(dirname $0)
PWD=`(cd $BASEDIR && pwd)`

if [[ $PWD == /opt/* ]] ; then
    for testcase in `ls $BASEDIR/tst_*`; do
        $testcase
        echo "RET CODE: $?"
        sleep 2
    done
else
    export LD_LIBRARY_PATH=$PWD/../../../qtmozembed/objdir-mer-dbg/src
    export LOW_MEMORY_DISABLED=1

    find $BASEDIR -name "*.gcda" | xargs rm

    for casedir in `ls -d $BASEDIR/tst_*`; do
        CASENAME=$(basename $casedir)
        (cd $casedir && ./$CASENAME)
        echo "RET CODE: $?"
        sleep 2
    done
fi

