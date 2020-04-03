import datetime
from datetime import timedelta

from base.src.calculation_engine import getSpectrumGLSD
from base.src.cellularSpectrum import convertCellular


SpecResp = {
		"type": "AVAIL_SPECTRUM_RESP",
		"version": "1.0",
		"deviceDesc": {
			"serialNumber": "123456",
			"typeApprovalId": "TA-2019/001",
			"modelId": "OCLTE12345",
			"deviceType": "fixed",
			"deviceCategory": "master",
			"emissionClass": "3",
			"rulesetIds": ["CRASACWS-2019"]
		},
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
			"owner": "[\"vcard\",[[\"version\",{},\"text\",\"4.0\"],[\"fn\",{},\"text\",\"Size Testing\"],[\"tel\",{\"type\":\"work\"},\"text\",\"(012) 841-4766\"],[\"email\",{\"type\":\"work\"},\"text\",\"djohnson@cs.uct.ac.za\"],[\"adr\",{\"type\":\"work\"},\"text\",[\"\",\"\",\"CSIR Meraka Institute\",\"Brummeria\",\"Pretoria\",\"0184\",\"RSA\"]],[\"prodid\",{},\"text\",\"ez-vcard ${version}\"]]]",
			"operator": "[\"vcard\",[[\"version\",{},\"text\",\"4.0\"],[\"fn\",{},\"text\",\"Size Testing\"],[\"tel\",{\"type\":\"work\"},\"text\",\"(012) 841-3028\"],[\"email\",{\"type\":\"work\"},\"text\",\"djohnson@cs.uct.ac.za\"],[\"adr\",{\"type\":\"work\"},\"text\",[\"\",\"\",\"CSIR Meraka Institute\",\"Brummeria\",\"Pretoria\",\"0184\",\"RSA\"]],[\"prodid\",{},\"text\",\"ez-vcard ${version}\"]]]"
		}
		
	}


def  AvailSpecReq(params):
	# Extract coordinats from request

	if 'location' in params:

		print(params['location'])
		if 'point' in params['location']:
			print(params['location']['point'])
			if 'center' in params['location']['point']:
				
				print(params['location']['point']['center'])
				location = params['location']
				latitude = location['point']['center']['latitude']
				longitude = location['point']['center']['longitude']
				
				 # Iterate through spectra

				print ('coords: ',latitude,longitude)
				resPawsC = SpecResp

				# Get result from CSIR database
				resGLSD = getSpectrumGLSD('CSIR',latitude,longitude,'AGL',30)

				# Get Cellular spectrum result using list of channel Widths requested
				if 'capabilities' in params:
					if 'ChannelWidths' in params['capabilities'] and 'technology' in params['capabilities'] and 'band' in params['capabilities']:
						resCellular  = convertCellular(params['capabilities']['ChannelWidths'], resGLSD, params['capabilities']['technology'], params['capabilities']['band'])
						print('result:',  resGLSD)
						spectrumSchedule = []
						spectrumScheduleEntry = {}
						eventTime = {}
						eventTime['startTime'] =  datetime.datetime.utcnow().replace(microsecond=0).isoformat() + 'Z'
						eventTime['stopTime'] = (datetime.datetime.utcnow().replace(microsecond=0) + timedelta(hours=2, days = 6)).isoformat() + 'Z'
						spectrumScheduleEntry['eventTime'] = eventTime
						spectrumScheduleEntry['technology'] = params['capabilities']['technology']
						spectrumScheduleEntry['band'] = params['capabilities']['band']
						spectrumScheduleEntry['duplex'] = 'FDD'
						spectrumScheduleEntry['spectra'] =  resCellular

						spectrumSchedule.append(spectrumScheduleEntry)

						resPawsC['location'] = location
						resPawsC['spectrumSchedule'] = spectrumSchedule
						resPawsC['CSIRGLSD'] = resGLSD
						
						return resPawsC
			
	

   