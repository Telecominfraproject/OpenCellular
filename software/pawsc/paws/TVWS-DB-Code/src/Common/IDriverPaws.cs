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
    /// Represents the driver's interface used by the Paws Manager for handling all paws request.
    /// </summary>
    public interface IDriverPaws
    {
        /// <summary>
        /// Saves the specified spectrum usage.
        /// </summary>
        /// <param name="notifyRequest">Contains the Paws spectrum usage notification request information.</param>
        /// <param name="deviceIndex">Contains the Device Index.</param>
        /// <returns>Spectrum Usage information.</returns>
        int NotifySpectrumUsage(INotifyRequest notifyRequest, out string errorMessage);

        /// <summary>
        /// Returns Device Information of the specified Id.
        /// </summary>
        /// <param name="id">The device Id.</param>
        /// <returns>Device Descriptor information.</returns>
        DeviceDescriptor GetDeviceInfo(string id);

        /// <summary>
        /// Returns all of the device descriptors.
        /// </summary>
        /// <returns>All of the device descriptors.</returns>
        DeviceDescriptor[] GetDevices();

        /// <summary>
        /// Returns response after registration.
        /// </summary>
        /// <param name="registerrequest">Registration request.</param>
        /// <returns>Response after Registration</returns>
        int Register(IRegisterRequest registerrequest);

        /// <summary>
        /// Returns response after initialization.
        /// </summary>
        /// <param name="initRequest">Init request.</param>
        /// <returns>Response after Initialization</returns>
        int Initialize(IInitRequest initRequest);

        /// <summary>
        /// Returns response after device validation
        /// </summary>
        /// <param name="deviceValidRequest">Device Validate Request</param>
        /// <param name="deviceIndex">Device Index</param>
        /// <returns>Response after Device Validation</returns>
        int ValidateDevice(IDeviceValidityRequest deviceValidRequest, int deviceIndex);

        /// <summary>
        /// Returns response after interference query
        /// </summary>
        /// <param name="interferenceRequest">Interference Query Request</param>
        /// <returns>Response after Device Validation</returns>
        int InterferenceQuery(IInterferenceQueryRequest interferenceRequest);

        /// <summary>
        /// Returns RulesSetInfo after initialization.
        /// </summary>   
        /// <param name="deviceDescriptor">Device Descriptor</param>
        /// <returns>Response after Initialization</returns>
        RulesetInfo[] GetRuleSetInfo(DeviceDescriptor deviceDescriptor);

        /// <summary>
        /// Returns Available Spectrums
        /// </summary>
        /// <param name="spectrumRequest">spectrum Request Query</param>
        /// <returns>Spectrum Schedules</returns>
        SpectrumSchedule[] GetAvailableSpectrum(IAvailableSpectrumRequest spectrumRequest);

        /// <summary>
        /// Gets the available batch spectrum.
        /// </summary>
        /// <param name="spectrumRequest">The spectrum request.</param>
        /// <returns>returns GeoSpectrumSchedule[][].</returns>
        GeoSpectrumSpec[] GetAvailableSpectrumBatch(IBatchAvailableSpectrumRequest spectrumRequest);
    }
}
