#!/bin/bash

# Use this script to run numofnodes processes
# i could make it read the parameters from input, but i am lazy

echo

numofnodes=40

# kill any processes that is still alive from previous run
#pidof Graded-VSS.o && killall Graded-VSS.o && sleep 1

# clean recompile to delete traces as well
#make clean
#make || exit
T="$(date +%s%N)"


# run the processes numofnodes time and redirect output to file
for i in $(seq 0 $((numofnodes-1))); do ./Graded-VSS.o $i $numofnodes > result$i.dmp & done

# wait for them to finish
wait

# Time interval in nanoseconds
T="$(($(date +%s%N)-T))"
# Seconds
S="$((T/1000000000))"
# Milliseconds
M="$((T/1000000))"

printf "Pretty format: %02d:%02d:%02d.%03d\n" "$((S/3600%24))" "$((S/60%60))" "$((S%60))" "${M}"

grep -ohi "SimpleGradedRecover\*exit\[.*\]" result* --color=auto | sort | uniq
grep -ohi "tally is \[.*\]" result* --color=auto | sort | uniq

#spd-say -i -40 done


sleep 0.5

exit
