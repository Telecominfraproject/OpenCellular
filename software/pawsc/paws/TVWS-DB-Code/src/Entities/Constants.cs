// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;

    /// <summary>
    /// Static helper class of constants used within the Entities components.
    /// </summary>
    public static class Constants
    {
        // Method Names

        /// <summary>
        /// Initialize method used by the used by the Paws MVC API Controller.
        /// </summary>
        public const string MethodNameInit = "spectrum.paws.init";

        /// <summary>
        /// Register method used by the Paws MVC API Controller.
        /// </summary>
        public const string MethodNameRegister = "spectrum.paws.register";

        /// <summary>
        /// Spectrum Available method used by the Paws API Controller.
        /// </summary>
        public const string MethodNameAvailableSpectrum = "spectrum.paws.getSpectrum";

        /// <summary>
        /// Available Spectrum batch method used by the Paws API Controller.
        /// </summary>
        public const string MethodNameAvailableSpectrumBatch = "spectrum.paws.getSpectrumBatch";

        /// <summary>
        /// Spectrum Use method used by the Paws API Controller.
        /// </summary>
        public const string MethodNameNotify = "spectrum.paws.notifySpectrumUse";

        /// <summary>
        /// Validate Device method used by the Paws API Controller.
        /// </summary>
        public const string MethodNameValidateDevice = "spectrum.paws.verifyDevice";

        /// <summary>
        /// Interference Query method used by the Paws API Controller.
        /// </summary>
        public const string MethodNameInterferenceQuery = "spectrum.paws.interferenceQuery";

        /// <summary>
        /// Identifies the Paws Init Request parameter type.
        /// </summary>
        public const string TypeInitRequest = "INIT_REQ";

        /// <summary>
        /// Identifies the Paws Register Request parameter type.
        /// </summary>
        public const string TypeRegisterRequest = "REGISTRATION_REQ";

        /// <summary>
        /// Identifies the Paws Available Spectrum Request parameter type.
        /// </summary>
        public const string TypeAvailableSpectrumRequest = "AVAIL_SPECTRUM_REQ";

        /// <summary>
        /// Identifies the Batch Available Spectrum Request parameter type.
        /// </summary>
        public const string TypeBatchAvailableSpectrumRequest = "AVAIL_SPECTRUM_BATCH_REQ";

        /// <summary>
        /// Identifies the Notify Spectrum Usage Request parameter type.
        /// </summary>
        public const string TypeNotifySpectrumUsageRequest = "SPECTRUM_USE_NOTIFY";

        /// <summary>
        /// Identifies the Device Validation Request parameter type.
        /// </summary>
        public const string TypeDeviceValidationRequest = "DEV_VALID_REQ";

        /// <summary>
        /// Identifies the Interference Query Request parameter type.
        /// </summary>
        public const string TypeInterferenceQueryRequest = "INTERFERENCE_QUERY_REQ";

        /// <summary>
        /// Identifies the Paws Device Init Response type.
        /// </summary>
        public const string TypeInitResponse = "INIT_RESP";

        /// <summary>
        /// Identifies the Paws Register Response type.
        /// </summary>
        public const string TypeRegisterResponse = "REGISTRATION_RESP";

        /// <summary>
        /// Identifies the Paws Available Spectrum Response type.
        /// </summary>
        public const string TypeAvailableSpectrumResponse = "AVAIL_SPECTRUM_RESP";

        /// <summary>
        /// Identifies the Paws Batch Available Spectrum Response type.
        /// </summary>
        public const string TypeBatchAvailableSpectrumResponse = "AVAIL_SPECTRUM_BATCH_RESP";

        /// <summary>
        /// Identifies the Paws Spectrum Use Response type.
        /// </summary>
        public const string TypeNotifySpectrumUsageResponse = "SPECTRUM_USE_RESP";

        /// <summary>
        /// Identifies the Paws Valid Device Response type.
        /// </summary>
        public const string TypeDeviceValidationResponse = "DEV_VALID_RESP";

        /// <summary>
        /// Identifies the Interference Query Response type.
        /// </summary>
        public const string TypeInterferenceQueryResponse = "INTERFERENCE_QUERY_RESP";

        /// <summary>
        /// Identifies the Region Management Add Users Response type.
        /// </summary>
        public const string TypeAddUsersResponse = "ADD_USERS_RESP";

        /// <summary>
        /// Identifies the Region Management Add Users Response type.
        /// </summary>
        public const string TypeGrantAccessResponse = "GRANT_ACCESS_RESP";

        /// <summary>
        /// Identifies the Region Management Add Users Response type.
        /// </summary>
        public const string TypeRequestElevatedResponse = "REQUEST_ELEVATED_RESP";

        /// <summary>
        /// Identifies the Region Management Update Users Response type.
        /// </summary>
        public const string TypeUpdateUsersResponse = "UPDATE_USERS_RESP";

        /// <summary>
        /// Identifies the Region Management Get Users Response type.
        /// </summary>
        public const string TypeGetUsersResponse = "GET_USERS_RESP";

        /// <summary>
        /// Identifies the Region Management Request Elevated Access Response type.
        /// </summary>
        public const string TypeReqElevatedAccessResponse = "REQ_ELEVATED_ACCESS_RESP";

        /// <summary>
        /// Identifies the Region Management Delete Users Response type.
        /// </summary>
        public const string TypeDeleteUsersResponse = "DELETE_USERS_RESP";

        /// <summary>
        /// Identifies the Region Management Add Incumbents Response type.
        /// </summary>
        public const string TypeRegisterDeviceResponse = "REGISTER_DEVICE_RESP";

        /// <summary>
        /// Identifies the Region Management Delete Incumbents Response type.
        /// </summary>
        public const string TypeDeleteIncumbentsResponse = "DELETE_INCUMBENTS_RESP";

        /// <summary>
        /// Identifies the Region Management exclude channels Response type.
        /// </summary>
        public const string TypeExcludeChannelsResponse = "EXCLUDE_CHANNELS_RESP";

        /// <summary>
        /// Identifies the Region Management exclude id Response type.
        /// </summary>
        public const string TypeExcludeIdsResponse = "EXCLUDE_IDS_RESP";

        /// <summary>
        /// Identifies the Region Management get incumbents Response type.
        /// </summary>
        public const string TypeGetIncumbentsResponse = "GET_INCUMBENTS_RESP";

        /// <summary>The type get ULS call signs response</summary>
        public const string TypeGetULSCallSignsResponse = "GET_ULS_CALL_SIGNS_RESP";

        /// <summary>The type get authorized devices information</summary>
        public const string TypeGetAuthorizedDevicesInfo = "GET_AUTHORIZED_DEVICES_RESP";

        /// <summary>The type get public data response</summary>
        public const string TypeGetPublicDataResponse = "GET_PUBLIC_DATA_RESP";

        /// <summary>The type get public data with events response</summary>
        public const string TypeGetPublicDataWithEventsResponse = "GET_PUBLIC_DATA_WITH_EVENTS_RESP";

        /// <summary>The type get license LPAUX information response</summary>
        public const string TypeGetLicenseLpAuxInfoResponse = "GET_LICENSE_LPAUX_INFO_RESP";

        /// <summary>The type get UNLICENSED LPAUX information response</summary>
        public const string TypeGetULSFileNumbersResponse = "GET_ULS_FILE_NUMBERS_RESP";

        /// <summary>The type get search TV stations response</summary>
        public const string TypeGetSearchMVPDCallSignsResponse = "GET_SEARCH_MVPD_CALL_SIGNS_RESP";

        /// <summary>The type search TV stations response</summary>
        public const string TypeSearchTvStationsResponse = "GET_UNLICENSE_LPAUX_INFO_RESP";

        /// <summary>
        /// Identifies the Region Management get device list Response type.
        /// </summary>
        public const string TypeGetDeviceListResponse = "GET_DEVICE_LIST_RESP";

        /// <summary>
        /// Identifies the Region Management get channels Response type.
        /// </summary>
        public const string TypeGetChannelsResponse = "GET_CHANNELS_RESP";

        /// <summary>The type get contour data response</summary>
        public const string TypeGetContourDataResponse = "GET_CONTOUR_DATA_RESP";

        ///<summary>Identifies the Region Management get callsign info response type</summary>
        public const string GetCallsignInfoResponse = "GET_CALLSIGN_INFO_RESP";

        /// <summary>
        /// Identifies the JSON property name street.
        /// </summary>
        public const string PropertyNameStreet = "street";

        /// <summary>
        /// Identifies the JSON property name locality.
        /// </summary>
        public const string PropertyNameLocality = "locality";

        /// <summary>
        /// Identifies the JSON property name region.
        /// </summary>
        public const string PropertyNameRegion = "region";

        /// <summary>
        /// Identifies the JSON property name code.
        /// </summary>
        public const string PropertyNameCode = "code";

        /// <summary>
        /// Identifies the JSON property name country.
        /// </summary>
        public const string PropertyNameCountry = "country";

        /// <summary>
        /// Identifies the JSON property Antenna height.
        /// </summary>
        public const string PropertyNameHeight = "height";

        /// <summary>
        /// Identifies the JSON property Antenna height type.
        /// </summary>
        public const string PropertyNameHeightType = "heightType";

        /// <summary>
        /// Identifies the JSON property Antenna height uncertainty.
        /// </summary>
        public const string PropertyNameHeightUncertainty = "heightUncertainty";

        /// <summary>
        /// Identifies the JSON Available Spectrum Request device descriptor.
        /// </summary>
        public const string PropertyNameDeviceDescriptor = "deviceDesc";

        /// <summary>
        /// Identifies the JSON Available Spectrum Request device .
        /// </summary>
        public const string PropertyNameLocation = "location";

        /// <summary>The property name master device location</summary>
        public const string PropertyNameMasterDeviceLocation = "masterDeviceLocation";

        /// <summary>
        /// Identifies the JSON Available Spectrum Request device antenna.
        /// </summary>
        public const string PropertyNameAntenna = "antenna";

        /// <summary>
        /// Identifies the JSON Available Spectrum Request device owner.
        /// </summary>
        public const string PropertyNameOwner = "owner";

        /// <summary>
        /// Identifies the JSON Available Spectrum Request device capabilities.
        /// </summary>
        public const string PropertyNameCapabilities = "capabilities";

        /// <summary>
        /// Identifies the JSON Available Spectrum Request mater device descriptors.
        /// </summary>
        public const string PropertyNameMasterDeviceDescriptors = "masterDeviceDesc";

        /// <summary>
        /// Identifies the JSON Available Spectrum Request device type.
        /// </summary>
        public const string PropertyNameRequestType = "requestType";

        /// <summary>
        /// Identifies the JSON Available Spectrum Request device .
        /// </summary>
        public const string ErrorMessageAvailableSpectrumLocation = "availableSpectrumRequest.location";

        /// <summary>
        /// The error message server error
        /// </summary>
        public const string ErrorMessageServerError = "Server Error";

        /// <summary>
        /// The error message server error
        /// </summary>
        public const string ErrorMessageUnavailableSpectrum = "Unavailable Spectrum";

        /// <summary>
        /// The error message for outside coverage area.
        /// </summary>
        public const string ErrorMessageOutsideCoverage = "The specified geo-location is outside the coverage area of the Database";

        /// <summary>
        /// The error message unauthorized
        /// </summary>
        public const string ErrorMessageUnAuthorized = "301";

        /// <summary>
        /// The error code unauthorized
        /// </summary>
        public const int ErrorCodeUnAuthorized = 301;

        /// <summary>
        /// The error message version unsupported
        /// </summary>
        public const string ErrorMessageDatabaseUnsupported = "The Database does not support the specified version of the message.";

        /// <summary>
        /// Represents the JsonRPC property.
        /// </summary>
        public const string PropertyNameJsonRpc = "jsonrpc";

        /// <summary>
        /// Represents the JSon Id property.
        /// </summary>
        public const string PropertyNameId = "id";

        // BatchAvailableSpectrumRequest 

        /// <summary>
        /// Represents the Paws Batch Available Spectrum Request JSon location property.
        /// </summary>
        public const string PropertyNameLocations = "locations";

        /// <summary>
        /// Represents the Paws Batch Available Spectrum Request JSon error message for location.
        /// </summary>
        public const string ErrorMessageBatchAvailableSpectrumLocations = "batchAvailableSpectrumRequest.locations";

        /// <summary>
        /// Represents the Paws Database Spec DatabaseSpec Name field.
        /// </summary>
        public const string PropertyNameName = "name";

        /// <summary>
        /// Represents the Paws Database Spec DatabaseSpec Uri field.
        /// </summary>
        public const string PropertyNameUri = "uri";

        /// <summary>
        /// Represents the Paws DBUpdate Spec database field.
        /// </summary>
        public const string PropertyNameDatabases = "databases";

        /// <summary>
        /// Represents the Paws DeviceCapabilities Frequency Ranges field.
        /// </summary>
        public const string PropertyNameFrequencyRanges = "frequencyRanges";

        /// <summary>The property name profiles</summary>
        public const string PropertyNameProfiles = "profiles";

        /// <summary>
        /// Represents the Paws DeviceDescriptor Serial Number field.
        /// </summary>
        public const string PropertyNameSerialNumber = "serialNumber";

        /// <summary>
        /// Represents the Height Type field.
        /// </summary>
        public const string ErrorMessageParametersHeightTypeRequired = "heightType Required";

        /// <summary>
        /// Represents the User Id field.
        /// </summary>
        public const string PropertyNameUserId = "userID";

        /// <summary>
        /// Represents the Venue field.
        /// </summary>
        public const string PropertyNameVenue = "venue";

        /// <summary>
        /// Represents the User First Name field.
        /// </summary>
        public const string PropertyNameUserFirstName = "userFirstName";

        /// <summary>
        /// Represents the User Last Name field.
        /// </summary>
        public const string PropertyNameUserLastName = "userLastName";

        /// <summary>
        /// Represents the City field.
        /// </summary>
        public const string PropertyNameCity = "city";

        /// <summary>
        /// Represents the access field.
        /// </summary>
        public const string PropertyNameAccess = "access";

        /// <summary>
        /// Represents the Create Date field.
        /// </summary>
        public const string PropertyNameCreateDate = "createDate";

        /// <summary>
        /// Represents the Super Admin field.
        /// </summary>
        public const string PropertyNameSuperAdmin = "superAdmin";

        /// <summary>
        /// Represents the Justification field.
        /// </summary>
        public const string PropertyNameJustification = "justification";

        /// <summary>
        /// Represents the Access Level field.
        /// </summary>
        public const string PropertyNameAccessLevel = "accessLevel";

        /// <summary>
        /// Represents the Is Admin Access Requested field.
        /// </summary>
        public const string PropertyNameIsAdminAccessRequested = "isAdminAccessRequested";

        /// <summary>
        /// Represents the Requested Access Level field.
        /// </summary>
        public const string PropertyNameRequestedAccessLevel = "RequestedAccessLevel";

        /// <summary>
        /// Represents the Paws DeviceDescriptor Manufacturer Id field.
        /// </summary>
        public const string PropertyNameManufacturerId = "manufacturerId";

        /// <summary>
        /// Represents the Paws DeviceDescriptor Model Id field.
        /// </summary>
        public const string PropertyNameModelId = "modelId";

        /// <summary>
        /// Represents the Paws DeviceDescriptor Rule set Ids field.
        /// </summary>
        public const string PropertyNameRulesetIds = "maxDttEIRP";

        /// <summary>
        /// Represents the Paws DeviceDescriptor Rule set Ids field.
        /// </summary>
        public const string PropertyNameMaxEirpHz = "maxEirpHz";

        /// <summary>
        /// Represents the Paws MaxTotal bandwidth in KHZ Rule set Ids field.
        /// </summary>
        public const string PropertyNameMaxTotalBwMhz = "maxTotalBwMhz";

        /// <summary>
        /// Represents the Paws MaxNominalChannel bandwidth in MHZ Rule set Ids field.
        /// </summary>
        public const string PropertyNameMaxNominalChannelBwMhz = "maxNominalChannelBwMhz";

        /// <summary>
        /// Represents the Paws DeviceDescriptor FCC Id field.
        /// </summary>
        public const string PropertyNameFccId = "fccId";

        /// <summary>
        /// Represents the Paws DeviceDescriptor FCC Device Type field.
        /// </summary>
        public const string PropertyNameFccTvbdDeviceType = "fccTvbdDeviceType";

        /// <summary>
        /// Represents the Paws DeviceDescriptor ETSI Device Type field.
        /// </summary>
        public const string PropertyNameEtsiDeviceType = "etsiEnDeviceType";

        /// <summary>
        /// Represents the Paws DeviceDescriptor ETSI Device Type A.
        /// </summary>
        public const string PropertyNameEtsiDeviceTypeA = "A";

        /// <summary>
        /// Represents the Paws DeviceDescriptor ETSI Emission Class field.
        /// </summary>
        public const string PropertyNameEtsiEnDeviceEmissionsClass = "etsiEnDeviceEmissionsClass";

        /// <summary>
        /// Represents the Paws DeviceDescriptor ETSI Device Category field.
        /// </summary>
        public const string PropertyNameEtsiDeviceCategory = "etsiEnDeviceCategory";

        /// <summary>
        /// Represents the Paws DeviceDescriptor ETSI Device Category field.
        /// </summary>
        public const string PropertyNameEtsiDeviceCategoryMaster = "Master";

        /// <summary>
        /// Represents the Paws DeviceDescriptor ETSI Device Category field.
        /// </summary>
        public const string PropertyNameEtsiDeviceCategorySlave = "Slave";

        /// <summary>
        /// Represents the Paws DeviceDescriptor ETSI Technology Identifier field.
        /// </summary>
        public const string PropertyNameEtsiEnTechnologyId = "etsiEnTechnologyId";

        /// <summary>
        /// Represents the Paws DeviceDescriptor ETSI Unique Device Identifier field.
        /// </summary>
        public const string PropertyNameEtsiUDI = "deviceId";

        /// <summary>
        /// Represents the Paws error message for when a FCC Device Type is missing.
        /// </summary>
        public const string ErrorMessageFccTvbdDeviceTypeRequired = "deviceDescriptor.fccTvbdDeviceType";

        /// <summary>
        /// Represents the Paws error message for when a ETSI Device Category is missing.
        /// </summary>
        public const string ErrorMessagEtsiDeviceCategory = "deviceDescriptor.EtsiDeviceCategory";

        /// <summary>The error message device identifier not authorized</summary>
        public const string ErrorMessagDeviceIdNotAuthorized = "Device is not authorized";

        /// <summary>
        /// Represents the Paws error message for when a ETSI Device Type is missing.
        /// </summary>
        public const string ErrorMessagEtsiDeviceType = "deviceDescriptor.EtsiDeviceType";

        /// <summary>
        /// Represents the Paws error message for when a ETSI ManufacturerId is missing.
        /// </summary>
        public const string ErrorMessagEtsiManufacturerId = "deviceDescriptor.EtsiManufacturerId";

        /// <summary>
        /// Represents the Paws error message for when a ETSI ModelId is missing.
        /// </summary>
        public const string ErrorMessagEtsiModelId = "deviceDescriptor.EtsiModelId";

        /// <summary>
        /// Represents the Paws error message for when the ETSI Manufacturer Id exceeds 64 characters.
        /// </summary>
        public const string ErrorMessageEtsiManufacturerIdLength = "ManufacturerId  can not exceed 64 characters";

        /// <summary>
        /// Represents the Paws error message for when the ETSI Model Id exceeds 64 characters.
        /// </summary>
        public const string ErrorMessageEtsiModelIdLength = "ModelId can not exceed 64 characters";

        /// <summary>
        /// Represents the Paws error message for Invalid ETSI Device Category.
        /// </summary>
        public const string ErrorMessagInvalidEtsiDeviceCategory = "Device Category can be master or slave.";

        /// <summary>
        /// Represents the Paws error message for when a serial number is missing.
        /// </summary>
        public const string ErrorMessageSerialNumberRequired = "deviceDescriptor.serialNumber";

        /// <summary>
        /// Represents the Paws error message for when a serial number is missing.
        /// </summary>
        public const string ErrorMessageUserIdRequired = "UserId";

        /// <summary>
        /// Represents the Paws error message for when a serial number is missing.
        /// </summary>
        public const string ErrorMessageUserFirstNameRequired = "UserFirstName";

        /// <summary>
        /// Represents the Paws error message for when a serial number is missing.
        /// </summary>
        public const string ErrorMessageUserLastNameRequired = "UserLastName";

        /// <summary>
        /// Represents the Paws error message for when a serial number is missing.
        /// </summary>
        public const string ErrorMessageCountryRequired = "Country";

        /// <summary>
        /// Represents the Paws error message for when a serial number is missing.
        /// </summary>
        public const string ErrorMessageCityRequired = "city";

        /// <summary>
        /// Represents the Paws error message for when a serial number is missing.
        /// </summary>
        public const string ErrorMessageRegionIdRequired = "Region";

        /// <summary>
        /// Represents the Paws error message for when a serial number is missing.
        /// </summary>
        public const string ErrorMessageAccessLevelRequired = "accessLevel";

        /// <summary>
        /// Represents the Paws error message for when a device Id is missing.
        /// </summary>
        public const string ErrorMessageDeviceIdRequired = "deviceDescriptor.deviceId";

        /// <summary>The error message request type required</summary>
        public const string ErrorMessageParametersRequestTypeRequired = "parameters.requestType";

        /// <summary>The error message parameters unique identifier required</summary>
        public const string ErrorMessageParametersUniqueIdRequired = "parameters.uniqueId";

        /// <summary>The error message parameters device descriptor required</summary>
        public const string ErrorMessageParametersDeviceDescriptorRequired = "parameters.deviceDescriptor";

        /// <summary>The error message parameters device category required</summary>
        public const string ErrorMessageParametersDeviceCategoryRequired = "parameters.etsiDeviceCategory";

        /// <summary>The error message parameters ETSI device type required</summary>
        public const string ErrorMessageParametersEtsiDeviceTypeRequired = "parameters.etsiEnDeviceType";

        /// <summary>
        /// Represents the Region Management error message for when a device Id is missing.
        /// </summary>
        public const string ErrorMessageRegionDeviceIdRequired = "deviceId";

        /// <summary>
        /// Represents the Paws error message for when a FCC Id is missing.
        /// </summary>
        public const string ErrorMessageFccIdRequired = "deviceDescriptor.fccId";

        /// <summary>
        /// Represents the Paws error message for when a FCC Id is missing.
        /// </summary>
        public const string ErrorMessageEtsiDeviceTypeLength = "deviceDescriptor.etsiDeviceType";

        /// <summary>
        /// Represents the Paws error message for when a FCC Id is missing.
        /// </summary>
        public const string ErrorMessageEtsiEmissionClass = "Emission Class can not exceed 1 characters";

        /// <summary>
        /// Represents the Paws error message for when a serial number exceeds 64 characters.
        /// </summary>
        public const string ErrorMessageSerialNumberLength = "SerialNumber can not exceed 64 characters";

        /// <summary>
        /// Represents the Paws error message for when FCC Id exceeds 19 characters.
        /// </summary>
        public const string ErrorMessageFccIdLength = "FCC Id can not exceed 19 characters";

        /// <summary>
        /// Represents the Paws error message for when FCC Id is less than 5 characters.
        /// </summary>
        public const string ErrorMessageFccIdMinimumLength = "FCC Id must be greater than 5 characters";

        /// <summary>
        /// Represents the Paws error message for when FCC Id exceeds 17 characters.
        /// </summary>
        public const string ErrorMessageFccIdAlphabetLength = "FCC Id can not exceed 17 characters";

        /// <summary>
        /// Represents the Paws error message for when FCC Id is less than 5 characters.
        /// </summary>
        public const string ErrorMessageFccIdAlphabetMinimumLength = "FCC Id must be greater than 3 characters";

        /// <summary>
        /// Represents the Paws FCC Id valid expression.
        /// </summary>
        public const string RegExFCCId = "^[2-9a-zA-Z]*$";

        /// <summary>
        /// Represents the Paws FCC Id invalid message.
        /// </summary>
        public const string ErrorMessageInvalidFccId = "FCC Id cannot contain zero or one";

        /// <summary>
        /// Represents the Paws error message for when the Manufacturer Id exceeds 64 characters.
        /// </summary>
        public const string ErrorMessageManufacturerIdLength = "ManufacturerId  can not exceed 64 characters";

        /// <summary>
        /// Represents the Paws error message for when the Model Id exceeds 64 characters.
        /// </summary>
        public const string ErrorMessageModelIdLength = "ModelId can not exceed 64 characters";

        /// <summary>
        /// Represents the Paws error message for when the Location contains both point and region.
        /// </summary>
        public const string ErrorMessageMutuallyExclusive = "Point and Region are mutually exclusive";

        /// <summary>
        /// Represents the Paws error message for when the Location does not contain SemiMajorAxis.
        /// </summary>
        public const string ErrorMessageSemiMajorAxis = "SemiMajorAxis is required";

        /// <summary>
        /// Represents the Paws error message for when the Location contains either Region or Confidence.
        /// </summary>
        public const string ErrorMessageRegionConfidence = "The location must be specified as a Point";

        /// <summary>
        /// Represents the Paws error message for when the Model Id exceeds 64 characters.
        /// </summary>
        public const string ErrorMessageTechnologyIdLength = "Technology identifier can not exceed 64 characters";

        /// <summary>
        /// Represents the Paws DeviceOwner Operator field.
        /// </summary>
        public const string PropertyNameOperator = "operator";

        /// <summary>
        /// Represents the Paws DeviceOwner required field missing.
        /// </summary>
        public const string ErrorMessageOwnerRequired = "deviceOwner.owner";

        /// <summary>
        /// Represents the Paws Ellipse's Center field name.
        /// </summary>
        public const string PropertyNameCenter = "center";

        /// <summary>
        /// Represents the Paws Ellipse's Major Axis field name.
        /// </summary>
        public const string PropertyNameSemiMajorAxis = "semiMajorAxis";

        /// <summary>
        /// Represents the Paws Ellipse's Minor Axis field name.
        /// </summary>
        public const string PropertyNameSemiMinorAxis = "semiMinorAxis";

        /// <summary>
        /// Represents the Paws Ellipse's Orientation field name.
        /// </summary>
        public const string PropertyNameOrientation = "orientation";

        /// <summary>
        /// Represents the Paws Ellipse's Center error message.
        /// </summary>
        public const string ErrorMessageCenterRequired = "ellipse.center";

        /// <summary>
        /// Represents the Paws EMail text field name.
        /// </summary>
        public const string PropertyNameText = "text";

        /// <summary>
        /// Represents the Paws EMail valid character regular expression.
        /// </summary>
        public const string RegExEmail = @"[a-z0-9._%+-]+@[a-z0-9.-]+\.[a-z]{2,4}";

        /// <summary>
        /// Represents the Paws EMail invalid email message.
        /// </summary>
        public const string ErrorMessageInvalidEmail = "email";

        /// <summary>The error message invalid incumbent type</summary>
        public const string ErrorMessageInvalidIncumbentType = "parameters.incumbentType";

        /// <summary>
        /// Represents the Paws generic error message field name.
        /// </summary>
        public const string PropertyNameMessage = "message";

        /// <summary>
        /// Represents the Paws generic data field name for error messages.
        /// </summary>
        public const string PropertyNameData = "data";

        /// <summary>
        /// Represents the Paws generic error field name.
        /// </summary>
        public const string PropertyNameError = "error";

        /// <summary>
        /// Represents the Paws event start time.
        /// </summary>
        public const string PropertyNameStartTime = "startTime";

        /// <summary>
        /// Represents the Paws event stop time.
        /// </summary>
        public const string PropertyNameStopTime = "stopTime";

        /// <summary>
        /// Represents the Paws event start time missing error.
        /// </summary>
        public const string ErrorMessageStartTimeRequired = "eventTime.startTime";

        /// <summary>
        /// Represents the Paws event stop time missing error.
        /// </summary>
        public const string ErrorMessageStopTimeRequired = "eventTime.stopTime";

        /// <summary>
        /// Represents the Paws FrequencyRange start Hz.
        /// </summary>
        public const string PropertyNameStartHz = "startHz";

        /// <summary>The property name hz</summary>
        public const string PropertyNameHz = "hz";

        /// <summary>
        /// Represents the Paws FrequencyRange stop Hz.
        /// </summary>
        public const string PropertyNameStopHz = "stopHz";

        /// <summary>
        /// Represents the Paws FrequencyRange max power in DB.
        /// </summary>
        public const string PropertyNameMaxPowerDBm = "maxPowerDBm";

        /// <summary>The property name maximum power DBM</summary>
        public const string PropertyNameDBm = "dbm";

        /// <summary>
        /// Represents the Paws FrequencyRange Channel Id.
        /// </summary>
        public const string PropertyNameChannelId = "channelId";

        /// <summary>
        /// Represents the Paws FrequencyRange start Hz missing error message.
        /// </summary>
        public const string ErrorMessageStartHzRequired = "profiles.hz";

        /// <summary>
        /// Represents the Paws FrequencyRange stop Hz missing error message.
        /// </summary>
        public const string ErrorMessageStopHzRequired = "frequencyRange.stopHz";

        /// <summary>
        /// Represents the Paws GeoLocation Point field name.
        /// </summary>
        public const string PropertyNamePoint = "point";

        /// <summary>
        /// Represents the Paws GeoLocation Confidence field name.
        /// </summary>
        public const string PropertyNameConfidence = "confidence";

        /// <summary>
        /// Represents the Paws GeoLocation Point missing error.
        /// </summary>
        public const string ErrorMessagePointRequired = "geoLocation.point";

        /// <summary>
        /// Represents the Paws GeoSpectrumSchedule Schedules field name.
        /// </summary>
        public const string PropertyNameSpectrumSchedules = "spectrumSchedules";

        /// <summary>
        /// Represents the Paws GeoSpectrumSchedule location field missing.
        /// </summary>
        public const string ErrorMessageGeoSpectrumLocationRequired = "geoSpectrumSchedule.location";

        /// <summary>
        /// Represents the Paws GeoSpectrumSchedule Schedules field missing.
        /// </summary>
        public const string ErrorMessageSpectrumSchedulesRequired = "geoSpectrumSchedule.spectrumSchedules";

        /// <summary>
        /// Represents the Paws InitRequest Location field missing.
        /// </summary>
        public const string ErrorMessageInitLocationRequired = "location";

        /// <summary>The error message distance less than 100</summary>
        public const string ErrorMessageDistanceLessThan100 = "Distance between receiver Location and transmit Location should be greater than or equal to 100 mtr";

        /// <summary>
        /// Represents the Paws UseSpectrumNotifyRequest Location field missing.
        /// </summary>
        public const string ErrorMessageSpectrumNotifyLocationRequired = "location";

        /// <summary>
        /// Represents the Paws Interference Query timestamp missing.
        /// </summary>
        public const string ErrorMessageTimeStampRequired = "timestamp";

        /// <summary>
        /// Represents the Paws Interference Query start time missing.
        /// </summary>
        public const string ErrorMessageInterferenceQueryStartTimeRequired = "starttime";

        /// <summary>
        /// Represents the Paws Interference Query end time missing.
        /// </summary>
        public const string ErrorMessageEndTimeRequired = "endtime";

        /// <summary>The error message LPAUX area required</summary>
        public const string ErrorMessageLpAuxAreaRequired = "either PointsArea or QuadilateralArea is required for lpaux";

        /// <summary>The error message LPAUX quad area invalid vertices</summary>
        public const string ErrorMessageLpAuxQuadAreaInvalidVertices = "distance between any two pair of vertices should not be more than 3 km";

        /// <summary>
        /// Represents the Paws Interference Query requestor missing.
        /// </summary>
        public const string ErrorMessageRequestorRequired = "requestor";

        /// <summary>
        /// Represents the Paws Interference Query requestor email missing.
        /// </summary>
        public const string ErrorMessageRequestorEmailRequired = "requestor.email";

        /// <summary>
        /// Represents the Paws Interference Query requestor organization missing.
        /// </summary>
        public const string ErrorMessageRequestorOrgRequired = "requestor.org";

        /// <summary>
        /// Represents the Paws InitRequest Descriptor field missing.
        /// </summary>
        public const string ErrorMessageInitDeviceDescriptorRequired = "deviceDesc";

        /// <summary>
        /// Represents the Paws Register Request's Owner field name.
        /// </summary>
        public const string PropertyNameDeviceOwner = "deviceOwner";

        /// <summary>
        /// Represents the Paws Interference Query Request's Owner field name.
        /// </summary>
        public const string PropertyNameRequestor = "requestor";

        /// <summary>
        /// Represents the Region Management TV Spectrum field name.
        /// </summary>
        public const string PropertyNameTvSpectrum = "tvSpectrum";

        /// <summary>
        /// Represents the Region Management registration disposition field name.
        /// </summary>
        public const string PropertyNameRegistrationDisposition = "registrationDisposition";

        /// <summary>
        /// Represents the Region Management TV Spectrum field name.
        /// </summary>
        public const string PropertyNameTvSpectra = "tvSpectra";

        /// <summary>
        /// Represents the Region Management Registrant field name.
        /// </summary>
        public const string PropertyNameRegistrant = "registrant";

        /// <summary>
        /// The property name LPAUX registrant
        /// </summary>
        public const string PropertyNameLPAuxRegistrant = "lpAuxRegistrant";

        /// <summary>The property name temporary BAS registrant</summary>
        public const string PropertyNameTempBASRegistrant = "tempBASRegistrant";

        /// <summary>
        /// Represents the Region Management Registrant field name.
        /// </summary>
        public const string PropertyNameContact = "contact";

        /// <summary>
        /// Represents the LP-Aux Registration Id field name.
        /// </summary>
        public const string PropertyNameLPAUXId = "lpAuxRegId";

        /// <summary>
        /// Represents the Region Management Registrant field name.
        /// </summary>
        public const string PropertyNameTransmitLocation = "transmitLocation";

        /// <summary>
        /// Represents the Region Management QuadrilateralArea field name.
        /// </summary>
        public const string PropertyNameQuadrilateralArea = "quadrilateralArea";

        /// <summary>
        /// Represents the Region Management Event field name.
        /// </summary>
        public const string PropertyNameEvent = "event";

        /// <summary>
        /// Represents the Region Management PointsArea field name.
        /// </summary>
        public const string PropertyNamePointsArea = "pointsArea";

        /// <summary>
        /// Represents the Region Management MVPDLocation field name.
        /// </summary>
        public const string PropertyNameMVPDLocation = "mvpdLocation";

        /// <summary>
        /// Represents the Region Management TEMPBASLocation field name.
        /// </summary>
        public const string PropertyNameTempBasLocation = "tempBasLocation";

        /// <summary>
        /// Represents the Region Management device type field name.
        /// </summary>
        public const string PropertyNameIncumbentType = "incumbentType";

        /// <summary>
        /// Represents the Region Management device type field name.
        /// </summary>
        public const string PropertyNameRegistrationId = "registrationId";

        /// <summary>
        /// Represents the Region Management device Id field name.
        /// </summary>
        public const string PropertyNameDeviceId = "deviceId";

        /// <summary>
        /// Represents the Paws Interference Query Request's start time.
        /// </summary>
        public const string PropertyNameInterferenceQueryStartTime = "starttime";

        /// <summary>
        /// Represents the Paws Interference Query Request's end time.
        /// </summary>
        public const string PropertyNameInterferenceQueryEndTime = "endtime";

        /// <summary>The property name reference sensitivity</summary>
        public const string PropertyNameReferenceSensitivity = "prefsens";

        /// <summary>The maximum EIRP master</summary>
        public const string PropertyNameMaxMasterEirp = "maxMasterEIRP";

        /// <summary>
        /// Represents the Paws Interference Query Request's request type.
        /// </summary>
        public const string PropertyNameInterferenceQueryRequestType = "requestType";

        /// <summary>
        /// Represents the Paws Register Request's Owner field missing error.
        /// </summary>
        public const string ErrorMessageDeviceOwnerRequired = "deviceOwner";

        /// <summary>
        /// Represents the Paws Point's latitude field name.
        /// </summary>
        public const string PropertyNameLatitude = "latitude";

        /// <summary>
        /// Represents the Paws Point's longitude field name.
        /// </summary>
        public const string PropertyNameLongitude = "longitude";

        /// <summary>
        /// Represents the Paws Point's latitude field missing error.
        /// </summary>
        public const string ErrorMessageLatitudeRequired = "point.latitude";

        /// <summary>
        /// Represents the Paws Point's longitude field missing error.
        /// </summary>
        public const string ErrorMessageLongitudeRequired = "point.longitude";

        /// <summary>
        /// Represents the Paws Polygon's exterior field name.
        /// </summary>
        public const string PropertyNameExterior = "exterior";

        /// <summary>
        /// Represents the Paws Request method field name.
        /// </summary>
        public const string PropertyNameMethod = "method";

        /// <summary>
        /// Represents the Paws Request parameters field name.
        /// </summary>
        public const string PropertyNameParams = "params";

        /// <summary>
        /// Represents the Paws Response Result field name.
        /// </summary>
        public const string PropertyNameResult = "result";

        /// <summary>
        /// Represents the Paws Response type field name.
        /// </summary>
        public const string PropertyNameType = "type";

        /// <summary>
        /// Represents the Paws Response code field name.
        /// </summary>
        public const string PawsPropertyNameCode = "code";

        /// <summary>
        /// Represents the Paws Response version field name.
        /// </summary>
        public const string PropertyNameVersion = "version";

        /// <summary>
        /// Represents the Paws Response rule set info field name.
        /// </summary>
        public const string PropertyNameRulesetInfo = "rulesetInfo";

        /// <summary>
        /// Represents the Paws Response database change field name.
        /// </summary>
        public const string PropertyNameDatabaseChange = "databaseChange";

        /// <summary>
        /// Represents the Paws Response time stamp field name.
        /// </summary>
        public const string PropertyNameTimeStamp = "timestamp";

        /// <summary>
        /// Represents the Paws Response Geo Spectrum Schedules field name.
        /// </summary>
        public const string PropertyNameGeoSpectrumSchedules = "geoSpectrumSchedules";

        /// <summary>
        /// Represents the Paws Response Geo Spectrum Schedules field name.
        /// </summary>
        public const string PropertyNameGeoSpectrumSpecs = "geoSpectrumSpecs";

        /// <summary>
        /// Represents the Paws Response Needs Spectrum Report field name.
        /// </summary>
        public const string PropertyNameNeedsSpectrumReport = "needsSpectrumReport";

        /// <summary>
        /// Represents the Paws Response Max Total Bandwidth in HZ field name.
        /// </summary>
        public const string PropertyNameMaxTotalBwHz = "maxTotalBwHz";

        /// <summary>
        /// Represents the Paws Response max contiguousBandwidth in HZ field name.
        /// </summary>
        public const string PropertyNameMaxContiguousBwHz = "maxContiguousBwHz";

        /// <summary>
        /// Represents the Paws RulesSetInfo's authority field name.
        /// </summary>
        public const string PropertyNameAuthority = "authority";

        /// <summary>
        /// Represents the Paws RulesSetInfo's max location change field name.
        /// </summary>
        public const string PropertyNameMaxLocationChange = "maxLocationChange";

        /// <summary>
        /// Represents the Paws RulesSetInfo's max polling seconds field name.
        /// </summary>
        public const string PropertyNameMaxPollingSecs = "maxPollingSecs";

        /// <summary>
        /// Represents the Paws Spectrum's bandwidth field name.
        /// </summary>
        public const string PropertyNameResolutionBwHz = "resolutionBwHz";

        /// <summary>
        /// Represents the Paws Spectrum's latitude field missing error.
        /// </summary>
        public const string ErrorMessageBandwidthRequired = "notifySpectrumUse.spectra.resolutionBwHz";

        /// <summary>
        /// Represents the Paws Spectrum's longitude field missing error.
        /// </summary>
        public const string ErrorMessageFrequencyRangesRequired = "notifySpectrumUse.spectra.FrequencyRanges";

        /// <summary>The error message spectrum profile required</summary>
        public const string ErrorMessageSpectrumProfileRequired = "notifySpectrumUse.spectra.profiles";

        /// <summary>
        /// Represents the Paws Location field missing error.
        /// </summary>
        public const string ErrorMessageLocationRequired = "Location is Required";

        /// <summary>
        /// Represents the region management number of location points error.
        /// </summary>
        public const string ErrorMessageLocationsArrayLength = "Must contain four sets of location points";

        /// <summary>
        /// Represents the Region Management Exclude Channels Locations field missing error.
        /// </summary>
        public const string ErrorMessageLocationsRequired = "Locations is Required";

        /// <summary>The error message invalid location</summary>
        public const string ErrorMessageInvalidLocation = "either one or all Locations provided are invalid";

        /// <summary>
        /// Represents the Paws Location field missing error.
        /// </summary>
        public const string ErrorMessageLocationParameterMutuallyExclusion = "Location.point and Loction.Region are mutually exclusive";

        /// <summary>
        /// Represents the Paws SpectrumSchedule's event time field name.
        /// </summary>
        public const string PropertyNameEventTime = "eventTime";

        /// <summary>
        /// Represents the Paws SpectrumSchedule's spectra time field name.
        /// </summary>
        public const string PropertyNameSpectra = "spectra";

        /// <summary>
        /// Represents the Paws SpectrumSchedule's event time field missing error.
        /// </summary>
        public const string ErrorMessageEventTimeRequired = "spectrumSchedule.eventTime";

        /// <summary>
        /// Represents the Paws SpectrumSchedule's spectra field missing error.
        /// </summary>
        public const string ErrorMessageSpectraRequired = "spectrumSchedule.spectra";

        /// <summary>
        /// Represents the Paws Spectrum Use Notify spectra field missing error.
        /// </summary>
        public const string ErrorMessageSpectrumRequired = "notifySpectrumUse.spectra";

        /// <summary>
        /// Represents the Paws VCard's full name field name.
        /// </summary>
        public const string PropertyNameFullName = "fn";

        /// <summary>
        /// Represents the Paws VCard's organization field name.
        /// </summary>
        public const string PropertyNameOrganization = "org";

        /// <summary>
        /// Represents the Paws VCard's address field name.
        /// </summary>
        public const string PropertyNameAddress = "adr";

        /// <summary>
        /// Represents the Paws VCard's phone number field name.
        /// </summary>
        public const string PropertyNamePhone = "tel";

        /// <summary>
        /// Represents the Paws VCard's phone number field name.
        /// </summary>
        public const string PropertyNamePhoneNumber = "Uri";

        /// <summary>
        /// Represents the Paws VCard's email field name.
        /// </summary>
        public const string PropertyNameEmail = "email";

        /// <summary>
        /// Represents the Paws NotifyRequired Spectra field missing error.
        /// </summary>
        public const string ErrorMessageNotifySpectraRequired = "Spectra";

        /// <summary>
        /// Represents the Paws Device Validity's IsValid field name.
        /// </summary>
        public const string PropertyNameIsValid = "isValid";

        /// <summary>
        /// Represents the Paws Device Validity's IsNotValid field name.
        /// </summary>
        public const string PropertyNameReason = "reason";

        /// <summary>
        /// Represents the Paws error message for when the reason of invalid device exceeds 128 characters.
        /// </summary>
        public const string ErrorMessageReasonLength = "Reason can not exceed 128 characters";

        /// <summary>
        /// Represents the Paws reason message for when the slave device is not found valid.
        /// </summary>
        public const string InvalidDeviceReasonMessage = "The Device is not authorized to use the Database";

        /// <summary>
        /// Represents the Paws Device Validity's IsValid field missing error.
        /// </summary>
        public const string ErrorMessageDeviceValidityIsValidRequired = "deviceValidity.isValid";

        /// <summary>
        /// Represents the Paws Device Validity's IsValid field name.
        /// </summary>
        public const string ErrorMessageDeviceValidityDeviceDescriptorRequired = "deviceValidity.deviceDescriptor";

        /// <summary>
        /// Represents the Paws Device IDeviceValidRequest's Device Descriptors field name.
        /// </summary>
        public const string PropertyNameDeviceDescriptors = "deviceDescs";

        /// <summary>
        /// Represents the Paws Device IDeviceValidRequest's Device Descriptors field missing error.
        /// </summary>
        public const string ErrorMessageDeviceDescriptorsRequired = "deviceDescs";

        /// <summary>
        /// Represents the Paws Device IDeviceValidRequest's Device Descriptors field missing error.
        /// </summary>
        public const string ErrorMessageNotRegistered = "Device not Registered";

        /// <summary>
        /// The error message mode not implemented
        /// </summary>
        public const string ErrorMessageModeNotImplemented = "Device Mode not Implemented";

        /// <summary>
        /// Represents the Paws Device IDeviceValidRequest's Device validities field name.
        /// </summary>
        public const string PropertyNameDeviveValidities = "deviceValidities";

        /// <summary>
        /// Represents the Region Management Channel Info field name.
        /// </summary>
        public const string PropertyNameChannelInfo = "channelInfo";

        /// <summary>
        /// Represents the Region Management Incumbent List field name.
        /// </summary>
        public const string PropertyNameIncumbentList = "incumbentList";

        /// <summary>
        /// Represents the Region Management Users field name.
        /// </summary>
        public const string PropertyNameUserList = "Users";

        /// <summary>
        /// Represents the Region Management Device List field name.
        /// </summary>
        public const string PropertyNameDeviceList = "deviceList";

        /// <summary>
        /// Represents the LPAUXUnlicensed List field name.
        /// </summary>
        public const string PropertyNameLpAuxUnlicensedList = "lpAuxUnlicensedList";

        /// <summary>The property name region management device identifier</summary>
        public const string PropertyNameRegionManagementUniqueId = "uniqueId";

        /// <summary>The property name region management request type</summary>
        public const string PropertyNameRegionManagementRequestType = "requestType";

        /// <summary>The property name region management start time</summary>
        public const string PropertyNameRegionManagementStartTime = "startTime";

        /// <summary>The property name region management end time</summary>
        public const string PropertyNameRegionManagementEndTime = "endTime";

        /// <summary>The property name region management channels</summary>
        public const string PropertyNameRegionManagementChannels = "channels";

        /// <summary>The property name callsign info</summary>
        public const string PropertyNameCallsignInfo = "callsignInfo";

        /// <summary>
        /// Represents the Paws generic parameter missing error.
        /// </summary>
        public const string ExceptionMessageParametersRequired = "Parameters Required";

        /// <summary>
        /// Represents the Paws generic invalid method error.
        /// </summary>
        public const string ExceptionMessageInvalidMethod = "Invalid method";

        /// <summary>
        /// Represents the Paws generic parameter name field.
        /// </summary>
        public const string ParameterNameParameters = "parameters";

        /// <summary>
        /// Represents the reference table for Paws registration.
        /// </summary>
        public const string DeviceRegistrationTable = "DeviceRegistrationMap";

        /// <summary>
        /// The PMSE assignments table
        /// </summary>
        public const string PMSEAssignmentsTable = "PMSEAssignments";

        /// <summary>
        /// Represents the table for Paws registration.
        /// </summary>
        public const string FixedTVBDRegistration = "Fixed_TVBD_Registration";

        /// <summary>
        /// Represents the table for Paws registration.
        /// </summary>
        public const string LPAuxRegistration = "LP_Aux_Registration";

        /// <summary>
        /// Represents the destination table for Paws registration.
        /// </summary>
        public const string RGN1FixedTVBDRegistrationTable = "RGN1FixedTVBDRegistration";

        /// <summary>
        /// Represents the table to retrieve the actual table name for Paws initialization.
        /// </summary>
        public const string InitializedDeviceDetailsTable = "InitializedDeviceDetails";

        /// <summary>
        /// Represents the table to retrieve the actual table name for Paws Spectrum Usage Notify.
        /// </summary>
        public const string SpectrumUsageTable = "UsedSpectrum";

        /// <summary>
        /// Represents the response table for Paws initialization.
        /// </summary>
        public const string RGN1RuleSetInformationTable = "RGN1RuleSetInformation";

        /// <summary>
        /// Represents the response table for Paws initialization.
        /// </summary>
        public const string RuleSetInformationTable = "RuleSetInformation";

        /// <summary>
        /// Represents the variable to derive the destination table for Paws registration.
        /// </summary>
        public const string FixedTVBDRegistrationTable = "FixedTVBDRegistration";

        /// <summary>
        /// Represents the table for Paws registration.
        /// </summary>
        public const string LPAuxRegistrationTable = "LPAuxRegistration";

        /// <summary>
        /// Represents the table for region management registration.
        /// </summary>
        public const string RGN1LPAuxRegistrationTable = "RGN1LPAuxRegistration";

        /// <summary>
        /// Represents the table for region management registration.
        /// </summary>
        public const string RGN1LPAuxRegistrationDetailsTable = "RGN1LPAuxRegistrationDetails";

        /// <summary>
        /// Represents the table for region management registration.
        /// </summary>
        public const string RGN1TempBasRegistrationTable = "RGN1TempBASRegistration";

        /// <summary>
        /// Represents the table for Slave Device Validation.
        /// </summary>
        public const string RegisteredDeviceValidationTable = "RegisteredDevices";

        /// <summary>
        /// Represents the destination table for Slave Device Validation.
        /// </summary>
        public const string RGN1RegisteredDeviceValidationTable = "RGN1RegisteredDevices";

        /// <summary>
        /// Represents the table for Incumbent Information.
        /// </summary>
        public const string IncumbentInformationTable = "tvtowers";

        /// <summary>
        /// Represents the table for Authorized Users.
        /// </summary>
        public const string AuthorizedUsersTable = "AuthorizedUsers";

        /// <summary>
        /// Represents the table for Region Access.
        /// </summary>
        public const string RegionAccessTable = "RegionAccess";

        /// <summary>
        /// Represents the table for Access Elevation.
        /// </summary>
        public const string ElevateAccessTable = "ElevateAccess";

        /// <summary>
        /// Represents the table for Paws Setting.
        /// </summary>
        public const string SettingsTable = "ConfigSettings";

        /// <summary>
        /// Represents table for portal contours
        /// </summary>
        public const string PortalContoursTable = "PortalContours";

        /// <summary>
        /// Represents table for portal summary
        /// </summary>
        public const string PortalSummaryTable = "PortalSummary";

        /// <summary>
        /// Represents the Key for date from Paws Setting.
        /// </summary>
        public const string SettingsDateKey = "LastRegDate";

        /// <summary>
        /// Represents the Key for sequence for Paws Setting.
        /// </summary>
        public const string SettingsSequenceKey = "LastRegSeq";

        /// <summary>
        /// Represents the Partition Key for Settings table.
        /// </summary>
        public const string SettingsPartitionKey = "PawsRegistration";

        /// <summary>
        /// Represents the VCards xml namespace value.
        /// </summary>
        public const string VCardXmlns = "urn:ietf:params:xml:ns:vcard-4.0";

        /// <summary>
        /// Represents the ICal xml namespace value.
        /// </summary>
        public const string ICalXmlns = "urn:ietf:params:xml:ns:icalendar-2.0";

        /// <summary>
        /// Represents the GML xml namespace value.
        /// </summary>
        public const string GMLXmlns = "http://www.opengis.net/gml";

        /// <summary>
        /// Represents the RegistrationRecordEnsemble xml namespace value. 
        /// </summary>RegistrationRecordEnsembleXmlns
        public const string RegistrationRecordEnsembleXmlns = "http://www.whitespace-db-providers.org/2011//InterDB/xsd";

        /// <summary>
        /// Represents the SPBR's Certificate Subject 
        /// </summary>RegistrationRecordEnsembleXmlns
        public const string SPBRWSDBACertificateSubject = "CN=ftp.tvws.demo.spectrumbridge.com, C=US, S=FL, L=Lake Mary, O=Spectrum Bridge Inc.";

        /// <summary>The configuration table name</summary>
        public const string ConfigTableName = "ConfigSettings";

        /// <summary>
        /// Represents MVPD Registration Azure Table Name
        /// </summary>
        public const string MVPDRegistrationTableName = "MVPDRegistration";

        /// <summary>
        /// Represents MVPD Registration Azure Table Name
        /// </summary>
        public const string RGN1MVPDRegistrationTableName = "RGN1MVPDRegistration";

        /// <summary>
        /// Represents Temp Bas Registrations Azure Table Name
        /// </summary>
        public const string TempBasRegistrationTableName = "TempBASRegistration";

        /// <summary>
        /// Represents Poll Info
        /// </summary>
        public const string DBAdminInfoTable = "DBAdminInfo";

        /// <summary>
        /// Represents Fixed TVBD Table Name
        /// </summary>
        public const string FixedTVBDRegistrationTablename = "FixedTVBDRegistration";

        /// <summary>
        /// Represents Fixed TVBD Table Name
        /// </summary>
        public const string TVReceiveSiteRegistrationTablename = "TVReceiveSiteRegistration";

        /// <summary>
        /// Represents Next Transaction ID Table Name
        /// </summary>
        public const string NextTransactionIdTablename = "TransactionIdIssued";

        /// <summary>The MVPD waiver call sign table name</summary>
        public const string MVPDWaiverCallSignTableName = "MVPDWaiverCallSigns";

        /// <summary>The translator waiver call sign table name</summary>
        public const string TranslatorWaiverCallSignTableName = "TranslatorWaiverCallSigns";

        /// <summary>The authorized device models table name</summary>
        public const string AuthorizedDeviceModelsTableName = "AuthorizedDeviceModels";

        /// <summary>The CDBS canada TVENG data table name</summary>
        public const string CDBSCanadaTvEngDataTableName = "CDBSCanadaTvEngDataTable";

        /// <summary>
        /// The CDBS TV ENG data table name
        /// </summary>
        public const string CDBSUSMexicoTVEngDataTableName = "CDBSUSMexicoTVEngData";

        /// <summary>The CDBS translator data table name</summary>
        public const string CDBSTranslatorDataTableName = "CDBSTranslatorData";

        /// <summary>The ULS MICRO table name</summary>
        public const string ULSBroadcastAuxiliaryTableName = "BroadcastAuxiliaryStations";

        /// <summary>The ULS CMRS and PLMRS table name</summary>
        public const string ULSPLCMRSTableName = "ULSPLCMRSData";

        /// <summary>The ULS licensed LPAUX table name</summary>
        public const string ULSLicensedLPAuxTableName = "ULSLicensedLPAuxData";

        /// <summary>The ULS unlicensed LPAUX table name</summary>
        public const string ULSUnLicensedLPAuxTableName = "ULSUnLicensedLPAuxData";

        /// <summary>The ULS LM COMM table name</summary>
        public const string ULSLMCommTableName = "ULSLMComm";

        /// <summary>The LPAUX Registration Details</summary>
        public const string LPAuxRegDetails = "LPAuxRegistrationDetails";

        /// <summary>The TBAND protection table</summary>
        public const string TBandProtectionTable = "TBandProtection";

        /// <summary>The off shore protection table</summary>
        public const string OffShoreProtectionTable = "OffshoreRadioService";

        /// <summary>The radio astronomy protection</summary>
        public const string RadioAstronomyProtection = "RadioAstronomyData";

        /// <summary>The merged CDBS data table name</summary>
        public const string MergedCDBSDataTableName = "MergedCDBSData";

        /// <summary>
        /// The CDBS antenna pattern table name
        /// </summary>
        public const string CDBSAntennaPatternTableName = "CDBSAntennaPattern";

        /// <summary>
        /// The CDBS application table name
        /// </summary>
        public const string CDBSApplicationTableName = "CDBSApplication";

        /// <summary>
        /// The CDBS facility table name
        /// </summary>
        public const string CDBSFacilityTableName = "CDBSFacility";

        /// <summary>
        /// The RegionContours table name.
        /// </summary>
        public const string RegionContourTableName = "RegionContour";

        /// <summary>
        /// The FCC region code.
        /// </summary>
        public const string FccRegionCode = "RGN1";

        /// <summary>
        /// The CDBS app tracking table name
        /// </summary>
        public const string CDBSAppTrackingTableName = "CDBSAppTracking";

        /// <summary>
        /// The region sync status table name
        /// </summary>
        public const string RegionSyncStatusTableName = "RegionSyncStatus";

        /// <summary>
        /// The Next Transaction ID Azure table name
        /// </summary>
        public const string NextTransactionIdTableName = "TransactionIDIssued";

        /// <summary>
        /// Represents the table for Excluded Ids.
        /// </summary>
        public const string ExcludedIds = "ExcludedIds";

        /// <summary>
        /// Represents the table for Excluded Regions.
        /// </summary>
        public const string ExcludedChannels = "ExcludedChannels";

        /// <summary>
        /// Represents that incumbent type is required.
        /// </summary>
        public const string ErrorMessageIncumbentTypeRequired = "incumbentType";

        /// <summary>
        /// Represents that registration disposition is required.
        /// </summary>
        public const string ErrorMessageRegistrationDispositionRequired = "Registration Disposition is required";

        /// <summary>
        /// Represents that registration id is required.
        /// </summary>
        public const string ErrorMessageRegIdRequired = "Reg Id is required";

        /// <summary>
        /// Represents that channel and call sign is required.
        /// </summary>
        public const string ErrorMessageCallSignAndChannelRequired = "Call Sign and Channel are missing";

        /// <summary>The error message TV spectrum required</summary>
        public const string ErrorMessageTvSpectrumRequired = "TvSpectrum Information is required";

        /// <summary>
        /// Represents that event is required.
        /// </summary>
        public const string ErrorMessageEventRequired = "One or more events are required";

        /// <summary>
        /// Represents that event times stamp is required.
        /// </summary>
        public const string ErrorMessageEventTimesStampRequired = "event.Times.Stamp";

        /// <summary>
        /// Represents that event times is required.
        /// </summary>
        public const string ErrorMessageEventTimesRequired = "event.Times";

        /// <summary>
        /// Represents that event stamp start is required.
        /// </summary>
        public const string ErrorMessageEventTimesStartRequired = "event.Times.Start";

        /// <summary>
        /// Represents that event stamp end is required.
        /// </summary>
        public const string ErrorMessageEventTimesEndRequired = "event.Times.End";

        /// <summary>The error message event times invalid duration</summary>
        public const string ErrorMessageEventTimesInvalidDuartion = "Invalid event registration duration. Registrations are valid for only one year,";

        /// <summary>The error message event time recurrence frequency required</summary>
        public const string ErrorMessageEventTimeRecurrenceFrequencyRequired = "event.Times.Recurrence.Frequency";

        /// <summary>The error message event time recurrence interval count invalid</summary>
        public const string ErrorMessageEventTimeRecurrenceIntervalCountInvalid = "count should be single digit and required for Interval type Count";

        /// <summary>The error message event time recurrence interval until invalid</summary>
        public const string ErrorMessageEventTimeRecurrenceIntervalUntilInvalid = "until should greater than start time and required for Interval type Until";

        /// <summary>
        /// Represents that event start and end times difference cannot exceed 720 hours.
        /// </summary>
        public const string ErrorMessageEventTimesDifference = "total duration of event can not exceed 720 hours";

        /// <summary>
        /// The error message full name required
        /// </summary>
        public const string ErrorMessageFullNameRequired = "registrant.Name";

        /// <summary>
        /// The error message phone number required
        /// </summary>
        public const string ErrorMessagePhoneNumberRequired = "registrant.Telephone";

        /// <summary>
        /// Represents that channels in event is required.
        /// </summary>
        public const string ErrorMessageChannelsRequired = "event.Channels";

        /// <summary>
        /// Represents that TVSpectra is required.
        /// </summary>
        public const string ErrorMessageTvSpectraRequired = "TV Spectra are missing";

        /// <summary>
        /// Represents that latitude and longitude is required.
        /// </summary>
        public const string ErrorMessageLatitudeAndLongitudeRequired = "Latitude and Longitude are missing";

        /// <summary>
        /// Represents that Point Area and Quadrilateral Area are mutually exclusive.
        /// </summary>
        public const string ErrorMessagePointAreaQuadAreaMutuallyExclusive = "Point Area and Quadrilateral Area are mutually exclusive";

        /// <summary>
        /// Represents that Transmit Location is required.
        /// </summary>
        public const string ErrorMessageTransmitLocationRequired = "Transmit location is missing";

        /// <summary>
        /// Represents that contact information is required.
        /// </summary>
        public const string ErrorMessageContactInfoRequired = "Contact Information is missing";

        /// <summary>
        /// The error message venue required
        /// </summary>
        public const string ErrorMessageVenueRequired = "Venue Name is missing";

        /// <summary>
        /// The error message ULS file number required
        /// </summary>
        public const string ErrorMessageULSFileNumRequired = "ULS File Number is missing";

        /// <summary>
        /// Represents that call sign is required.
        /// </summary>
        public const string ErrorMessageCallSignRequired = "Call Sign is missing";

        /// <summary>The error message call sign not exist</summary>
        public const string ErrorMessageCallSignNotExist = "Call Sign do not exist in ULS data";

        /// <summary>
        /// Represents that channel is required.
        /// </summary>
        public const string ErrorMessageChannelRequired = "Channel is missing";

        /// <summary>
        /// Represents the length of Quad points.
        /// </summary>
        public const string ErrorMessageNumberOfVertices = "Cannot contain more than 4 sets of four vertices defining quadrilaterals";

        /// <summary>
        /// Represents the length of point areas.
        /// </summary>
        public const string ErrorMessageNumberOfPoints = "Cannot contain more than 25 Points";

        /// <summary>
        /// Represents that either points area or quadrilateral area is required.
        /// </summary>
        public const string ErrorMessagePointAreaOrQuadAreaRequired = "Either pointArea or quadrilateralArea is required";

        /// <summary>
        /// Represents that latitude is required.
        /// </summary>
        public const string ErrorMessageLatitudeMissing = "Latitude is missing";

        /// <summary>
        /// Represents that longitude is required.
        /// </summary>
        public const string ErrorMessageLongitudeMissing = "Longitude is missing";

        /// <summary>
        /// Represents invalid transmit location
        /// </summary>
        public const string ErrorMessageInvalidTransmitLocation = "Invalid Transmit Location";

        /// <summary>
        /// Represents invalid MVPD Location
        /// </summary>
        public const string ErrorMessageInvalidMvpdLocation = "Invalid MVPD Location";

        /// <summary>
        /// Represents invalid TempBAS Location
        /// </summary>
        public const string ErrorMessageInvalidTempBasLocation = "Invalid TempBAS Location";

        /// <summary>
        /// Represents that Organization name is required.
        /// </summary>
        public const string ErrorMessageOrgNameMissing = "Organization Name is missing";

        /// <summary>
        /// Represents that Registrant is required.
        /// </summary>
        public const string ErrorMessageRegistrantMissing = "Registrant is missing";

        /// <summary>
        /// Represents that contact name is required.
        /// </summary>
        public const string ErrorMessageContactNameMissing = "Contact Name is missing";

        /// <summary>
        /// Represents that contact address is required.
        /// </summary>
        public const string ErrorMessageContactAddressMissing = "Contact Address is missing";

        /// <summary>
        /// Represents that street is required.
        /// </summary>
        public const string ErrorMessageStreetMissing = "Street Address is missing";

        /// <summary>
        /// Represents that city is required.
        /// </summary>
        public const string ErrorMessageCityMissing = "City is missing";

        /// <summary>
        /// Represents that State is required.
        /// </summary>
        public const string ErrorMessageStateMissing = "State is missing";

        /// <summary>
        /// Represents that Country is required.
        /// </summary>
        public const string ErrorMessageCountryMissing = "Country is missing";

        /// <summary>The error message ULS file number not match</summary>
        public const string ErrorMessageULSFileNumberNotMatch = "ULS File Number not authorized";

        /// <summary>The error message ULS venue name not match</summary>
        public const string ErrorMessageULSVenueNameNotMatch = "Venue name not match with ULS data";

        /// <summary>The error message invalid start date</summary>
        public const string ErrorMessageInvalidStartDate = "start date cannot be later than or equal to the expiration date of the venues";

        /// <summary>The error message invalid end date</summary>
        public const string ErrorMessageInvalidEndDate = "end date cannot be later than the expiration date of the venues & less than start date";

        /// <summary>The error message no data</summary>
        public const string ErrorMessageNoData = "NO_DATA_FOUND";

        /// <summary>The error message device already excluded</summary>
        public const string ErrorMessageDeviceAlreadyExcluded = "Device already excluded";

        /// <summary>The error message LPAUX region not available</summary>
        public const string ErrorMessageLPAUXRegionNotAvailable = "Provided LPAUX Region Not available";

        /// <summary>
        /// Represents that code is required.
        /// </summary>
        public const string ErrorMessageZipCodeMissing = "ZipCode is missing";

        /// <summary>The error message MVPD call sign do not exist</summary>
        public const string ErrorMessageMVPDCallSignDoNotExist = "Specified CallSign do not exists in USTVstations";

        /// <summary>The error message MVPD location required</summary>
        public const string ErrorMessageMVPDLocationRequired = "MVPD Location is Required";

        /// <summary>The error message no contour exists</summary>
        public const string ErrorMessageNoContourExists = "Specified CallSign do not contain any contour information";

        /// <summary>The error message MVPD inside contour</summary>
        public const string ErrorMessageMVPDInsideContour = "MVPD Location lies inside transmitter station contour";

        /// <summary>The error message MVPD outside valid distance</summary>
        public const string ErrorMessageMVPDOutsideValidDistance = "Failed to register.  Please refer to Interference protection requirements §15.712 3(b)";

        /// <summary>The error message invalid points</summary>
        public const string ErrorMessageInvalidPoints = "Invalid Points for transmitter and receiver";

        /// <summary>The error message wrong incumbent type</summary>
        public const string ErrorMessageWrongIncumbentType = "Invalid IncumbentType";

        /// <summary>The error message required contour call sign</summary>
        public const string ErrorMessageRequiredContourCallSign = "Contour CallSign required";

        /// <summary>The error message contour call sign not found</summary>
        public const string ErrorMessageContourCallSignNotFound = "Contour CallSign not found in database";

        /// <summary>
        /// Represents the super admin access level.
        /// </summary>
        public const string SuperAdmin = "SuperAdmin";

        /// <summary>
        /// Represents the admin access level.
        /// </summary>
        public const string Admin = "Admin";

        /// <summary>
        /// Represents the Licensee access level.
        /// </summary>
        public const string Licensee = "Licensee";

        /// <summary>
        /// Represents the Licensee access level.
        /// </summary>
        public const string ErrorLPAUXDataMissing = "Registration Data Mising";

        /// <summary>The default type a height_ device parameter</summary>
        public const string ConfigSettingDefaultTypeAHeightDeviceParam = "DefaultTypeAHeight_DeviceParam";

        /// <summary>The configuration setting default device emission class device parameter</summary>
        public const string ConfigSettingDefaultDeviceEmissionClassDeviceParam = "DefaultDeviceEmissionClass_DeviceParam";

        /// <summary>The configuration setting default type B height device parameter</summary>
        public const string ConfigSettingDefaultTypeBHeightDeviceParam = "DefaultTypeBHeight_DeviceParam";

        /// <summary>The configuration setting default height uncertain device parameter</summary>
        public const string ConfigSettingDefaultHeightUncertinityDeviceParam = "DefaultHeightUncertinity_DeviceParam";

        /// <summary>The configuration setting default tech identifier device parameter</summary>
        public const string ConfigSettingDefaultTechIdentifierDeviceParam = "DefaultTechIdentifier_DeviceParam";

        /// <summary>The configuration setting default PREFSENS device parameter</summary>
        public const string ConfigSettingDefaultPREFSENSDeviceParam = "DefaultPREFSENS_DeviceParam";

        /// <summary>The configuration setting default maximum master EIRP</summary>
        public const string ConfigSettingDefaultMaxMasterEIRP = "DefaultMaxMasterEIRP";

        /// <summary>The configuration setting antenna gain type A</summary>
        public const string ConfigSettingAntennaGainTypeA = "AntennaGain_TypeA";

        /// <summary>The configuration setting antenna gain type B</summary>
        public const string ConfigSettingAntennaGainTypeB = "AntennaGain_TypeB";

        /// <summary>The configuration setting WSDBA name</summary>
        public const string ConfigSettingWSDBAName = "WSDBA";

        /// <summary>The configuration setting database synchronize file interval</summary>
        public const string ConfigSettingDbSyncFileInterval = "DbSyncFileInterval_MilliSec";

        /// <summary>The configuration setting MVPDT TV station query distance</summary>
        public const string ConfigSettingMVPDTTvStationQueryDistance = "MVPDTTvStationQueryDistance_KM";

        /// <summary>The configuration setting ARC1 data directory name</summary>
        public const string ConfigSettingArc1DataDirectoryName = "arc1DataDirectory";

        /// <summary>The configuration setting ARC2 data directory name</summary>
        public const string ConfigSettingArc2DataDirectoryName = "arc2DataDirectory";

        /// <summary>The configuration setting DTT synchronize table suffix</summary>
        public const string ConfigSettingDTTSyncTableSuffix = "DttSyncTableSuffix";

        public const string ConfigSettingDefaultDTTSyncTableSuffix = "v01112013";

        /// <summary>The configuration setting DTT synchronize status table</summary>
        public const string ConfigSettingDTTSyncStatusTable = "DttSyncStatus";

        /// <summary>The configuration setting DTT synchronize source container</summary>
        public const string ConfigSettingDttSyncSourceContainer = "DttSyncSourceContainer";

        /// <summary>The entity type US station</summary>
        public const string EntityTypeUSStation = "TV_US";

        /// <summary>The entity type mexico station</summary>
        public const string EntityTypeMexicoStation = "TV_MX";

        /// <summary>The entity type canada station</summary>
        public const string EntityTypeCanadaStation = "TV_CA";

        /// <summary>
        /// The data cache update table name
        /// </summary>
        public const string DataCacheUpdateStatusTableName = "DataCacheUpdateStatus";

        /// <summary>
        /// The Unscheduled Adjustments Table Name
        /// </summary>timeRange
        public const string UnscheduledAdjustmentsTableName = "UnscheduledAdjustments";

        /// <summary>
        /// The PMSE Sync Status Table Name
        /// </summary>
        public const string PmseSyncStatusTableName = "PMSESyncStatus";

        /// <summary>
        /// The PMSE Sync
        /// </summary>
        public const string PmseSync = "PmseSync";

        /// <summary>The property name time range</summary>
        public const string PropertyNameTimeRange = "timeRange";

        /// <summary>The property name needs spectrum specs</summary>
        public const string PropertyNameSpectrumSpecs = "spectrumSpecs";

        /// <summary>The T band adjacent channel distance</summary>
        public const string TBandAdjacentChannelDistance = "TBandAdjChannelDistance_KM";

        /// <summary>The T band same channel distance</summary>
        public const string TBandSameChannelDistance = "TBandCoChannelDistance_KM";

        /// <summary>The TV station search distance</summary>
        public const string TvStationSearchDistance = "TvStationSearchDistance_KM";

        /// <summary>The TV station fixed device co channel distance</summary>
        public const string TvStationFixedDeviceCoChannelDistance = "TvStationFixedDeviceCoChannelDistance_KM";

        /// <summary>The TV station fixed device adjacent channel distance</summary>
        public const string TvStationFixedDeviceAdjChannelDistance = "TvStationFixedDeviceAdjChannelDistance_KM";

        /// <summary>The TV station portable device co channel distance</summary>
        public const string TvStationPortableDeviceCoChannelDistance = "TvStationPortableDeviceCoChannelDistance_KM";

        /// <summary>The TV station portable device adjacent channel distance</summary>
        public const string TvStationPortableDeviceAdjChannelDistance = "TvStationPortableDeviceAdjChannelDistance_KM";

        /// <summary>The keyhole inner co channel distance</summary>
        public const string KeyholeInnerCoChannelDistance = "KeyholeInnerCoChannelDistance_KM";

        /// <summary>The keyhole inner adjacent channel distance</summary>
        public const string KeyholeInnerAdjChannelDistance = "KeyholeInnerAdjChannelDistance_KM";

        /// <summary>The keyhole outer co channel distance</summary>
        public const string KeyholeOuterCoChannelDistance = "KeyholeOuterCoChannelDistance_KM";

        /// <summary>The keyhole outer adjacent channel distance</summary>
        public const string KeyholeOuterAdjChannelDistance = "KeyholeOuterAdjChannelDistance_KM";

        /// <summary>The CMRS adjacent channel distance</summary>
        public const string CMRSAdjacentChannelDistance = "CMRSAdjacentChannelDistance_KM";

        /// <summary>The CMRS co channel distance</summary>
        public const string CMRSCoChannelDistance = "CMRSCoChannelDistance_KM";

        /// <summary>The LPAUX fixed device distance</summary>
        public const string LpAuxFixedDeviceDistance = "LpAuxFixedDeviceDistance_KM";

        /// <summary>The LPAUX portable device distance</summary>
        public const string LpAuxPortableDeviceDistance = "LpAuxPortableDeviceDistance_KM";

        /// <summary>The radio astronomy distance</summary>
        public const string RadioAstronomyDistance = "RadioAstronomyDistance_KM";

        /// <summary>The port value</summary>
        public const string PortValue = "SMTPPort";

        /// <summary>The host value</summary>
        public const string HostValue = "SMTPMail";

        /// <summary>The user name</summary>
        public const string UserName = "UserName";

        /// <summary>The password</summary>
        public const string Password = "Password";

        /// <summary>The CSV file name</summary>
        public const string CSVFileName = "CSVFileName";

        /// <summary>The subject</summary>
        public const string Subject = "Subject";

        /// <summary>The body</summary>
        public const string Body = "Body";

        /// <summary>From address</summary>
        public const string FromAddress = "FromAddress";

        /// <summary>The CSV file path</summary>
        public const string CSVFilePath = "CSVFilePath";

        /// <summary>The date format</summary>
        public const string DateFormat = "DateFormat";

        /// <summary>The paws API version</summary>
        public const string PawsApiVersion = "PawsApiVersion";

        /// <summary> The setting Key value for Validate WSD Location </summary>
        public const string ValidateWSDLocation = "ValidateWSDLocation";

        /// <summary>The invalid request</summary>
        public const string InvalidRequest = "Invalid Request";

        /// <summary>The invalid time stamp</summary>
        public const string InvalidTimeStamp = "Invalid Timestamp";

        /// <summary>The invalid time stamp</summary>
        public const string InvalidStartDate = "Invalid Starttime";

        /// <summary>The invalid time stamp</summary>
        public const string InvalidEndDate = "Invalid Endtime";

        /// <summary>The incumbent not found</summary>
        public const string IncumbentNotFound = "Used Spectrum not found for this location";

        /// <summary>Identifies starting channel in operating channels allowed by PMSE</summary>       
        public const int PmseStartChannel = 21;

        /// <summary>Identities ending channel in operating channels allowed by PMSE</summary>       
        public const int PmseEndChannel = 60;

        /// <summary>Identifies total number of operating channels allowed by PMSE</summary>
        public const int PmseTotalChannels = 40;

        /// <summary>
        /// Gets the allowed recurrence by day values.
        /// </summary>
        /// <value>The allowed recurrence by day values.</value>
        public static string[] AllowedRecurrenceByDayValues
        {
            get
            {
                return new[] { "SU", "MO", "TU", "WE", "TH", "FR", "SA" };
            }
        }

        /// <summary>
        /// Gets the allowed recurrence frequency values.
        /// </summary>
        /// <value>The allowed recurrence frequency values.</value>
        public static string[] AllowedRecurrenceFrequencyValues
        {
            get
            {
                return new[] { "HOURLY", "WEEKLY", "DAILY" };
            }
        }

        /// <summary>
        /// Gets the allowed recurrence interval values.
        /// </summary>
        /// <value>The allowed recurrence interval values.</value>
        public static string[] AllowedRecurrenceIntervalValues
        {
            get
            {
                return new[] { "UNTIL", "COUNT" };
            }
        }

        /// <summary>
        /// Gets the error message event time recurrence frequency invalid.
        /// </summary>
        /// <value>The error message event time recurrence frequency invalid.</value>
        public static string ErrorMessageEventTimeRecurrenceFrequencyInvalid
        {
            get
            {
                return "only allowed values are " + CombineValues(AllowedRecurrenceFrequencyValues);
            }
        }

        /// <summary>
        /// Gets the error message event time recurrence interval invalid.
        /// </summary>
        /// <value>The error message event time recurrence interval invalid.</value>
        public static string ErrorMessageEventTimeRecurrenceIntervalInvalid
        {
            get
            {
                return "only allowed values are " + CombineValues(AllowedRecurrenceIntervalValues);
            }
        }

        /// <summary>
        /// Gets the error message event time recurrence by day value invalid.
        /// </summary>
        /// <value>The error message event time recurrence by day value invalid.</value>
        public static string ErrorMessageEventTimeRecurrenceByDayValueInvalid
        {
            get
            {
                return "only allowed values are " + CombineValues(AllowedRecurrenceByDayValues);
            }
        }

        /// <summary>
        /// Gets the combine error.
        /// </summary>
        /// <param name="allowedValue">The allowed value.</param>
        /// <returns>returns System.String.</returns>
        private static string CombineValues(string[] allowedValue)
        {
            return allowedValue.Aggregate((a, b) => string.Concat(a, ",", b));
        }
    }
}
