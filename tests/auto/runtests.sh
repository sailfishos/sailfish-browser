#!/bin/sh

BASEDIR=$(dirname $0)
PWD=`(cd $BASEDIR && pwd)`

export LD_LIBRARY_PATH=$PWD/../../../qtmozembed/objdir-mer-dbg/src
export LOW_MEMORY_DISABLED=1

find $BASEDIR -name "*.gcda" | xargs rm

for casedir in `ls -d $BASEDIR/tst_*`; do
    CASENAME=$(basename $casedir)
    (cd $casedir && ./$CASENAME)
    echo "RET CODE: $?"
    sleep 2
done
