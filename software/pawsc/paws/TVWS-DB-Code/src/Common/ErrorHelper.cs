// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common
{
    using System.Collections.Generic;
    using System.Linq;
    using Entities;

    /// <summary>
    /// Represents ErrorHelper
    /// </summary>
    public static class ErrorHelper
    {
        /// <summary>
        /// Gets the error.
        /// </summary>
        /// <param name="responseType">Type of the response.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>returns Result.</returns>
        public static Result GetError(string responseType, params string[] errorMessages)
        {
            Result errorResult = new Result();
            string errorMessage = errorMessages.FirstOrDefault();

            errorResult.Type = responseType;
            errorResult.Version = "2.0";
            errorResult.Data = errorMessage;

            switch (errorMessage)
            {
                case Constants.ErrorMessageInvalidEmail:
                case Constants.ErrorMessageSerialNumberLength:
                case Constants.ErrorMessageManufacturerIdLength:
                case Constants.ErrorMessageFccIdLength:
                case Constants.ErrorMessageFccIdMinimumLength:
                case Constants.ErrorMessageFccIdAlphabetLength:
                case Constants.ErrorMessageFccIdAlphabetMinimumLength:
                case Constants.ErrorMessageInvalidFccId:
                    {
                        errorResult.Code = "-202";
                        errorResult.Message = "INVALID_VALUE";
                        break;
                    }

                case Constants.ErrorMessageNotRegistered:
                    {
                        errorResult.Code = "-302";
                        errorResult.Message = "NOT_REGISTERED";
                        break;
                    }

                case Constants.ErrorMessageModeNotImplemented:
                    {
                        errorResult.Code = "-103";
                        errorResult.Message = "UNIMPLEMENTED";
                        break;
                    }

                case Constants.ErrorMessageOutsideCoverage:
                    {
                        errorResult.Code = "-104";
                        errorResult.Message = "OUTSIDE_COVERAGE";
                        break;
                    }

                case Constants.ExceptionMessageParametersRequired:
                case Constants.ErrorMessageFccTvbdDeviceTypeRequired:
                case Constants.ErrorMessageInitLocationRequired:
                case Constants.ErrorMessageFccIdRequired:
                case Constants.ErrorMessageSerialNumberRequired:
                case Constants.ErrorMessagePointRequired:
                case Constants.ErrorMessageNotifySpectraRequired:
                case Constants.ErrorMessageOwnerRequired:
                case Constants.ErrorMessageSpectrumSchedulesRequired:
                case Constants.ErrorMessageStartHzRequired:
                case Constants.ErrorMessageStopHzRequired:
                case Constants.ErrorMessageStopTimeRequired:
                case Constants.ErrorMessageStartTimeRequired:
                case Constants.ErrorMessageDeviceDescriptorsRequired:
                case Constants.ErrorMessageDeviceValidityDeviceDescriptorRequired:
                case Constants.ErrorMessageSpectrumRequired:
                case Constants.ErrorMessageFrequencyRangesRequired:
                case Constants.ErrorMessageBandwidthRequired:
                case Constants.ErrorMessageTimeStampRequired:
                case Constants.ErrorMessageInterferenceQueryStartTimeRequired:
                case Constants.ErrorMessageEndTimeRequired:
                case Constants.ErrorMessageRequestorRequired:
                case Constants.ErrorMessageRequestorEmailRequired:
                case Constants.ErrorMessageRequestorOrgRequired:
                case Constants.ErrorMessageLocationRequired:
                case Constants.ErrorMessagEtsiManufacturerId:
                case Constants.ErrorMessagEtsiDeviceType:
                case Constants.ErrorMessagEtsiModelId:
                case Constants.ErrorMessagEtsiDeviceCategory:
                case Constants.ErrorMessageSemiMajorAxis:
                    {
                        errorResult.Code = "-201";
                        errorResult.Message = "MISSING";
                        break;
                    }

                case Constants.ErrorMessageDatabaseUnsupported:
                    {
                        errorResult.Code = "-101";
                        errorResult.Message = "VERSION";
                        break;
                    }

                case Constants.ErrorMessageUnAuthorized:
                case Constants.ErrorMessagDeviceIdNotAuthorized:
                    {
                        errorResult.Code = "-301";
                        errorResult.Message = "UNAUTHORIZED";
                        break;
                    }

                case Constants.ErrorMessageUnavailableSpectrum:
                    {
                        errorResult.Code = "-202";
                        errorResult.Message = "INVALID_VALUE";
                        break;
                    }

                default:
                    {
                        errorResult.Code = "-202";
                        errorResult.Message = "INVALID_VALUE";
                        break;
                    }
            }

            return errorResult;
        }

        /// <summary>
        /// Gets the error.
        /// </summary>
        /// <param name="responseType">Type of the response.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>returns Result.</returns>
        public static Result GetRegionError(string responseType, params string[] errorMessages)
        {
            Result errorResult = new Result();
            string errorMessage = errorMessages.FirstOrDefault();

            errorResult.Type = responseType;
            errorResult.Data = errorMessage;

            switch (errorMessage)
            {
                case Constants.ErrorMessageIncumbentTypeRequired:
                case Constants.ErrorMessageCallSignAndChannelRequired:
                case Constants.ErrorMessageLatitudeAndLongitudeRequired:
                case Constants.ErrorMessageTransmitLocationRequired:
                case Constants.ErrorMessageCallSignRequired:
                case Constants.ErrorMessageChannelRequired:
                case Constants.ErrorMessageLatitudeMissing:
                case Constants.ErrorMessageLongitudeMissing:
                case Constants.ErrorMessageOrgNameMissing:
                case Constants.ErrorMessageRegistrantMissing:
                case Constants.ErrorMessagePointAreaOrQuadAreaRequired:
                case Constants.ErrorMessageEventRequired:
                case Constants.ErrorMessageRegIdRequired:
                case Constants.ErrorMessageRegistrationDispositionRequired:
                case Constants.ErrorMessageTvSpectraRequired:
                case Constants.ErrorMessageLocationRequired:
                case Constants.ErrorMessageLocationsRequired:
                case Constants.ErrorMessageRegionDeviceIdRequired:
                case Constants.ErrorMessageChannelsRequired:
                case Constants.ErrorMessageRegionIdRequired:
                case Constants.ErrorMessageUserIdRequired:
                case Constants.ErrorMessageUserFirstNameRequired:
                case Constants.ErrorMessageUserLastNameRequired:
                case Constants.ErrorMessageCountryRequired:
                case Constants.ErrorMessageAccessLevelRequired:
                case Constants.ErrorMessageCityRequired:
                case Constants.ErrorLPAUXDataMissing:
                case Constants.ErrorMessageDeviceIdRequired:
                case Constants.ErrorMessageFullNameRequired:
                case Constants.ErrorMessagePhoneNumberRequired:
                case Constants.ErrorMessageContactInfoRequired:
                case Constants.ErrorMessageTvSpectrumRequired:
                case Constants.ErrorMessageULSFileNumRequired:
                case Constants.ErrorMessageVenueRequired:
                case Constants.ErrorMessageMVPDLocationRequired:
                case Constants.ErrorMessageParametersRequestTypeRequired:
                case Constants.ErrorMessageParametersUniqueIdRequired:
                case Constants.ErrorMessageParametersDeviceDescriptorRequired:
                case Constants.ErrorMessageParametersDeviceCategoryRequired:
                case Constants.ErrorMessageParametersEtsiDeviceTypeRequired:
                case Constants.ErrorMessageRequiredContourCallSign:
                case Constants.ErrorMessageLpAuxAreaRequired:
                    {
                        errorResult.Code = "-401";
                        errorResult.Message = "REQUIRED";
                        break;
                    }

                case Constants.ErrorMessagePointAreaQuadAreaMutuallyExclusive:
                    {
                        errorResult.Code = "-402";
                        errorResult.Message = "MUTUALLY EXCLUSIVE";
                        break;
                    }

                case Constants.ErrorMessageNumberOfPoints:
                case Constants.ErrorMessageNumberOfVertices:
                case Constants.ErrorMessageLocationsArrayLength:
                case Constants.ErrorMessageEventTimesDifference:
                case Constants.ErrorMessageCallSignNotExist:
                case Constants.ErrorMessageULSFileNumberNotMatch:
                case Constants.ErrorMessageMVPDOutsideValidDistance:
                case Constants.ErrorMessageMVPDInsideContour:
                case Constants.ErrorMessageInvalidPoints:
                case Constants.ErrorMessageWrongIncumbentType:
                case Constants.ErrorMessageULSVenueNameNotMatch:
                case Constants.ErrorMessageLPAUXRegionNotAvailable:
                case Constants.ErrorMessageDeviceAlreadyExcluded:
                case Constants.ErrorMessageDistanceLessThan100:
                case Constants.ErrorMessageInvalidIncumbentType:
                case Constants.ErrorMessageContourCallSignNotFound:
                case Constants.ErrorMessageMVPDCallSignDoNotExist:
                case Constants.ErrorMessageEventTimeRecurrenceIntervalCountInvalid:
                case Constants.ErrorMessageEventTimeRecurrenceIntervalUntilInvalid:
                case Constants.ErrorMessageLpAuxQuadAreaInvalidVertices:
                case Constants.ErrorMessageInvalidStartDate:
                case Constants.ErrorMessageInvalidEndDate:
                case Constants.ErrorMessageEventTimesInvalidDuartion:
                case Constants.ErrorMessageInvalidLocation:
                    {
                        errorResult.Code = "-403";
                        errorResult.Message = "INVALID_VALUE";
                        break;
                    }

                case Constants.ErrorMessageNoData:
                    {
                        errorResult.Code = "-404";
                        errorResult.Message = "NO_DATA";
                        break;
                    }

                default:
                    {
                        errorResult.Code = "-405";
                        errorResult.Message = "Server Error";
                        break;
                    }
            }

            if (errorMessage == Constants.ErrorMessageEventTimeRecurrenceFrequencyInvalid || errorMessage == Constants.ErrorMessageEventTimeRecurrenceIntervalInvalid || errorMessage == Constants.ErrorMessageEventTimeRecurrenceByDayValueInvalid)
            {
                errorResult.Code = "-403";
                errorResult.Message = "INVALID_VALUE";
            }

            return errorResult;
        }

        /// <summary>
        /// Gets the error.
        /// </summary>
        /// <param name="responseType">Type of the response.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>returns Result.</returns>
        public static Result GetError(string responseType, IEnumerable<string> errorMessages)
        {
            return GetError(responseType, errorMessages.ToArray());
        }

        /// <summary>
        /// Creates the error response.
        /// </summary>
        /// <param name="responseType">Type of the response.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>returns PawsResponse.</returns>
        public static PawsResponse CreateErrorResponse(string responseType, params string[] errorMessages)
        {
            return new PawsResponse()
                   {
                       JsonRpc = "2.0",
                       Error = GetError(responseType, errorMessages)
                   };
        }

        /// <summary>
        /// Create the error response.
        /// </summary>
        /// <param name="responseType">Type of the response.</param>
        /// <param name="location">Geo-location details of the request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>PawsResponse object.</returns>
        public static PawsResponse CreateErrorResponse(string responseType, GeoLocation location, params string[] errorMessages)
        {
            PawsResponse response = CreateErrorResponse(responseType, errorMessages);

            // TODO: Take a necessary action based on "location" parameter values. 

            //// Eg: If error occurred because of "OUTSIDE_COVERAGE", then initialize DbUpdateSpec element that provides a list of alternative databases that 
            //// might be appropriate for the requested location. 

            return response;
        }

        /// <summary>
        /// Create the error response for the batch request.
        /// </summary>
        /// <param name="responseType">Type of the response.</param>
        /// <param name="locations">Geo-locations details of the batch request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>PawsResponse object.</returns>
        public static PawsResponse CreateErrorResponse(string responseType, GeoLocation[] locations, params string[] errorMessages)
        {
            PawsResponse response = CreateErrorResponse(responseType, errorMessages);

            // TODO: Take a necessary action based on "locations" parameter values. 

            //// Eg: If error occurred because of "OUTSIDE_COVERAGE", then initialize DbUpdateSpec element that provides a list of alternative databases that 
            //// might be appropriate for the requested location. 

            return response;
        }

        /// <summary>
        /// Creates the error response.
        /// </summary>
        /// <param name="responseType">Type of the response.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>returns RegionManagementResponse.</returns>
        public static RegionManagementResponse CreateRegionErrorResponse(string responseType, params string[] errorMessages)
        {
            return new RegionManagementResponse()
            {
                Error = GetRegionError(responseType, errorMessages)
            };
        }

        /// <summary>
        /// Creates the error response.
        /// </summary>
        /// <param name="responseType">Type of the response.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>returns RegionManagementResponse.</returns>
        public static PMSEResponse CreatePMSEErrorResponse(string responseType, params string[] errorMessages)
        {
            return new PMSEResponse()
            {
                Error = GetRegionError(responseType, errorMessages)
            };
        }

        /// <summary>
        /// Creates the exception response.
        /// </summary>
        /// <param name="responseType">Type of the response.</param>
        /// <param name="errorMessage">The error message.</param>
        /// <returns>returns PawsResponse.</returns>
        public static PawsResponse CreateExceptionResponse(string responseType, string errorMessage)
        {
            return new PawsResponse()
            {
                JsonRpc = "2.0",
                Error = new Result()
                        {
                            Code = "-201",
                            Type = responseType,
                            Data = errorMessage,
                        }
            };
        }

        /// <summary>
        /// Creates the exception response.
        /// </summary>
        /// <param name="responseType">Type of the response.</param>
        /// <param name="errorMessage">The error message.</param>
        /// <returns>returns RegionManagementResponse.</returns>
        public static RegionManagementResponse CreateRegionExceptionResponse(string responseType, string errorMessage)
        {
            return new RegionManagementResponse()
            {
                Error = new Result()
                {
                    Type = responseType,
                    Data = errorMessage,
                }
            };
        }
    }
}
