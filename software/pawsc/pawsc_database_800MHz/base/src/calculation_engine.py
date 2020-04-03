import re, json
import requests
from io import BytesIO
import sys


def getSpectrum_CSIR(databaseID, apiKey, height_type, height, latitude, longitude, serialNum, modelID, regulatorID  ):

    calc_url = 'https://glsdceapis.meraka.csir.co.za/CSIRCE'

    #response = StringIO()
    postdata = {}

    # Fill in dictionary
    postdata['id'] = "123456"
    postdata['jsonrpc'] = "2.0"
    postdata['method'] = "csirce.getSpectrum"

    params = {}
    params['type'] = "AVAIL_SPECTRUM_REQ"
    params['apiVersion'] = "1.2"

    uniqueID = {}
    uniqueID['databaseID'] = databaseID
    uniqueID['apiKey'] = apiKey
    params['uniqueID'] = uniqueID

    antenna = {}
    antenna['heightType'] = height_type
    antenna['height'] = height
    params['antenna'] = antenna

    coord = {}
    coord['latitude'] = latitude
    coord['longitude'] = longitude

    center = {}
    center['center'] = coord

    point = {}
    point['point'] = center
    params['location'] = point

    devicedesc = {}
    devicedesc['serialNumber'] = serialNum
    devicedesc['modelId'] = modelID
    devicedesc['regulatorId'] = regulatorID
    params['deviceDesc'] = devicedesc

    postdata['params'] = params
    
   
    headerdata = ['Content-Type:application/json']
    #print (postdata)

    headers={'Content-type': 'application/json; charset=UTF-8'}
    r = requests.post (calc_url, data=json.dumps(postdata),headers=headers)
    return r.text


def getSpectrumGLSD(calcEngine, latitude, longitude, height_type, height):

    # Get result from spectrum database
    if (calcEngine == 'CSIR'):
        rawresult = getSpectrum_CSIR("UCT-ZA","7f9f3dcb-5a09-463b-b105-d49df68e67ff",\
        height_type,height,latitude,longitude,"SN504","MN110","FCC110")

    #print (rawresult)
    rawresult_json = json.loads(rawresult)
    if not('error' in rawresult):
        if ('spectrumSpecs' in rawresult):
            profileHz = rawresult_json['result']['spectrumSpecs']
            return profileHz
        else:
            return rawresult
    else:
        return rawresult





'''
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
'''