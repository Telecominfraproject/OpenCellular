#!/bin/python

import pycurl,cStringIO,re
import sys, json

#Get WiFi channel width and coordinates
#Set 5/10/20 MHz
# echo 10 > /sys/kernel/debug/ieee80211/phy0/ath5k/bwmode
# Set channel
# iwconfig wlan0 channel x

#if len(sys.argv) != 4:
#	print "Usage: prog <WiFi Channel width> <latitude> <longitude>"
#	print "Example (Cape Town): query_paws 20 -34.129 18.380"
#	print "Example (Karoo): query_paws 5 -30.000 20.000"
#	exit()

#latitude = sys.argv[2]
#longitude = sys.argv[3]
#channel_width = sys.argv[1]


#Constants
#http_proxy = 'stretch.cs.uct.ac.za'
#proxy_port = 3128
paws_url = 'https://whitespaces.meraka.csir.co.za/api/paws'
ftvs=470 # Starting frequency of the UHF band in Southern Africa
ctvs=21 # First channel in the UHF band
fd=ftvs-ctvs
D=1872 #Downconversion of doodle lab card
cwtv=8 #Bandwidth of TV channels in Southern Africa
fwfs=2422 #Starting frequency of 2.4GHz Wifi
#cwwf= int(channel_width) #WiFi channel width 5/10/20


def getSpectrum():
	response = cStringIO.StringIO()
	#postdata = '{"jsonrpc":"2.0", "method":"spectrum.paws.getSpectrum", \
	#"params":{"type":"AVAIL_SPECTRUM_REQ", "version":"1.0",\
	#"deviceDesc":{"serialNumber":"SN504", "fccId":"FCC110",\
	#"ModelID":"MN502"},"location":{"point":{"center":{"latitude":-34.129,"longitude":18.380}}},\
	#"antenna":{"height":10.2,"heightType":"AGL"} }, "id":"103"}'
	register_postdata_json = {
		"id": "45455",
		"jsonrpc": "2.0",
		"method": "spectrum.paws.register",
		"params": {
			"apiKey": "xxxxxxxx-xxxx-Mxxx-Nxxx-xxxxxxxxxxxx",
			"type": "REGISTRATION_REQ",
			"version": "1.0",
			"deviceDesc": {
				"serialNumber": "asdad1748kopol",
				"typeApprovalId": "TA-2016/001",
				"modelId": "asdsa444",
				"deviceType": "fixed",
				"deviceCategory": "master",
				"emissionClass": "3",
				"rulesetIds": ["ICASATVWS-2018", "FccTvBandWhiteSpace-2010"]
			},
			"masterDeviceDesc":{
				"serialNumber": "asdad1748kopol"
			},
			"deviceDescs":[{
				"serialNumber": "asdad1748kopol",
				"typeApprovalId": "csir",
				"modelId": "wireless",
				"deviceType": "fixed",
				"deviceCategory": "Client",
				"emissionClass": "1",
				"rulesetIds": ["ICASATVWS-2018", "FccTvBandWhiteSpace-2010"]
			},
			{
				"serialNumber": "asdad1748kopol",
				"typeApprovalId": "lpm",
				"modelId": "lpm",
				"deviceType": "fixed",
				"deviceCategory": "Client",
				"emissionClass": "5",
				"rulesetIds": ["ICASATVWS-2018", "FccTvBandWhiteSpace-2010"]
			}],
			"location": {
				"point": {
					"center": {
						"latitude": -25.752660,
						"longitude": 28.253984
					}
				},
				"confidence": 95
			},
			"locations": [
				{
					"point": {
						"center": {
							"latitude": -25.7,
							"longitude": 40.5
						}
					},
					"confidence": 95
				},
				{
				"point": {
					"center": {
						"latitude": -99.15,
						"longitude": 15.8
					}
				},
				"confidence": 95
			}
			],
			"antenna": {
				"height": 3.1,
				"heightType": "AGL"
			},
			"deviceOwner": {
				"owner": "[\"vcard\",[[\"version\",{},\"text\",\"4.0\"],[\"fn\",{},\"text\",\"Size Testing\"],[\"tel\",{\"type\":\"work\"},\"text\",\"(012) 841-4766\"],[\"email\",{\"type\":\"work\"},\"text\",\"mmofolo@csir.co.za\"],[\"adr\",{\"type\":\"work\"},\"text\",[\"\",\"\",\"CSIR Meraka Institute\",\"Brummeria\",\"Pretoria\",\"0184\",\"RSA\"]],[\"prodid\",{},\"text\",\"ez-vcard ${version}\"]]]",
				"operator": "[\"vcard\",[[\"version\",{},\"text\",\"4.0\"],[\"fn\",{},\"text\",\"Size Testing\"],[\"tel\",{\"type\":\"work\"},\"text\",\"(012) 841-3028\"],[\"email\",{\"type\":\"work\"},\"text\",\"mmofolo@csir.co.za\"],[\"adr\",{\"type\":\"work\"},\"text\",[\"\",\"\",\"CSIR Meraka Institute\",\"Brummeria\",\"Pretoria\",\"0184\",\"RSA\"]],[\"prodid\",{},\"text\",\"ez-vcard ${version}\"]]]"
			},
			"spectra": [{
				"resolutionBwHz": 8e6,
				"profiles": [[{
					"hz": 5.18e8,
					"dbm": 30.0
				},
				{
					"hz": 5.26e8,
					"dbm": 30.0
				}]]
			}]
		}
	}

	queryspectrum_postdata_json = {
		"id": "45455",
		"jsonrpc": "2.0",
		"method": "spectrum.paws.getSpectrum",
		"params": {
			"apiKey": "xxxxxxxx-xxxx-Mxxx-Nxxx-xxxxxxxxxxxx",
			"type": "AVAIL_SPECTRUM_REQ",
			"version": "1.0",
			"deviceDesc": {
				"serialNumber": "asdad1748kopol",
				"typeApprovalId": "TA-2016/001",
				"modelId": "asdsa444",
				"deviceType": "fixed",
				"deviceCategory": "master",
				"emissionClass": "3",
				"rulesetIds": ["ICASATVWS-2018", "FccTvBandWhiteSpace-2010"]
			},
			"masterDeviceDesc":{
				"serialNumber": "asdad1748kopol"
			},
			"deviceDescs":[{
				"serialNumber": "asdad1748kopot",
				"typeApprovalId": "TA-2016/001",
				"modelId": "asdsa444",
				"deviceType": "nomadic",
				"deviceCategory": "Client",
				"emissionClass": "3",
				"rulesetIds": ["ICASATVWS-2018", "FccTvBandWhiteSpace-2010"]
			},
			{
				"serialNumber": "mofolo",
				"typeApprovalId": "TA-2016/001",
				"modelId": "mofolo",
				"deviceType": "fixed",
				"deviceCategory": "master",
				"emissionClass": "5",
				"rulesetIds": ["ICASATVWS-2018", "FccTvBandWhiteSpace-2010"]
			}],
			"location": {
				"point": {
					"center": {
						"latitude": -25.752660,
						"longitude": 28.253984
					}
				},
				"confidence": 95
			},
			"antenna": {
				"height": 3.1,
				"heightType": "AGL"
			},
			"deviceOwner": {
				"owner": "[\"vcard\",[[\"version\",{},\"text\",\"4.0\"],[\"fn\",{},\"text\",\"Size Testing\"],[\"tel\",{\"type\":\"work\"},\"text\",\"(012) 841-4766\"],[\"email\",{\"type\":\"work\"},\"text\",\"marge@meraka.co.za\"],[\"adr\",{\"type\":\"work\"},\"text\",[\"\",\"\",\"CSIR Meraka Institute\",\"Brummeria\",\"Pretoria\",\"0184\",\"RSA\"]],[\"prodid\",{},\"text\",\"ez-vcard ${version}\"]]]",
				"operator": "[\"vcard\",[[\"version\",{},\"text\",\"4.0\"],[\"fn\",{},\"text\",\"Size Testing\"],[\"tel\",{\"type\":\"work\"},\"text\",\"(012) 841-3028\"],[\"email\",{\"type\":\"work\"},\"text\",\"marge@meraka.co.za\"],[\"adr\",{\"type\":\"work\"},\"text\",[\"\",\"\",\"CSIR Meraka Institute\",\"Brummeria\",\"Pretoria\",\"0184\",\"RSA\"]],[\"prodid\",{},\"text\",\"ez-vcard ${version}\"]]]"
			},
			"spectra": [{
				"resolutionBwHz": 8e6,
				"profiles": [[{
					"hz": 5.18e8,
					"dbm": 30.0
				},
				{
					"hz": 5.26e8,
					"dbm": 30.0
				}]]
			}]
		}
	}

	#postdata = json.dumps(queryspectrum_postdata_json)
	postdata = json.dumps(register_postdata_json)
	headerdata = ['Content-Type:application/json']
	#print (postdata)
	c = pycurl.Curl()
	#c.setopt(pycurl.PROXY, http_proxy)
	#c.setopt(pycurl.PROXYPORT, proxy_port)
	c.setopt(pycurl.URL, paws_url)
	c.setopt(pycurl.HTTPHEADER,headerdata)
	c.setopt(pycurl.POST, 1)
	c.setopt(pycurl.POSTFIELDS, postdata)
	c.setopt(c.WRITEFUNCTION, response.write)
	c.perform()
	return response.getvalue()

# Get result from spectrum database
rawresult = getSpectrum()
#print (json.dumps(rawresult, sort_keys=True, indent=4, ))
str = json.loads(rawresult)
print (json.dumps(str,indent=4))




