from django.contrib.auth.models import User, Group
from rest_framework import viewsets
from base.src.serializers import UserSerializer, GroupSerializer
from rest_framework.views import APIView, Response
from base.src import constants
from base.src.PAWSCMessage import PawscJson
from django.http import JsonResponse 
import json
from base.src.models import RegisteredDevices
#from django.apps import apps
#RegisteredDevices = apps.get_model(app_label='rest_framework', model_name='RegisteredDevices')

from base.src.PAWSCManager import pawscFunction
#from time import gmtime, strftime 
import datetime
from datetime import timedelta
from django import forms
from django.shortcuts import render, redirect
from django.core.files.storage import FileSystemStorage

"""
TODO
1) extract 'band', 'tech' and 'bw' from AVAILABLE_SPECTRUM_REQ and use in function call pawscFunction.get_spectrum('900E', 'GSM', 0.2)
2) ?
"""

SpecResp_ORIGINAL_IDEA = {    
    
	"id": "45455",
	"jsonrpc": "2.0",
	"method": "spectrum.pawsc.getSpectrum",
	"params": {
		"apiKey": "xxxxxxxx-xxxx-Mxxx-Nxxx-xxxxxxxxxxxx",
		"type": "SPECTRUM_RESP",
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
			"serialNumber": "ioig"
		},
		"deviceDescs":[{
			"serialNumber": "meraka",
			"typeApprovalId": "csir",
			"modelId": "wireless",
			"deviceType": "fixed",
			"deviceCategory": "Client",
			"emissionClass": "1",
			"rulesetIds": ["ICASATVWS-2018", "FccTvBandWhiteSpace-2010"]
		},
		{
			"serialNumber": "lpm",
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
			"owner": "[\"vcard\",[[\"version\",{},\"text\",\"4.0\"],[\"fn\",{},\"text\",\"Size Testing\"],[\"tel\",{\"type\":\"work\"},\"text\",\"(012) 841-4766\"],[\"email\",{\"type\":\"work\"},\"text\",\"djohnson@cs.uct.ac.za\"],[\"adr\",{\"type\":\"work\"},\"text\",[\"\",\"\",\"CSIR Meraka Institute\",\"Brummeria\",\"Pretoria\",\"0184\",\"RSA\"]],[\"prodid\",{},\"text\",\"ez-vcard ${version}\"]]]",
			"operator": "[\"vcard\",[[\"version\",{},\"text\",\"4.0\"],[\"fn\",{},\"text\",\"Size Testing\"],[\"tel\",{\"type\":\"work\"},\"text\",\"(012) 841-3028\"],[\"email\",{\"type\":\"work\"},\"text\",\"djohnson@cs.uct.ac.za\"],[\"adr\",{\"type\":\"work\"},\"text\",[\"\",\"\",\"CSIR Meraka Institute\",\"Brummeria\",\"Pretoria\",\"0184\",\"RSA\"]],[\"prodid\",{},\"text\",\"ez-vcard ${version}\"]]]"
		}, 
                "eventTime": {
                "startTime": "2019-06-02T14:30:21Z",
                "stopTime": "2019-06-02T20:00:00Z",
                "technology": "LTE",
                "band": "20",
                "duplex": "FDD"                 
                },
                               
                                         
		"spectra":  [{
			"resolutionBwHz": 8e6,
			"profilesHz":  [pawscFunction.get_spectrum('900E', 'GSM', 0.2)]
                        #[[{	"hz": 5.18e8,	"dbm": 30.0},{"hz": 5.26e8,"dbm": 30.0	}]] #original hint at format
		}]
                
	}
}

start_time = datetime.datetime.now().isoformat()
spec_resp = {
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



class UserViewSet(viewsets.ModelViewSet):
    """
    API endpoint that allows users to be viewed or edited.
    """
    queryset = User.objects.all().order_by('-date_joined')
    serializer_class = UserSerializer


class GroupViewSet(viewsets.ModelViewSet):
    """
    API endpoint that allows groups to be viewed or edited.
    """
    queryset = Group.objects.all()
    serializer_class = GroupSerializer

#class UploadFileForm(forms.Form):
#    title = forms.CharField(max_length=50)
#    file = forms.FileField()

def home(request):
    documents = Document.objects.all()
    return render(request, 'home.html', { 'documents': documents })

def simple_upload(request):
    if request.method == 'POST' and request.FILES['myfile']:
        myfile = request.FILES['myfile']
        fs = FileSystemStorage()
        filename = fs.save(myfile.name, myfile)
        uploaded_file_url = fs.url(filename)
        return render(request, 'simple_upload.html', {
            'uploaded_file_url': uploaded_file_url
        })
    return render(request, 'simple_upload.html')

 

class InitViewSet(APIView):
    
    def Method_Device_Reg(self, params):        
        print('Received REGISTRATION_REQ')         
        #return {"type": "REGISTRATION_RES"}
        return pawscFunction.register_device(self, params)


    def Method_Init_Req(self,params):
        print('Received INIT_REQ')              
        print ("Parameters :", params['deviceDesc']['serialNumber']) #focus on serialNumber in deviceDesc
        #return init_resp
        #return pawscFunction.device_init(params['deviceDesc']['serialNumber']) #focus on serialNumber in deviceDesc
        return pawscFunction.device_init(self, params)
        #return {"type": "INIT_RESP"} #???

    def Method_Spec_Req(self,params):
        print('Received SPEC_REQ')
        #return spec_resp
        return pawscFunction.avail_spec_resp(self, params)
    
    def Method_scan_data_notify(self, params):
        print('Received SCAN_DATA_NOTIFY')
        return 0 #pawscFunction.upload_file(self, params)

    def Unknown_Req(self,params):
        print('Received Unknown Method: ', params)
        return {'code': 'xxx', 'message': 'Unknown Method: ' + params, 'data': 'zzz'}

    def Malformed_Req(self):
        print('Received Malformed Request')
        return {'code': 'xxx', 'message': 'MALFORMED_REQUEST', 'data': 'zzz'}


    def get(self, request, format=None):
        return Response('GET NOT IMPLMENTED')


    def post(self, request, format=None):

        PostString = request.data
        print(PostString)

        RD = PawscJson('2.0')
        if ('id' in PostString):
            JsonID = PostString['id']
            RD.IDSet(JsonID)

            if (('method' in PostString) and ('params' in PostString)):
                PAWSCMethod = PostString['method']
                PAWSCParams = PostString['params']
                print('Received: ', PAWSCMethod, PAWSCParams)

                if (PAWSCMethod == constants.MethodNameInit):
                    Result = self.Method_Init_Req(PAWSCParams)
                    RD.MethodSet(constants.MethodNameInit)
                    RD.ParamSet(Result)

                elif (PAWSCMethod == constants.MethodNameAvailableSpectrum):
                    #print ('hello ')
                    #pawscFunction.get_spectrum()
                    Result = self.Method_Spec_Req(PAWSCParams)
                    RD.MethodSet(constants.MethodNameAvailableSpectrum)
                    RD.ParamSet(Result)

                # Add more methods here
                elif (PAWSCMethod == constants.MethodNameRegister):
                    Result = self.Method_Device_Reg(PAWSCParams)
                    RD.MethodSet(constants.MethodNameRegister)
                    RD.ParamSet(Result) 
                
                elif (PAWSCMethod == constants.MethodNameNotify):
                    Result = self.Method_scan_data_notify(PAWSCParams)
                    RD.MethodSet(constants.MethodNameNotify)
                    RD.ParamSet(Result)

                # Case for method not known
                else:
                    Result = self.Unknown_Req(PAWSCMethod)
                    RD.ErrorSet(constants.ExceptionMessageInvalidMethod)
                    RD.ParamSet(Result)

            # Case fo malformed request
            else:
                Result = self.Malformed_Req()
                RD.ErrorSet(constants.ExceptionMessageParametersRequired)
                RD.ParamSet(Result)

        # Case of malformed request
        else:
            Result = self.Malformed_Req()
            RD.ErrorSet(constants.ExceptionMessageParametersRequired)
            RD.ParamSet(Result)

        print(RD.Get())
        return Response(RD.Get())
    