#!/bin/bash

runs_copied=""
rez='^0+'
re='^[0-9]+$'
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

check="/mnt/analysis/e21018/evtS800/"

for file in $check/*; do
  tmp_i=${file##*"/run"}
  while [[ $tmp_i =~ $rez ]]; do
    tmp_i="${tmp_i#0}"
  done
  #tmp_f=${tmp_i:(-4)}
  #if [[ $tmp_f =~ $re ]]; then
  #   runs_copied="$runs_copied $tmp_f"
  if [[ $tmp_i =~ $re ]]; then
     runs_copied="$runs_copied $tmp_i"
  fi
done
#printf '-%.0s' {1..${cols}}
echo -e "${NC}Runs that are already copied :"
echo -e "${GREEN}$runs_copied${NC}"

