#!/bin/bash

rm -f q1 q2

#==================Compilation and error checking===============================
gcc -o q1 -pthread -Wall -Werror q1.c
#if [ $? -eq 0 ]; then
#	gcc -o q1 -lpthread -Wall q1.c
#	if [ $? -eq 0 ]; then
#		echo "Error during compilation.  Uhh...crap."
#	fi
#fi

# gcc -o q2 -pthread -Wall q2.c
# if [ $? -eq 0 ]; then
# 	gcc -o q2 -lpthread -Wall -Werror q2.c
# fi
# if [ $? -eq 0 ]; then
# 	echo "Error during compilation.  Uhh...crap."
# fi
#===============Run Program 1===================================================

#q1.txt is the plain-text format of the results.  
#q1_tex.txt is plain-text but with some added formatting for copying and
#pasting straight into a LaTeX table.  
printf "nthreads\ttotal_time\taverage_time\n" > q1.txt
printf "\\hline Number of Threads & Total Time(s) & Average Time(s)\\\\\\hline\n" > q1_tex.txt
for i in {1..19}
do
	./q1 $i | awk '/nthreads/ {printf "%d\t\t%.9f\t%.9f\n", $2, $5, $9}' >> q1.txt
	./q1 $i | awk '/nthreads/ {printf "%d\t\t& %.9f\t& %.9f\\\\\\hline\n", $2, $5, $9}' >> q1_tex.txt
done
for i in {20..50..5}
do
	./q1 $i | awk '/nthreads/ {printf "%d\t\t%.9f\t%.9f\n", $2, $5, $9}' >> q1.txt
	./q1 $i | awk '/nthreads/ {printf "%d\t\t& %.9f\t& %.9f\\\\\\hline\n", $2, $5, $9}' >> q1_tex.txt
done


#This makes 2 plots of the data: one is a PNG and the other is an EPS.  I used
#the EPS in a LaTeX report, but PNG can be used anywhere so I included that one
#as a backup.  It was no extra trouble.
cat << __EOF | gnuplot
set term png size 800,600 font "DejaVuSans-Bold" 
set output "q1_plot.png"
set title "Individual Thread Time vs. Number of Threads"
set xlabel "Number of Threads"
set ylabel "Average Thread Creation and Destruction Time (s)"
set autoscale
plot "q1.txt" using 1:3 title "Average Time" with linespoints pointtype 6 lw 5

set term postscript eps size 8,6 font "DejaVuSans-Bold, 30" enhanced color
set output "q1_plot.eps"
plot "q1.txt" using 1:3 title "Average Time" with linespoints pointtype 6 lw 10
__EOF

#=================================Run Program 2=================================