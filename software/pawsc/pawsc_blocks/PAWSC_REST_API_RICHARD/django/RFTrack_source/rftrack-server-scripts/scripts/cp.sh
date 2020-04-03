#!/bin/bash
# ------------------------------------------------------------
# Script insert campaign info in dbindex database
#
# Author: Richard Maliwatu
# Ver. 1.0, 7 October 2019
#
# -------------------------------------

if [ "$#" -ne 1 ]; then	
   echo "Use: $0 <database name>"
	exit 1
fi


# list of full path of programs
SCRIPT=$(realpath $0)					# full script name with path
SCRIPTPATH=$(dirname $SCRIPT)			# script path dir

DIRDB="../../../uploaded_spec_scans" #where uploads land
dbDownloadDir="../../../base/static/reports/" #dir for reports generated

campaignName=$1

mkdir -p $dbDownloadDir"${campaignName%.*}" #strip off extension

newCampaignDir=$dbDownloadDir"${campaignName%.*}"

cd $newCampaignDir
mkdir -p filedown images

cd $SCRIPTPATH #get back to working directory

# -----------------------------------------------------------------
# Generate the html report
# -----------------------------------------------------------------

cat index_template.html > tmp/index.html
cd tmp

source info.txt

echo $campaignDate
echo $campaignDuration
echo $routeLength
echo $nPoints
echo $country
echo $countryCode
echo $placeOfMeasurements

flag=$(echo "$countryCode" | awk '{print tolower($0)}') #change country code to lower case

#sed -Ei "0,/(<insertionPoint>)/s//<textToInsert>/"  index.html #replaces the text at the insertion point
sed -Ei "0,/(<campaignDate>)/s//$campaignDate/"  index.html
sed -Ei "0,/(<campaignDuration>)/s//$campaignDuration/"  index.html
sed -Ei "0,/(<routeLength>)/s//$routeLength/"  index.html
sed -Ei "0,/(<nPoints>)/s//$nPoints/"  index.html
sed -Ei "0,/(<country>)/s//$country/"  index.html
sed -Ei "0,/(<countryCode>)/s//$countryCode/"  index.html
sed -Ei "0,/(<placeOfMeasurements>)/s//$placeOfMeasurements/"  index.html
sed -Ei "0,/(<region>)/s//$region/"  index.html
sed -Ei "0,/(<population>)/s//$population/"  index.html
sed -Ei "0,/(<reportDir>)/s//$reportDir/"  index.html
sed -Ei "0,/(<dbName>)/s//$dbName/"  index.html

cd .. #return to script working directory

#cp db
cp $DIRDB/$campaignName $newCampaignDir/filedown/$campaignName

#cp plots
cp tmp/foutput.png $newCampaignDir/images/spectral-map.png
cp tmp/histogram_85.png $newCampaignDir/images/perc_85.png
cp tmp/histogram_90.png $newCampaignDir/images/perc_90.png
cp tmp/histogram_95.png $newCampaignDir/images/perc_95.png
cp tmp/histogram_100.png $newCampaignDir/images/perc_100.png

#cp , html and pdf report
cp tmp/index.html $newCampaignDir/index.html

xvfb-run wkhtmltopdf $newCampaignDir/index.html $newCampaignDir/filedown/report.pdf #syntax: xvfb-run wkhtmltopdf <in_htmlfile> <out_pdffile> 

#cp tmp/report.pdf $newCampaignDir/filedown/report.pdf

cp $dbDownloadDir/ico -r $newCampaignDir

#cp flag
cp flags-normal/$flag.png $newCampaignDir/images/flagcountry.png



exit 0

# -----------------------------------------------------------------------
# end of script
