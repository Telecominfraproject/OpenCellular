// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System.Collections;
    using System.Runtime.Serialization;

    /// <summary>
    /// Represents Class OSGLocation.
    /// </summary>
    public struct OSGLocation
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="OSGLocation" /> struct.
        /// </summary>
        /// <param name="easting">The easting.</param>
        /// <param name="northing">The northing.</param>
        public OSGLocation(int easting, int northing)
            : this()
        {
            this.Easting = easting;
            this.Northing = northing;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="OSGLocation"/> struct.
        /// </summary>
        /// <param name="easting">The easting.</param>
        /// <param name="northing">The northing.</param>
        public OSGLocation(double easting, double northing)
            : this()
        {
            this.OriginalEasting = easting;
            this.OriginalNorthing = northing;
        }

        /// <summary>
        /// Gets or sets the easting.
        /// </summary>
        /// <value>The easting.</value>
        public int Easting
        {
            get { return (int)this.OriginalEasting; }

            set { this.OriginalEasting = value; }
        }

        /// <summary>
        /// Gets or sets the northing.
        /// </summary>
        /// <value>The northing.</value>
        public int Northing
        {
            get { return (int)this.OriginalNorthing; }

            set { this.OriginalNorthing = value; }
        }

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
        /// To the OSG location string.
        /// </summary>
        /// <returns>returns System.String.</returns>
        public string ToOSGLocationString()
        {
            return string.Format("{0}, {1}", this.Easting, this.Northing);
        }

        #region Overrides of ValueType

        /// <summary>
        /// Returns a hash code for this instance.
        /// </summary>
        /// <returns>A hash code for this instance, suitable for use in hashing algorithms and data structures like a hash table.</returns>
        public override int GetHashCode()
        {
            return (this.Easting ^ this.Northing).GetHashCode();
        }

        #endregion
    }
}
