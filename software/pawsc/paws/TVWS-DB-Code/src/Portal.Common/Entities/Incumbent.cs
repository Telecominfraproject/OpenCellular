// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common
{
    using System.Collections.Generic;
    using Microsoft.Whitespace.Entities;

    public class Incumbent
    {
        public Incumbent(string callSign, int channel, List<Position> contourPoints, Event events, IncumbentType incumbentType, Location receiveLocation, Location transmitLocation)
        {
            this.CallSign = callSign;
            this.Channel = channel;
            this.ContourPoints = contourPoints;
            this.Type = incumbentType;
            this.Events = events;
            this.ReceiveLocation = receiveLocation;
            this.TransmitLocation = transmitLocation;
        }

        public string CallSign { get; private set; }

        public int Channel { get; private set; }

        public List<Position> ContourPoints { get; private set; }

        public Event Events { get; private set; }

        public IncumbentType Type { get; private set; }

        public Location ReceiveLocation { get; private set; }

        public Location TransmitLocation { get; private set; }

        public string IncumbentType
        {
            get
            {
                return this.Type.ToString();
            }
        }

        // TODO: Add missing fields, if any.
    }
}
