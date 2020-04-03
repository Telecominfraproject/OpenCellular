// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Portal.Models
{
    public class RegionChannelsDetail
    {
        public string RegionName { get; set; }

        public double Latitude { get; set; }

        public double Longitude { get; set; }

        public int LowPowerChannelsCount { get; set; }

        public int HighPowerChannelsCount { get; set; }

        public int UnAvailableChannelsCount { get; set; }

        public string LatitudeDirection
        {
            get
            {
                if (this.Latitude > 0)
                {
                    return "N";
                }
                else if (this.Latitude < 0)
                {
                    return "S";
                }
                else
                {
                    return "Equator";
                }
            }
        }

        public string LongitudeDirection
        {
            get
            {
                if (this.Longitude > 0)
                {
                    return "E";
                }
                else if (this.Longitude < 0)
                {
                    return "W";
                }
                else
                {
                    return "Prime Meridian";
                }
            }
        }
    }
}
