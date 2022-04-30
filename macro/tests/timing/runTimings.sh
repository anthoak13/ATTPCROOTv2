#!/bin/bash
RED="\e[31m"
GREEN="\e[32m"
ENDCOLOR="\e[0m"


tests=("unpack_e20020_full" "unpack_e12014" "unpack_e12014_full")
numEvents=(100 98 98)
numRuns=3

for i in ${!tests[@]}; do
    echo -e "${GREEN}Test $i: running ${tests[$i]}${ENDCOLOR}"
    timeInitReal=()
    timeInitCPU=()
    timeRunReal=()
    timeRunCPU=()
    for (( j=0; j < $numRuns; j++ )); do
	#echo "Iteration: $j"
	root -l -q "${tests[$i]}.C(${numEvents[$i]})" &> output/${tests[$i]}.log


	timeInitReal+=( $( grep "Finished init .* s real" output/${tests[$i]}.log | grep -Eo '[+-]?[0-9]+([.][0-9]+)?' ) )
	timeInitCPU+=( $( grep "Finished init .* s CPU" output/${tests[$i]}.log | grep -Eo '[+-]?[0-9]+([.][0-9]+)?' ) )
	timeRunReal+=( $( grep "Finished run .* s real" output/${tests[$i]}.log | grep -Eo '[+-]?[0-9]+([.][0-9]+)?' ) )
	timeRunCPU+=( $( grep "Finished run .* s CPU" output/${tests[$i]}.log | grep -Eo '[+-]?[0-9]+([.][0-9]+)?' ) )

	#grep "Finished run" output/${tests[$i]}.log | grep -Eo '[+-]?[0-9]+([.][0-9]+)?'
    done
    avg=$( printf '%s\n' "${timeInitReal[@]}" |
	       awk '{sum+=$1}END{print sum/NR}')
    stdDev=$( printf '%s\n' "${timeInitReal[@]}" |
		  awk -v a=$avg '{sum+=($1-a)**2;}END{print sqrt(sum/(NR-1))}')
    echo "Init real: $avg +- $stdDev"

    avg=$( printf '%s\n' "${timeInitCPU[@]}" |
	       awk '{sum+=$1}END{print sum/NR}')
    stdDev=$( printf '%s\n' "${timeInitCPU[@]}" |
		  awk -v a=$avg '{sum+=($1-a)**2;}END{print sqrt(sum/(NR-1))}')
    echo "Init CPU: $avg +- $stdDev"

    avg=$( printf '%s\n' "${timeRunReal[@]}" |
	       awk '{sum+=$1}END{print sum/NR}')
    stdDev=$( printf '%s\n' "${timeRunReal[@]}" |
		  awk -v a=$avg '{sum+=($1-a)**2;}END{print sqrt(sum/(NR-1))}')
    echo "Run real: $avg +- $stdDev"

    avg=$( printf '%s\n' "${timeRunCPU[@]}" |
	       awk '{sum+=$1}END{print sum/NR}')
    stdDev=$( printf '%s\n' "${timeRunCPU[@]}" |
		  awk -v a=$avg '{sum+=($1-a)**2;}END{print sqrt(sum/(NR-1))}')
    echo "Run CPU: $avg +- $stdDev"
done    
