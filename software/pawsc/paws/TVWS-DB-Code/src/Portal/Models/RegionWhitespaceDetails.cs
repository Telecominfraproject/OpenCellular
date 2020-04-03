// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Portal.Models
{
    using System.Collections.Generic;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WhiteSpaces.Common.Entities;
    using INCU = Microsoft.WhiteSpaces.Common.Incumbent;

    public class RegionWhitespaceDetails
    {
        public double? AntennaHeight { get; set; }

        public List<ChannelInformation> Channels { get; set; }

        public Dictionary<string, ChannelInfo[]> AvailableChannelsLookup { get; set; }

        public List<INCU> OccupiedChannes { get; set; }

        public string Location { get; set; }

        public bool EnableProtectedAreas { get; set; }

        public RegionChannelsDetail RegionChannelsDetail { get; set; }

        public WhitespaceAnalysisResult AnalysisResult { get; set; }

        public string SelectedIncumbentType { get; set; }

        public List<DeviceType> DeviceTypes { get; set; }

        public int StartChannelNo { get; set; }

        public int EndChannelNo { get; set; }

        public double PowerDBmTransitionPoint { get; set; }
    }
}
