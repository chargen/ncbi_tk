#! /bin/sh
# $Id$

outlog()
{
  logfile="$1"
  if [ -s "$logfile" ]; then
    echo "=== $logfile ==="
    if [ "`cat $logfile 2>/dev/null | wc -l`" -gt "200" ]; then
      head -100 "$logfile"
      echo '...'
      tail -100 "$logfile"
    else
      cat "$logfile"
    fi
  fi
}

exit_code=0
log=test_ncbi_conn.log
rm -f $log

trap 'echo "`date`."' 0 1 2 3 15

ext="`expr $$ '%' 3`"

if   [ -r /am/ncbiapdata/test_data/proxy/test_ncbi_proxy.$ext ]; then
  .       /am/ncbiapdata/test_data/proxy/test_ncbi_proxy.$ext
elif [ -r /cygdrive/z/test_data/proxy/test_ncbi_proxy.$ext    ]; then
  .       /cygdrive/z/test_data/proxy/test_ncbi_proxy.$ext
fi

if [ "`expr '(' $$ / 10 ')' '%' 2`" = "0" ]; then
  CONN_FIREWALL=TRUE
  export CONN_FIREWALL
fi

$CHECK_EXEC test_ncbi_conn -nopause 2>&1
exit_code=$?

if [ "$exit_code" != "0" ]; then
  outlog "$log"
  uptime
fi

exit $exit_code
