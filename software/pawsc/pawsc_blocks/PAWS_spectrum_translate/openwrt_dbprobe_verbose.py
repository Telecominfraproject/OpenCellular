#!/usr/bin/python

import pycurl,cStringIO,re
import sys, commands, time, json

#Get WiFi channel width and coordinates
#Set 5/10/20 MHz
# echo 10 > /sys/kernel/debug/ieee80211/phy0/ath5k/bwmode
# Set channel
# iwconfig wlan0 channel x

def get_uci(ustr):
    return commands.getoutput("uci get " + ustr)

#Constants

# Get GPS coordinates
# First check if GPS coords are hard coded if not pull coordinates from gps
if int(get_uci("whitespace.@general[0].gps_static")):
    lat = get_uci("whitespace.@general[0].lat")
    lng = get_uci("whitespace.@general[0].long")
else:
    json_data=open('/pmt/state/gpsstate')
    data = json.load(json_data)
    lat = data["lat"]
    lng = data["long"]
    json_data.close()

# get general_properties
D = int(get_uci("whitespace.@general[0].down_mhz"))
ctvs = int(get_uci("whitespace.@general[0].ch_tv_start"))
ctve = int(get_uci("whitespace.@general[0].ch_tv_end"))
cwfs = int(get_uci("whitespace.@general[0].ch_wf_start"))
cwfe = int(get_uci("whitespace.@general[0].ch_wf_end"))
fwfs = int(get_uci("whitespace.@general[0].freq_wifi_start"))
ftvs = int(get_uci("whitespace.@general[0].freq_tv_start"))
cwtv = int(get_uci("whitespace.@general[0].tv_bw"))

# get paws properties
query_interval = int(get_uci("whitespace.@paws[0].query_interval"))
paws_url = get_uci("whitespace.@paws[0].paws_url")
height = get_uci("whitespace.@paws[0].height")
height_type = get_uci("whitespace.@paws[0].height_type")
modelID = get_uci("whitespace.@paws[0].modelID")
serialnum = get_uci("whitespace.@paws[0].serialnum")
ID = get_uci("whitespace.@paws[0].ID")
fccID = get_uci("whitespace.@paws[0].fccID")

# get radio properties
pwr = int(get_uci("wireless.radio3.txpower"))
cwwf = int(get_uci("wireless.radio3.chanbw"))

#http_proxy = 'stretch.cs.uct.ac.za'
#proxy_port = 3128
#paws_url = 'http://whitespaces.meraka.csir.co.za/PawsService'
#ftvs=470 # Starting frequency of the UHF band in Southern Africa
#ctvs=21 # First channel in the UHF band
#D=1872 #Downconversion of doodle lab card
#cwtv=8 #Bandwidth of TV channels in Southern Africa
#fwfs=2422 #Starting frequency of 2.4GHz Wifi
#cwwf= int(channel_width) #WiFi channel width 5/10/20

            

print query_interval, lat, lng, height

def getSpectrum():
	response = cStringIO.StringIO()
	#postdata = '{"jsonrpc":"2.0", "method":"spectrum.paws.getSpectrum", \
	#"params":{"type":"AVAIL_SPECTRUM_REQ", "version":"1.0",\
	#"deviceDesc":{"serialNumber":"SN504", "fccId":"FCC110",\
	#"ModelID":"MN502"},"location":{"point":{"center":{"latitude":-34.129,"longitude":18.380}}},\
	#"antenna":{"height":10.2,"heightType":"AGL"} }, "id":"103"}'

	postdata = '{"jsonrpc":"2.0", "method":"spectrum.paws.getSpectrum", \
	"params":{"type":"AVAIL_SPECTRUM_REQ", "version":"1.0",\
	"deviceDesc":{"serialNumber":"' + str(serialnum) + '", "fccId":"' + str(fccID) + '",\
	"ModelID":"' + str(modelID) + '"},"location":{"point":{"center":{"latitude":'+str(lat)+',"longitude":'+str(lng)+'}}},\
	"antenna":{"height":' + str(height) + ',"heightType":"' + str(height_type) + '"} }, "id":"'+ str(ID) +'"}'

	print postdata

	headerdata = ['Content-Type:application/json']
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
		# Weird fix for python problem on openwrt where int(25.0) = 24
		ch = ctvs+(white_freq - (ftvs-(cwtv/2)))/cwtv + 0.1
		#ch = ctvs+(white_freq - (ftvs-(cwtv/2)))/cwtv 
		chi = int(ch)
		pair.append(chi)
	if "dbm" in rawfr:
		m = re.search('\"dbm\":(.+?)\}',rawfr)
		dbm = m.group(1)
		pwr = float(dbm)
		pair.append(pwr)
	if len(pair)==4:
		if pair[0] >= (ftvs-cwtv/2):
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

cwf = 14
print "WiFi chan: ",cwf, "Wifi centre freq: ",fwfs+(cwf-1)*5+7, "-- down -->",fwfs+(cwf-1)*5-D+7

print 
print "TV Channels and power levels available"
print "--------------------------------------" 
for p in freq_power_list:
	print p[0],p[1],p[2],p[3]

available = []
# Fill available matrix
for j in range(1,80):
	available.append(0)

for p in freq_power_list:
	available[p[2]]=p[3]

wifi_freq_power_list = []


fd=ftvs-ctvs


for cwf in range(1,15):

	wffb = fwfs+(cwf-1)*5-float(cwwf)/2
	wfft = fwfs+(cwf-1)*5+float(cwwf)/2

	if cwf == 14:
		wffb = wffb + 7
		wfft = wfft + 7
		
		
	#cb=(wffb-D+7*ftvs-8*fd)/8
	#ct=(wfft-D+7*ftvs-8*fd)/8
	C = ftvs-cwtv/2-8*ctvs
	cb=int((wffb-D-C)/8)
	ct=int((wfft-D-C)/8)	

	lowpower=1000
	for i in range(cb,ct+1):
		curpower=available[i]
		if curpower < lowpower:
			lowpower=available[i]

	if lowpower > 0:
		#print float(cwwf)/2
		#print float(fwfs+(cwf-1)*5)-float(cwwf)/2
		wifi_freq_power_list.append((wffb,wfft,cwf,lowpower)) 


print 
print "WiFi Channels and power levels available"
print "----------------------------------------"
for p in wifi_freq_power_list:
	print p[0],p[1],p[2],p[3]

#store time since epoch
tepoch = int(time.time())

# Build up json string
jstring = '{"querytime":"' + str(tepoch)\
+'","downconvert":"' + str(D)\
+'","cwwf":"' + str(cwwf)\
+'","cwtv":"' + str(cwtv)\
+'","ctvs":"' + str(ctvs)\
+'","ctve":"' + str(ctve)\
+'","cwfs":"' + str(cwfs)\
+'","cwfe":"' + str(cwfe)\
+'","fwfs":"' + str(fwfs)\
+'","ftvs":"' + str(ftvs)\
+'","tvspectra":['
if len(freq_power_list) > 0:
	for p in freq_power_list[0:-1]:
		jstring = jstring + '{"start":"'+str(p[0])+'","end":"'+str(p[1])+'","chan":"'+str(p[2])+'","dbm":"'+str(p[3])+'"},'
	p = freq_power_list[-1]
	jstring = jstring + '{"start":"'+str(p[0])+'","end":"'+str(p[1])+'","chan":"'+str(p[2])+'","dbm":"'+str(p[3])+'"}],'
	jstring = jstring + '"wfspectra":['

if len(wifi_freq_power_list) > 0:
	for wp in wifi_freq_power_list[0:-1]:
		jstring = jstring + '{"start":"'+str(wp[0])+'","end":"'+str(wp[1])+'","chan":"'+str(wp[2])+'","dbm":"'+str(p[3])+'"},'
	wp = wifi_freq_power_list[-1]
	jstring = jstring + '{"start":"'+str(wp[0])+'","end":"'+str(wp[1])+'","chan":"'+str(wp[2])+'","dbm":"'+str(p[3])+'"}]}'
else:
	jstring = jstring + ']}'

jload = json.loads(jstring)
print json.dumps(jload, indent=4, sort_keys=False)
json_dbdata=open('/pmt/state/dbstate','w')
json_dbdata.write(json.dumps(jload, indent=4))
json_dbdata.close()
