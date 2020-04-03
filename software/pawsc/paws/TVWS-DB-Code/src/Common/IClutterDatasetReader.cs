// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common
{
    /// <summary>
    /// Interface IClutterDatasetReader
    /// </summary>
    public interface IClutterDatasetReader
    {
        /// <summary>
        ///     Calculates the elevation.
        /// </summary>
        /// <param name="easting">The easting.</param>
        /// <param name="northing">The northing.</param>
        /// <returns>returns elevation.</returns>
        int CalculateClutter(double easting, double northing);
    }
}
