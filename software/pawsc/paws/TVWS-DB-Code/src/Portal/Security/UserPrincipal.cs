// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Portal
{    
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Security.Principal;
    using System.Web;
    using Microsoft.Practices.Unity;
    using Microsoft.WhiteSpaces.AzureOAuthProvider;
    using Microsoft.WhiteSpaces.Common;
    using WebMatrix.WebData;

    public class UserPrincipal : IPrincipal
    {
        public UserPrincipal(string accessToken, string userName, bool isSuperAdmin, bool isRegionAdmin)
        {
            Check.IsNotNull(accessToken, "AccessToken");
            Check.IsNotNull(userName, "UserName");

            this.AccessToken = accessToken;
            this.UserName = userName;
            this.IsSuperAdmin = isSuperAdmin;
            this.IsRegionAdmin = isRegionAdmin;
            this.Identity = new GenericIdentity(userName);

            this.UserManager = ConfigHelper.CurrentContainer.Resolve<IUserManager>();
        }

        public string AccessToken { get; set; }

        public string UserName { get; set; }

        public bool IsSuperAdmin { get; set; }

        public bool IsRegionAdmin { get; set; }

        [Dependency]
        public IUserManager UserManager { get; set; }

        public IIdentity Identity { get; private set; }        

        public bool IsInRole(string role)
        {
            throw new NotImplementedException();
        }

        public bool IsInRole(AccessLevels accessLevel, Authorities authority)
        {
            UserDetails userInfo = this.UserManager.GetUserDetailsByAccessToken(this.AccessToken);

            return userInfo.AccessInfo.Any(x => (x.AccessLevel == accessLevel) && (x.Authority == authority));
        }
    }
}
