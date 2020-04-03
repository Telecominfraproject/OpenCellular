// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common.Entities
{
    public class WhitespaceChannelsInfo
    {
        public WhitespaceChannelsInfo(int startChannel, int endChannel, int channelBandwidth, string propagationModel, double powerDBmTanisitionPoint)
        {
            this.StartChannel = startChannel;
            this.EndChannel = endChannel;
            this.ChannelBandwidth = channelBandwidth;
            this.PropagationModel = propagationModel;
            this.PowerDBmTransitionPoint = powerDBmTanisitionPoint;
        }

        public int StartChannel { get; private set; }

        public int EndChannel { get; private set; }

        public int ChannelBandwidth { get; private set; }

        public string PropagationModel { get; private set; }

        public double PowerDBmTransitionPoint { get; private set; }
    }
}
