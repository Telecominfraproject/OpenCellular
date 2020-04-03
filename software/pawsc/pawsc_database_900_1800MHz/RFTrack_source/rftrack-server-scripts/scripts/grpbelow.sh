#   #! /usr/bin/gnuplot 
#load "histograms.dem"

reset
set datafile separator " "					# separator from colums
set term png truecolor size xdim,ydim
set output foutput							# foutput: var with file name
set tics font ftype
set xlabel "Frequency (MHz)" font ftype
# set ylabel ylab font ftype
set ylabel ylab font ftype
set autoscale yfix
set autoscale xfix
set grid
set boxwidth 0.95 relative
set style fill transparent solid 0.5 noborder
plot data u 1:(column(ycol)) w boxes lc rgb"green" notitle
