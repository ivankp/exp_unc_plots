#!/bin/bash

if [ $1 ] && [ "$1" == "corr" ]; then
  ../bin/plot ../data/uncert_corr.dat -o uncert_corr.pdf \
    -y "#it{#Deltacf}/#it{cf}" \
    -s brown.sty --y-offset=0.8
else
  ../bin/plot ../data/uncert.dat -o uncert.pdf \
    -y "#it{#Delta#sigma}_{fid} / #it{#sigma}_{fid}^{SM}"
fi
