// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    /// <summary>
    /// Represents Class ServiceCacheRequestParameters.
    /// </summary>
    public class ServiceCacheRequestParameters
    {
        /// <summary>
        /// Gets or sets the search area.
        /// </summary>
        /// <value>The search area.</value>
        public SquareArea SearchArea { get; set; }

        /// <summary>
        /// Gets or sets the call sign.
        /// </summary>
        /// <value>The call sign.</value>
        public string CallSign { get; set; }

        /// <summary>
        /// Gets or sets the VSD service.
        /// </summary>
        /// <value>The VSD service.</value>
        public string VsdService { get; set; }

        /// <summary>
        /// Gets or sets the easting.
        /// </summary>
        /// <value>The easting.</value>
        public double Easting { get; set; }

        /// <summary>
        /// Gets or sets the northing.
        /// </summary>
        /// <value>The northing.</value>
        public double Northing { get; set; }
    }
}
