// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;

    /// <summary>
    /// Represents the Radial HAT.
    /// </summary>
    public class RadialHAT
    {
        /// <summary>
        /// Gets or sets the azimuth radial HAAT.
        /// </summary>
        /// <value>The azimuth radial HAAT.</value>
        public Dictionary<int, double> AzimuthRadialHaats { get; set; }
    }
}
