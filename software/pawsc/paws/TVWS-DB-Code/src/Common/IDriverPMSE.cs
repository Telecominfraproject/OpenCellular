// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using Microsoft.Whitespace.Entities;

    /// <summary>
    /// Represents the driver's interface used by the PMSE web service for handling event registrations.
    /// </summary>
    public interface IDriverPMSE
    {
        /// <summary>
        /// Registered the specified device.
        /// </summary>
        /// <param name="parameters">Registration parameters</param>
        /// <param name="userId">user id</param>
        /// <returns>PMSE response object</returns>
        PMSEResponse RegisterProtectedDevice(Parameters parameters, string userId);

        /// <summary>
        /// Cancels the specified registration.
        /// </summary>
        /// <param name="parameters">Id of the registration.</param>
        /// <returns>PMSE response object</returns>
        PMSEResponse CancelRegistration(Parameters parameters);

        /// <summary>
        /// Retrieves the specified registration.
        /// </summary>
        /// <param name="id">Id of the registration.</param>
        /// <returns>PMSE response object</returns>
        PMSEResponse GetRegistration(string id);

        /// <summary>
        /// Returns an array of all the PMSE registrations.
        /// </summary>
        /// <returns>PMSE response object</returns>
        PMSEResponse GetRegistration();
    }
}
