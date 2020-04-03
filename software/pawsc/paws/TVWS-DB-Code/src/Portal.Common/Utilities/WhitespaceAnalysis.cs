// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common.Utilities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WhiteSpaces.Common.Entities;

    public class WhitespaceAnalysis
    {
        public static WhitespaceSummary AnalyseWhitespaceSummary(IEnumerable<ChannelInfo> freeChannels, string deviceType, int startChannelId, int endChannelId)
        {
            if (string.IsNullOrWhiteSpace(deviceType))
            {
                throw new ArgumentException("deviceType");
            }

            if (freeChannels == null)
            {
                return new WhitespaceSummary(deviceType, 0, 0, 0, 0);
            }

            List<int> widthList = new List<int>();
            int startChannel = -1;
            int endChannel = 0;
            int minContiguousWidth = 0;
            int medianContiguousWidth = 0;
            int maxContiguousWidth = 0;

            for (int channelId = startChannelId; channelId <= endChannelId; channelId++)
            {
                if (!freeChannels.Any(channel => channel.ChannelId == channelId))
                {
                    if (startChannel != -1)
                    {
                        endChannel = channelId;

                        int width = endChannel - startChannel;
                        widthList.Add(width);

                        startChannel = -1;
                    }
                }
                else
                {
                    if (startChannel == -1)
                    {
                        startChannel = channelId;
                    }
                }
            }

            if (startChannel != -1)
            {
                widthList.Add(endChannelId - startChannel);
            }

            widthList.Sort();

            if (widthList.Any())
            {
                minContiguousWidth = widthList[0];
                medianContiguousWidth = widthList[widthList.Count / 2];
                maxContiguousWidth = widthList[widthList.Count - 1];
            }

            return new WhitespaceSummary(deviceType, freeChannels.Count(), maxContiguousWidth, medianContiguousWidth, minContiguousWidth);
        }
    }
}
