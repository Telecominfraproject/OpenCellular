#!/bin/bash
#****************************************************************************
# To remove the “|” in the output, I use sed by running the script as follows:
# ./testSQL.sh |  sed 's/|/ /g' > out.txt.
# Alternatively I’m sure the default gnuplot behaviour can be changed by e.g.
# set datafile separate “ “ 
# or something like that
#****************************************


#echo ${dbname%.*} > tmp/dbfilename.txt #remove extension

#note other campaing info

source tmp/info.txt

echo $campaignDate
echo $campaignDuration
echo $routeLength
echo $nPoints
echo $country
echo $countryCode
echo $placeOfMeasurements

#echo "Campaign date: "$campaignDate > tmp/info.txt
#echo "Campaign duration: "$campaignDuration >> tmp/info.txt
#echo "Route length: "$routeLength >> tmp/info.txt
#echo "N. points: "$numberOfPoints >> tmp/info.txt
#echo "Country: "$countryName >> tmp/info.txt
#echo "Country code: "$countryCode >> tmp/info.txt
#echo "Place of measurements: "$placeOfMeasurements >> tmp/info.txt



