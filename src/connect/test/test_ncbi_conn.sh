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

if [ -r /am/ncbiapdata/test_data/proxy/test_ncbi_proxy ]; then
  . /am/ncbiapdata/test_data/proxy/test_ncbi_proxy
  export CONN_HTTP_USER_HEADER="Client-Host: 1.1.1.1"
  export CONN_LB_DISABLE=1
fi

$CHECK_EXEC test_ncbi_conn -nopause 2>&1
exit_code=$?

if [ "$exit_code" != "0" ]; then
  outlog "$log"
  uptime
fi

exit $exit_code
