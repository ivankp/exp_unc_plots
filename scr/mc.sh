#!/bin/bash

vars=$(sed -rn 's/([^.])\.bins:.*/\1/p' $2 | paste -sd '|')

sed -rn "s/([^.]+)\.(${vars})\.(bins|fid): *(.+)/\2.\3.\1: \4/p" $1 \
| sed 's/bins\..\+:/bins:/' \
| tr -s ' ' \
| sort -u \
| sed 's/\.fid\././' \
| sed -r 's/(Dphi_yy_jj_30\.bins:).+/\1 0.00 3.01 3.10 3.15/' \
| sed -r 's/(Dphi_yy_jj_30\.[^ ]+:) +([^ ]+) +([^ ]+) +([^ ]+)$/\1 \4 \3 \2/'
