// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common.Validators
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Linq;
    using System.Text.RegularExpressions;
    using Entities;
    using Microsoft.Whitespace.Common.Utilities;
    using Microsoft.WindowsAzure.Storage.Table;
    using System.Net.Http;
    using System.Text;
    using System.Xml;
    using System.IO;

    /// <summary>
    /// Class FCC RegionManagementValidator.
    /// </summary>
    public class FCCRegionManagementValidator : BaseRegionManagementValidator
    {
        /// <summary>
        /// Validates the Add Incumbents request.
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        public override bool ValidateAddIncumbentRequestMVPD(Parameters parameters, out List<string> errorMessages)
        {
            var valResult = base.ValidateAddIncumbentRequestMVPD(parameters, out errorMessages);
            if (valResult)
            {
                if (!this.TvSpectrumMVPD(parameters, out errorMessages))
                {
                    return false;
                }

                if (!this.MVPDLocation(parameters, out errorMessages))
                {
                    return false;
                }

                if (!this.TransmitLocation(parameters, out errorMessages))
                {
                    return false;
                }

                if (!this.Registrant(parameters, out errorMessages))
                {
                    return false;
                }
            }

            return valResult;
        }

        /// <summary>
        /// Validates the Add Incumbents request.
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        public override bool ValidateAddIncumbentRequestTBAS(Parameters parameters, out List<string> errorMessages)
        {
            var valResult = base.ValidateAddIncumbentRequestTBAS(parameters, out errorMessages);
            if (valResult)
            {
                if (!this.TvSpectrum(parameters, out errorMessages))
                {
                    return false;
                }

                if (!this.TempBasLocation(parameters, out errorMessages))
                {
                    return false;
                }

                if (!this.TransmitLocation(parameters, out errorMessages))
                {
                    return false;
                }

                if (!this.EventTBAS(parameters, out errorMessages))
                {
                    return false;
                }

                if (parameters.TvSpectrum.CallSign == null)
                {
                    errorMessages.Add(Constants.ErrorMessageCallSignRequired);
                    return false;
                }

                var serviceCacheRequestParameter = new ServiceCacheRequestParameters()
                                                   {
                                                       VsdService = "DT"
                                                   };
                var callSigns = DatabaseCache.ServiceCacheHelper.SearchCacheObjects(ServiceCacheObjectType.TvEngData, SearchCacheRequestType.ByVsdService, serviceCacheRequestParameter) as IEnumerable<CacheObjectTvEngdata>;

                if (callSigns.FirstOrDefault(obj => obj.CallSign == parameters.TvSpectrum.CallSign) == null)
                {
                    errorMessages.Add(Constants.ErrorMessageCallSignNotExist);
                    return false;
                }             

                if (GeoCalculations.GetDistance(parameters.TransmitLocation, parameters.TempBasLocation).InMeter() < 100)
                {
                    errorMessages.Add(Constants.ErrorMessageDistanceLessThan100);
                    return false;
                }
            }

            return valResult;
        }

        /// <summary>
        /// Validates the Add Incumbents request.
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        public override bool ValidateAddIncumbentRequestLPAux(Parameters parameters, out List<string> errorMessages)
        {
            var valResult = base.ValidateAddIncumbentRequestLPAux(parameters, out errorMessages);
            if (valResult)
            {
                if (!this.TvSpectrumLicensedLPAUX(parameters, out errorMessages))
                {
                    return false;
                }

                if (!this.LicensedLpauxRegistrant(parameters, out errorMessages))
                {
                    return false;
                }

                if (!this.PointAreaQuadArea(parameters, out errorMessages))
                {
                    return false;
                }

                if (!this.EventLicensedLpAux(parameters, out errorMessages))
                {
                    return false;
                }

                //if (string.IsNullOrEmpty(parameters.Venue))
                //{
                //    errorMessages.Add(Constants.ErrorMessageVenueRequired);
                //    return false;
                //}
            }

            return valResult;
        }

        /// <summary>
        /// Validates the add incumbent request unlicensed LPAUX.
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        public override bool ValidateRegisterDeviceRequestUnlicensedLPAux(Parameters parameters, out List<string> errorMessages)
        {
            var valResult = base.ValidateRegisterDeviceRequestUnlicensedLPAux(parameters, out errorMessages);
            if (valResult)
            {
                if (!this.TvSpectrum(parameters, out errorMessages))
                {
                    return false;
                }

                if (!this.PointAreaQuadArea(parameters, out errorMessages))
                {
                    return false;
                }

                if (!this.EventUnlicensedLPAux(parameters, out errorMessages))
                {
                    return false;
                }

                if (!this.RegistrantUnlicensedLPAux(parameters, out errorMessages))
                {
                    return false;
                }

                if (parameters.Contact == null || parameters.Contact.Address == null)
                {
                    errorMessages.Add(Constants.ErrorMessageContactInfoRequired);
                    return false;
                }

                if (string.IsNullOrEmpty(parameters.Venue))
                {
                    errorMessages.Add(Constants.ErrorMessageVenueRequired);
                    return false;
                }

                if (string.IsNullOrEmpty(parameters.ULSFileNumber))
                {
                    errorMessages.Add(Constants.ErrorMessageULSFileNumRequired);
                    return false;
                }
            }

            return valResult;
        }

        /// <summary>
        /// Validates the Delete Incumbents request.
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        public override bool ValidateDeleteIncumbentRequest(Parameters parameters, out List<string> errorMessages)
        {
            var valResult = base.ValidateDeleteIncumbentRequest(parameters, out errorMessages);
            if (valResult)
            {
                if (!this.RegistrationDisposition(parameters, out errorMessages))
                {
                    return false;
                }
            }

            return valResult;
        }

        /// <summary>
        /// Validates the Get Incumbents request.
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        public override bool ValidateRequiredIncumbentType(Parameters parameters, out List<string> errorMessages)
        {
            var valResult = base.ValidateRequiredIncumbentType(parameters, out errorMessages);
            if (valResult)
            {
                if (!this.IncumbentType(parameters, out errorMessages))
                {
                    return false;
                }
            }

            return valResult;
        }

        /// <summary>
        /// Validates the Exclude Channels request.
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        public override bool ValidateExcludeChannels(Parameters parameters, out List<string> errorMessages)
        {
            var valResult = base.ValidateExcludeChannels(parameters, out errorMessages);
            if (valResult)
            {
                if (!this.TvSpectra(parameters, out errorMessages))
                {
                    return false;
                }

                if (!this.Locations(parameters, out errorMessages))
                {
                    return false;
                }
            }

            return valResult;
        }

        /// <summary>
        /// Validates the Exclude Ids request.
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        public override bool ValidateExcludeIds(Parameters parameters, out List<string> errorMessages)
        {
            var valResult = base.ValidateExcludeIds(parameters, out errorMessages);
            errorMessages = new List<string>();

            if (valResult)
            {
                if (!this.DeviceId(parameters, out errorMessages))
                {
                    return false;
                }

                var deviceId = this.CommonDalc.FetchEntity<DynamicTableEntity>(Utils.GetRegionalTableName(Constants.ExcludedIds), new { DeviceId = parameters.DeviceId });
                if (deviceId != null && deviceId.Count > 0)
                {
                    errorMessages.Add(Constants.ErrorMessageDeviceAlreadyExcluded);
                    return false;
                }

                if (!string.IsNullOrEmpty(parameters.SerialNumber))
                {
                    var device = this.CommonDalc.FetchEntity<DynamicTableEntity>(Utils.GetRegionalTableName(Constants.ExcludedIds), new { SerialNumber = parameters.SerialNumber, DeviceId = parameters.DeviceId });
                    if (device != null && device.Count > 0)
                    {
                        errorMessages.Add(Constants.ErrorMessageDeviceAlreadyExcluded);
                        return false;
                    }
                }
            }

            return valResult;
        }

        /// <summary>
        /// Validates the Get Device List request.
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        public override bool ValidateGetDeviceList(Parameters parameters, out List<string> errorMessages)
        {
            if (!this.ValidateRequiredIncumbentType(parameters, out errorMessages))
            {
                return false;
            }

            errorMessages = new List<string>();

            Entities.IncumbentType incumbentType = Conversion.ToIncumbentType(parameters.IncumbentType);
            if (incumbentType == Entities.IncumbentType.None)
            {
                errorMessages.Add(Constants.ErrorMessageInvalidIncumbentType);
                return false;
            }

            if (!(incumbentType == Entities.IncumbentType.Fixed || incumbentType == Entities.IncumbentType.Mode_1 || incumbentType == Entities.IncumbentType.Mode_2))
            {
                errorMessages.Add(Constants.ErrorMessageInvalidIncumbentType);
                return false;
            }

            if (!this.LocationDeviceList(parameters, out errorMessages))
            {
                return false;
            }

            return errorMessages.Count == 0;
        }

        /// <summary>
        /// Validates the Get Device List request.
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        public override bool ValidateGetChannelListRequest(Parameters parameters, out List<string> errorMessages)
        {
            if (!this.ValidateRequiredIncumbentType(parameters, out errorMessages))
            {
                return false;
            }

            errorMessages = new List<string>();

            Entities.IncumbentType incumbentType = Conversion.ToIncumbentType(parameters.IncumbentType);
            if (incumbentType == Entities.IncumbentType.None)
            {
                errorMessages.Add(Constants.ErrorMessageInvalidIncumbentType);
                return false;
            }

            var allowedIncumbentTypes = new[] { Entities.IncumbentType.Fixed, Entities.IncumbentType.Mode_1, Entities.IncumbentType.Mode_2, Entities.IncumbentType.LPAux, Entities.IncumbentType.TBAS, Entities.IncumbentType.UnlicensedLPAux };

            if (!allowedIncumbentTypes.Contains(incumbentType))
            {
                errorMessages.Add(Constants.ErrorMessageInvalidIncumbentType);
                return false;
            }

            if (incumbentType == Entities.IncumbentType.LPAux || incumbentType == Entities.IncumbentType.UnlicensedLPAux)
            {
                if (parameters.QuadrilateralArea == null && parameters.PointsArea == null)
                {
                    errorMessages.Add(Constants.ErrorMessageLpAuxAreaRequired);
                    return false;
                }

                if (parameters.QuadrilateralArea != null)
                {
                    foreach (var quadrilateralArea in parameters.QuadrilateralArea)
                    {
                        if (!this.AreQuadrilateralVerticesValid(quadrilateralArea))
                        {
                            errorMessages.Add(Constants.ErrorMessageLpAuxQuadAreaInvalidVertices);
                            return false;
                        }
                    }
                }
            }
            else
            {
                if (!this.Location(parameters, out errorMessages))
                {
                    return false;
                }
            }

            return errorMessages.Count == 0;
        }

        /// <summary>
        /// Validates the specified object.
        /// </summary>
        /// <param name="parameters">The request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>Validator Result.</returns>
        public bool TvSpectrumMVPD(Parameters parameters, out List<string> errorMessages)
        {
            errorMessages = new List<string>();

            if (parameters.TvSpectrum == null)
            {
                errorMessages.Add(Constants.ErrorMessageTvSpectrumRequired);
            }
            else
            {
                if (parameters.TvSpectrum.CallSign == null)
                {
                    errorMessages.Add(Constants.ErrorMessageCallSignRequired);
                }
                else if (parameters.TvSpectrum.Channel == 0)
                {
                    errorMessages.Add(Constants.ErrorMessageChannelRequired);
                }
            }

            return errorMessages.Count == 0;
        }

        /// <summary>
        /// Validates the specified object.
        /// </summary>
        /// <param name="parameters">The request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>Validator Result.</returns>
        public bool TvSpectrumLicensedLPAUX(Parameters parameters, out List<string> errorMessages)
        {
            errorMessages = new List<string>();

            if (parameters.TvSpectrum == null)
            {
                errorMessages.Add(Constants.ErrorMessageTvSpectrumRequired);
            }
            else
            {
                if (parameters.TvSpectrum.CallSign == null)
                {
                    errorMessages.Add(Constants.ErrorMessageCallSignRequired);
                }
                else if (parameters.TvSpectrum.Channel == 0)
                {
                    errorMessages.Add(Constants.ErrorMessageChannelRequired);
                }
            }

            return errorMessages.Count == 0;
        }

        /// <summary>
        /// Validates the specified object.
        /// </summary>
        /// <param name="parameters">The request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>Validator Result.</returns>
        public bool TvSpectrum(Parameters parameters, out List<string> errorMessages)
        {
            errorMessages = new List<string>();

            if (parameters.TvSpectrum == null)
            {
                errorMessages.Add(Constants.ErrorMessageTvSpectrumRequired);
            }

            return errorMessages.Count == 0;
        }

        /// <summary>
        /// Validates the specified object.
        /// </summary>
        /// <param name="parameters">The request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>Validator Result.</returns>
        public bool TvSpectra(Parameters parameters, out List<string> errorMessages)
        {
            errorMessages = new List<string>();

            if (parameters == null || parameters.TvSpectra == null)
            {
                errorMessages.Add(Constants.ErrorMessageTvSpectraRequired);
            }

            return errorMessages.Count == 0;
        }

        /// <summary>
        /// Validates the specified object.
        /// </summary>
        /// <param name="parameters">The request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>Validator Result.</returns>
        public bool Locations(Parameters parameters, out List<string> errorMessages)
        {
            errorMessages = new List<string>();

            if (parameters.Locations == null)
            {
                errorMessages.Add(Constants.ErrorMessageLocationsRequired);
                return false;
            }

            for (int i = 0; i < parameters.Locations.Length; i++)
            {
                var location = parameters.Locations[i].ToLocation();
                if (location.Latitude == 0.0 || location.Longitude == 0.0)
                {
                    errorMessages.Add(Constants.ErrorMessageInvalidLocation);
                    return false;
                }
            }

            return errorMessages.Count == 0;
        }

        /// <summary>
        /// Validates the specified object.
        /// </summary>
        /// <param name="parameters">The request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>Validator Result.</returns>
        public bool Location(Parameters parameters, out List<string> errorMessages)
        {
            errorMessages = new List<string>();

            if (parameters.Location == null)
            {
                errorMessages.Add(Constants.ErrorMessageLocationRequired);
                return false;
            }
            else if (parameters.Location.Point != null && parameters.Location.Point.Center != null)
            {
                if ((parameters.Location.Point.Center.Latitude == null) || (parameters.Location.Point.Center.Latitude == string.Empty))
                {
                    errorMessages.Add(Constants.ErrorMessageLatitudeMissing);
                }
                else if ((parameters.Location.Point.Center.Longitude == null) || (parameters.Location.Point.Center.Longitude == string.Empty))
                {
                    errorMessages.Add(Constants.ErrorMessageLongitudeMissing);
                }
            }

            var location = parameters.Location.ToLocation();
            if (location.Latitude == 0.0 || location.Longitude == 0.0)
            {
                errorMessages.Add(Constants.ErrorMessageInvalidLocation);
                return false;
            }

            return errorMessages.Count == 0;
        }

        /// <summary>
        /// Validates the specified object.
        /// </summary>
        /// <param name="parameters">The request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>Validator Result.</returns>
        public bool LocationDeviceList(Parameters parameters, out List<string> errorMessages)
        {
            errorMessages = new List<string>();

            if (parameters.Location == null)
            {
                errorMessages.Add(Constants.ErrorMessageLocationRequired);
            }

            return errorMessages.Count == 0;
        }

        /// <summary>
        /// Validates the specified object.
        /// </summary>
        /// <param name="parameters">The request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>Validator Result.</returns>
        public bool IncumbentType(Parameters parameters, out List<string> errorMessages)
        {
            errorMessages = new List<string>();

            if (parameters.IncumbentType == null || parameters.IncumbentType == string.Empty)
            {
                errorMessages.Add(Constants.ErrorMessageIncumbentTypeRequired);
            }

            return errorMessages.Count == 0;
        }

        /// <summary>
        /// Validates the specified object.
        /// </summary>
        /// <param name="parameters">The request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>Validator Result.</returns>
        public bool MVPDLocation(Parameters parameters, out List<string> errorMessages)
        {
            errorMessages = new List<string>();

            if (parameters.MVPDLocation == null)
            {
                errorMessages.Add(Constants.ErrorMessageLatitudeAndLongitudeRequired);
            }
            else if (parameters.MVPDLocation != null && parameters.MVPDLocation.Latitude.CompareTo(0.0) == 0)
            {
                errorMessages.Add(Constants.ErrorMessageLatitudeMissing);
            }
            else if (parameters.MVPDLocation != null && parameters.MVPDLocation.Longitude.CompareTo(0.0) == 0)
            {
                errorMessages.Add(Constants.ErrorMessageLongitudeMissing);
            }
            else if(!this.ValidateLocation(parameters.MVPDLocation))
            {
                errorMessages.Add(Constants.ErrorMessageInvalidMvpdLocation);
            }

            return errorMessages.Count == 0;
        }

        /// <summary>
        /// Validates the specified object.
        /// </summary>
        /// <param name="parameters">The request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>Validator Result.</returns>
        public bool TransmitLocation(Parameters parameters, out List<string> errorMessages)
        {
            errorMessages = new List<string>();

            if (parameters.TransmitLocation == null)
            {
                errorMessages.Add(Constants.ErrorMessageTransmitLocationRequired);
            }
            else if (parameters.TransmitLocation != null && parameters.TransmitLocation.Latitude.CompareTo(0.0) == 0)
            {
                errorMessages.Add(Constants.ErrorMessageLatitudeMissing);
            }
            else if (parameters.TransmitLocation != null && parameters.TransmitLocation.Longitude.CompareTo(0.0) == 0)
            {
                errorMessages.Add(Constants.ErrorMessageLongitudeMissing);
            }
            else if (!this.ValidateLocation(parameters.TransmitLocation))
            {
                errorMessages.Add(Constants.ErrorMessageInvalidTransmitLocation);
            }
            
            return errorMessages.Count == 0;
        }

        /// <summary>
        /// Validates the specified object.
        /// </summary>
        /// <param name="parameters">The request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>Validator Result.</returns>
        public bool TempBasLocation(Parameters parameters, out List<string> errorMessages)
        {
            errorMessages = new List<string>();

            if (parameters.TempBasLocation == null)
            {
                errorMessages.Add(Constants.ErrorMessageLatitudeAndLongitudeRequired);
            }
            else if (parameters.TempBasLocation != null && parameters.TempBasLocation.Latitude == 0)
            {
                errorMessages.Add(Constants.ErrorMessageLatitudeMissing);
            }
            else if (parameters.TempBasLocation != null && parameters.TempBasLocation.Longitude == 0)
            {
                errorMessages.Add(Constants.ErrorMessageLongitudeMissing);
            }
            else if (!this.ValidateLocation(parameters.TempBasLocation))
            {
                errorMessages.Add(Constants.ErrorMessageInvalidTempBasLocation);
            }

            return errorMessages.Count == 0;
        }

        /// <summary>
        /// Validates the specified object.
        /// </summary>
        /// <param name="parameters">The request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>Validator Result.</returns>
        public bool LicensedLpauxRegistrant(Parameters parameters, out List<string> errorMessages)
        {
            errorMessages = new List<string>();

            if (parameters.LPAuxRegistrant == null)
            {
                errorMessages.Add(Constants.ErrorMessageRegistrantMissing);
            }
            else
            {
                ////if (parameters.LPAuxRegistrant.Org == null)
                ////{
                ////    errorMessages.Add(Constants.ErrorMessageOrgNameMissing);
                ////}
                if (parameters.LPAuxRegistrant.Org != null && parameters.LPAuxRegistrant.Org.OrganizationName == null)
                {
                    errorMessages.Add(Constants.ErrorMessageOrgNameMissing);
                }
                else if (parameters.LPAuxRegistrant.Address == null)
                {
                    errorMessages.Add(Constants.ErrorMessageContactAddressMissing);
                }
            }

            return errorMessages.Count == 0;
        }

        /// <summary>
        /// Validates the specified object.
        /// </summary>
        /// <param name="parameters">The request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>Validator Result.</returns>
        public bool Registrant(Parameters parameters, out List<string> errorMessages)
        {
            errorMessages = new List<string>();

            if (parameters.MVPDRegistrant == null)
            {
                errorMessages.Add(Constants.ErrorMessageRegistrantMissing);
            }
            else
            {
                if (parameters.MVPDRegistrant.Org == null)
                {
                    errorMessages.Add(Constants.ErrorMessageOrgNameMissing);
                }
                else if (parameters.MVPDRegistrant.Org.OrganizationName == null)
                {
                    errorMessages.Add(Constants.ErrorMessageOrgNameMissing);
                }
                else if (parameters.MVPDRegistrant.Address == null)
                {
                    errorMessages.Add(Constants.ErrorMessageContactAddressMissing);
                }
            }

            return errorMessages.Count == 0;
        }

        /// <summary>
        /// Registrants the unlicensed LPAUX.
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        public bool RegistrantUnlicensedLPAux(Parameters parameters, out List<string> errorMessages)
        {
            errorMessages = new List<string>();

            if (parameters.LPAuxRegistrant == null)
            {
                errorMessages.Add(Constants.ErrorMessageRegistrantMissing);
                return false;
            }
            else if (parameters.LPAuxRegistrant.Org == null || parameters.LPAuxRegistrant.Org.OrganizationName == null)
            {
                errorMessages.Add(Constants.ErrorMessageOrgNameMissing);
            }
            else if (parameters.LPAuxRegistrant.Name == null || parameters.LPAuxRegistrant.Name.ContactName == null)
            {
                errorMessages.Add(Constants.ErrorMessageFullNameRequired);
            }
            else if (parameters.LPAuxRegistrant.Telephone == null || parameters.LPAuxRegistrant.Telephone.Length == 0)
            {
                errorMessages.Add(Constants.ErrorMessagePhoneNumberRequired);
            }

            return errorMessages.Count == 0;
        }

        /// <summary>
        /// Validates the specified object.
        /// </summary>
        /// <param name="parameters">The request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>Validator Result.</returns>
        public bool EventLicensedLpAux(Parameters parameters, out List<string> errorMessages)
        {
            errorMessages = new List<string>();

            if (parameters.Event == null)
            {
                errorMessages.Add(Constants.ErrorMessageEventRequired);
            }
            else if (parameters.Event.Times == null)
            {
                errorMessages.Add(Constants.ErrorMessageEventTimesRequired);
            }
            else if (parameters.Event.Channels == null)
            {
                errorMessages.Add(Constants.ErrorMessageChannelsRequired);
            }
            else if (parameters.Event.Channels != null && parameters.Event.Channels.Length == 0)
            {
                errorMessages.Add(Constants.ErrorMessageChannelsRequired);
            }
            else if (parameters.Event.Times != null)
            {
                for (int k = 0; k < parameters.Event.Times.Length; k++)
                {
                    if (parameters.Event.Times[k].Stamp == null)
                    {
                        errorMessages.Add(Constants.ErrorMessageEventTimesStampRequired);
                    }
                    else if (parameters.Event.Times[k].Start == null)
                    {
                        errorMessages.Add(Constants.ErrorMessageEventTimesStartRequired);
                        continue;
                    }
                    else if (parameters.Event.Times[k].End == null)
                    {
                        errorMessages.Add(Constants.ErrorMessageEventTimesEndRequired);
                    }

                    if (parameters.Event.Times[k].Recurrence != null)
                    {
                        var recurrence = parameters.Event.Times[k].Recurrence;
                        if (string.IsNullOrWhiteSpace(recurrence.Frequency))
                        {
                            errorMessages.Add(Constants.ErrorMessageEventTimeRecurrenceFrequencyRequired);
                        }
                        else if (!Constants.AllowedRecurrenceFrequencyValues.Contains(recurrence.Frequency.ToUpper()))
                        {
                            errorMessages.Add(Constants.ErrorMessageEventTimeRecurrenceFrequencyInvalid);
                        }

                        if (!string.IsNullOrEmpty(recurrence.Interval) && !Constants.AllowedRecurrenceIntervalValues.Contains(recurrence.Interval.ToUpper()))
                        {
                            errorMessages.Add(Constants.ErrorMessageEventTimeRecurrenceIntervalInvalid);
                        }
                        else if (recurrence.Interval.ToUpper() == "COUNT" && (recurrence.Count <= 0 || recurrence.Count >= 10))
                        {
                            errorMessages.Add(Constants.ErrorMessageEventTimeRecurrenceIntervalCountInvalid);
                        }
                        else if (recurrence.Interval.ToUpper() == "UNTIL" && recurrence.Until.ToDateTime() <= parameters.Event.Times[k].Start.ToDateTime())
                        {
                            errorMessages.Add(Constants.ErrorMessageEventTimeRecurrenceIntervalUntilInvalid);
                        }

                        if (!string.IsNullOrEmpty(recurrence.ByDay) && !Constants.AllowedRecurrenceByDayValues.Contains(recurrence.ByDay.ToUpper()))
                        {
                            errorMessages.Add(Constants.ErrorMessageEventTimeRecurrenceByDayValueInvalid);
                        }
                    }
                }

                var totalTime = parameters.Event.Times.Select(obj => obj.End.ToDateTime() - obj.Start.ToDateTime());
                if (totalTime.Sum(obj => obj.TotalHours) > 720)
                {
                    errorMessages.Add(Constants.ErrorMessageEventTimesDifference);
                }
            }

            return errorMessages.Count == 0;
        }

        /// <summary>
        /// Events the unlicensed LPAUX.
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        public bool EventUnlicensedLPAux(Parameters parameters, out List<string> errorMessages)
        {
            errorMessages = new List<string>();

            if (parameters.Event == null)
            {
                errorMessages.Add(Constants.ErrorMessageEventRequired);
            }
            else if (parameters.Event.Times == null || parameters.Event.Times.Length == 0)
            {
                errorMessages.Add(Constants.ErrorMessageEventTimesRequired);
            }
            else if (parameters.Event.Channels == null)
            {
                errorMessages.Add(Constants.ErrorMessageChannelsRequired);
            }
            else if (parameters.Event.Channels != null && parameters.Event.Channels.Length == 0)
            {
                errorMessages.Add(Constants.ErrorMessageChannelsRequired);
            }
            else if (parameters.Event.Times != null)
            {
                for (int k = 0; k < parameters.Event.Times.Length; k++)
                {
                    if (parameters.Event.Times[k].Stamp == null)
                    {
                        errorMessages.Add(Constants.ErrorMessageEventTimesStampRequired);
                    }
                    else if (parameters.Event.Times[k].Start == null)
                    {
                        errorMessages.Add(Constants.ErrorMessageEventTimesStartRequired);
                    }
                    else if (parameters.Event.Times[k].End == null)
                    {
                        errorMessages.Add(Constants.ErrorMessageEventTimesEndRequired);
                    }
                    else if (parameters.Event.Times[k].End != null && parameters.Event.Times[k].Start != null)
                    {
                        if ((parameters.Event.Times[k].End.ToDateTime() - parameters.Event.Times[k].Start.ToDateTime()).TotalDays > 365)
                        {
                            errorMessages.Add(Constants.ErrorMessageEventTimesInvalidDuartion);
                        }

                        if (parameters.Event.Times[k].End.ToDateTime() <= parameters.Event.Times[k].Start.ToDateTime())
                        {
                            errorMessages.Add(Constants.ErrorMessageInvalidEndDate);
                        }
                    }

                    if (parameters.Event.Times[k].Recurrence != null)
                    {
                        var recurrence = parameters.Event.Times[k].Recurrence;
                        if (string.IsNullOrWhiteSpace(recurrence.Frequency))
                        {
                            errorMessages.Add(Constants.ErrorMessageEventTimeRecurrenceFrequencyRequired);
                        }
                        else if (!Constants.AllowedRecurrenceFrequencyValues.Contains(recurrence.Frequency.ToUpper()))
                        {
                            errorMessages.Add(Constants.ErrorMessageEventTimeRecurrenceFrequencyInvalid);
                        }

                        if (!string.IsNullOrEmpty(recurrence.Interval) && !Constants.AllowedRecurrenceIntervalValues.Contains(recurrence.Interval.ToUpper()))
                        {
                            errorMessages.Add(Constants.ErrorMessageEventTimeRecurrenceIntervalInvalid);
                        }
                        else if (recurrence.Interval.ToUpper() == "COUNT" && (recurrence.Count <= 0 || recurrence.Count >= 10))
                        {
                            errorMessages.Add(Constants.ErrorMessageEventTimeRecurrenceIntervalCountInvalid);
                        }
                        else if (recurrence.Interval.ToUpper() == "UNTIL" && recurrence.Until.ToDateTime() <= parameters.Event.Times[k].Start.ToDateTime())
                        {
                            errorMessages.Add(Constants.ErrorMessageEventTimeRecurrenceIntervalUntilInvalid);
                        }

                        if (!string.IsNullOrEmpty(recurrence.ByDay) && !Constants.AllowedRecurrenceByDayValues.Contains(recurrence.ByDay.ToUpper()))
                        {
                            errorMessages.Add(Constants.ErrorMessageEventTimeRecurrenceByDayValueInvalid);
                        }
                    }
                }
            }

            return errorMessages.Count == 0;
        }

        /// <summary>
        /// Validates the specified object.
        /// </summary>
        /// <param name="parameters">The request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>Validator Result.</returns>
        public bool EventTBAS(Parameters parameters, out List<string> errorMessages)
        {
            errorMessages = new List<string>();

            if (parameters.Event == null)
            {
                errorMessages.Add(Constants.ErrorMessageEventRequired);
            }
            else if (parameters.Event.Times == null)
            {
                errorMessages.Add(Constants.ErrorMessageEventTimesRequired);
            }
            else if (parameters.Event.Channels == null)
            {
                errorMessages.Add(Constants.ErrorMessageChannelsRequired);
            }
            else if (parameters.Event.Channels != null && parameters.Event.Channels.Length == 0)
            {
                errorMessages.Add(Constants.ErrorMessageChannelsRequired);
            }
            else if (parameters.Event.Times != null)
            {
                for (int k = 0; k < parameters.Event.Times.Length; k++)
                {
                    if (parameters.Event.Times[k].Stamp == null)
                    {
                        errorMessages.Add(Constants.ErrorMessageEventTimesStampRequired);
                    }
                    else if (parameters.Event.Times[k].Start == null)
                    {
                        errorMessages.Add(Constants.ErrorMessageEventTimesStartRequired);
                    }
                    else if (parameters.Event.Times[k].End == null)
                    {
                        errorMessages.Add(Constants.ErrorMessageEventTimesEndRequired);
                    }
                    else if ((parameters.Event.Times[k].Start != null) && (parameters.Event.Times[k].End != null))
                    {
                        DateTime endDate = DateTime.Parse(parameters.Event.Times[k].End.Trim(), CultureInfo.InvariantCulture);
                        DateTime endStart = DateTime.Parse(parameters.Event.Times[k].Start.Trim(), CultureInfo.InvariantCulture);
                        var hours = (endDate - endStart).TotalHours;
                        if (hours > 720)
                        {
                            errorMessages.Add(Constants.ErrorMessageEventTimesDifference);
                        }
                    }
                }
            }

            return errorMessages.Count == 0;
        }

        /// <summary>
        /// Validates the specified object.
        /// </summary>
        /// <param name="parameters">The request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>Validator Result.</returns>
        public bool PointAreaQuadArea(Parameters parameters, out List<string> errorMessages)
        {
            errorMessages = new List<string>();

            if ((parameters.PointsArea == null || parameters.PointsArea.Length == 0) && (parameters.QuadrilateralArea == null || parameters.QuadrilateralArea.Length == 0))
            {
                errorMessages.Add(Constants.ErrorMessagePointAreaOrQuadAreaRequired);
            }
            else if (parameters.PointsArea != null && parameters.QuadrilateralArea != null)
            {
                errorMessages.Add(Constants.ErrorMessagePointAreaQuadAreaMutuallyExclusive);
            }
            else if (parameters.PointsArea != null && parameters.PointsArea.Length > 25)
            {
                errorMessages.Add(Constants.ErrorMessageNumberOfPoints);
            }
            else if (parameters.QuadrilateralArea != null && parameters.QuadrilateralArea.Length > 4)
            {
                errorMessages.Add(Constants.ErrorMessageNumberOfVertices);
            }
            else if (parameters.QuadrilateralArea != null)
            {
                foreach (var quadrilateralArea in parameters.QuadrilateralArea)
                {
                    if (!this.AreQuadrilateralVerticesValid(quadrilateralArea))
                    {
                        errorMessages.Add(Constants.ErrorMessageLpAuxQuadAreaInvalidVertices);
                        return false;
                    }
                }
            }

            return errorMessages.Count == 0;
        }

        /// <summary>
        /// Validates the specified object.
        /// </summary>
        /// <param name="parameters">The request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>Validator Result.</returns>
        public bool RegistrationDisposition(Parameters parameters, out List<string> errorMessages)
        {
            errorMessages = new List<string>();

            if (parameters.RegistrationDisposition == null)
            {
                errorMessages.Add(Constants.ErrorMessageRegistrationDispositionRequired);
            }
            else if (parameters.RegistrationDisposition.RegId == null)
            {
                errorMessages.Add(Constants.ErrorMessageRegIdRequired);
            }

            return errorMessages.Count == 0;
        }

        /// <summary>
        /// Validates the specified object.
        /// </summary>
        /// <param name="parameters">The request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>Validator Result.</returns>
        public bool DeviceId(Parameters parameters, out List<string> errorMessages)
        {
            errorMessages = new List<string>();

            if (parameters == null || parameters.DeviceId == null || parameters.DeviceId == string.Empty)
            {
                errorMessages.Add(Constants.ErrorMessageDeviceIdRequired);
            }

            return errorMessages.Count == 0;
        }

        /// <summary>
        /// Validates the get incumbents request.
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        public override bool ValidateGetIncumbentsRequest(Parameters parameters, out List<string> errorMessages)
        {
            if (!this.ValidateRequiredIncumbentType(parameters, out errorMessages))
            {
                return false;
            }

            errorMessages = new List<string>();

            Entities.IncumbentType incumbentType = Conversion.ToIncumbentType(parameters.IncumbentType);
            if (incumbentType == Entities.IncumbentType.None)
            {
                errorMessages.Add(Constants.ErrorMessageInvalidIncumbentType);
                return false;
            }

            if (incumbentType != Entities.IncumbentType.Fixed || incumbentType != Entities.IncumbentType.Mode_1 || incumbentType != Entities.IncumbentType.Mode_2)
            {
                errorMessages.Add(Constants.ErrorMessageInvalidIncumbentType);
                return false;
            }

            return errorMessages.Count == 0;
        }

        /// <summary>
        /// Validates the get contour data request.
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        public override bool ValidateGetContourDataRequest(Parameters parameters, out List<string> errorMessages)
        {
            errorMessages = new List<string>();
            if (parameters == null || string.IsNullOrEmpty(parameters.ContourRequestCallSign))
            {
                errorMessages.Add(Constants.ErrorMessageRequiredContourCallSign);
                return false;
            }

            return errorMessages.Count == 0;
        }

        /// <summary>
        /// Ares the quadrilateral vertices valid.
        /// </summary>
        /// <param name="quadrilateralArea">The quadrilateral area.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        private bool AreQuadrilateralVerticesValid(QuadrilateralArea quadrilateralArea)
        {
            List<Location> locations = new List<Location>();
            locations.AddRange(new[] { quadrilateralArea.NEPoint.ToLocation(), quadrilateralArea.NWPoint.ToLocation(), quadrilateralArea.SEPoint.ToLocation(), quadrilateralArea.SWPoint.ToLocation(), quadrilateralArea.NEPoint.ToLocation() });
            for (int locationIndex = 0; locationIndex < locations.Count - 1; locationIndex++)
            {
                var distance = GeoCalculations.GetDistance(locations[locationIndex], locations[locationIndex + 1]);
                if (distance.InKm() > 3)
                {
                    return false;
                }
            }

            return true;
        }

        /// <summary>
        /// Validates whether given location is falls within United States or not 
        /// </summary>
        /// <param name="location">location coordinates</param>
        /// <returns>truth value</returns>
        private bool ValidateLocation(Location location)
        {
            HttpClient client = new HttpClient();

            var bingLocationFinderUrl = string.Format(@"http://dev.virtualearth.net/REST/v1/Locations/{0},{1}?includeEntityTypes=countryRegion&o=xml&key={2}", location.Latitude, location.Longitude, Utils.BingKey);

            string response = client.GetStringAsync(new Uri(bingLocationFinderUrl)).Result;

            StringBuilder output = new StringBuilder();

            using (XmlReader reader = XmlReader.Create(new StringReader(response)))
            {
                reader.ReadToFollowing("StatusCode");
                var statusCode = reader.ReadElementContentAsInt();

                if (statusCode == 200)
                {
                    reader.ReadToFollowing("ResourceSets");
                    reader.ReadToFollowing("ResourceSet");
                    reader.ReadToFollowing("Resources");
                    reader.ReadToFollowing("Location");
                    reader.ReadToFollowing("Name");

                    string countryRegion = reader.ReadElementContentAsString();

                    if(countryRegion == "United States")
                    {
                        return true;
                    }
                }               
            }

            return false;
        }
    }
}
