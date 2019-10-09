#!/bin/bash
#threads="1 2 4 8 16"
#threads="1 2 4"
threads="1 1 1 1 1 1"
#skips="skip cskip vcskip"
skips="skip"
num=90000
#benchmarks="1 2 3" ##put get scan
benchmarks="1 1 1"

cp ~/skip_vchain/src/skip .
#cp ~/skip_vchain/src/cskip .
#cp ~/skip_vchain/src/vcskip .
for bench in $benchmarks
do
	echo "   "  "skip" "cskip" "vcksip" 
	for thread in $threads
	do
		skip=$(./skip $thread $num $bench)
		sleep 1 
#		cskip=$(./cskip $thread $num $bench)
#		sleep 1 
#		vcskip=$(./vcskip $thread $num $bench)
#		sleep 1
#		echo "$thread" "$skip" "$cskip" "$vcskip" 
		echo "$thread" "$skip"
	done
	echo " "
	
done
