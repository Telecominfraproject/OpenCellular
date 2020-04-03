// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common.Entities
{
    public class WhitespaceSummary
    {
        public WhitespaceSummary(string deviceType, int totalFreeChannels, int maxContiguousWidth, int medianContiguousWidth, int minContiguousWidth)
        {
            this.DeviceType = deviceType;
            this.TotalFreeChannels = totalFreeChannels;
            this.MaxContiguousWidth = maxContiguousWidth;
            this.MedianContiguousWidth = medianContiguousWidth;
            this.MinContiguousWidth = minContiguousWidth;
        }

        public string DeviceType { get; private set; }

        public int TotalFreeChannels { get; private set; }

        public int MaxContiguousWidth { get; private set; }

        public int MedianContiguousWidth { get; private set; }

        public int MinContiguousWidth { get; private set; }
    }
}
