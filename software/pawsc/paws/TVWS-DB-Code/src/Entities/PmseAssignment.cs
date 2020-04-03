// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Web;
    using Microsoft.WindowsAzure.Storage.Table;

    /// <summary>
    /// Class PMSEAssignment.
    /// </summary>
    public class PmseAssignment : TableEntity
    {
        /// <summary>
        /// Gets or sets the Manufacturer Id.
        /// </summary>
        public string Assignment_ID { get; set; }

        /// <summary>
        /// Gets or sets the Manufacturer Id.
        /// </summary>
        public string Equipment_Type_ID { get; set; }

        /// <summary>
        /// Gets or sets the Manufacturer Id.
        /// </summary>
        public int Easting { get; set; }

        /// <summary>
        /// Gets or sets the Manufacturer Id.
        /// </summary>
        public int Northing { get; set; }

        /// <summary>
        /// Gets or sets the Manufacturer Id.
        /// </summary>
        public double AntennaHeightMetres { get; set; }

        /// <summary>
        /// Gets or sets the Manufacturer Id.
        /// </summary>
        public double FrequencyMHz { get; set; }

        /// <summary>
        /// Gets or sets the Manufacturer Id.
        /// </summary>
        public double BandwidthMHz { get; set; }

        /// <summary>
        /// Gets or sets the Manufacturer Id UTC format
        /// </summary>
        public string Start { get; set; }

        /// <summary>
        /// Gets or sets the Manufacturer Id.
        /// </summary>
        public string Finish { get; set; }

        /// <summary>
        /// Gets or sets the Manufacturer Id.
        /// </summary>
        public string SituationID { get; set; }

        /// <summary>
        /// Gets or sets the latitude.
        /// </summary>
        /// <value>The latitude.</value>
        public double Latitude { get; set; }

        /// <summary>
        /// Gets or sets the longitude.
        /// </summary>
        /// <value>The longitude.</value>
        public double Longitude { get; set; }

        /// <summary>
        /// Gets or sets the Manufacturer Id.
        /// </summary>
        public int Channel { get; set; }

        /// <summary>
        /// Gets or sets the clutter value.
        /// </summary>
        /// <value>The clutter value.</value>
        public int ClutterValue { get; set; }

        /// <summary>
        /// Gets or sets the original easting.
        /// </summary>
        /// <value>The original easting.</value>
        public double OriginalEasting { get; set; }

        /// <summary>
        /// Gets or sets the original northing.
        /// </summary>
        /// <value>The original northing.</value>
        public double OriginalNorthing { get; set; }

        /// <summary>
        /// Gets or sets the co-channel.
        /// </summary>
        /// <value>The co-channel.</value>
        public int CoChannel { get; set; }
    }
}
