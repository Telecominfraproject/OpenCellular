// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common
{
    public class UserRegulatoryAccess
    {
        public string UserId { get; set; }

        public string Regulatory { get; set; }

        public int CurrentAccessLevel { get; set; }

        public int RequestedAccessLevel { get; set; }

        public string Justification { get; set; }
    }
}
