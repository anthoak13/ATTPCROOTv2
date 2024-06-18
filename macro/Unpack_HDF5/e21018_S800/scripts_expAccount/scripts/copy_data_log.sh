#!/bin/bash

runs=""
re='^[0-9]+$'
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

check="/mnt/analysis/e21018/evtS800/"
#from=/mnt/analysis/e21018/evtS800/tests_before_exp/
from=/user/e21018/stagearea/experiment/
logFile=/user/e21018/scripts/log_copy.txt

dir=$(pwd)
keep=true
statement=""

while $keep; do
if [[ "$statement" ]]; then
 echo $statement
 echo "Press a key to continue"
 read -p ""
 statement=""
 clear
else
 clear
fi
 
ssh -Yq e21018@expanalysis001 'bash -s' < /user/e21018/scripts/search_copy.sh

runs=""
unset -v latest
for file in $from*; do
  tmp_i=${file##*"/run"}
  #tmp_f=${tmp_i:(-4)}
  #echo $tmp_i
  #filesize=$(stat -c%s "$file")
  #echo $filesize
  #if [[ $tmp_f =~ $re ]]; then
  if [[ $tmp_i =~ $re ]]; then
     #runs="$runs $tmp_f"
     runs="$runs $tmp_i"
     [[ $file -nt $latest ]] && latest=$file
  fi
done
last_run=${latest##*"/run"}

echo -e "${NC}\nAll runs :"
echo -e "${RED}$runs${NC}"
echo -e "\n" >> "$logFile"
printf "%s" "--------------" >> "$logFile"
date=$(date '+%Y-%m-%d %H:%M:%S')
printf "%s" "$date" >> "$logFile"
printf "%s" "--------------" >> "$logFile"
echo -e "\n" >> "$logFile"
printf "%s" "$runs" >> "$logFile"
#cat <<< "----------- ""$date""-----------" > "$logFile"
echo -e " last run : $last_run --> check if still writing"
echo -e " "


echo -e 'Run(s) to be copied ? (ex : 1 2 3), or CTRL+C to exit'; read answer
#runs_to_cp=$(printf "%04d " $answer)

runs_to_cp=$answer

# Format run number with leading zeros
# do that because more convenient to script the auto unpack after
complete_run_number() {
  local number="$1"
  local completed_number=$(printf "%04d" "$number")
  echo "$completed_number"
}

runs_list=($runs)

for i in $runs_to_cp; do
    run_to_cp="run"$i
    completed_run_number="run"$(complete_run_number "${i}")
    #echo " run_to_cp "$run_to_cp
    #echo " completed_run_number "$completed_run_number
    if [[ " $completed_run_number " =~ "run0000" ]]; then
    	statement=" ! ERROR "$run_to_cp" already copied"
    elif [[ " $runs " =~ " $i " ]]; then
	#ssh -Yq e21018@expanalysis001 'bash -s' < /user/e21018/scripts/mkRemoteDir.sh $run_to_cp
	#rsync -rah --progress $from$run_to_cp e21018@expanalysis001:$check
	ssh -Yq e21018@expanalysis001 'bash -s' < /user/e21018/scripts/mkRemoteDir.sh $completed_run_number
	rsync -rah --progress "$from$run_to_cp/" "e21018@expanalysis001:$check$completed_run_number/"
        echo -e "${GREEN} $run_to_cp copied${NC}"
    else
       statement=" ! ERROR "$run_to_cp" not in "$from
    fi
done

done;

cd $dir

