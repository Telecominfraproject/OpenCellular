// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    /// <summary>
    /// Represents Class TVStationInfo.
    /// </summary>
    public class MVPDCallSignsInfo
    {
        /// <summary>
        ///     Gets or sets the VSD_SERVICE
        /// </summary>
        /// <value>The VSD_SERVICE.</value>
        public string ServiceType { get; set; }

        /// <summary>
        ///     Gets or sets the channel.
        /// </summary>
        /// <value>The channel.</value>
        public int Channel { get; set; }

        /// <summary>
        ///     Gets or sets the latitude.
        /// </summary>
        /// <value>The latitude.</value>
        public double Latitude { get; set; }

        /// <summary>
        ///     Gets or sets the longitude.
        /// </summary>
        /// <value>The longitude.</value>
        public double Longitude { get; set; }

        /// <summary>Gets or sets the call sign.</summary>
        /// <value>The call sign.</value>
        public string CallSign { get; set; }

        /// <summary>
        /// Gets or sets the contour.
        /// </summary>
        /// <value>The contour.</value>
        public string Contour { get; set; }
    }
}
