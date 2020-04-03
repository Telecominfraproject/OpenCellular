// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Portal
{
    using System.Collections.Generic;
    using Microsoft.WhiteSpaces.AzureOAuthProvider;
    using Microsoft.WhiteSpaces.Common;

    public class UserProfileModel : RegisterExternalLoginModel
    {
        public List<AccessDetails> AccessInfo { get; set; }

        public string UserId { get; set; }

        public List<AccessRequest> ElevationRequests { get; set; }
    }
}
