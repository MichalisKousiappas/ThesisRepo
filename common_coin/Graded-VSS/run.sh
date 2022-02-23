#!/bin/bash

# Use this script to run numofnodes processes
# dealer is the global dealer
# i could make it read the parameters from input, but i am lazy

echo

numofnodes=31
dealer=2

# kill any processes that is still alive from previous run
pidof Graded-VSS.o && killall Graded-VSS.o && sleep 1

# clean recompile to delete traces as well
make clean
make debug || exit

# run the processes numofnodes time and redirect output to file
for i in $(seq 0 $((numofnodes-1))); do ./Graded-VSS.o $i $numofnodes $dealer > result$i.dmp & done

# wait for them to finish
wait

# Check processes output
echo "Parameters:"

echo "number of nodes: $numofnodes"
echo "dealer process: $dealer"

echo ""

echo -n "GoodPieces: "
grep -irn "checkforgoodpiece\*exit\[1\]" result* --color=auto | wc -l

grep -oi "Rootofunity\[.*\]" result0.dmp --color=auto

grep -m 1 -A 1 -i "Root polynomial:" result$dealer.dmp --color=auto
grep -ohi "finale\[.*\]\|I can't do this\|Interpolation cannot be performed" result* --color=auto | sort | uniq
echo -n "number of processes that got it: "
grep -i "finale" result* | uniq | wc -l

echo -e "tally's were: "
grep -ohi "tally is \[.*\]" result* --color=auto | sort | uniq

# tell me when you are done
spd-say -i -40 done
