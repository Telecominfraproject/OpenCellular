// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common.Validators
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text.RegularExpressions;
    using System.Web.UI.WebControls;
    using Entities;
    using Practices.EnterpriseLibrary.Validation;

    /// <summary>
    /// Represents OFCOM Validator
    /// </summary>
    public class OfcomPawsValidator : BasePawsValidator
    {
        /// <summary>
        /// Represents the Paws EMail valid character regular expression.
        /// </summary>
        private readonly Regex email = new Regex(@"[a-z0-9._%+-]+@[a-z0-9.-]+\.[a-z]{2,4}", RegexOptions.Compiled);

        /// <summary>
        /// Validates the register request.
        /// </summary>
        /// <param name="registerRequest">The register request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        public override bool ValidateRegisterRequest(IRegisterRequest registerRequest, out List<string> errorMessages)
        {
            errorMessages = new List<string>();

            if (!this.IsValid(registerRequest.DeviceDescriptor, out errorMessages))
            {
                errorMessages.AddRange(errorMessages);
                return false;
            }

            if (registerRequest.DeviceDescriptor.EtsiDeviceCategory == null || registerRequest.DeviceDescriptor.EtsiDeviceCategory == string.Empty)
            {
                errorMessages.Add(Constants.ErrorMessagEtsiDeviceCategory);
                return false;
            }
            else if (registerRequest.DeviceDescriptor.EtsiDeviceCategory.ToLower() != Constants.PropertyNameEtsiDeviceCategoryMaster.ToLower() && registerRequest.DeviceDescriptor.EtsiDeviceCategory.ToLower() != Constants.PropertyNameEtsiDeviceCategorySlave.ToLower())
            {
                errorMessages.Add(Constants.ErrorMessagInvalidEtsiDeviceCategory);
                return false;
            }

            if (registerRequest.DeviceDescriptor.ManufacturerId == null || registerRequest.DeviceDescriptor.ManufacturerId == string.Empty)
            {
                errorMessages.Add(Constants.ErrorMessagEtsiManufacturerId);
                return false;
            }

            if (registerRequest.DeviceDescriptor.ModelId == null || registerRequest.DeviceDescriptor.ModelId == string.Empty)
            {
                errorMessages.Add(Constants.ErrorMessagEtsiModelId);
                return false;
            }

            if (!this.IsValidLocation(registerRequest.Location, out errorMessages))
            {
                return false;
            }

            var result = this.OfcomDeviceIdValidate(out errorMessages, registerRequest.DeviceDescriptor);
            return result;
        }

        /// <summary>
        /// This method validates the OFCOM Device Id 
        /// </summary>
        /// <param name="errorMessages">The error messages.</param>
        /// <param name="deviceDescriptors">The device descriptors.</param>
        /// <returns>response boolean value</returns>
        public bool OfcomDeviceIdValidate(out List<string> errorMessages, params DeviceDescriptor[] deviceDescriptors)
        {
            // validate the device id here
            errorMessages = new List<string>();
            return true;
        }

        /// <summary>
        /// Validates the initialization request.
        /// </summary>
        /// <param name="initRequest">The init request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        public override bool ValidateInitializationRequest(IInitRequest initRequest, out List<string> errorMessages)
        {
            errorMessages = new List<string>();

            if (!this.IsValid(initRequest.DeviceDescriptor, out errorMessages))
            {
                errorMessages.AddRange(errorMessages);
                return false;
            }

            if (initRequest.DeviceDescriptor.EtsiDeviceCategory == null || initRequest.DeviceDescriptor.EtsiDeviceCategory == string.Empty)
            {
                errorMessages.Add(Constants.ErrorMessagEtsiDeviceCategory);
                return false;
            }
            else if (initRequest.DeviceDescriptor.EtsiDeviceCategory.ToLower() != Constants.PropertyNameEtsiDeviceCategoryMaster.ToLower() && initRequest.DeviceDescriptor.EtsiDeviceCategory.ToLower() != Constants.PropertyNameEtsiDeviceCategorySlave.ToLower())
            {
                errorMessages.Add(Constants.ErrorMessagInvalidEtsiDeviceCategory);
                return false;
            }

            if (initRequest.DeviceDescriptor.ManufacturerId == null || initRequest.DeviceDescriptor.ManufacturerId == string.Empty)
            {
                errorMessages.Add(Constants.ErrorMessagEtsiManufacturerId);
                return false;
            }

            if (initRequest.DeviceDescriptor.ModelId == null || initRequest.DeviceDescriptor.ModelId == string.Empty)
            {
                errorMessages.Add(Constants.ErrorMessagEtsiModelId);
                return false;
            }

            if (!this.IsValidLocation(initRequest.Location, out errorMessages))
            {
                return false;
            }

            var result = this.OfcomDeviceIdValidate(out errorMessages, initRequest.DeviceDescriptor);
            return result;
        }

        /// <summary>
        ///     Validates the device request.
        /// </summary>
        /// <param name="deviceValidRequest">The device valid request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        public override bool ValidateDeviceRequest(IDeviceValidityRequest deviceValidRequest, out List<string> errorMessages)
        {
            errorMessages = new List<string>();
            foreach (var deviceDescriptor in deviceValidRequest.DevicesDescriptors)
            {
                if (!this.IsValid(deviceDescriptor, out errorMessages))
                {
                    return false;
                }

                if (!this.IsValidDeviceDescriptor(deviceDescriptor, out errorMessages))
                {
                    return false;
                }
            }

            if (deviceValidRequest.MasterDeviceDescriptors != null)
            {
                if (!this.IsValid(deviceValidRequest.MasterDeviceDescriptors, out errorMessages))
                {
                    return false;
                }

                if (!this.IsValidDeviceDescriptor(deviceValidRequest.MasterDeviceDescriptors, out errorMessages))
                {
                    return false;
                }
            }

            return true;
        }

        /// <summary>
        ///     Validates the device descriptor.
        /// </summary>
        /// <param name="deviceDescriptor">The device descriptor.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        public bool IsValidDeviceDescriptor(DeviceDescriptor deviceDescriptor, out List<string> errorMessages)
        {
            errorMessages = new List<string>();

            if (deviceDescriptor.ManufacturerId == null || deviceDescriptor.ManufacturerId == string.Empty)
            {
                errorMessages.Add(Constants.ErrorMessagEtsiManufacturerId);
                return false;
            }

            if (deviceDescriptor.ModelId == null || deviceDescriptor.ModelId == string.Empty)
            {
                errorMessages.Add(Constants.ErrorMessagEtsiModelId);
                return false;
            }

            if (string.IsNullOrEmpty(deviceDescriptor.EtsiEnDeviceType))
            {
                errorMessages.Add(Constants.ErrorMessagEtsiDeviceType);
            }

            if (string.IsNullOrEmpty(deviceDescriptor.SerialNumber))
            {
                errorMessages.Add(Constants.ErrorMessageSerialNumberRequired);
            }
            else if (deviceDescriptor.SerialNumber.Length > 64)
            {
                errorMessages.Add(Constants.ErrorMessageSerialNumberLength);
            }

            if (!string.IsNullOrEmpty(deviceDescriptor.ManufacturerId))
            {
                if (deviceDescriptor.ManufacturerId.Length > 64)
                {
                    errorMessages.Add(Constants.ErrorMessageEtsiManufacturerIdLength);
                }
            }

            if (!(deviceDescriptor.ModelId == null || deviceDescriptor.ModelId == string.Empty) && (deviceDescriptor.ModelId.Length > 64))
            {
                errorMessages.Add(Constants.ErrorMessageEtsiModelIdLength);
            }

            List<string> errorMsg = new List<string>();
            if (this.IsDeviceIdExcluded(deviceDescriptor.SerialNumber + deviceDescriptor.ManufacturerId + deviceDescriptor.ModelId, out errorMsg))
            {
                return false;
            }

            return errorMessages.Count == 0;
        }

        /// <summary>
        /// Determines whether [is valid location] [the specified location].
        /// </summary>
        /// <param name="location">The device location.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if [is valid location] [the specified location]; otherwise, <c>false</c>.</returns>
        public bool IsValidLocation(GeoLocation location, out List<string> errorMessages)
        {
            errorMessages = new List<string>();

            if (location == null)
            {
                errorMessages.Add(Constants.ErrorMessageInitLocationRequired);
            }

            if ((location != null) && !(location.Point == null) && (!(location.Region == null)))
            {
                errorMessages.Add(Constants.ErrorMessageMutuallyExclusive);
            }

            return errorMessages.Count == 0;
        }

        /// <summary>
        /// Determines whether [is valid locations] [the specified location].
        /// </summary>
        /// <param name="locations">The device locations.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if [is valid locations] [the specified locations]; otherwise, <c>false</c>.</returns>
        public bool IsValidLocations(GeoLocation[] locations, out List<string> errorMessages)
        {
            errorMessages = new List<string>();
            if (locations != null)
            {
                foreach (GeoLocation location in locations)
                {
                    if (location == null)
                    {
                        errorMessages.Add(Constants.ErrorMessageInitLocationRequired);
                    }

                    if ((location != null) && !(location.Point == null) && (!(location.Region == null)))
                    {
                        errorMessages.Add(Constants.ErrorMessageMutuallyExclusive);
                    }
                }
            }
            else
            {
                errorMessages.Add(Constants.ErrorMessageInitLocationRequired);
            }

            return errorMessages.Count == 0;
        }

        /// <summary>
        ///     Validates the specified object.
        /// </summary>
        /// <param name="spectrumRequest">The spectrum request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>Validator Result.</returns>
        public override bool ValidateSpectrumQueryRequest(IAvailableSpectrumRequest spectrumRequest, out List<string> errorMessages)
        {
            var valResult = base.ValidateSpectrumQueryRequest(spectrumRequest, out errorMessages);
            if (valResult)
            {
                if ((!this.IsValid(spectrumRequest.MasterDeviceLocation, out errorMessages)) || (!this.IsValidLocation(spectrumRequest.MasterDeviceLocation, out errorMessages)))
                {
                    if (!this.IsValid(spectrumRequest.Location, out errorMessages))
                    {
                        return false;
                    }

                    if (!this.IsValidLocation(spectrumRequest.Location, out errorMessages))
                    {
                        return false;
                    }
                }

                if (spectrumRequest.MasterDeviceDescriptors != null)
                {
                    if (!this.IsValid(spectrumRequest.MasterDeviceDescriptors, out errorMessages))
                    {
                        return false;
                    }

                    if (this.IsValidDeviceDescriptor(spectrumRequest.MasterDeviceDescriptors, out errorMessages))
                    {
                        if (spectrumRequest.MasterDeviceDescriptors.EtsiEnDeviceType == null || spectrumRequest.MasterDeviceDescriptors.EtsiEnDeviceType == string.Empty)
                        {
                            errorMessages.Add(Constants.ErrorMessagEtsiDeviceType);
                            return false;
                        }

                        if (spectrumRequest.MasterDeviceDescriptors.EtsiDeviceCategory == null || spectrumRequest.MasterDeviceDescriptors.EtsiDeviceCategory == string.Empty)
                        {
                            errorMessages.Add(Constants.ErrorMessagEtsiDeviceCategory);
                            return false;
                        }

                        if (spectrumRequest.MasterDeviceDescriptors.EtsiDeviceCategory.ToLower() == Constants.PropertyNameEtsiDeviceCategoryMaster.ToLower())
                        {
                            if (spectrumRequest.MasterDeviceDescriptors.EtsiEnDeviceType.ToLower() == Constants.PropertyNameEtsiDeviceTypeA.ToLower())
                            {
                                // Registration Information Provided so FetchRegistered Device
                                if (!this.PawsDalc.IsDeviceRegistered<FixedTVBDRegistration>(Utils.GetRegionalTableName(Constants.FixedTVBDRegistrationTable), spectrumRequest.MasterDeviceDescriptors.SerialNumber))
                                {
                                    // Device already registered so can proceed
                                    errorMessages.Add(Constants.ErrorMessageNotRegistered);
                                    return false;
                                }
                                else
                                {
                                    return true;
                                }
                            }

                            return true;
                        }

                        errorMessages.Add(Constants.ErrorMessageModeNotImplemented);
                        return false;
                    }
                }

                if (spectrumRequest.DeviceDescriptor != null)
                {
                    if (!this.IsValid(spectrumRequest.DeviceDescriptor, out errorMessages))
                    {
                        return false;
                    }

                    if (this.IsValidDeviceDescriptor(spectrumRequest.DeviceDescriptor, out errorMessages))
                    {
                        if (spectrumRequest.DeviceDescriptor.EtsiEnDeviceType == null || spectrumRequest.DeviceDescriptor.EtsiEnDeviceType == string.Empty)
                        {
                            errorMessages.Add(Constants.ErrorMessagEtsiDeviceType);
                            return false;
                        }

                        if (spectrumRequest.DeviceDescriptor.EtsiDeviceCategory == null || spectrumRequest.DeviceDescriptor.EtsiDeviceCategory == string.Empty)
                        {
                            errorMessages.Add(Constants.ErrorMessagEtsiDeviceCategory);
                            return false;
                        }

                        if (this.IsValidDeviceDescriptor(spectrumRequest.DeviceDescriptor, out errorMessages))
                        {
                            if (spectrumRequest.DeviceDescriptor.EtsiDeviceCategory.ToLower() == Constants.PropertyNameEtsiDeviceCategoryMaster.ToLower())
                            {
                                if (spectrumRequest.DeviceDescriptor.EtsiEnDeviceType.ToLower() == Constants.PropertyNameEtsiDeviceTypeA.ToLower())
                                {
                                    // Registration Information Provided so FetchRegistered Device
                                    if (!this.PawsDalc.IsDeviceRegistered<FixedTVBDRegistration>(Utils.GetRegionalTableName(Constants.FixedTVBDRegistrationTable), spectrumRequest.DeviceDescriptor.SerialNumber))
                                    {
                                        // Device already registered so can proceed
                                        errorMessages.Add(Constants.ErrorMessageNotRegistered);
                                        return false;
                                    }
                                    else
                                    {
                                        return true;
                                    }
                                }

                                return true;
                            }

                            errorMessages.Add(Constants.ErrorMessageModeNotImplemented);
                            return false;
                        }
                    }
                }

                return valResult;
            }

            return false;
        }

        /// <summary>
        ///     Validates the batch spectrum query request.
        /// </summary>
        /// <param name="spectrumRequest">The spectrum request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        public override bool ValidateBatchSpectrumQueryRequest(BatchAvailableSpectrumRequestBase spectrumRequest, out List<string> errorMessages)
        {
            var valResult = base.ValidateBatchSpectrumQueryRequest(spectrumRequest, out errorMessages);
            if (valResult)
            {
                if ((!this.IsValid(spectrumRequest.MasterDeviceLocation, out errorMessages)) || (!this.IsValidLocation(spectrumRequest.MasterDeviceLocation, out errorMessages)))
                {
                    if (!this.IsValid(spectrumRequest.Locations, out errorMessages))
                    {
                        return false;
                    }

                    if (!this.IsValidLocations(spectrumRequest.Locations, out errorMessages))
                    {
                        return false;
                    }
                }

                if (spectrumRequest.MasterDeviceDescriptors != null)
                {
                    if (!this.IsValid(spectrumRequest.MasterDeviceDescriptors, out errorMessages))
                    {
                        return false;
                    }

                    if (this.IsValidDeviceDescriptor(spectrumRequest.MasterDeviceDescriptors, out errorMessages))
                    {
                        if (spectrumRequest.MasterDeviceDescriptors.EtsiEnDeviceType == null || spectrumRequest.MasterDeviceDescriptors.EtsiEnDeviceType == string.Empty)
                        {
                            errorMessages.Add(Constants.ErrorMessagEtsiDeviceType);
                            return false;
                        }

                        if (spectrumRequest.MasterDeviceDescriptors.EtsiDeviceCategory == null || spectrumRequest.MasterDeviceDescriptors.EtsiDeviceCategory == string.Empty)
                        {
                            errorMessages.Add(Constants.ErrorMessagEtsiDeviceCategory);
                            return false;
                        }

                        if (spectrumRequest.MasterDeviceDescriptors.EtsiDeviceCategory.ToLower() == Constants.PropertyNameEtsiDeviceCategoryMaster.ToLower())
                        {
                            if (spectrumRequest.MasterDeviceDescriptors.EtsiEnDeviceType.ToLower() == Constants.PropertyNameEtsiDeviceTypeA.ToLower())
                            {
                                // Registration Information Provided so FetchRegistered Device
                                if (!this.PawsDalc.IsDeviceRegistered<FixedTVBDRegistration>(Utils.GetRegionalTableName(Constants.FixedTVBDRegistrationTable), spectrumRequest.MasterDeviceDescriptors.SerialNumber))
                                {
                                    // Device already registered so can proceed
                                    errorMessages.Add(Constants.ErrorMessageNotRegistered);
                                    return false;
                                }
                                else
                                {
                                    return true;
                                }
                            }

                            return true;
                        }

                        errorMessages.Add(Constants.ErrorMessageModeNotImplemented);
                        return false;
                    }
                    else
                    {
                        return false;
                    }
                }

                if (spectrumRequest.DeviceDescriptor != null)
                {
                    if (!this.IsValid(spectrumRequest.DeviceDescriptor, out errorMessages))
                    {
                        return false;
                    }

                    if (this.IsValidDeviceDescriptor(spectrumRequest.DeviceDescriptor, out errorMessages))
                    {
                        if (spectrumRequest.DeviceDescriptor.EtsiEnDeviceType == null || spectrumRequest.DeviceDescriptor.EtsiEnDeviceType == string.Empty)
                        {
                            errorMessages.Add(Constants.ErrorMessagEtsiDeviceType);
                            return false;
                        }

                        if (spectrumRequest.DeviceDescriptor.EtsiDeviceCategory == null || spectrumRequest.DeviceDescriptor.EtsiDeviceCategory == string.Empty)
                        {
                            errorMessages.Add(Constants.ErrorMessagEtsiDeviceCategory);
                            return false;
                        }

                        if (this.IsValidDeviceDescriptor(spectrumRequest.DeviceDescriptor, out errorMessages))
                        {
                            if (spectrumRequest.DeviceDescriptor.EtsiDeviceCategory.ToLower() == Constants.PropertyNameEtsiDeviceCategoryMaster.ToLower())
                            {
                                if (spectrumRequest.DeviceDescriptor.EtsiEnDeviceType.ToLower() == Constants.PropertyNameEtsiDeviceTypeA.ToLower())
                                {
                                    // Registration Information Provided so FetchRegistered Device
                                    if (!this.PawsDalc.IsDeviceRegistered<FixedTVBDRegistration>(Utils.GetRegionalTableName(Constants.FixedTVBDRegistrationTable), spectrumRequest.DeviceDescriptor.SerialNumber))
                                    {
                                        // Device already registered so can proceed
                                        errorMessages.Add(Constants.ErrorMessageNotRegistered);
                                        return false;
                                    }
                                    else
                                    {
                                        return true;
                                    }
                                }

                                return true;
                            }

                            errorMessages.Add(Constants.ErrorMessageModeNotImplemented);
                            return false;
                        }
                        else
                        {
                            return false;
                        }
                    }
                }
            }

            return valResult;
        }

        /// <summary>
        ///     Validates the spectrum use notify request.
        /// </summary>
        /// <param name="notifyRequest">The notify request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        public override bool ValidateNotifyRequest(INotifyRequest notifyRequest, out List<string> errorMessages)
        {
            if (!this.IsValidDeviceDescriptor(notifyRequest.DeviceDescriptor, out errorMessages))
            {
                return false;
            }

            if (!this.IsValid(notifyRequest.Location, out errorMessages))
            {
                return false;
            }

            if (!this.IsValid(notifyRequest.Spectra, out errorMessages))
            {
                return false;
            }

            if (!this.IsValidSpectra(notifyRequest.Spectra, out errorMessages))
            {
                return false;
            }

            if (!this.IsValidLocation(notifyRequest.Location, out errorMessages))
            {
                return false;
            }

            if (notifyRequest.DeviceDescriptor.ManufacturerId == null || notifyRequest.DeviceDescriptor.ManufacturerId == string.Empty)
            {
                errorMessages.Add(Constants.ErrorMessagEtsiManufacturerId);
                return false;
            }

            if (notifyRequest.DeviceDescriptor.ModelId == null || notifyRequest.DeviceDescriptor.ModelId == string.Empty)
            {
                errorMessages.Add(Constants.ErrorMessagEtsiModelId);
                return false;
            }

            return true;
        }

        /// <summary>
        /// Validates the interference query request.
        /// </summary>
        /// <param name="interferenceQueryRequest">The interference query request request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        public override bool ValidateInterferenceQueryRequest(IInterferenceQueryRequest interferenceQueryRequest, out List<string> errorMessages)
        {
            errorMessages = new List<string>();
                        
            if (string.IsNullOrEmpty(interferenceQueryRequest.TimeStamp))
            {
                errorMessages.Add(Constants.ErrorMessageTimeStampRequired);
                return false;
            }

            if (!this.IsValid(interferenceQueryRequest.TimeStamp, out errorMessages))
            {
                return false;
            }

            if (!this.ValidateTimeStamp(interferenceQueryRequest.TimeStamp))
            {
                errorMessages.Add(Constants.InvalidTimeStamp);
                return false;
            }

            if (string.IsNullOrEmpty(interferenceQueryRequest.StartTime))
            {
                errorMessages.Add(Constants.ErrorMessageInterferenceQueryStartTimeRequired);
                return false;
            }

            if (!this.ValidateTimeStamp(interferenceQueryRequest.StartTime))
            {
                errorMessages.Add(Constants.InvalidStartDate);
                return false;
            }

            if (string.IsNullOrEmpty(interferenceQueryRequest.EndTime))
            {
                errorMessages.Add(Constants.ErrorMessageEndTimeRequired);
                return false;
            }

            if (!this.ValidateTimeStamp(interferenceQueryRequest.EndTime))
            {
                errorMessages.Add(Constants.InvalidEndDate);
                return false;
            }

            if (!this.IsValid(interferenceQueryRequest.Location, out errorMessages))
            {
                return false;
            }

            if (!this.IsValid(interferenceQueryRequest.Requestor, out errorMessages))
            {
                return false;
            }

            if (interferenceQueryRequest.Requestor != null && !(interferenceQueryRequest.Requestor.Owner.Email == null) && !string.IsNullOrEmpty(interferenceQueryRequest.Requestor.Owner.Email.Text))
            {
                bool isValidEmail = this.email.IsMatch(interferenceQueryRequest.Requestor.Owner.Email.Text);
                if (!isValidEmail)
                {
                    return false;
                }
            }

            if (!this.IsValid(interferenceQueryRequest.StartTime, out errorMessages))
            {
                return false;
            }

            if (!this.IsValid(interferenceQueryRequest.EndTime, out errorMessages))
            {
                return false;
            }

            if (!this.IsValidRequestor(interferenceQueryRequest.Requestor, out errorMessages))
            {
                return false;
            }

            var result = this.IsValidLocationInterferenceQuery(out errorMessages, interferenceQueryRequest.Location);
            return result;
        }

        /// <summary>
        /// Determines whether [is valid location interference query] [the specified location].
        /// </summary>
        /// <param name="errorMessages">The error messages.</param>
        /// <param name="location">The device location.</param>
        /// <returns><c>true</c> if [is valid location] [the specified location]; otherwise, <c>false</c>.</returns>
        public bool IsValidLocationInterferenceQuery(out List<string> errorMessages, GeoLocation location)
        {
            errorMessages = new List<string>();

            if (location == null)
            {
                errorMessages.Add(Constants.ErrorMessageInitLocationRequired);
            }

            if ((location != null) && !(location.Point == null) && string.IsNullOrEmpty(location.Point.SemiMajorAxis.ToString()))
            {
                errorMessages.Add(Constants.ErrorMessageSemiMajorAxis);
            }

            if ((location != null) && (location.Point == null) && ((!(location.Region == null)) || (location.Confidence > 0)))
            {
                errorMessages.Add(Constants.ErrorMessageRegionConfidence);
            }

            if ((location != null) && !(location.Point == null) && (!(location.Region == null)))
            {
                errorMessages.Add(Constants.ErrorMessageMutuallyExclusive);
            }

            return errorMessages.Count == 0;
        }

        /// <summary>
        /// Determines whether [is valid location interference query] [the specified location].
        /// </summary>
        /// <param name="requestor">The device requestor.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if [is valid location] [the specified location]; otherwise, <c>false</c>.</returns>
        public bool IsValidRequestor(DeviceOwner requestor, out List<string> errorMessages)
        {
            errorMessages = new List<string>();

            if (requestor == null)
            {
                errorMessages.Add(Constants.ErrorMessageRequestorRequired);
            }

            if ((requestor != null) && ((requestor.Owner.Email == null) || string.IsNullOrEmpty(requestor.Owner.Email.Text)))
            {
                errorMessages.Add(Constants.ErrorMessageRequestorEmailRequired);
            }

            if ((requestor != null) && ((requestor.Owner.Organization == null) || string.IsNullOrEmpty(requestor.Owner.Organization.Text)))
            {
                errorMessages.Add(Constants.ErrorMessageRequestorOrgRequired);
            }

            return errorMessages.Count == 0;
        }

        /// <summary>
        /// Determines whether [is valid spectra] [the specified spectra].
        /// </summary>
        /// <param name="spectra">The device spectrum.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if [is valid spectra] [the specified spectra]; otherwise, <c>false</c>.</returns>
        public bool IsValidSpectra(Spectrum[] spectra, out List<string> errorMessages)
        {
            errorMessages = new List<string>();

            if (spectra == null)
            {
                errorMessages.Add(Constants.ErrorMessageSpectrumRequired);
            }
            else
            {
                foreach (Spectrum spectrum in spectra)
                {
                    if (spectrum.ResolutionBwHz == 0)
                    {
                        errorMessages.Add(Constants.ErrorMessageBandwidthRequired);
                    }
                    else if (spectrum.Profiles == null)
                    {
                        errorMessages.Add(Constants.ErrorMessageSpectrumProfileRequired);
                    }
                    else
                    {
                        foreach (SpectrumProfile freqRange in spectrum.Profiles)
                        {
                            if (freqRange.Hz == 0)
                            {
                                errorMessages.Add(Constants.ErrorMessageStartHzRequired);
                            }                           
                        }
                    }
                }
            }

            return errorMessages.Count == 0;
        }

        /// <summary>
        /// Validates the paws device.
        /// </summary>
        /// <param name="deviceId">The device identifier.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        public override bool ValidatePawsDevice(string deviceId)
        {
            return true;
        }

        /// <summary>
        /// Validates the time stamp.
        /// </summary>
        /// <param name="timestamp">The time stamp.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        public bool ValidateTimeStamp(string timestamp)
        {
            DateTimeOffset dateTimeOffset;
            return DateTimeOffset.TryParse(timestamp, out dateTimeOffset);
        }
    }
}
