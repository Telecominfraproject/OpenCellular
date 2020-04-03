// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using Newtonsoft.Json;

    /// <summary>
    /// Generic interface for all the paws parameters.
    /// </summary>
    public interface IParameters : IInitRequest, IRegisterRequest, IAvailableSpectrumRequest, IBatchAvailableSpectrumRequest, INotifyRequest, IDeviceValidityRequest, IInterferenceQueryRequest
    {
        /// <summary>
        /// Gets or sets the parameter type.
        /// </summary>
        [JsonProperty(Constants.PropertyNameType)]
        string Type { get; set; }

        /// <summary>
        /// Gets or sets the parameter code.
        /// </summary>
        [JsonProperty(Constants.PawsPropertyNameCode)]
        string Code { get; set; }

        /// <summary>
        /// Gets or sets the parameter message.
        /// </summary>
        [JsonProperty(Constants.PropertyNameMessage)]
        string Message { get; set; }

        /// <summary>
        /// Gets or sets the parameter message.
        /// </summary>
        [JsonProperty(Constants.PropertyNameData)]
        string Data { get; set; }

        /// <summary>
        /// Gets or sets the parameter version.
        /// </summary>
        [JsonProperty(Constants.PropertyNameVersion)]
        string Version { get; set; }  
    }
}
