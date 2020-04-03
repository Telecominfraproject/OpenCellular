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
    /// Represents Initialized Device Details.
    /// </summary>
public class InitializedDeviceDetails : TableEntity
    {
        /// <summary>
        /// Gets or sets the ModelId.
        /// </summary>
        public string ModelId { get; set; }

        /// <summary>
        /// Gets or sets the RuleSetIds.
        /// </summary>
        public string RuleSetIds { get; set; }

        /// <summary>
        /// Gets or sets the Latitude.
        /// </summary>
        public string Latitude { get; set; }

        /// <summary>
        /// Gets or sets the Longitude.
        /// </summary>
        public string Longitude { get; set; }

        /// <summary>
        /// Gets or sets the LocationJson.
        /// </summary>
        public string Location { get; set; }

        /// <summary>
        /// Gets or sets the DeviceDescriptorJson.
        /// </summary>
        public string DeviceDescriptor { get; set; }

        /// <summary>
        /// Gets or sets the WSDBA.
        /// </summary>
        public string WSDBA { get; set; }
    }
}
