// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common
{
    using System.Collections.Generic;
    using Microsoft.WhiteSpaces.Common.Entities;

    public interface IRegionSource
    {
        List<Region> GetAvailableRegions();
    }
}
