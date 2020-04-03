// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common.Entities
{
    public class DeviceType
    {
        public DeviceType(string friendlyName, string type)
        {
            this.FriendlyName = friendlyName;
            this.Type = type;
        }

        public string FriendlyName { get; private set; }

        public string Type { get; private set; }
    }
}
