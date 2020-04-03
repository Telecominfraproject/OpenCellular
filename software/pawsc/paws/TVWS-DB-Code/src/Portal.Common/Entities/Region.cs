// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common.Entities
{
    using System.Collections.Generic;
    using System.IO;

    public class Region
    {
        public Region(RegionInfo regionInfo, RegulatoryBody regulatoryBody)
        {
            this.RegionInformation = regionInfo;
            this.Regulatory = regulatoryBody;
        }

        public RegionInfo RegionInformation { get; private set; }

        public RegulatoryBody Regulatory { get; private set; }
    }
}
