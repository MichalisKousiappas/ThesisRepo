#!/bin/bash

echo

numofnodes=6
dealer=2

pidof temporary.o && killall temporary.o && sleep 1
make clean
make temporary || exit

for i in $(seq 0 $((numofnodes-1))); do ./temporary.o $i $numofnodes $dealer > temp_result$i.dmp & done

wait
#ps -ef | grep temporary.o

#grep -irn "send: \[.*\] Accept\.code\[" result* --color=auto
#echo -n "GoodPieces: "
#grep -irn "checkforgoodpiece\*exit\[1\]" result* --color=auto | wc -l
#
#grep -oi "Rootofunity\[.*\]" result0.dmp --color=auto
#
##grep -irn "send: \[.*\] Accept\.code\[" result* | wc -l
#grep -m 1 -A 1 -i "Root polynomial:" result$dealer.dmp --color=auto
#grep -ohi "finale\[.*\]\|I can't do this\|Interpolation cannot be performed" result* --color=auto | uniq
#grep -i "finale" result* | wc -l
