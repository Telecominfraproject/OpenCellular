# This file contains constants used by API

MethodNameInit = "spectrum.pawsc.init"
MethodNameRegister = "spectrum.pawsc.register"
MethodNameAvailableSpectrum = "spectrum.pawsc.getSpectrum"
MethodNameAvailableSpectrumBatch = "spectrum.pawsc.getSpectrumBatch"
MethodNameNotify = "spectrum.pawsc.notifySpectrumUse"
MethodNameValidateDevice = "spectrum.pawsc.verifyDevice"
MethodNameInterferenceQuery = "spectrum.pawsc.interferenceQuery"

TypeInitRequest = "INIT_REQ"
TypeInitResponse = "INIT_RESP"

TypeRegisterRequest = "REGISTRATION_REQ"
TypeRegisterResponse = "REGISTRATION_RESP"

TypeAvailableSpectrumRequest = "AVAIL_SPECTRUM_REQ"
TypeAvailableSpectrumResponse = "AVAIL_SPECTRUM_RESP"

TypeBatchAvailableSpectrumRequest = "AVAIL_SPECTRUM_BATCH_REQ"
TypeBatchAvailableSpectrumResponse = "AVAIL_SPECTRUM_BATCH_RESP"

TypeNotifySpectrumUsageRequest = "SPECTRUM_USE_NOTIFY"
TypeNotifySpectrumUsageResponse = "SPECTRUM_USE_RESP"

TypeDeviceValidationRequest = "DEV_VALID_REQ"
TypeDeviceValidationResponse = "DEV_VALID_RESP"

ErrorMessageNotRegistered = "Device not Registered"
ErrorMessageModeNotImplemented = "Device Mode not Implemented"
ErrorMessageDatabaseUnsupported = "The Database does not support the specified version of the message."
ErrorMessagDeviceIdNotAuthorized = "Device is not authorized"

ExceptionMessageInvalidMethod = "Invalid method"
ExceptionMessageParametersRequired = "Parameters Required";



LTE_arfcn_table = {
    'Band1':{'FDL_low':2110, 'FDL_high': 2170, 'NOffs-DL':0, 'FUL_low': 1920, 'FUL_high': 1980, 'NOffs-UL':18000, 'spacing': 190},
    'Band2':{'FDL_low':1930, 'FDL_high': 1990, 'NOffs-DL':0, 'FUL_low': 1850, 'FUL_high': 1910,'NOffs-UL':18000, 'spacing': 80},
    'Band3':{'FDL_low':1930, 'FDL_high': 1990, 'NOffs-DL':0, 'FUL_low': 1850, 'FUL_high': 1910,'NOffs-UL':18000, 'spacing': 80},
    'Band20':{'FDL_low':791, 'FDL_high': 821, 'NOffs-DL':6150, 'FUL_low': 832, 'FUL_high': 862, 'NOffs-UL':24150, 'spacing': -41 }
}

GSM_arfcn_table = {
    '900E':{'FDL_low': 925.2, 'FDL_high': 959.8, 'FUL_low': 880.2, 'FUL_high': 914.8, 'spacing': 45},
    '900R':{'FDL_low': 921.2, 'FDL_high': 959.8, 'FUL_low': 876.2, 'FUL_high': 914.8, 'spacing': 45},
    '900P':{'FDL_low': 935.2, 'FDL_high': 959.8, 'FUL_low': 890.2, 'FUL_high': 914.8, 'spacing': 45}
}
