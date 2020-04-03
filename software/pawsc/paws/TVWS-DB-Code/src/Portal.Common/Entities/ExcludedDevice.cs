// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;

    public class ExcludedDevice
    {
        public string RecordPartitionKey { get; set; }

        public string RecordRowKey { get; set; }

        public string DeviceId { get; set; }

        public string SerialNumber { get; set; }

        public string RegionCode { get; set; }

        public string Region { get; set; }
    }
}
