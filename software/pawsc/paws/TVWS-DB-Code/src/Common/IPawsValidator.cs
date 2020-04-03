// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common
{
    using System.Collections.Generic;
    using Entities;

    /// <summary>
    /// Interface IPawsValidator
    /// </summary>
    public interface IPawsValidator
    {
        /// <summary>
        /// Validates the spectrum query request.
        /// </summary>
        /// <param name="spectrumRequest">The spectrum request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        bool ValidateSpectrumQueryRequest(IAvailableSpectrumRequest spectrumRequest, out List<string> errorMessages);

        /// <summary>
        /// Validates the register request.
        /// </summary>
        /// <param name="registerRequest">The register request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        bool ValidateRegisterRequest(IRegisterRequest registerRequest, out List<string> errorMessages);

        /// <summary>
        /// Validates the interference query request.
        /// </summary>
        /// <param name="interferenceQueryRequest">The interference query request request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        bool ValidateInterferenceQueryRequest(IInterferenceQueryRequest interferenceQueryRequest, out List<string> errorMessages);

        /// <summary>
        /// Validates the notify request.
        /// </summary>
        /// <param name="notifyRequest">The register request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        bool ValidateNotifyRequest(INotifyRequest notifyRequest, out List<string> errorMessages);

        /// <summary>
        /// Validates the initialization request.
        /// </summary>
        /// <param name="initRequest">The init request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        bool ValidateInitializationRequest(IInitRequest initRequest, out List<string> errorMessages);

        /// <summary>
        /// Validates the device request.
        /// </summary>
        /// <param name="deviceValidRequest">The device valid request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        bool ValidateDeviceRequest(IDeviceValidityRequest deviceValidRequest, out List<string> errorMessages);
        
        /// <summary>
        /// Validates the batch spectrum query request.
        /// </summary>
        /// <param name="spectrumRequest">The spectrum request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        bool ValidateBatchSpectrumQueryRequest(BatchAvailableSpectrumRequestBase spectrumRequest, out List<string> errorMessages);

        /// <summary>
        /// Validates the paws device.
        /// </summary>
        /// <param name="deviceId">The device identifier.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        bool ValidatePawsDevice(string deviceId);
    }
}
