// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.AzureOAuthProvider
{    
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using Microsoft.WhiteSpaces.AzureTableAccess;

    public class UserDetails
    {
        public UserProfile UserInfo { get; set; }

        public WebpagesOauthMembership MembershipInfo { get; set; }

        public IEnumerable<AccessDetails> AccessInfo { get; set; }
    }
}
