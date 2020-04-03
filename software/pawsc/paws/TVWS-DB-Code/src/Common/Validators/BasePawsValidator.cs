// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common.Validators
{
    using System.Collections.Generic;
    using System.Linq;
    using System.Text.RegularExpressions;
    using Entities;
    using Microsoft.WindowsAzure.Storage.Table;
    using Practices.EnterpriseLibrary.Validation;
    using Practices.EnterpriseLibrary.Validation.Validators;
    using Practices.Unity;

    /// <summary>
    /// Class BasePawsValidator
    /// </summary>
    public class BasePawsValidator : IPawsValidator
    {
        /// <summary>
        /// Gets or sets the common DALC.
        /// </summary>
        /// <value>The common DALC.</value>
        [Dependency]
        public IDalcCommon CommonDalc { get; set; }

        /// <summary>
        /// Gets or sets the paws DALC.
        /// </summary>
        /// <value>The paws DALC.</value>
        [Dependency]
        public IDalcPaws PawsDalc { get; set; }

        /// <summary>
        /// Validates the specified object.
        /// </summary>
        /// <param name="spectrumRequest">The spectrum request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>Validator Result.</returns>
        public virtual bool ValidateSpectrumQueryRequest(IAvailableSpectrumRequest spectrumRequest, out List<string> errorMessages)
        {
            errorMessages = new List<string>();
            return true;
        }

        /// <summary>
        /// Validates the batch spectrum query request.
        /// </summary>
        /// <param name="spectrumRequest">The spectrum request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        public virtual bool ValidateBatchSpectrumQueryRequest(BatchAvailableSpectrumRequestBase spectrumRequest, out List<string> errorMessages)
        {
            errorMessages = new List<string>();
            return true;
        }

        /// <summary>
        /// Validates the register request.
        /// </summary>
        /// <param name="registerRequest">The register request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        public virtual bool ValidateRegisterRequest(IRegisterRequest registerRequest, out List<string> errorMessages)
        {
            errorMessages = new List<string>();
            return true;
        }

        /// <summary>
        /// Determines whether the specified object is valid.
        /// </summary>
        /// <typeparam name="T">Generic Type</typeparam>
        /// <param name="obj">The object.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if the specified object is valid; otherwise, <c>false</c>.</returns>
        public bool IsValid<T>(T obj, out List<string> errorMessages)
        {
            errorMessages = new List<string>();
            Validator cusValidator = new ObjectValidator();
            ValidationResults valResults = cusValidator.Validate(obj);
            if (!valResults.IsValid)
            {
                errorMessages.AddRange(valResults.Select(objVal => objVal.Message));
                return false;
            }

            return true;
        }

        /// <summary>
        /// Validates the initialization request.
        /// </summary>
        /// <param name="initRequest">The init request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        public virtual bool ValidateInitializationRequest(IInitRequest initRequest, out List<string> errorMessages)
        {
            errorMessages = new List<string>();
            return true;
        }

        /// <summary>
        /// Validates the device request.
        /// </summary>
        /// <param name="deviceValidRequest">The device valid request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        public virtual bool ValidateDeviceRequest(IDeviceValidityRequest deviceValidRequest, out List<string> errorMessages)
        {
            errorMessages = new List<string>();
            return true;
        }

        /// <summary>
        /// Validates the notify spectrum request.
        /// </summary>
        /// <param name="notifyRequest">The notify request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        public virtual bool ValidateNotifyRequest(INotifyRequest notifyRequest, out List<string> errorMessages)
        {
            errorMessages = new List<string>();
            return true;
        }

        /// <summary>
        /// Validates the paws device.
        /// </summary>
        /// <param name="deviceID">The device identifier.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        public virtual bool ValidatePawsDevice(string deviceID)
        {            
            return false;
        }

         /// <summary>
        /// Validates the Interference Query request.
        /// </summary>
        /// <param name="interferenceQueryRequest">The Interference Query request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        public virtual bool ValidateInterferenceQueryRequest(IInterferenceQueryRequest interferenceQueryRequest, out List<string> errorMessages)
        {
            errorMessages = new List<string>();
            return true;
        }

        /// <summary>
        /// Checks for excluded identifier.
        /// </summary>
        /// <param name="deviceId">The device identifier.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        public bool IsDeviceIdExcluded(string deviceId, out List<string> errorMessages)
        {
            errorMessages = new List<string>();
            var deviceIds = this.CommonDalc.FetchEntity<DynamicTableEntity>(Utils.GetRegionalTableName(Constants.ExcludedIds), new { DeviceId = deviceId });
            if (deviceIds.Count > 0)
            {
                errorMessages.Add(Constants.ErrorMessagDeviceIdNotAuthorized);
                return true;
            }

            return false;
        }

        /// <summary>
        /// Determines whether [is device serial no excluded] [the specified serial number].
        /// </summary>
        /// <param name="serialNumber">The serial number.</param>
        /// <param name="deviceId">The device identifier.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if [is device serial no excluded] [the specified serial number]; otherwise, <c>false</c>.</returns>
        public bool IsDeviceSerialNoExcluded(string serialNumber, string deviceId, out List<string> errorMessages)
        {
            errorMessages = new List<string>();
            var deviceIds = this.CommonDalc.FetchEntity<DynamicTableEntity>(Utils.GetRegionalTableName(Constants.ExcludedIds), new { SerialNumber = serialNumber, DeviceId = deviceId });
            if (deviceIds.Count > 0)
            {
                errorMessages.Add(Constants.ErrorMessagDeviceIdNotAuthorized);
                return true;
            }

            return false;
        }
    }
}
