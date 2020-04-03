// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace ConsoleTesting
{
    using System.Collections.Generic;
    using Microsoft.Whitespace.Entities;

    /// <summary>
    ///     Represents Class DistinctTupleCoordinates.
    /// </summary>
    public class DistinctDTTQueryParams : IEqualityComparer<DTTQueryParams>
    {
        #region Implementation of IEqualityComparer<in OSGLocation>

        /// <summary>
        ///     Determines whether the specified objects are equal.
        /// </summary>
        /// <param name="x">The first object of type <paramref name="T" /> to compare.</param>
        /// <param name="y">The second object of type <paramref name="T" /> to compare.</param>
        /// <returns>true if the specified objects are equal; otherwise, false.</returns>
        public bool Equals(DTTQueryParams x, DTTQueryParams y)
        {
            return x.Easting == y.Easting && x.Northing == y.Northing;
        }

        /// <summary>
        ///     Returns a hash code for this instance.
        /// </summary>
        /// <param name="obj">The <see cref="T:System.Object" /> for which a hash code is to be returned.</param>
        /// <returns>A hash code for this instance, suitable for use in hashing algorithms and data structures like a hash table.</returns>
        public int GetHashCode(DTTQueryParams obj)
        {
            return (obj.Easting ^ obj.Northing).GetHashCode();
        }

        #endregion
    }
}
