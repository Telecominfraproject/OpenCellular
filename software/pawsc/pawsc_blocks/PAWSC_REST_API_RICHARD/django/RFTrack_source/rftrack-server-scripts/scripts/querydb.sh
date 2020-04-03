#!/bin/bash
#****************************************************************************
# To remove the “|” in the output, I use sed by running the script as follows:
# ./testSQL.sh |  sed 's/|/ /g' > out.txt.
# Alternatively I’m sure the default gnuplot behaviour can be changed by e.g.
# set datafile separate “ “ 
# or something like that
#****************************************

if [ "$#" -ne 1 ]; then	
   echo "Use: $0 <database name>"
	exit 1
fi

DIRDB="../../../uploaded_spec_scans"
dbname=$1

#Determine number of measurements
count=$(sqlite3 $DIRDB/$dbname  "SELECT MAX(id) FROM dbmdata";) 

#note the start and end time of measurement campaign
startTime=$(sqlite3 $DIRDB/$dbname  "SELECT MIN(created_at) FROM dbmdata";) 
endTime=$(sqlite3 $DIRDB/$dbname  "SELECT MAX(created_at) FROM dbmdata";)

if [ ! -d tmp ]
  then mkdir tmp
fi

echo $startTime > tmp/start_time.txt
echo $endTime > tmp/end_time.txt
echo $dbname > tmp/dbfilename.txt #not sure if this is needed??
#echo ${dbname%.*} > tmp/dbfilename.txt #remove extension

#note other campaing info
campaignDate=$(sqlite3 $DIRDB/$dbname "SELECT date FROM acqinfo";)
campaignDuration=$(sqlite3 $DIRDB/$dbname "SELECT timeDelta FROM acqinfo";)
routeLength=$(sqlite3 $DIRDB/$dbname "SELECT ROUND(lenRoute, 1) FROM acqinfo";)
numberOfPoints=$(sqlite3 $DIRDB/$dbname "SELECT len_dbmdata FROM acqinfo";)
countryName=$(sqlite3 $DIRDB/$dbname "SELECT countryName FROM acqinfo";)
countryCode=$(sqlite3 $DIRDB/$dbname "SELECT countryCode FROM acqinfo";)
placeOfMeasurements=$(sqlite3 $DIRDB/$dbname "SELECT toponymName FROM acqinfo";)
startFreq=$(sqlite3 $DIRDB/$dbname "SELECT start_freq/1000 FROM acqinfo";)
endFreq=$(sqlite3 $DIRDB/$dbname "SELECT end_freq/1000 FROM acqinfo";)
region=$(sqlite3 $DIRDB/$dbname "SELECT adminName1 FROM acqinfo";)
population=$(sqlite3 $DIRDB/$dbname "SELECT population FROM acqinfo";)

echo "campaignDate="\"$campaignDate\" > tmp/info.txt #create/overwrite file
echo "campaignDuration="\"$campaignDuration\" >> tmp/info.txt #append to file
echo "routeLength="\"$routeLength\" >> tmp/info.txt
echo "nPoints="\"$numberOfPoints\" >> tmp/info.txt
echo "country="\"$countryName\" >> tmp/info.txt
echo "countryCode="\"$countryCode\" >> tmp/info.txt
echo "placeOfMeasurements="\"$placeOfMeasurements\" >> tmp/info.txt
echo "freqRange="\"$startFreq..$endFreq\" >> tmp/info.txt
echo "region="\"$region\" >> tmp/info.txt
echo "population="\"$population\" >> tmp/info.txt

echo "reportDir="\"${dbname%.*}\" >> tmp/info.txt #dbname without extension
echo "dbName="\"$dbname\" >> tmp/info.txt

outfile="tmp/spec_map.txt"
>$outfile
for ((j=1; j<=$count; j++))
do
	for i in {000..111};
	do		
 		line=$(sqlite3 $DIRDB/$dbname "SELECT dbmdata.id, f$i, v$i FROM config, dbmdata WHERE dbmdata.id=$j";)
	echo $line | sed 's/|/ /g' >> $outfile #assuming empty file, append to it. Use sed to remove "|" in the output	
	done
        echo >> $outfile #insert blank line for gnuplot map plot
done

#<query db and prep data for any other plots>

outfile="tmp/graph.txt"
>$outfile #create empty file
for i in {000..111};
do	 		
	line=$(sqlite3  $DIRDB/$dbname "SELECT 
				(SELECT f$i FROM config) AS x,
				(SELECT ((COUNT(v$i)*1.0)/$count)*100 FROM dbmdata WHERE dbmdata.v$i < -85) AS col2,
				(SELECT ((COUNT(v$i)*1.0)/$count)*100 FROM dbmdata WHERE dbmdata.v$i < -90) AS col3,
				(SELECT ((COUNT(v$i)*1.0)/$count)*100 FROM dbmdata WHERE dbmdata.v$i < -95) AS col4,
				(SELECT ((COUNT(v$i)*1.0)/$count)*100 FROM dbmdata WHERE dbmdata.v$i < -100) AS col5";)
	echo $line | sed 's/|/ /g' >> $outfile #assuming empty file, append to it. Use sed to remove "|" in the output
		
		
done

#when done, call plotting script
./specmap.sh
./genGraph_histogram.sh

#call other plotting scripts

./cp.sh $dbname #move plots to where they need to be

#makes sense to update the htmlindex file here

./update-index.sh $dbname

#assuming everything went ok: clean up all tmp files

rm -r tmp/*
exit
