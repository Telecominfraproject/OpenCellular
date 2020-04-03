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
    /// Represents the Paws Business Layer interface.
    /// </summary>
    public interface IPawsBL
    {
        /// <summary>
        /// Initializes Master Device
        /// </summary>
        /// <param name="parameters">parameters coming from master device</param>
        /// <returns>Returns the paws response.</returns>
        PawsResponse Initialize(Parameters parameters);

        /// <summary>
        /// Registers Master Device
        /// </summary>
        /// <param name="parameters">parameters coming from master device</param>
        /// <returns>Returns the paws response.</returns>
        PawsResponse Register(Parameters parameters);

        /// <summary>
        /// Master Device query for available spectrum
        /// </summary>
        /// <param name="parameters">parameters coming from master device</param>
        /// <returns>Returns the paws response.</returns>
        PawsResponse AvailableSpectrum(Parameters parameters);

        /// <summary>
        /// Master Device query for batch available spectrum
        /// </summary>
        /// <param name="parameters">parameters coming from master device</param>
        /// <returns>Returns the paws response.</returns>
        PawsResponse AvailableSpectrumBatch(Parameters parameters);

        /// <summary>
        /// Master Device query for Interference Query
        /// </summary>
        /// <param name="parameters">parameters coming from master device</param>
        /// <returns>Returns the paws response.</returns>
        PawsResponse InterferenceQuery(Parameters parameters);

        /// <summary>
        /// validates slave 
        /// </summary>
        /// <param name="parameters">parameters coming from master device</param>
        /// <returns>Returns the paws response.</returns>
        PawsResponse ValidateDevice(Parameters parameters);

        /// <summary>
        /// Mater device notify about spectrum usage
        /// </summary>
        /// <param name="parameters">parameters coming from master device</param>
        /// <returns>Returns the paws response.</returns>
        PawsResponse NotifySpectrumUsage(Parameters parameters);
    }
}
