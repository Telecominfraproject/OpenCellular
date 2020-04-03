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
    /// Represents the Contour of an incumbent.
    /// </summary>
    public class ContourDetailsInfo : Contour
    {
        /// <summary>
        /// Gets or sets the contour station data.
        /// </summary>
        /// <value>The contour station data.</value>
        public Incumbent ContourStationData { get; set; }
    }
}
