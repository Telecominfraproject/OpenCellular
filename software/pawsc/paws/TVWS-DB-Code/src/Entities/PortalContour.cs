// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using Microsoft.WindowsAzure.Storage.Table;

    /// <summary>
    ///  Holds required incumbent contour information for Portal
    /// </summary>
    public class PortalContour : TableEntity
    {
        /// <summary>
        /// Incumbent Type
        /// </summary>
        public  int Type{get;set;}

        /// <summary>
        /// Operating Channel
        /// </summary>
        public int Channel { get; set; }

        /// <summary>
        /// Latitude of incumbent location
        /// </summary>
        public double Latitude { get; set; }

        /// <summary>
        /// Longitude of incumbent location
        /// </summary>
        public double Longitude { get; set; }

        /// <summary>
        /// Latitude of parent incumbent location
        /// </summary>
        public double ParentLatitude { get; set; }

        /// <summary>
        /// Longitude of parent incumbent location
        /// </summary>
        public double ParentLongitude { get; set; }

        /// <summary>
        /// Incumbent Callsign
        /// </summary>
        public string CallSign { get; set; }

        /// <summary>
        /// Incumbent Contours
        /// </summary>
        public string Contour { get; set; }
    }
}
