#!/bin/python

import pycurl,cStringIO,re
import sys

#Get WiFi channel width and coordinates
#Set 5/10/20 MHz
# echo 10 > /sys/kernel/debug/ieee80211/phy0/ath5k/bwmode
# Set channel
# iwconfig wlan0 channel x

if len(sys.argv) != 4:
	print "Usage: prog <WiFi Channel width> <latitude> <longitude>"
	print "Example (Cape Town): query_paws 20 -34.129 18.380"
	print "Example (Karoo): query_paws 5 -30.000 20.000"
	exit()

latitude = sys.argv[2]
longitude = sys.argv[3]
channel_width = sys.argv[1]


#Constants
#http_proxy = 'stretch.cs.uct.ac.za'
#proxy_port = 3128
paws_url = 'http://whitespaces.meraka.csir.co.za/PawsService'
ftvs=470 # Starting frequency of the UHF band in Southern Africa
ctvs=21 # First channel in the UHF band
fd=ftvs-ctvs
D=1872 #Downconversion of doodle lab card
cwtv=8 #Bandwidth of TV channels in Southern Africa
fwfs=2422 #Starting frequency of 2.4GHz Wifi
cwwf= int(channel_width) #WiFi channel width 5/10/20


def getSpectrum():
	response = cStringIO.StringIO()
	#postdata = '{"jsonrpc":"2.0", "method":"spectrum.paws.getSpectrum", \
	#"params":{"type":"AVAIL_SPECTRUM_REQ", "version":"1.0",\
	#"deviceDesc":{"serialNumber":"SN504", "fccId":"FCC110",\
	#"ModelID":"MN502"},"location":{"point":{"center":{"latitude":-34.129,"longitude":18.380}}},\
	#"antenna":{"height":10.2,"heightType":"AGL"} }, "id":"103"}'
	postdata = '{"jsonrpc":"2.0", "method":"spectrum.paws.getSpectrum", \
	"params":{"type":"AVAIL_SPECTRUM_REQ", "version":"1.0",\
	"deviceDesc":{"serialNumber":"SN504", "fccId":"FCC110",\
	"ModelID":"MN502"},"location":{"point":{"center":{"latitude":'+latitude+',"longitude":'+longitude+'}}},\
	"antenna":{"height":10.2,"heightType":"AGL"} }, "id":"103"}'
	headerdata = ['Content-Type:application/json']
	print postdata
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
print rawresult

#Parse result from spectrum database
freqpowerlist = rawresult.split(',')
freq_power_list = []

pair=[]
prev_pair = []
for rawfr in freqpowerlist:
	if "hz" in rawfr:
		m = re.search('\"hz\":(.+?)E8',rawfr)
		fr = m.group(1)
		white_freq = 100*float(fr)
		pair.append(white_freq)
		pair.append(white_freq+cwtv)
		pair.append(int(ctvs+(white_freq-ftvs)/cwtv))
	if "dbm" in rawfr:
		m = re.search('\"dbm\":(.+?)\}',rawfr)
		dbm = m.group(1)
		pwr = float(dbm)
		pair.append(pwr)
	if len(pair)==4:
		if pair[0] >= ftvs:
			if len(prev_pair) > 0 and (pair[0] == prev_pair[0]):
				if (pair[3] > prev_pair[3]):
					freq_power_list[-1][3] = pair[3]
			else:
				freq_power_list.append(pair)
		prev_pair = pair
		pair = []


# Calculate available WiFi channels based on the channels returned

print 
print "Configuration"
print "-------------"
print "Start Freq Wifi:",fwfs
print "Start Freq UHF:",ftvs 
print "Downconverter (MHz):",D
print "Mapping:"
for cwf in range(1,14):
	print "WiFi chan: ",cwf, "Wifi centre freq: ",fwfs+(cwf-1)*5, "-- down -->",fwfs+(cwf-1)*5-D

print 
print "TV Channels and power levels available"
print "--------------------------------------" 
for p in freq_power_list:
	print p[0],p[1],p[2],p[3]

available = []
# Fill available matrix
for j in range(1,70):
	available.append(0)

for p in freq_power_list:
	available[p[2]]=p[3]

wifi_freq_power_list = []

for cwf in range(1,14):
	cb=int(((fwfs+(cwf-1)*5-float(cwwf/2)-D+7*ftvs-8*fd)/8))
	ct=int(((fwfs+(cwf-1)*5+float(cwwf/2)-D+7*ftvs-8*fd)/8))
	lowpower=1000
	for i in range(cb,ct+1):
		curpower=available[i]
		if curpower < lowpower:
			lowpower=available[i]

	if lowpower > 0:
		print float(cwwf)/2
		print float(fwfs+(cwf-1)*5)-float(cwwf)/2
		wifi_freq_power_list.append((fwfs+(cwf-1)*5-float(cwwf)/2,fwfs+(cwf-1)*5+float(cwwf)/2,cwf,lowpower)) 


print 
print "WiFi Channels and power levels available"
print "----------------------------------------"
for p in wifi_freq_power_list:
	print p[0],p[1],p[2],p[3]



