// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using Microsoft.WindowsAzure.Storage.Table;

    /// <summary>
    /// Represents a Device Registration Map.
    /// </summary>
    public class DeviceRegistrationMap : TableEntity
    {
        /// <summary>
        /// Gets or sets the Device Type of the request.
        /// </summary>
        public string DeviceType { get; set; }

        /// <summary>
        /// Gets or sets the RegistrationType of the request.
        /// </summary>
        public string RegistrationType { get; set; }
    }
}
