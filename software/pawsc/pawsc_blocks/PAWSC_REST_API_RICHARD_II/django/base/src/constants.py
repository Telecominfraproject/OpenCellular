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

#TypeNotifySpectrumUsageRequest = "SPECTRUM_USE_NOTIFY"
#TypeNotifySpectrumUsageResponse = "SPECTRUM_USE_RESP"
TypeNotifySpectrumUsageRequest = "SCAN_DAT_NOTIFY"
TypeNotifySpectrumUsageResponse = "SCAN_DATA_RESP"

TypeDeviceValidationRequest = "DEV_VALID_REQ"
TypeDeviceValidationResponse = "DEV_VALID_RESP"

ErrorMessageNotRegistered = "Device not Registered"
ErrorMessageModeNotImplemented = "Device Mode not Implemented"
ErrorMessageDatabaseUnsupported = "The Database does not support the specified version of the message."
ErrorMessagDeviceIdNotAuthorized = "Device is not authorized"

ExceptionMessageInvalidMethod = "Invalid method"
ExceptionMessageParametersRequired = "Parameters Required";


