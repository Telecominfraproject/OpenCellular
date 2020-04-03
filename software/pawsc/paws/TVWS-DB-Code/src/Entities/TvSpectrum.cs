// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using System.Xml.Serialization;

    /// <summary>
    /// Represents a TV Spectrum.
    /// </summary>
    [XmlInclude(typeof(TvSpectrum))]
    public class TvSpectrum
    {
        /// <summary>
        /// Gets or sets the channel of the spectrum.
        /// </summary>
        public int? Channel { get; set; }

        /// <summary>
        /// Gets or sets the call sign of the spectrum.
        /// </summary>
        public string CallSign { get; set; }
    }
}
