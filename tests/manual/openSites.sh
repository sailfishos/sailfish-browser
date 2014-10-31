#!/bin/bash

SITES="sites.txt"

if [ $# -gt 0 ]; then
  SITES=$1
fi
webSites=`wc -l $SITES | cut -d ' ' -f 1`
echo "Opening $webSites sites from $SITES"
i=0

MAX_SITES=100
if [ $# -gt 1 ]; then
  MAX_SITES=$2
fi

for line in `cat $SITES`; do
  sqlite3 /home/nemo/.local/share/org.sailfishos/sailfish-browser/sailfish-browser.sqlite "insert into link (url) values (\"http://$line\");"
  sqlite3 /home/nemo/.local/share/org.sailfishos/sailfish-browser/sailfish-browser.sqlite "insert into tab_history (tab_id, link_id) values ((select max(tab_id)+1 from tab), (select max(link_id) from link)); insert into tab (tab_id, tab_history_id) values ((select max(tab_id)+1 from tab), (select max(id) from tab_history));"
  i=$((i+1))
  echo "Adding $line ($i/$webSites)"

  if [ $i -ge $MAX_SITES ]; then
    break
  fi
done
