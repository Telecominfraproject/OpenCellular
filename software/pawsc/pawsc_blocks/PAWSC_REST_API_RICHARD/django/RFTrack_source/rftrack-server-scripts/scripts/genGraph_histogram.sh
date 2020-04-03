#!/bin/bash


data="tmp/graph.txt"
xdim=800			
ydim=600
ftype="sans Bold, 14"
# graph -85 dBm
ylab="Percentage of Measurements below -85 dBm"
ycol=2
foutput="tmp/histogram_85.png"
gnuplot -e "data='${data}'; ylab='${ylab}'; xdim='${xdim}'; ydim='${ydim}'; ftype='${ftype}';ycol=${ycol}; foutput='${foutput}'" grpbelow.sh
# graph -90 dBm
ylab="Percentage of Measurements below -90 dBm"
ycol=3
foutput="tmp/histogram_90.png"
gnuplot -e "data='${data}'; ylab='${ylab}'; xdim='${xdim}'; ydim='${ydim}'; ftype='${ftype}';ycol=${ycol}; foutput='${foutput}'" grpbelow.sh
# graph -95 dBm
ylab="Percentage of Measurements below -95 dBm"
ycol=4
foutput="tmp/histogram_95.png"
gnuplot -e "data='${data}'; ylab='${ylab}'; xdim='${xdim}'; ydim='${ydim}'; ftype='${ftype}';ycol=${ycol}; foutput='${foutput}'" grpbelow.sh
# graph -100 dBm
ylab="Percentage of Measurements below -100 dBm"
ycol=5
foutput="tmp/histogram_100.png"
gnuplot -e "data='${data}'; ylab='${ylab}'; xdim='${xdim}'; ydim='${ydim}'; ftype='${ftype}';ycol=${ycol}; foutput='${foutput}'" grpbelow.sh





