"""
This file contains essential functions to process pawsc requests
"""
import json
from base.src.models import * #UnassignedFreq
from base.src.arfcn import FreqRangeToArfcnRange
import datetime
from datetime import timedelta
#useful libs for the upload stuff
#from django.http import HttpResponseRedirect
from django.db import transaction




LTE_arfcn_table = {
    'Band1':{'FDL_low':2110, 'NOffs-DL':0, 'FUL_low': 1920, 'NOffs-UL':18000, 'spacing': 190},
    'Band2':{'FDL_low':2110, 'NOffs-DL':0, 'FUL_low': 1920, 'NOffs-UL':18000, 'spacing': 80},
    'Band3':{'FDL_low':2110, 'NOffs-DL':0, 'FUL_low': 1920, 'NOffs-UL':18000, 'spacing': 95},
    'Band20':{'FDL_low':791, 'NOffs-DL':6150, 'FUL_low': 832, 'NOffs-UL':24150, 'spacing': -41 }
}

GSM_arfcn_table = {
    '900E':{'FDL_low': 925.2, 'FDL_high': 959.8, 'FUL_low': 880.2, 'FUL_high': 914.8, 'spacing': 45},
    '900R':{'FDL_low': 921.2, 'FDL_high': 959.8, 'FUL_low': 876.2, 'FUL_high': 914.8, 'spacing': 45},
    '900P':{'FDL_low': 935.2, 'FDL_high': 959.8, 'FUL_low': 890.2, 'FUL_high': 914.8, 'spacing': 45}
}

class pawscFunction:
    """"""
    def get_spectrum_hz (freq_band,tech,bw):
        """
        json dump output is of the form: [{"freqend": <value>, "freqstart": <value>}, {"freqend": <value>, "freqstart": <value>}, ...]
        """       
        
        #return json.dumps([dict(item) for item in UnassignedFreq.objects.all().values("freqstart", "freqend")])
        
        #return UnassignedFreq.objects.all().values("freqstart", "freqend") #returns raw unassigned start and stop frequecy ranges
        #return FreqRangeToArfcnRange('900E','GSM',880,885,0.2)
        """
        To fetch all:
        UnassignedFreq.objects.all().values("freqstart", "freqend")
        
        To fetch subset e.g. for specific band:          
        if (freq_band == '900E' ):
            data = UnassignedFreq.objects.all().values("freqstart", "freqend").filter(band = 900)
        """    
        '''
        Initialise some semi-local temporal variables to work with
        '''
        temp_FUL_low = 0
        temp_FUL_high = 0            
        temp_FDL_low = 0
        temp_FDL_high = 0        
       
        data = UnassignedFreq.objects.all().values("freqstart", "freqend").filter(band = 900)
        #data = UnassignedFreq.objects.all().values("freqstart", "freqend").filter(band = 900
        freq_ranges = []
        freq_ranges_Hz = []
        for item in data: #UnassignedFreq.objects.all().values("freqstart", "freqend"):
            #print (item['freqstart'], item['freqend'])
            freq_ranges.append(FreqRangeToArfcnRange(freq_band,tech, item['freqstart'], item['freqend'], bw))
            #FreqRangeToArfcnRange(freq_band,tech, item['freqstart'], item['freqend'] , bw)
            '''
            Attempting to get profilesHz out of available spectrum.
            For now, we assume 'tech' == gsm
            '''
            '''
            Initialise some temporal local variables to work with
            '''
            FUL_low = 0
            FUL_high = 0            
            FDL_low = 0
            FDL_high = 0
                                    
            #print ('FUL_low:', GSM_arfcn_table[freq_band]['FUL_low'])
            #print ('FUL_low:', GSM_arfcn_table[freq_band]['FUL_high'])
            if freq_band == '900E':
                if ((item['freqstart'] >=  GSM_arfcn_table[freq_band]['FUL_low']) and (item['freqstart'] <  GSM_arfcn_table[freq_band]['FUL_high'])):
                    FUL_low = item['freqstart']
                    if (item['freqend'] >= GSM_arfcn_table[freq_band]['FUL_high'] ):
                        FUL_high = GSM_arfcn_table[freq_band]['FUL_high']
                    else:
                        FUL_high = item['freqend'] #later check that uhz-dhz range is multiple of 0.2e6
                elif (((item['freqstart'] >=  GSM_arfcn_table[freq_band]['FDL_low']) and (item['freqstart'] <  GSM_arfcn_table[freq_band]['FDL_high']))):
                    FDL_low =item['freqstart']
                    if (item['freqend'] >= GSM_arfcn_table[freq_band]['FDL_high']):
                        FDL_high = GSM_arfcn_table[freq_band]['FDL_high']
                    else:
                        FDL_high = item['freqend'] #later ensure that uhz-dhz range is multiple of 0.2e6
                elif ((item['freqstart'] <  GSM_arfcn_table[freq_band]['FUL_low']) and (item['freqend'] >  GSM_arfcn_table[freq_band]['FUL_low'])):
                    FUL_low = GSM_arfcn_table[freq_band]['FUL_low']
                    if (item['freqend'] >= GSM_arfcn_table[freq_band]['FUL_high']):
                        FUL_high = GSM_arfcn_table[freq_band]['FUL_high']
                    else:
                        FUL_high = item['freqend'] #later ensure that uhz-dhz range is multiple of 0.2e6
                elif ((item['freqstart'] <  GSM_arfcn_table[freq_band]['FDL_low']) and (item['freqend'] >  GSM_arfcn_table[freq_band]['FDL_low'])):
                    FDL_low = GSM_arfcn_table[freq_band]['FDL_low']
                    if (item['freqend'] >= GSM_arfcn_table[freq_band]['FDL_high']):
                        FDL_high = GSM_arfcn_table[freq_band]['FDL_high']
                    else:
                        FDL_high = item['freqend'] #later ensure that uhz-dhz range is multiple of 0.2e6                        
                '''
                Attemping to shift the values to avoid this kind of output: 
                [{"DHz": 0, "UHz": 880.2 }, {"DHz": 0, "UHz": 889} ], [{ "DHz": 925.2, "UHz": 0}, {"DHz": 934,"UHz": 0}]
                '''                 
            if (FDL_low == 0) and (FDL_high == 0) and (FUL_low != 0) and (FUL_high != 0):
                if (temp_FDL_low != 0) and (temp_FDL_high != 0):
                    freq_ranges_Hz.append([{'UHz': FUL_low*1000000, 'DHz': temp_FDL_low*1000000, "Ddbm": 23.0, "Udbm": 15.0}, {'UHz': FUL_high*1000000, 'DHz': temp_FDL_high*1000000, "Ddbm": 23.0, "Udbm": 15.0}])
                    temp_FDL_low = 0 #reset the temporal variables
                    temp_FDL_high = 0
                else:
                    temp_FUL_low = FUL_low
                    temp_FUL_high = FUL_high                    
                                   
              
                
            if (FUL_low == 0) and (FUL_high == 0) and (FDL_low != 0) and (FDL_high != 0):
                if (temp_FUL_low != 0) or (temp_FUL_high != 0):
                    freq_ranges_Hz.append([{'UHz': temp_FUL_low*1000000, 'DHz': FDL_low*1000000, "Ddbm": 23.0, "Udbm": 15.0}, {'UHz': temp_FUL_high*1000000, 'DHz': FDL_high*1000000, "Ddbm": 23.0, "Udbm": 15.0}])
                    temp_FUL_low = 0 #reset the temporal variables
                    temp_FUL_high = 0                    
                else:                   
                    temp_FDL_low = FDL_low
                    temp_FDL_high = FDL_high                                    
                
                
                
            if (FDL_low != 0) and (FDL_high != 0) and (FUL_low != 0) and (FUL_high != 0):
                freq_ranges_Hz.append([{'UHz': FUL_low, 'DHz': FDL_low, "Ddbm": 23.0, "Udbm": 15.0}, {'UHz': FUL_high, 'DHz': FDL_high, "Ddbm": 23.0, "Udbm": 15.0}])
            #freq_ranges_Hz.append({'freqstart': item['freqstart']*1000000, 'freqend': item['freqend']*1000000})
            #freq_ranges_Hz.append([{'UHz': FUL_low, 'DHz': FDL_low}, {'UHz': FUL_high, 'DHz': FDL_high}])
        #print (freq_ranges)
        #for item in UnassignedFreq.objects.all().values("freqstart", "freqend"):
            #print item
        #item = (880, 885)
        #return (FreqRangeToArfcnRange('900E','GSM',item[0], item[1], 0.2) ) 
        '''
        Attempting to format the frequency ranges in a way client expects
        '''
        arfcn = []
        for item in freq_ranges:
            #print ("DARFCN:", item['arfcn_start']['arfcn_DL'], "UARFCN:", item['arfcn_start']['arfcn_UL'], "Ddbm:  30.0, Udbm: 20.0" )
            arfcn.append({"DARFCN": item['arfcn_start']['arfcn_DL'], "UARFCN": item['arfcn_start']['arfcn_UL'], "Ddbm": 30.0, "Udbm": 20.0})
            arfcn.append({"DARFCN": item['arfcn_end']['arfcn_DL'], "UARFCN": item['arfcn_end']['arfcn_UL'], "Ddbm": 30.0, "Udbm": 20.0})        
              
         
        return freq_ranges_Hz
        """
        May need to strip off the brackets depending on output format requirements
        """    
        #return str(freq_ranges).strip("[ ]")      
        

    #def __init__(self):
     #   """Constructor"""

    def get_spectrum(freq_band,tech,bw):
        """
        json dump output is of the form: [{"freqend": <value>, "freqstart": <value>}, {"freqend": <value>, "freqstart": <value>}, ...]
        """       
        
        #return json.dumps([dict(item) for item in UnassignedFreq.objects.all().values("freqstart", "freqend")])
        
        #return UnassignedFreq.objects.all().values("freqstart", "freqend") #returns raw unassigned start and stop frequecy ranges
        #return FreqRangeToArfcnRange('900E','GSM',880,885,0.2)
        """
        To fetch all:
        UnassignedFreq.objects.all().values("freqstart", "freqend")
        
        To fetch subset e.g. for specific band:          
        if (freq_band == '900E' ):
            data = UnassignedFreq.objects.all().values("freqstart", "freqend").filter(band = 900)
        """      
       
        data = UnassignedFreq.objects.all().values("freqstart", "freqend").filter(band = 900)
        #data = UnassignedFreq.objects.all().values("freqstart", "freqend").filter(band = 900
        freq_ranges = []
        freq_ranges_Hz = []
        for item in data: #UnassignedFreq.objects.all().values("freqstart", "freqend"):
            #print (item['freqstart'], item['freqend'])
            freq_ranges.append(FreqRangeToArfcnRange(freq_band,tech, item['freqstart'], item['freqend'], bw))
            #FreqRangeToArfcnRange(freq_band,tech, item['freqstart'], item['freqend'] , bw)
            freq_ranges_Hz.append({'freqstart': item['freqstart']*1000000, 'freqend': item['freqend']*1000000})
        #print (freq_ranges)
        #for item in UnassignedFreq.objects.all().values("freqstart", "freqend"):
            #print item
        #item = (880, 885)
        #return (FreqRangeToArfcnRange('900E','GSM',item[0], item[1], 0.2) ) 
        '''
        Attempting to format the frequency ranges in a way client expects
        '''
        arfcn = []
        for item in freq_ranges:
            #print ("DARFCN:", item['arfcn_start']['arfcn_DL'], "UARFCN:", item['arfcn_start']['arfcn_UL'], "Ddbm:  30.0, Udbm: 20.0" )
            arfcn.append({"DARFCN": item['arfcn_start']['arfcn_DL'], "UARFCN": item['arfcn_start']['arfcn_UL'], "Ddbm": 30.0, "Udbm": 20.0})
            arfcn.append({"DARFCN": item['arfcn_end']['arfcn_DL'], "UARFCN": item['arfcn_end']['arfcn_UL'], "Ddbm": 30.0, "Udbm": 20.0})        
              
         
        return arfcn,
        """
        May need to strip off the brackets depending on output format requirements
        """    
        #return str(freq_ranges).strip("[ ]")   
    
    '''
      serve INIT_REQ
    ''' 
    def device_init(self,params):
        if RegisteredDevices.objects.filter(serial_number = params['deviceDesc']['serialNumber']).exists():
            init_resp = {
                            "type": "INIT_RESP",
                            "rulesetInfo": {
                                "authority": "MZ",
                                "rulesetId": ["ICASATVWS-2018", "FccTvBandWhiteSpace-2010", "ETSI-EN-301-598-1.1.1"],
                                "maxLocationChange": 0.00,
                                "maxPollingSecs": 0
                               },
                            "dataBaseChange":{"dbUpdateSpec":{"databases":[{"databaseSpec":{"name":"OpenCellular","uri":"http://pawsc.info:8001/api/pawsc"}}]}}
                        }  
            '''
            mark device as initialised if not initialised already
            ''' 
            if InitialisedDevices.objects.filter(serial_number = params['deviceDesc']['serialNumber']).exists():
                print ('Device is initialised already') 
            else:
                InitialisedDevices(serial_number=params['deviceDesc']['serialNumber'], 
                                   latitude=params['location']['point']['center']['latitude'], 
                                   longitude=params['location']['point']['center']['longitude'], 
                                   antenna_height=params['antenna']['height'], 
                                   antenna_type=params['antenna']['heightType'],
                                   #<other parameters>
                                   time = datetime.datetime.utcnow().replace(microsecond=0).isoformat()).save()
        else: #if device is not registered with authority
            init_resp = {
                "type": "INIT_RESP",
                "Error":{"code": "302", "message":"Device not registered"}            
            } 
            
        return init_resp
     
    '''
    serve REGISTRATION_REQ
    '''
    def register_device(self, params):
        if RegisteredDevices.objects.filter(serial_number = params['deviceDesc']['serialNumber']).exists():
            print("Device registered already")
            registration_resp = {
                "type": "REGISTRATION_RESP",
                "status": "Device already registered"
            }
       
        else: 
            RegisteredDevices(serial_number=params['deviceDesc']['serialNumber'], 
                                   latitude=params['location']['point']['center']['latitude'], 
                                   longitude=params['location']['point']['center']['longitude'], 
                                   antenna_height=params['antenna']['height'], 
                                   antenna_type=params['antenna']['heightType'],
                                   #<other parameters>
                                   date = datetime.datetime.utcnow().replace(microsecond=0).isoformat()).save()
            
            registration_resp = {
                "type": "REGISTRATION_RESP",
                "status": "Registration successfull"
            } 
        
        """
        In future validate all other necessary conditions required for registration 
        """
        return registration_resp
    
    def avail_spec_resp(self, params):
        
        start_time = datetime.datetime.now().isoformat()
        
        if InitialisedDevices.objects.filter(serial_number = params['deviceDesc']['serialNumber']).exists():
            spec_resp = {
                "type": "AVAIL_SPECTRUM_RESP",
                "spectrumSchedules": [
                {
                "eventTime": {
                "startTime": datetime.datetime.utcnow().replace(microsecond=0).isoformat() + 'Z', 
                "stopTime":  (datetime.datetime.utcnow().replace(microsecond=0) + timedelta(hours=12, days = 0)).isoformat() + 'Z' 
                },
                "technology": "GSM",
                "band": "900E",
                "duplex": "FDD",
                
                "spectra": [
                {
                    "resolutionBwHz": 0.2e6, #3e6,
                #"profilesHz": [ pawscFunction.get_spectrum('Band20', 'LTE', 0.2)
                "profilesHz": [ pawscFunction.get_spectrum_hz('900E', 'GSM', 0.2)
                #[
                #{"Dhz": 7.910e8, "UHz":8.320e8, "Ddbm": 23.0, "Udbm": 15.0}, 
                #{"Dhz": 7.970e8, "UHz":8.380e8, "Ddbm": 23.0, "Udbm": 15.0}
                #],
                #[
                #{"Dhz": 8.050e8, "UHz":8.460e8, "Ddbm": 30.0, "Udbm": 20.0},
                #{"Dhz": 8.140e8, "UHz":8.550e8, "Ddbm": 30.0, "Udbm": 20.0}
                #]
                ],
                "profilesN": [ pawscFunction.get_spectrum('900E', 'GSM', 0.2)
               # [
               # {"DARFCN": 6165, "UARFCN": 24165, "Ddbm": 23.0, "Udbm": 15.0}, 
               # {"DARFCN": 6195, "UARFCN": 24195, "Ddbm": 23.0, "Udbm": 15.0} 
               # ],
               # [
               # {"DARFCN": 6305, "UARFCN": 24305, "Ddbm": 30.0, "Udbm": 20.0},
               # {"DARFCN": 6365, "UARFCN": 24365, "Ddbm": 30.0, "Udbm": 20.0} 
               # ]
                ]            
                }
                ]
                }
                    
                ]
                
            
            }
        else:
            spec_resp = {
                 "type": "AVAIL_SPECTRUM_RESP",
                 "Error":{"code": "300", "message":"Device not initialised"}
            }
        
        return spec_resp
    
    @transaction.atomic #commit once, instead of at each save()
    def scan_data_resp(self, params):
        #pair1=data['3.3']
        #print('pair1: ', pair1)
        for data_list in params['scanData']:
            count=0
            if( len(Notifyspectrumusedbmdata.objects.all()) == 0):
                next_rec_dbm=1
            else:
                next_rec_dbm=Notifyspectrumusedbmdata.objects.last().id + 1
            if(len(Notifyspectrumuseconfig.objects.all()) == 0):
                next_rec_config=1
            else:
                next_rec_config=Notifyspectrumuseconfig.objects.last().id + 1
           
            Notifyspectrumusedbmdata(id=next_rec_dbm,
                                     created_at = params['scanInfo']['time'],
                                     config_id = next_rec_config,
                                     min = params['scanInfo']['min'],
                                     max = params['scanInfo']['max'],
                                     med = params['scanInfo']['med'],                               
                                     nsteps =  params['scanInfo']['nsteps'],
                                     ).save() #enter new empty record
            
            Notifyspectrumuseconfig(id=next_rec_config, 
                                    created_at=params['config']['created_at'], #date and time scan started
                                    campaign_id=params['config']['campaign_id'],
                                    start_freq=params['config']['start_freq'],
                                    end_freq=params['config']['end_freq'],
                                    amp_top=params['config']['amp_top'],
                                    amp_bottom=params['config']['amp_bottom'],
                                    nsteps=params['config']['nsteps']).save()
            #data = json.dumps(data, sort_keys=True)
            #for item in data:  #if "data":{...} not nested inside params          
        #for data_list in params['scanData']:
            for item in data_list['scanDataList']:
                # print(item, data_sorted[item])
                column=str(count).zfill(3)
                pref_dbm="v"
                pref_config="f"
                field_dbm = pref_dbm+column #dynamically define field name i.e. v000, v001, v002, ..., v**n
                field_config=pref_config+column #dynamically define field name i.e. f000, f001, f002, ..., f**n
                #dataset.append(field_dbm +'='+ data[item])       
                #Notifyspectrumusedbmdata(**{field: data[item]})
                Notifyspectrumusedbmdata.objects.filter(id=next_rec_dbm).update(**{field_dbm: data_list['scanDataList'][item]}) #populate the record
                count += 1
                Notifyspectrumuseconfig.objects.filter(id=next_rec_config).update(**{field_config: item})
                
        #Notifyspectrumusedbmdata(v000=12, v001=2, v002=4, v003=10, v004=6, v005=8).save()
        #print(dataset)
        #print(str(dataset).strip("[''], ''"))
        #print (','.join(dataset))
        #print (json.dumps(data, sort_keys=True))
        #print (data_sorted)
        #print (data)
        #transaction.commit()
        
        #Assume data successfully uploaded:
        scan_data_resp = {
             "type": "SCAN_DATA_RESP",             
             "reportingInterval":"xx",
             "ScanInformation":{
                 "sampleRate":"xx",
                 "dwellTime":"xx",
                 "windowType":"zzz",
                 "detector":"zzz",
                 "statistics":"zzz",
                 "startFreq":"xxx",
                 "stopFreq":"xxx",
                 "fftsize":"xxx"
             },
             "Ack":{"code": "000", "status":"Spectrum scan data successfully uploaded"}
        }            
            
                
       
        return scan_data_resp
    
    
    def __init__(self):
        """Constructor"""

