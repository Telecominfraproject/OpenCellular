#!/bin/sh
# this is a dispatcher script which will call the bts-model-specific
# script based on the bts model specified as command line argument

bts_model="$1"

if [ "x$bts_model" = "x" ]; then
	echo "Error: You have to specify the BTS model as first argument, e.g. $0 sysmo"
	exit 2
fi

if [ ! -d "./contrib" ]; then
  echo "Run ./contrib/jenkins_bts_model.sh from the root of the osmo-bts tree"
  exit 1
fi

set -x -e

case "$bts_model" in

  sysmo)
    ./contrib/jenkins_sysmobts.sh
  ;;

  oct)
    ./contrib/jenkins_oct.sh
  ;;

  lc15)
    ./contrib/jenkins_lc15.sh
  ;;

  trx)
    ./contrib/jenkins_bts_trx.sh
  ;;

  oct+trx)
    ./contrib/jenkins_oct_and_bts_trx.sh
  ;;

  *)
    set +x
    echo "Unknown BTS model '$bts_model'"
  ;;
esac
