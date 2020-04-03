// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using Microsoft.Whitespace.Entities;

    /// <summary>
    /// Interface used for calculating elevation at a particular point on the globe.
    /// </summary>
    public interface ITerrainElevation
    {
        /// <summary>
        /// Calculates the terrain elevation at the specified position.
        /// </summary>
        /// <param name="position">Position used for calculating terrain elevation.</param>
        /// <returns>Returns the elevation level.</returns>
        Distance CalculateElevation(Location position);

        /// <summary>
        /// Calculates the elevation.
        /// </summary>
        /// <param name="easting">The easting.</param>
        /// <param name="northing">The northing.</param>
        /// <returns>returns value at specified easting, northing</returns>
        double CalculateElevation(double easting, double northing);
    }
}
