#!/bin/bash

runss800=(2271 2272 2273 2274 2275 2276 2277 2278 2279)

runstpc=(271 272 273 274 275 276 277 278 279)

#initi is the number of jobs wanted to run in parallel
initi="10"

PID_LIST=""
nbJobs="0"
irun="0"
len=${#runss800[@]}
remain=$len
maxJobs="0"
let "maxJobs = $initi + 1"
if [[ $len -lt $maxJobs ]]; then
	#let "initi = $len"
	let "initi = $len - 1"
fi

while [ $remain -gt 0 ]
do
if [[ irun -eq 0 ]]; then
	#for i in {0..$initi}; do
	for (( i = 0; i <= $initi; i++ ))
	do
   		echo "Run" ${runss800[($i)]} " " ${runstpc[($i)]}
  		# root -b -q "unpack.C(${runss800[($i)]},${runstpc[($i)]})" >& "logAna/unpack_run${runstpc[($i)]}.log" &
  		root -b -q "../e18008_S800/unpack_new.C(${runss800[($i)]},${runstpc[($i)]})" >& "logAna/unpack_run${runstpc[($i)]}.log" &
		PID="$!"
		echo "PID " "$PID"
  		PID_LIST+="$PID "
		let nbJobs++
		let irun++
		let remain--
	done
fi

if [[ nbJobs -lt $maxJobs ]]; then
	echo "Run" ${runss800[($i)]} " " ${runstpc[($i)]}
  	# root -b -q "unpack.C(${runss800[$irun]},${runstpc[$irun]})" >& "logAna/unpack_run${runstpc[$irun]}.log" &
  	root -b -q "../e18008_S800/unpack_new.C(${runss800[$irun]},${runstpc[$irun]})" >& "logAna/unpack_run${runstpc[$irun]}.log" &
	PID="$!"
	echo "PID " "$PID"
  	PID_LIST+="$PID "
	let nbJobs++
	let irun++
	let remain--
fi

for process in ${PID_LIST[@]};do

	if pgrep -x "$process" </dev/null
	then
		echo "$process stopped"
	fi

	echo "Process " $process
	wait $process
	echo "PID " "${PID_LIST[@]}"
	let nbJobs--
	#let remain--
	break
  # exit_status=$?
  #script_name=`egrep $process $tmp_file | awk -F ":" '{print $2}' | rev | awk -F "/" '{print $2}' | rev`
done

sleep 3

done
