!#/bin/bash

make

sche_types="FIFO RR SJF PSJF"

dmesg -c
./my_sched < OS_PJ1_Test/TIME_MEASUREMENT.txt > output/TIME_MEASUREMENT_stdout.txt
dmesg | grep project1 | cut -d ' ' -f 3-6 > output/TIME_MEASUREMENT_dmesg.txt


for sche_type in $sche_types
do
	for i in $(seq 1 4)
	do
		dmesg -c
		./my_sched < OS_PJ1_Test/${sche_type}_${i}.txt > output/${sche_type}_${i}_stdout.txt
		dmesg | grep project1 | cut -d ' ' -f 3-6 > output/${sche_type}_${i}_dmesg.txt

	done
done
