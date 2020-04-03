// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common
{
    using System.Collections.Generic;
    using Entities;

    /// <summary>
    /// Interface IRegionManagementValidator
    /// </summary>
    public interface IRegionManagementValidator
    {
        /// <summary>
        /// Validates the region management request.
        /// </summary>
        /// <param name="parameters">The request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        bool ValidateAddIncumbentRequestMVPD(Parameters parameters, out List<string> errorMessages);

        /// <summary>
        /// Validates the region management request.
        /// </summary>
        /// <param name="parameters">The request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        bool ValidateAddIncumbentRequestTBAS(Parameters parameters, out List<string> errorMessages);

        /// <summary>
        /// Validates the region management request.
        /// </summary>
        /// <param name="parameters">The request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        bool ValidateAddIncumbentRequestLPAux(Parameters parameters, out List<string> errorMessages);

        /// <summary>
        /// Validates the region management request.
        /// </summary>
        /// <param name="parameters">The request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        bool ValidateDeleteIncumbentRequest(Parameters parameters, out List<string> errorMessages);

        /// <summary>
        /// Validates the region management request.
        /// </summary>
        /// <param name="parameters">The request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        bool ValidateRequiredIncumbentType(Parameters parameters, out List<string> errorMessages);

        /// <summary>
        /// Validates the get incumbents request.
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        bool ValidateGetIncumbentsRequest(Parameters parameters, out List<string> errorMessages);

        /// <summary>
        /// Validates the region management request.
        /// </summary>
        /// <param name="parameters">The request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        bool ValidateExcludeChannels(Parameters parameters, out List<string> errorMessages);

        /// <summary>
        /// Validates the region management request.
        /// </summary>
        /// <param name="parameters">The request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        bool ValidateExcludeIds(Parameters parameters, out List<string> errorMessages);

        /// <summary>
        /// Validates the region management request.
        /// </summary>
        /// <param name="parameters">The request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        bool ValidateGetDeviceList(Parameters parameters, out List<string> errorMessages);

        /// <summary>
        /// Validates the region management request.
        /// </summary>
        /// <param name="parameters">The request.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise</returns>
        bool ValidateGetChannelListRequest(Parameters parameters, out List<string> errorMessages);

        /// <summary>
        /// Validates the register device request unlicensed LPAUX.
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        bool ValidateRegisterDeviceRequestUnlicensedLPAux(Parameters parameters, out List<string> errorMessages);

        /// <summary>
        /// Validates the get contour data request.
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        bool ValidateGetContourDataRequest(Parameters parameters, out List<string> errorMessages);
    }
}
