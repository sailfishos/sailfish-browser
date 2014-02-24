#!/bin/bash

SITES="sites.txt"

if [ $# -gt 0 ]; then
  SITES=$1
fi
echo "Opening sites from $SITES"

for line in `cat $SITES`; do
  xdg-open "http://$line"
  sleep 5
done
