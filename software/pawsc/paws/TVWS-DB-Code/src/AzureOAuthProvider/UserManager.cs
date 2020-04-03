// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.AzureOAuthProvider
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Net.Http;
    using System.Net.Http.Headers;
    using System.Transactions;
    using Microsoft.WhiteSpaces.AzureTableAccess;
    using Microsoft.WhiteSpaces.Common;
    using Microsoft.WhiteSpaces.Common.Enums;

    public sealed class UserManager : IUserManager
    {
        private readonly IAzureTableOperation azureTableOperations;

        [Microsoft.Practices.Unity.InjectionConstructor]
        public UserManager()
            : this(new AzureTableOperation())
        { 
        }

        internal UserManager(IAzureTableOperation azureTableOperations)
        {
            this.azureTableOperations = azureTableOperations;
        }

        public UserDetails GetUserDetailsByProviderUserId(string providerUserId)
        {
            WebpagesOauthMembership membershipData = this.azureTableOperations.FetchEntity<WebpagesOauthMembership>("1", providerUserId);

            if (membershipData != null)
            {
                UserDetails userDetails = this.GetUserAndAccessDetailsByUserId(membershipData.UserId.ToString());

                if (userDetails != null)
                {
                    userDetails.MembershipInfo = membershipData;
                    return userDetails;
                }
            }

            return null;
        }

        public UserDetails GetUserDetailsByAccessToken(string accessToken)
        {
            var clientUserData = this.GetClientUserDataByAccessToken(accessToken);

            return this.GetUserDetailsByProviderUserId(clientUserData.ProviderUserId);
        }

        public WebpagesOauthMembership GetMembershipFromProvider(string provider, string providerId)
        {
            var membershipData = this.azureTableOperations.FetchEntity<WebpagesOauthMembership>("1", providerId);

            if (membershipData != null && string.Equals(provider, membershipData.Provider, StringComparison.OrdinalIgnoreCase))
            {
                return membershipData;
            }

            return null;
        }

        public UserDetails GetUserAndAccessDetailsByUserId(string userId)
        {
            UserProfile userInfo = this.azureTableOperations.FetchEntity<UserProfile>("1", userId);

            if (userInfo != null)
            {
                return new UserDetails
                {
                    UserInfo = userInfo,
                    AccessInfo = this.GetUserAccessDetails(userId)
                };
            }

            return null;
        }

        public AuthorityAccess GetAccessLevelForAuthority(string userId, string authorityId)
        {
            return this.azureTableOperations.FetchEntity<AuthorityAccess>(userId, authorityId);
        }

        public UserProfile GetUserProfileFromProvider(string provider, string providerUserId)
        {
            var membershipData = this.azureTableOperations.FetchEntity<WebpagesOauthMembership>("1", providerUserId);

            if (membershipData != null && string.Equals(provider, membershipData.Provider, StringComparison.OrdinalIgnoreCase))
            {
                return this.GetUserProfileFromUserId(membershipData.UserId.ToString());
            }

            return null;
        }

        public UserProfile GetUserProfileFromUserId(string userId)
        {
            return this.azureTableOperations.FetchEntity<UserProfile>("1", userId);
        }

        public ClientUserData GetClientUserDataByAccessToken(string accessToken)
        {
            // return GetClientDataAsyn(accessToken).Result;
            using (var client = new HttpClient())
            {
                client.BaseAddress = new Uri("https://apis.live.net/v5.0/");

                client.DefaultRequestHeaders.Accept.Clear();
                client.DefaultRequestHeaders.Accept.Add(new MediaTypeWithQualityHeaderValue("application/json"));

                // New code:
                string callingMethod = string.Format("me?access_token={0}", accessToken);
                HttpResponseMessage response = client.GetAsync(callingMethod).Result;
                if (response.IsSuccessStatusCode)
                {
                    string jsonUserData = response.Content.ReadAsStringAsync().Result;
                    ClientUserData userData = JsonHelper.DeserializeObject<ClientUserData>(jsonUserData);
                    return userData;
                }
                else
                {
                    string jsonErrorData = response.Content.ReadAsStringAsync().Result;
                    AuthenticationResponse errorData = JsonHelper.DeserializeObject<AuthenticationResponse>(jsonErrorData);
                    throw new AccessDeniedException(errorData.AuthenticationResponseError);
                }
            }
        }

        public int GetUserIdByName(string userName)
        {
            IEnumerable<UserProfile> userProfiles = this.GetAllUserProfiles();
            if (userProfiles != null)
            {
                var requiredProfile = userProfiles.Where(x => string.Equals(x.UserName, userName, StringComparison.OrdinalIgnoreCase)).FirstOrDefault();

                if (requiredProfile != null)
                {
                    return Convert.ToInt32(requiredProfile.RowKey);
                }
            }

            return 0;
        }

        public void SaveUserDetails(UserDetails userDetail)
        {
            if (userDetail != null)
            {
                using (TransactionScope scope = new TransactionScope())
                {
                    if (userDetail.UserInfo != null)
                    {
                        this.SaveUserProfile(userDetail.UserInfo);
                    }

                    if (userDetail.MembershipInfo != null)
                    {
                        this.SaveUserMemberShipData(userDetail.MembershipInfo);
                    }

                    foreach (var access in userDetail.AccessInfo)
                    {
                        this.SaveUserAccessLevel(
                            new AuthorityAccess
                            {
                                AccessLevel = (int)access.AccessLevel,
                                PartitionKey = userDetail.UserInfo.RowKey,
                                RowKey = ((int)access.Authority).ToString()
                            });
                    }
                }
            }
        }

        public void SaveUserProfile(UserProfile userInfo)
        {
            this.azureTableOperations.InsertEntity(userInfo);
        }

        public void SaveUserMemberShipData(WebpagesOauthMembership membershipData)
        {
            this.azureTableOperations.InsertEntity(membershipData);
        }

        public void SaveUserAccessLevel(AuthorityAccess access)
        {
            this.azureTableOperations.InsertEntity(access);
        }

        public void RequestAccessElevation(AccessElevationRequest request)
        {
            this.azureTableOperations.InsertEntity(request);
        }

        public IEnumerable<AccessLevel> GetAllAccessLevels()
        {
            return this.azureTableOperations.GetEntityByPartitionKey<AccessLevel>("1");
        }

        public IEnumerable<Authority> GetAllAuthorities()
        {
            return this.azureTableOperations.GetEntityByPartitionKey<Authority>("1");
        }

        public IEnumerable<AccessDetails> GetUserAccessDetails(string userId)
        {
            IEnumerable<AuthorityAccess> authorityAccess = this.azureTableOperations.GetEntityByPartitionKey<AuthorityAccess>(userId);
            List<AccessDetails> accessDetails = new List<AccessDetails>();

            if (authorityAccess != null)
            {
                foreach (var access in authorityAccess)
                {
                    accessDetails.Add(
                        new AccessDetails
                        {
                            Authority = (Authorities)System.Enum.Parse(typeof(Authorities), access.RowKey),
                            AccessLevel = (AccessLevels)Convert.ToInt16(access.AccessLevel),
                        });
                }

                return accessDetails;
            }

            return null;
        }

        public IEnumerable<UserProfile> GetAllUserProfiles()
        {
            return this.azureTableOperations.GetEntityByPartitionKey<UserProfile>("1");
        }

        public IEnumerable<WebpagesOauthMembership> GetMemshipDataofAllUsers()
        {
            return this.azureTableOperations.GetEntityByPartitionKey<WebpagesOauthMembership>("1");
        }   
    
        public IEnumerable<AccessElevationRequest> GetPendingRequestForUser(string userId)
        {
            return this.azureTableOperations.GetEntityByPartitionKey<AccessElevationRequest>(userId).Where(x => x.RequestStatus == (int)RequestStatus.Pending);
        }
    }
}
