// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common.Validators
{
    using System;
    using System.Collections.Generic;
    using System.Text.RegularExpressions;
    using Entities;
    using Microsoft.WindowsAzure.Storage.Table;

    /// <summary>
    /// Class FCC PawsValidator.
    /// </summary>
    public class FccPawsValidator : BasePawsValidator
    {
        /// <summary>
        /// Regex for FCC Id starting with numeric.
        /// </summary>
        private readonly Regex fccIdNumericExp = new Regex(@"^[2-9][2-9a-zA-Z]*$", RegexOptions.Compiled);

        /// <summary>
        /// Regex for FCC Id starting with an alphabet.
        /// </summary>
        private readonly Regex fccAlphabeticExp = new Regex(@"^[a-zA-Z][2-9a-zA-Z]*$", RegexOptions.Compiled);

        /// <summary>
        /// Regex for FCC Id containing either zero or one.
        /// </summary>        
        private readonly Regex fccZeroOrOneExp = new Regex(@"^[0-9a-zA-Z]*$", RegexOptions.Compiled);

        /// <summary>
        ///     Validates the specified object.
        /// </summary>
        /// <param name="spectrumRequest">The spectrum request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>Validator Result.</returns>
        public override bool ValidateSpectrumQueryRequest(IAvailableSpectrumRequest spectrumRequest, out List<string> errorMessages)
        {
            var valResult = base.ValidateSpectrumQueryRequest(spectrumRequest, out errorMessages);

            if ((this.IsValid(spectrumRequest.MasterDeviceLocation, out errorMessages) == false) || (this.IsValidLocation(spectrumRequest.MasterDeviceLocation, out errorMessages) == false))
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
            
            if (spectrumRequest.DeviceDescriptor != null)
            {
                if (this.IsValidDeviceDescriptor(spectrumRequest.DeviceDescriptor, out errorMessages))
                {
                    if (!this.FCCIdValidate(out errorMessages, spectrumRequest.DeviceDescriptor))
                    {
                        return false;
                    }

                    if (spectrumRequest.DeviceDescriptor.FccTvbdDeviceType.ToLower() == "fixed")
                    {
                        // Registration Information Provided so FetchRegistered Device
                        if (!this.PawsDalc.IsDeviceRegistered<FixedTVBDRegistration>(Utils.GetRegionalTableName(Constants.FixedTVBDRegistrationTable), spectrumRequest.DeviceDescriptor.SerialNumber))
                        {
                            // Device already registered so can proceed
                            errorMessages.Add(Constants.ErrorMessageNotRegistered);
                            return false;
                        }

                        return true;
                    }

                    if (spectrumRequest.DeviceDescriptor.FccTvbdDeviceType.ToLower() == "mode_1" || spectrumRequest.DeviceDescriptor.FccTvbdDeviceType.ToLower() == "mode_2")
                    {
                        // No Other Check if Mode 2
                        return true;
                    }

                    errorMessages.Add(Constants.ErrorMessageModeNotImplemented);
                    return false;
                }

                return false;
            }

            return valResult;
        }

        /// <summary>
        ///     Validates the register request.
        /// </summary>
        /// <param name="registerRequest">The register request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        public override bool ValidateRegisterRequest(IRegisterRequest registerRequest, out List<string> errorMessages)
        {
            errorMessages = new List<string>();

            if (!this.IsValidDeviceDescriptor(registerRequest.DeviceDescriptor, out errorMessages))
            {
                return false;
            }

            if (!this.IsValid(registerRequest.Location, out errorMessages))
            {
                return false;
            }

            if (!this.IsValid(registerRequest.DeviceOwner, out errorMessages))
            {
                return false;
            }

            if (!this.IsValidLocation(registerRequest.Location, out errorMessages))
            {
                return false;
            }

            var result = this.FCCIdValidate(out errorMessages, registerRequest.DeviceDescriptor);

            return result;
        }

        /// <summary>
        ///     Validates the initialization request.
        /// </summary>
        /// <param name="initRequest">The init request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        public override bool ValidateInitializationRequest(IInitRequest initRequest, out List<string> errorMessages)
        {
            errorMessages = new List<string>();

            if (!this.IsValidDeviceDescriptor(initRequest.DeviceDescriptor, out errorMessages))
            {
                return false;
            }

            if (!this.IsValid(initRequest.Location, out errorMessages))
            {
                return false;
            }

            if (!this.IsValidLocation(initRequest.Location, out errorMessages))
            {
                return false;
            }

            var result = this.FCCIdValidate(out errorMessages, initRequest.DeviceDescriptor);

            return result;
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

            if (notifyRequest.MasterDeviceDescriptors != null)
            {
                if (!this.IsValidDeviceDescriptor(notifyRequest.MasterDeviceDescriptors, out errorMessages))
                {
                    return false;
                }

                if (!this.FCCIdValidate(out errorMessages, notifyRequest.MasterDeviceDescriptors))
                {
                    return false;
                }
            }

            if (notifyRequest.MasterDeviceLocation != null)
            {
                if (!this.IsValid(notifyRequest.MasterDeviceLocation, out errorMessages))
                {
                    return false;
                }

                if (!this.IsValidLocation(notifyRequest.MasterDeviceLocation, out errorMessages))
                {
                    return false;
                }
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

            var result = this.FCCIdValidate(out errorMessages, notifyRequest.DeviceDescriptor);

            return result;
        }

        /// <summary>
        ///     Validates the device request.
        /// </summary>
        /// <param name="deviceValidRequest">The device valid request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if valid, <c>false</c> otherwise</returns>
        public override bool ValidateDeviceRequest(IDeviceValidityRequest deviceValidRequest, out List<string> errorMessages)
        {
            foreach (var deviceDescriptor in deviceValidRequest.DevicesDescriptors)
            {
                if (!this.IsValidDeviceDescriptor(deviceDescriptor, out errorMessages))
                {
                    return false;
                }
            }

            if (deviceValidRequest.MasterDeviceDescriptors != null)
            {
                if (!this.IsValidDeviceDescriptor(deviceValidRequest.MasterDeviceDescriptors, out errorMessages))
                {
                    return false;
                }
            }

            var result = this.FCCIdValidate(out errorMessages, deviceValidRequest.DevicesDescriptors);

            return result;
        }

        /// <summary>
        /// This method validates the FCC Id length based on the first character and also verifies if it contains zero or one
        /// </summary>
        /// <param name="errorMessages">The error messages.</param>
        /// <param name="deviceDescriptors">The device descriptors.</param>
        /// <returns>response FCCIdValidation</returns>
        public bool FCCIdValidate(out List<string> errorMessages, params DeviceDescriptor[] deviceDescriptors)
        {
            errorMessages = new List<string>();
            for (int deviceIndex = 0; deviceIndex < deviceDescriptors.Length; deviceIndex++)
            {
                if (deviceDescriptors[deviceIndex].FccId == null)
                {
                    errorMessages.Add(Constants.ErrorMessageFccIdRequired);
                    return false;
                }

                bool isNumeric = this.fccIdNumericExp.IsMatch(deviceDescriptors[deviceIndex].FccId);
                bool isAlphabet = this.fccAlphabeticExp.IsMatch(deviceDescriptors[deviceIndex].FccId);
                bool hasZeroOrOne = this.fccZeroOrOneExp.IsMatch(deviceDescriptors[deviceIndex].FccId);
                if (isNumeric)
                {
                    hasZeroOrOne = false;
                    if (deviceDescriptors[deviceIndex].FccId.Length > 19)
                    {
                        errorMessages.Add(Constants.ErrorMessageFccIdLength);
                        return false;
                    }
                    else if (deviceDescriptors[deviceIndex].FccId.Length < 6)
                    {
                        errorMessages.Add(Constants.ErrorMessageFccIdMinimumLength);
                        return false;
                    }
                }

                if (isAlphabet)
                {
                    hasZeroOrOne = false;
                    if (deviceDescriptors[deviceIndex].FccId.Length > 17)
                    {
                        errorMessages.Add(Constants.ErrorMessageFccIdAlphabetLength);
                        return false;
                    }
                    else if (deviceDescriptors[deviceIndex].FccId.Length < 4)
                    {
                        errorMessages.Add(Constants.ErrorMessageFccIdAlphabetMinimumLength);
                        return false;
                    }
                }

                if (hasZeroOrOne)
                {
                    errorMessages.Add(Constants.ErrorMessageInvalidFccId);
                    return false;
                }
            }

            return true;
        }

        /// <summary>
        ///     Validates the batch spectrum query request.
        /// </summary>
        /// <param name="spectrumRequest">The spectrum request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        public override bool ValidateBatchSpectrumQueryRequest(BatchAvailableSpectrumRequestBase spectrumRequest, out List<string> errorMessages)
        {
            if ((this.IsValid(spectrumRequest.MasterDeviceLocation, out errorMessages) == false) || (this.IsValidLocation(spectrumRequest.MasterDeviceLocation, out errorMessages) == false))
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
                if (this.IsValidDeviceDescriptor(spectrumRequest.MasterDeviceDescriptors, out errorMessages))
                {
                    if (!this.FCCIdValidate(out errorMessages, spectrumRequest.MasterDeviceDescriptors))
                    {
                        return false;
                    }

                    if (spectrumRequest.MasterDeviceDescriptors.FccTvbdDeviceType.ToLower() == "fixed")
                    {
                        // Registration Information Provided so FetchRegistered Device
                        if (!this.PawsDalc.IsDeviceRegistered<FixedTVBDRegistration>(Utils.GetRegionalTableName(Constants.FixedTVBDRegistrationTable), spectrumRequest.MasterDeviceDescriptors.SerialNumber))
                        {
                            // Device not registered so can't proceed
                            errorMessages.Add(Constants.ErrorMessageNotRegistered);
                            return false;
                        }

                        return true;
                    }

                    if (spectrumRequest.MasterDeviceDescriptors.FccTvbdDeviceType.ToLower() == "mode_1" || spectrumRequest.MasterDeviceDescriptors.FccTvbdDeviceType.ToLower() == "mode_2")
                    {
                        // No Other Check if Mode 2
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
                if (this.IsValidDeviceDescriptor(spectrumRequest.DeviceDescriptor, out errorMessages))
                {
                    if (!this.FCCIdValidate(out errorMessages, spectrumRequest.DeviceDescriptor))
                    {
                        return false;
                    }

                    if (spectrumRequest.DeviceDescriptor.FccTvbdDeviceType.ToLower() == "fixed")
                    {
                        // Registration Information Provided so FetchRegistered Device
                        if (!this.PawsDalc.IsDeviceRegistered<FixedTVBDRegistration>(Utils.GetRegionalTableName(Constants.FixedTVBDRegistrationTable), spectrumRequest.DeviceDescriptor.SerialNumber))
                        {
                            // Device not registered so can't proceed
                            errorMessages.Add(Constants.ErrorMessageNotRegistered);
                            return false;
                        }

                        return true;
                    }

                    if (spectrumRequest.DeviceDescriptor.FccTvbdDeviceType.ToLower() == "mode_1" || spectrumRequest.DeviceDescriptor.FccTvbdDeviceType.ToLower() == "mode_2")
                    {
                        // No Other Check if Mode 2
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

            return true;
        }

        /// <summary>
        /// Determines whether [is valid device descriptor] [the specified device descriptor].
        /// </summary>
        /// <param name="deviceDescriptor">The device descriptor.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if [is valid device descriptor] [the specified device descriptor]; otherwise, <c>false</c>.</returns>
        public bool IsValidDeviceDescriptor(DeviceDescriptor deviceDescriptor, out List<string> errorMessages)
        {
            errorMessages = new List<string>();

            if (string.IsNullOrEmpty(deviceDescriptor.FccId))
            {
                errorMessages.Add(Constants.ErrorMessageFccIdRequired);
            }

            if (this.IsDeviceIdExcluded(deviceDescriptor.FccId, out errorMessages))
            {
                return false;
            }

            if (!string.IsNullOrEmpty(deviceDescriptor.SerialNumber) && this.IsDeviceSerialNoExcluded(deviceDescriptor.SerialNumber, deviceDescriptor.FccId, out errorMessages))
            {
                return false;
            }

            if (string.IsNullOrEmpty(deviceDescriptor.FccTvbdDeviceType))
            {
                errorMessages.Add(Constants.ErrorMessageFccTvbdDeviceTypeRequired);
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
                errorMessages.Add(Constants.ErrorMessageModelIdLength);
            }

            return errorMessages.Count == 0;
        }

        /// <summary>
        /// Determines whether [is valid locations] [the specified location].
        /// </summary>
        /// <param name="locations">The locations.</param>
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
            var authorizedDevice = this.CommonDalc.FetchEntity<AuthorizedDeviceRecord>(Utils.GetRegionalTableName(Constants.AuthorizedDeviceModelsTableName), new { FCCId = deviceId });
                    
            if (authorizedDevice.Count > 0)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
    }
}
