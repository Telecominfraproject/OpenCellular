// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;

    public class LocationRect
    {
        public LocationRect(Location northwest, Location southeast)
        {
            if (northwest == null)
            {
                throw new ArgumentNullException("northwest");
            }

            if (southeast == null)
            {
                throw new ArgumentNullException("southeast");
            }

            this.North = northwest.Latitude;
            this.West = northwest.Longitude;
            this.South = southeast.Latitude;
            this.East = southeast.Longitude;

            this.Northwest = northwest;
            this.Southeast = southeast;
            this.Northeast = new Location(northwest.Latitude, southeast.Longitude);
            this.Southwest = new Location(southeast.Latitude, northwest.Longitude);
        }

        public LocationRect(double north, double west, double south, double east)
            : this(new Location(north, west), new Location(south, east))
        {
        }

        public Location Center
        {
            get
            {
                double latitude = (this.North + this.South) / 2;
                double longitude = (this.West + this.East) / 2;

                return new Location(latitude, longitude);
            }
        }

        public double North { get; private set; }

        public double West { get; private set; }

        public double South { get; private set; }

        public double East { get; private set; }

        public Location Northeast { get; private set; }

        public Location Northwest { get; private set; }

        public Location Southeast { get; private set; }

        public Location Southwest { get; private set; }

        /// <summary>
        /// Finds a point lies inside the Bounding box or not.
        /// </summary>
        /// <param name="latitude">The latitude.</param>
        /// <param name="longitude">The longitude.</param>
        /// <returns>Boolean values indicating a point lies inside Bounding box.</returns>
        public bool Contains(double latitude, double longitude)
        {
            return (latitude <= this.North) &&
                   (latitude >= this.South) &&
                   (longitude >= this.West) &&
                   (longitude <= this.East);
        }

        /// <summary>
        /// Finds a point lies inside the Bounding box or not.
        /// </summary>
        /// <param name="location">The Location object.</param>
        /// <returns>Boolean values indicating a point lies inside Bounding box.</returns>
        public bool Contains(Location location)
        {
            if (location == null)
            {
                throw new ArgumentNullException("location");
            }

            return this.Contains(location.Latitude, location.Longitude);
        }
    }
}
