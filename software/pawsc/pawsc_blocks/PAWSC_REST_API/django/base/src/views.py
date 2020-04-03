from django.contrib.auth.models import User, Group
from rest_framework import viewsets
from base.src.serializers import UserSerializer, GroupSerializer
from rest_framework.views import APIView, Response
from base.src import constants
from base.src.PAWSCMessage import PawscJson
from base.src.AvailableSpectrumRequest import AvailSpecReq

from .models import Scan_Device






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



class InitViewSet(APIView):

	def Method_Init_Req(self,params):
		print('Received INIT_REQ')
		return {"type": "INIT_RESP"}
	
	def Method_Spec_Req(self,params):
		print('Received SPEC_REQ')
		SpecResp = AvailSpecReq(params)
		return SpecResp

	def Unknown_Req(self,params):
		print('Received Unknown Method: ', params)
		return {'code': 'xxx', 'message': 'UNKOWN_METHOD: ' + params, 'data': 'zzz'}
	  
	def Malformed_Req(self):
		print('Received Malformed Request')
		return {'code': 'xxx', 'message': 'MALFORMED_REQUEST', 'data': 'zzz'}
   
	def Method_Init_Scan_Req(self,params):
		print('Received INIT_SCAN_REQ')
		


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
					Result = self.Method_Spec_Req(PAWSCParams)
					RD.MethodSet(constants.MethodNameAvailableSpectrum)
					RD.ParamSet(Result)

				# Add more methods here




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

		# Case fo malformed request
		else:
			Result = self.Malformed_Req()
			RD.ErrorSet(constants.ExceptionMessageParametersRequired)
			RD.ParamSet(Result)

		#print(RD.Get())
		return Response(RD.Get())
