#!/bin/bash

bufsize=1
for i in `seq 1 20`;
do
	T="$(date +%s)"
	./a.out -b $i -o outfile.txt 1.txt
	T="$(($(date +%s)-T))"
	echo "${bufsize}: time in seconds: ${T}" >> statistics.txt
	buf=$(($bufsize * 2))
done
