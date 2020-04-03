// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Portal.Models
{
    using System.Collections.Generic;
    using Microsoft.Whitespace.Entities;

    public class IncumbentViewModel
    {
        public string CallSign { get; set; }

        public int ChannelId { get; set; }

        public List<Position> ContourPoints { get; set; }

        public Event Event { get; set; }

        public string IncumbentType { get; set; }

        public Position ReceviceLocation { get; set; }

        public Position TransmitLocation { get; set; }

        // TODO: Add missing incumbent field, if any.
    }
}
