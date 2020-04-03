// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Entities;

    /// <summary>
    /// Represents the PMSE interface into the data access layer component.
    /// </summary>
    public interface IDalcPMSE
    {
        /// <summary>
        /// Returns all of the LP-AUX Registrations.
        /// </summary>
        /// <returns>Returns an List of LP-AUX unlicensed Registrations</returns>
        List<LPAuxRegistration> GetLPAuxRegistrations();

        /// <summary>
        /// Returns LP-AUX Registrations for the given id.
        /// </summary>
        /// <param name="id">LP-AUX Registration ID</param>
        /// <returns>Returns LP-AUX unlicensed registration List.</returns>
        List<LPAuxRegistration> GetLPAuxRegistration(string id);

        /// <summary>
        /// Save the LP-AUX unlicensed Registration
        /// </summary>
        /// <param name="unlicensedPointAuxRegistration"> Register LP Aux Unlicensed Registration.</param>
        void RegisterDevice(LPAuxRegistration unlicensedPointAuxRegistration);

        /// <summary>
        /// cancel the LP-AUX unlicensed registration
        /// </summary>
        /// <param name="id">registration id</param>
        void CancelRegistrations(string id);
    }
}
