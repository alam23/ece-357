#!/bin/bash

bufsize=1
for i in `seq 1 20`;
do
	T="$(($(date +%s%N)/1000000))"
	./a.out -b $bufsize -o outfile.txt 1.txt
	T="$(($(($(date +%s%N)/1000000))-T))"
	echo "${bufsize}: time in milliseconds: ${T}" >> statistics.txt
	bufsize=$(($bufsize * 2))
done
