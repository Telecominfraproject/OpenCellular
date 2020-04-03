// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using Microsoft.Whitespace.Entities.Versitcard;
    using Microsoft.WindowsAzure.Storage.Table;

    /// <summary>
    /// Represents a RegisteredDevice class.
    /// </summary>
    public class RegisteredDevice : TableEntity
    {
        /// <summary>
        /// Gets or sets the FCC id.
        /// </summary>
        /// <value>The Device id.</value>
        public string DeviceId { get; set; }
    }
}
