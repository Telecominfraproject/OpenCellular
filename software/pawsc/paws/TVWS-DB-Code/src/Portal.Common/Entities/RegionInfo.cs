// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common.Entities
{
    public class RegionInfo
    {
        public RegionInfo(string name, string id, double latitude, double longitude, string imagePath)
        {
            this.Name = name;
            this.Id = id;
            this.Latitude = latitude;
            this.Longitude = longitude;
            this.ImagePath = imagePath;
        }

        public string Name { get; private set; }

        public string Id { get; private set; }

        public double Latitude { get; private set; }

        public double Longitude { get; private set; }

        public string ImagePath { get; private set; }
    }
}
