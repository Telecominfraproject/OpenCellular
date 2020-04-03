// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    /// <summary>Represents Area.
    /// </summary>
    public abstract class Area
    {
        /// <summary>
        /// The centre point
        /// </summary>
        private Location centrePoint;

        /// <summary>
        /// Initializes a new instance of the <see cref="Area"/> class.
        /// </summary>
        protected Area()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Area" /> class.
        /// </summary>
        /// <param name="centrePointLocation">The centre point location.</param>
        protected Area(Location centrePointLocation)
        {
            this.centrePoint = centrePointLocation;
        }

        /// <summary>
        /// Gets the centre point.
        /// </summary>
        /// <value>The centre point.</value>
        public Location CentrePoint
        {
            get { return this.centrePoint; }
        }
    }
}
