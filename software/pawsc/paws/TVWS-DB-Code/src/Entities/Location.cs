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
    /// Represents an incumbent location.
    /// </summary>
    public class Location : TableEntity
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="Location"/> class.
        /// </summary>
        public Location()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Location"/> class.
        /// </summary>
        /// <param name="latitude">The latitude.</param>
        /// <param name="longitude">The longitude.</param>
        public Location(double latitude, double longitude)
        {
            this.Latitude = latitude;
            this.Longitude = longitude;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Location"/> class.
        /// </summary>
        /// <param name="latitude">The latitude.</param>
        /// <param name="longitude">The longitude.</param>
        /// <param name="semiMajorAxis">The semi major axis.</param>
        /// <param name="semiMinorAxis">The semi minor axis.</param>
        public Location(double latitude, double longitude, double semiMajorAxis, double semiMinorAxis)
        {
            this.Latitude = latitude;
            this.Longitude = longitude;
            this.SemiMajorAxis = semiMajorAxis;
            this.SemiMinorAxis = semiMinorAxis;
        }

        /// <summary>
        /// Gets or sets the latitude position.
        /// </summary>
        public double Latitude { get; set; }

        /// <summary>
        /// Gets or sets the longitude position.
        /// </summary>
        public double Longitude { get; set; }

        /// <summary>
        /// Gets or sets the semi major axis.
        /// </summary>
        /// <value>The semi major axis.</value>
        public double SemiMajorAxis { get; set; }

        /// <summary>
        /// Gets or sets the semi minor axis.
        /// </summary>
        /// <value>The semi minor axis.</value>
        public double SemiMinorAxis { get; set; }

        /// <summary>
        /// Gets or sets the Datum.
        /// </summary>
        public string Datum { get; set; }

        /// <summary>
        /// Gets or sets the Radiation Center.
        /// </summary>
        public RadiationCenter RadiationCenter { get; set; }

        /// <summary>
        /// Returns a <see cref="System.String" /> that represents this instance.
        /// </summary>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder();
            stringBuilder.Append(this.Latitude);
            stringBuilder.Append(this.Latitude >= 0 ? 'N' : 'S');
            stringBuilder.Append(';');
            stringBuilder.Append(this.Longitude);
            stringBuilder.Append(this.Longitude >= 0 ? 'E' : 'W');
            stringBuilder.Append(';');
            return ((object)stringBuilder).ToString();
        }

        /// <summary>
        /// To the location string.
        /// </summary>
        /// <returns>returns System.String.</returns>
        public string ToLocationString()
        {
            StringBuilder stringBuilder = new StringBuilder();
            stringBuilder.Append(this.Latitude);
            stringBuilder.Append(",");
            stringBuilder.Append(this.Longitude);
            return stringBuilder.ToString();
        }
    }
}
