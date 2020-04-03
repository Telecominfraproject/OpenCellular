// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System.Linq;

    /// <summary>
    /// Struct DTT Data
    /// </summary>
    public struct DttData
    {
        /// <summary>
        /// Gets or sets the easting.
        /// </summary>
        /// <value>The easting.</value>
        public int Easting { get; set; }

        /// <summary>
        /// Gets or sets the northing.
        /// </summary>
        /// <value>The northing.</value>
        public int Northing { get; set; }

        /// <summary>
        /// Gets or sets the data values.
        /// </summary>
        /// <value>The data values.</value>
        public int[] DataValues { get; set; }
    }
}
