#!/bin/bash
#-------------------------------------------------------------
# Author: Richard Maliwatu
# Date created: 7 October 2091
# This scripts updates the reports landing page
#

if [ "$#" -ne 1 ]; then	
   echo "Use: $0 <database name>"
	exit 1
fi

echo "update-index.sh called"

#file to update
HTMLFILE="../../../base/src/templates/spectrum_measurement_reports.html" #assume this is the html file to update
dbFileName=$1
reportDir="${dbFileName%.*}" #strip off extension

#The following characters needing escaping: !, -, /, "
insertionPoint="<\!\-\- row table \-\->" #locate where to insert html code

source tmp/info.txt

textToInsert="<\!\-\- row table \-\->   \
<tr>  \
<td img> \
<a href=\"{% static 'reports\/$reportDir\/index.html' %}\"> \
{% load static %}  <img src=\"{% static  'reports\/$reportDir\/images\/spectral\-map.png'  %}\" <\/a> \
<\/td> \
<td img> \
<a href=\"{%static 'reports\/$reportDir\/filedown\/report.pdf' %}\"> \
{% load static %} <img src=\"{% static 'reports\/$reportDir\/ico\/pdfdownload.png' %}\" <\/a> \
<\/td> \
<td style=\"vertical\-align:middle;\"> \
${campaignDate% *} \
<\/td> \
<td style=\"vertical\-align:middle;\"> \
$country \
<\/td> \
<td style=\"vertical\-align:middle;\"> \
$placeOfMeasurements\
<\/td>\
<td style=\"vertical\-align:middle;\">\
$region\
<\/td>\
<td style=\"vertical\-align:middle;\">\
$routeLength Km\
<\/td>\
<td style=\"vertical\-align:middle;\">\
$campaignDuration\
<\/td>\
<td style=\"vertical\-align:middle;\">\
$freqRange MHz\
<\/td>\
<td style=\"vertical\-align:middle;\">\
 $nPoints\
<\/td>\
<td style=\"vertical\-align:middle;\">\
<a href=\"{%static  'reports\/$reportDir\/filedown\/$dbFileName' %}\">\
{% load static %} <img src=\"{% static 'reports\/$reportDir\/ico\/download.png' %}\">\
<\/a>\
<\/td>\
<\/tr>\
<\!\-\- row table \-\->"


#sed -i '/^hello2/ a file.txt' $HTMLFILE #'a' implies append, 'r' would be needed to read input from file instead
#sed -i '0,/INSERTION/ a XXX XX X' $HTMLFILE #'i' implies insert in specified file otherwise it send only displays on the 

#https://unix.stackexchange.com/questions/517762/how-to-insert-text-in-a-file-after-particular-pattern-in-shell-script?rq=1
#sed -Ei 's/(\here)/\1[delta6=delta6file;delta6Dir=755|workpm]/' $HTMLFILE #working version
#sed -Ei "0,/(\here)/s//[delta6=delta6file;delta6Dir=755|workpm]/" $HTMLFILE #replaces the search term

#https://unix.stackexchange.com/questions/77549/replacing-text-between-two-html-comments

#sed -Ei "0,/(<\!\-\- row table \-\->)/s//[delta6=delta6file;delta6Dir=755|workpm]/" $HTMLFILE #replaces the search term
sed -Ei "0,/($insertionPoint)/s//$textToInsert/" $HTMLFILE #replaces the text at the insertion point


#sed -Ei "0,/(<\!\-\-$STARTTAG\-\->)(.*)(<\!\-\-$ENDTAG\-\->)/s//[delta6=delta6file;delta6Dir=755|workpm]/" $HTMLFILE #replaces the search term

#echo "hello" >> $HTMLFILE

