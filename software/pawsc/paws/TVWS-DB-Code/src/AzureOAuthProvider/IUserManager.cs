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
    using Microsoft.WhiteSpaces.Common;

    public interface IUserManager
    {
        UserDetails GetUserDetailsByProviderUserId(string providerUserId);

        UserDetails GetUserDetailsByAccessToken(string accessToken);

        WebpagesOauthMembership GetMembershipFromProvider(string provider, string providerId);

        UserDetails GetUserAndAccessDetailsByUserId(string userId);        

        AuthorityAccess GetAccessLevelForAuthority(string userId, string authorityId);

        UserProfile GetUserProfileFromProvider(string provider, string providerUserId);

        UserProfile GetUserProfileFromUserId(string userId);

        ClientUserData GetClientUserDataByAccessToken(string accessToken);

        IEnumerable<AccessElevationRequest> GetPendingRequestForUser(string userId);
        
        int GetUserIdByName(string userName);

        void SaveUserDetails(UserDetails userDetail);

        void SaveUserProfile(UserProfile userInfo);

        void SaveUserMemberShipData(WebpagesOauthMembership membershipData);

        void SaveUserAccessLevel(AuthorityAccess access);

        void RequestAccessElevation(AccessElevationRequest request);    

        IEnumerable<AccessLevel> GetAllAccessLevels();

        IEnumerable<Authority> GetAllAuthorities();

        IEnumerable<UserProfile> GetAllUserProfiles();

        IEnumerable<WebpagesOauthMembership> GetMemshipDataofAllUsers();

        IEnumerable<AccessDetails> GetUserAccessDetails(string userId);
    }
}
