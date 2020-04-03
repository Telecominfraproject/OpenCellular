// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    /// <summary>
    ///     Represents Class DTTQueryParameters.
    /// </summary>
    public struct DTTQueryParams
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="DTTQueryParams"/> struct.
        /// </summary>
        /// <param name="easting">The easting.</param>
        /// <param name="northing">The northing.</param>
        public DTTQueryParams(int easting, int northing)
            : this()
        {
            this.Easting = easting;
            this.Northing = northing;
        }

        /// <summary>
        ///     Gets or sets the easting.
        /// </summary>
        /// <value>The easting.</value>
        public int Easting { get; set; }

        /// <summary>
        ///     Gets or sets the northing.
        /// </summary>
        /// <value>The northing.</value>
        public int Northing { get; set; }

        /// <summary>
        ///     Gets or sets the partition key.
        /// </summary>
        /// <value>The partition key.</value>
        public string PartitionKey { get; set; }
    }
}
