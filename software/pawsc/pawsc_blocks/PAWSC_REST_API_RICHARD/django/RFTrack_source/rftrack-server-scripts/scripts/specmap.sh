#! /usr/bin/gnuplot


reset

#Taking a stab at the spectrum map, what David calls waterfall plot:)


# This script creates the spectral map
# Input:
# data: file name with data
# xdim: x dimension
# ydim:  dimension
# ftype: font command
# foutput: file name output
# ------------------------------------------------------------

ftype="Helvetica,10"

set pm3d map
set palette color #Richard
set terminal png 
set title "\n"
#set term png truecolor size xdim,ydim #Richard: original line
#set term png truecolor size 640, 480 #Richard: default size
set terminal png medium size 640,480 
                     


# set label 1 "Location: `head -1 position.txt`\nFrom `head -1 start_time.txt` to `head -1 end_time.txt`" at graph 0.1,1.125 left
# set label 1 "From `head -1 tmp/start_time.txt` to `head -1 tmp/end_time.txt`" at graph 0.1,1.125 left
# set label 1 "From `head -c 19 tmp/start_time.txt` to `head -c 19 tmp/end_time.txt`" at graph 0.1,1.125 left font ftype


set title "From `head -c 19 tmp/start_time.txt` to `head -c 19 tmp/end_time.txt`" font ftype #Richard

#set title "From <time ... > to <time>"
#set lmargin 5 #Richard: original line
set lmargin at screen 0.2
#set rmargin 5 #Richard: original line
set rmargin at screen 0.8
set tmargin 5
set bmargin 5
# set output 'spectral-map.png'
#set output foutput #Richard
set output "tmp/foutput.png"
set tics font ftype
set xlabel "Measurement number" font ftype
set ylabel "Frequency (MHz)\n" font ftype
set cblabel "\nRSSI (dBm)" font ftype
# set format y "%g %cMHz";
set autoscale yfix
set autoscale xfix

set style line 12 lc rgb "black" lt 0 lw 1
set grid front ls 12



#splot data.txt #Richard: original line

splot 'tmp/spec_map.txt'

set cbrange [0 to GPVAL_DATA_Z_MAX] #Richard: original line
set xrange [GPVAL_DATA_X_MIN to GPVAL_DATA_X_MAX]
#set yrange [3.2 to 3.45]

set key left bottom Left title 'Legend' box 3
#unset key
replot
#unset output






