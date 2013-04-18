#!/bin/sh

TARGET_DIR=$1
if [ "$TARGET_DIR" = "" ]; then
  echo "TARGET_DIR ex: /usr/bin"
  TARGET_DIR=/usr/bin
fi

mkdir -p $TARGET_DIR

FILES_LIST="
src/sailfish-browser
"
for str in $FILES_LIST; do
    fname="${str##*/}"
    rm -f $TARGET_DIR/$fname;
    ln -s $(pwd)/$str $TARGET_DIR/$fname;
done
