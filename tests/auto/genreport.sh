#!/bin/sh

INFOS=
BASEDIR=$(dirname $0)
for casedir in `ls -d $BASEDIR/tst_*`; do
    CASENAME=$(basename $casedir)
    rm -f ${CASENAME}.info
    (cd $casedir && lcov --base-directory . --directory . -c -o ../${CASENAME}.info)
    if [ -s $BASEDIR/${CASENAME}.info ]; then
        INFOS="$INFOS ${CASENAME}.info"
    fi
done

echo $INFOS

rm -rf $BASEDIR/test_coverage

(cd $BASEDIR && genhtml -o test_coverage -t "Browser coverage" --num-spaces 4 $INFOS)
