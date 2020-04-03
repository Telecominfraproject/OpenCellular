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
    /// Represents the PMSE Validator interface.
    /// </summary>
    public interface IPMSEValidator
    {
        /// <summary>
        /// validated the LP-Aux unlicensed device registration
        /// </summary>
        /// <param name="parameters">parameters object</param>
        /// <param name="errList"> List of validation error</param>
        /// <returns> boolean value</returns>
        bool IsProtectedDeviceValid(Parameters parameters, out List<string> errList);
    }
}
