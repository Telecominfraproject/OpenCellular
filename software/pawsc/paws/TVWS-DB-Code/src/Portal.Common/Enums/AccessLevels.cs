// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;

    public enum AccessLevels
    {
        None = 0,
        PortalUser = 1,
        DeviceVendor = 2,
        Licensee = 3,
        Admin = 4,
        SuperAdmin = 5
    }
}
