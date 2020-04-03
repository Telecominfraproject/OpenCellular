// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using Microsoft.WindowsAzure.Storage.Table;

    public class RegionPolygon : TableEntity
    {
        public double MinLatitude { get; set; }

        public double MinLongitude { get; set; }

        public double MaxLatitude { get; set; }

        public double MaxLongitude { get; set; }

        public string PolygonsUri { get; set; }

        public int PolygonsCount { get; set; }

        public string RegionName { get; set; }

        public string LocationRectangles { get; set; }
    }
}
