// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.AzureOAuthProvider.Test.Unit
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Linq;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.WhiteSpaces.Common;
    using WhiteSpaces.AzureOAuthProvider.Fakes;
    using WhiteSpaces.Test.Common;
    using Microsoft.WhiteSpaces.AzureTableAccess.Fakes;
    using Microsoft.WhiteSpaces.AzureTableAccess;

    /// <summary>
    /// Unit Test cases for UserManager class methods
    /// </summary>
    [TestClass]
    public class UserManagerTests
    {
        /// <summary>
        /// StubIAzureTableOperation reference variable
        /// </summary>
        private readonly StubIAzureTableOperation azureTableOperation;

        /// <summary>
        /// UserManager reference variable
        /// </summary>
        private readonly UserManager userManager;

        /// <summary>
        /// Initializes a new instance of the <see cref="UserManagerTests"/> class
        /// </summary>
        public UserManagerTests()
        {
            this.azureTableOperation = new StubIAzureTableOperation();
            this.userManager = new UserManager(this.azureTableOperation);
        }

        /// <summary>
        /// Test GetMembershipFromProvider method on providing correct provider name
        /// </summary>
        [TestMethod]
        public void GetMembershipFromProvider_ForCorrectProvider_ReturnsCorrectMembershipData()
        {
            WebpagesOauthMembership expectedMembershipData = CommonData.GetOAuthMembershipData().ElementAt(0);
            this.azureTableOperation.FetchEntityOf1StringStringString<WebpagesOauthMembership>((partitionKey, rowKey, tableName) => expectedMembershipData);

            WebpagesOauthMembership actualMembershipData = this.userManager.GetMembershipFromProvider(expectedMembershipData.Provider, expectedMembershipData.UserId.ToString());

            CustomAssert.AreEqualByProperties(expectedMembershipData, actualMembershipData, CommonConstants.MembershipInfoParamList);
        }

        /// <summary>
        /// Test GetMembershipFromProvider method on providing incorrect provider name
        /// </summary>
        [TestMethod]
        public void GetMembershipFromProvider_ForIncorrectProvider_ReturnsNull()
        {
            WebpagesOauthMembership membershipData = CommonData.GetOAuthMembershipData().ElementAt(0);
            this.azureTableOperation.FetchEntityOf1StringStringString<WebpagesOauthMembership>((partitionKey, rowKey, tableName) => membershipData);

            WebpagesOauthMembership actualMembershipData = this.userManager.GetMembershipFromProvider(CommonConstants.InvalidProvider, membershipData.UserId.ToString());

            Assert.IsNull(actualMembershipData, "Membership data should be NULL for incorrect provider");
        }

        /// <summary>
        /// Test GetMembershipFromProvider method on providing incorrect provider name
        /// </summary>
        [TestMethod]
        public void GetMembershipFromProvider_WhenThereIsNoDataInWebpagesOauthMembershipTable_ReturnsNull()
        {
            WebpagesOauthMembership membershipData = CommonData.GetOAuthMembershipData().ElementAt(0);
            this.azureTableOperation.FetchEntityOf1StringStringString<WebpagesOauthMembership>((partitionKey, rowKey, tableName) => null);

            WebpagesOauthMembership webpagesOauthMembershipData = this.userManager.GetMembershipFromProvider(membershipData.Provider, membershipData.UserId.ToString());

            Assert.IsNull(webpagesOauthMembershipData);
        }

        /// <summary>
        /// Test GetUserProfileFromUserId method on providing valid user id
        /// </summary>
        [TestMethod]
        public void GetUserProfileFromUserId_ForValidUserId_ReturnsUserProfile()
        {
            UserProfile userProfile = CommonData.GetUserProfiles().ElementAt(0);
            this.azureTableOperation.FetchEntityOf1StringStringString<UserProfile>((partitionKey, rowKey, tableName) => userProfile);

            UserProfile actualUserProfile = this.userManager.GetUserProfileFromUserId(userProfile.RowKey);

            CustomAssert.AreEqualByProperties(userProfile, actualUserProfile, CommonConstants.UserInfoParamList);
        }

        /// <summary>
        /// Test GetUserProfileFromUserId method on providing invalid user id
        /// </summary>
        [TestMethod]
        public void GetUserProfileFromUserId_ForInValidUserId_ReturnsNull()
        {
            this.azureTableOperation.FetchEntityOf1StringStringString<UserProfile>((partitionKey, rowKey, tableName) => null);

            UserProfile actualUserProfile = this.userManager.GetUserProfileFromUserId(CommonConstants.InvalidUserId);

            Assert.IsNull(actualUserProfile, "For invalid user id, UserProfile should be null");
        }

        /// <summary>
        /// Test GetUserProfileFromProvider method on providing correct provider name
        /// </summary>
        [TestMethod]
        public void GetUserProfileFromProvider_ForCorrectProvider_ReturnsCorrectUserProfile()
        {
            UserProfile expectedUserProfile = CommonData.GetUserProfiles().ElementAt(0);
            WebpagesOauthMembership membershipData = CommonData.GetOAuthMembershipData().ElementAt(0);
            this.azureTableOperation.FetchEntityOf1StringStringString<WebpagesOauthMembership>((partitionKey, rowKey, tableName) => membershipData);
            this.azureTableOperation.FetchEntityOf1StringStringString<UserProfile>((partitionKey, rowKey, tableName) => expectedUserProfile);

            UserProfile actualUserProfile = this.userManager.GetUserProfileFromProvider(membershipData.Provider, membershipData.UserId.ToString());

            CustomAssert.AreEqualByProperties(expectedUserProfile, actualUserProfile, CommonConstants.UserInfoParamList);
        }

        /// <summary>
        /// Test GetUserProfileFromProvider method on providing incorrect provider name
        /// </summary>
        [TestMethod]
        public void GetUserProfileFromProvider_ForIncorrectProvider_ReturnsNull()
        {
            UserProfile expectedUserProfile = CommonData.GetUserProfiles().ElementAt(0);
            WebpagesOauthMembership membershipData = CommonData.GetOAuthMembershipData().ElementAt(0);
            this.azureTableOperation.FetchEntityOf1StringStringString<WebpagesOauthMembership>((partitionKey, rowKey, tableName) => membershipData);
            this.azureTableOperation.FetchEntityOf1StringStringString<UserProfile>((partitionKey, rowKey, tableName) => expectedUserProfile);

            UserProfile actualUserProfile = this.userManager.GetUserProfileFromProvider(CommonConstants.InvalidProvider, membershipData.UserId.ToString());

            Assert.IsNull(actualUserProfile, "UserProfile data should be null for incorrect provider");
        }

        /// <summary>
        /// Test GetAllUserProfiles method
        /// </summary>
        [TestMethod]
        public void GetAllUserProfiles()
        {
            List<UserProfile> expectedUserProfileList = CommonData.GetUserProfiles();            
            this.azureTableOperation.GetEntityByPartitionKeyOf1String<UserProfile>((partitionKey) => expectedUserProfileList);

            IEnumerable<UserProfile> actualUserProfileList = this.userManager.GetAllUserProfiles();

            List<UserProfile> result = actualUserProfileList.Except(expectedUserProfileList).ToList();

            Assert.IsTrue(result.Count == 0);
        }

        /// <summary>
        /// Test GetAllUserProfiles method when there is no data in UserProfile table
        /// </summary>
        [TestMethod]
        public void GetAllUserProfiles_WhenThereIsNoDataInUserProfileTable_ReturnsNull()
        {
            this.azureTableOperation.GetEntityByPartitionKeyOf1String<UserProfile>((partitionKey) => null);

            IEnumerable<UserProfile> userProfileList = this.userManager.GetAllUserProfiles();

            Assert.IsNull(userProfileList);
        }

        /// <summary>
        /// Test GetMembershipDataOfAllUsers method
        /// </summary>
        [TestMethod]
        public void GetMemshipDataofAllUsers()
        {
            List<WebpagesOauthMembership> expectedMembershipDataOfAllUsers = CommonData.GetOAuthMembershipData();
            this.azureTableOperation.GetEntityByPartitionKeyOf1String<WebpagesOauthMembership>((partitionKey) => expectedMembershipDataOfAllUsers);

            IEnumerable<WebpagesOauthMembership> actualMembershipDataOfAllUsers = this.userManager.GetMemshipDataofAllUsers();

            List<WebpagesOauthMembership> result = actualMembershipDataOfAllUsers.Except(expectedMembershipDataOfAllUsers).ToList();

            Assert.IsTrue(result.Count == 0);
        }

        /// <summary>
        /// Test GetMembershipDataOfAllUsers method when there is no data in <WebpagesOauthMembership/> table
        /// </summary>
        [TestMethod]
        public void GetMemshipDataofAllUsers__WhenThereIsNoDataInWebpagesOauthMembershipTable_ReturnsNull()
        {
            this.azureTableOperation.GetEntityByPartitionKeyOf1String<WebpagesOauthMembership>((partitionKey) => null);

            IEnumerable<WebpagesOauthMembership> membershipDataOfAllUsers = this.userManager.GetMemshipDataofAllUsers();

            Assert.IsNull(membershipDataOfAllUsers);
        }

        /// <summary>
        /// Test GetAllAuthorities method 
        /// </summary>
        [TestMethod]
        public void GetAllAuthorities()
        {
            IEnumerable<Authority> expectedAuthorityList = CommonData.GetListOfAllAuthorities();
            this.azureTableOperation.GetEntityByPartitionKeyOf1String<Authority>((partitionKey) => expectedAuthorityList);

            IEnumerable<Authority> actualAuthorityList = this.userManager.GetAllAuthorities();

            List<Authority> result = actualAuthorityList.Except(expectedAuthorityList).ToList();

            Assert.IsTrue(result.Count == 0);
        }

        /// <summary>
        /// Test GetAllAuthorities method when there is no data in Authority table
        /// </summary>
        [TestMethod]
        public void GetAllAuthorities_WhenThereIsNoDataInAuthorityTable_ReturnsNull()
        {
            this.azureTableOperation.GetEntityByPartitionKeyOf1String<Authority>((partitionKey) => null);

            IEnumerable<Authority> authorityList = this.userManager.GetAllAuthorities();

            Assert.IsNull(authorityList);
        }

        /// <summary>
        /// Test GetAllAccessLevels method 
        /// </summary>
        [TestMethod]
        public void GetAllAccessLevels()
        {
            IEnumerable<AccessLevel> expectedAccessLevels = CommonData.GetListOfAllAccessLevels();
            this.azureTableOperation.GetEntityByPartitionKeyOf1String<AccessLevel>((partitionKey) => expectedAccessLevels);

            IEnumerable<AccessLevel> actualAccessLevels = this.userManager.GetAllAccessLevels();

            List<AccessLevel> result = actualAccessLevels.Except(expectedAccessLevels).ToList();

            Assert.IsTrue(result.Count == 0);
        }

        /// <summary>
        /// Test GetAllAccessLevels method when there is no data in AccessLevel table
        /// </summary>
        [TestMethod]
        public void GetAllAccessLevels_WhenThereIsNoDataInAccessLevelTable_ReturnsNull()
        {
            this.azureTableOperation.GetEntityByPartitionKeyOf1String<AccessLevel>((partitionKey) => null);

            IEnumerable<AccessLevel> accessLevelsList = this.userManager.GetAllAccessLevels();

            Assert.IsNull(accessLevelsList);
        }

        /// <summary>
        /// Test GetAccessLevelForAuthority method for valid user
        /// </summary>
        [TestMethod]
        public void GetAccessLevelForAuthority_ForValidUser_ReturnsAccessLevel()
        {
            AuthorityAccess expectedAuthorityAccessData = CommonData.GetAuthorityAccessData().ElementAt(0);
            this.azureTableOperation.FetchEntityOf1StringStringString<AuthorityAccess>((partitionKey, rowKey, tableName) => expectedAuthorityAccessData);

            AuthorityAccess actualAuthorityAccessData = this.userManager.GetAccessLevelForAuthority(expectedAuthorityAccessData.PartitionKey, expectedAuthorityAccessData.RowKey);

            Assert.AreEqual(expectedAuthorityAccessData.AccessLevel, actualAuthorityAccessData.AccessLevel);
        }

        /// <summary>
        /// Test GetUserAccessDetails method 
        /// </summary>
        [TestMethod]
        public void GetUserAccessDetails()
        {
            List<AuthorityAccess> authorityAccessData = CommonData.GetAuthorityAccessData();
            IEnumerable<AccessDetails> expectedAccessDetails = CommonData.GetAccessDetails();
            this.azureTableOperation.GetEntityByPartitionKeyOf1String<AuthorityAccess>((partitionKey) => authorityAccessData);

            List<AccessDetails> actualAccessDetails = this.userManager.GetUserAccessDetails(CommonData.GetAuthorityAccessData().ElementAt(0).PartitionKey).ToList();

            bool result = actualAccessDetails.Count() == expectedAccessDetails.Count();
            Assert.IsTrue(result, "expected user access details and actual user access details counts are not same");
            
            for (int i = 0; i < actualAccessDetails.Count(); i++)
            {
                Assert.AreEqual(expectedAccessDetails.ElementAt(i).AccessLevel, actualAccessDetails[i].AccessLevel);
                Assert.AreEqual(expectedAccessDetails.ElementAt(i).Authority, actualAccessDetails[i].Authority);
                Assert.AreEqual(expectedAccessDetails.ElementAt(i).RequestedAccessLevel, actualAccessDetails[i].RequestedAccessLevel);
            }
        }

        /// <summary>
        /// Test GetUserAccessDetails method when there is no data in AuthorityAccess table
        /// </summary>
        [TestMethod]
        public void GetUserAccessDetails_WhenThereIsNoDataInAuthorityAccessTable_ReturnsNull()
        {
            this.azureTableOperation.GetEntityByPartitionKeyOf1String<AuthorityAccess>((partitionKey) => null);

            IEnumerable<AccessDetails> accessDetails = this.userManager.GetUserAccessDetails(CommonData.GetAuthorityAccessData().ElementAt(0).PartitionKey);

            Assert.IsNull(accessDetails);
        }

        /// <summary>
        /// Test GetUserIdByName method for valid user name
        /// </summary>
        [TestMethod]
        public void GetUserIdByName_ForValidUserName_ReturnsCorrectUserId()
        {
            List<UserProfile> expectedUserProfileList = CommonData.GetUserProfiles();
            this.azureTableOperation.GetEntityByPartitionKeyOf1String<UserProfile>((partitionKey) => expectedUserProfileList);

            int actualUserId = this.userManager.GetUserIdByName(expectedUserProfileList.ElementAt(0).UserName);

            Assert.AreEqual(int.Parse(expectedUserProfileList.Find(item => item.UserName == expectedUserProfileList.ElementAt(0).UserName).RowKey), actualUserId);
        }

        /// <summary>
        /// Test GetUserIdByName method for invalid user name
        /// </summary>
        [TestMethod]
        public void GetUserIdByName_ForInvalidUserName_ReturnsZero()
        {
            List<UserProfile> expectedUserProfileList = CommonData.GetUserProfiles();
            this.azureTableOperation.GetEntityByPartitionKeyOf1String<UserProfile>((partitionKey) => expectedUserProfileList);

            int actualUserId = this.userManager.GetUserIdByName(CommonConstants.InvalidUser);

            Assert.AreEqual(0, actualUserId);
        }

        /// <summary>
        /// Test GetUserIdByName method when there is no data in User Profile table
        /// </summary>
        [TestMethod]
        public void GetUserIdByName_WhenThereIsNoDataInUserProfileTable_ReturnsZero()
        {
            UserProfile expectedUserProfile = CommonData.GetUserProfiles().ElementAt(0); 
            this.azureTableOperation.GetEntityByPartitionKeyOf1String<UserProfile>((partitionKey) => null);

            int actualUserId = this.userManager.GetUserIdByName(expectedUserProfile.UserName);

            Assert.AreEqual(0, actualUserId);
        }

        /// <summary>
        /// Test GetUserAndAccessDetailsByUserId method for valid user id
        /// </summary>
        [TestMethod]
        public void GetUserAndAccessDetailsByUserId_ForValidUserId_ReturnsCorrectUserDetails()
        {
            UserProfile userProfile = CommonData.GetUserProfiles().ElementAt(0);
            UserDetails expectedUserDetails = CommonData.GetUserDetails().ElementAt(0);
            List<AuthorityAccess> authorityAccessData = CommonData.GetAuthorityAccessData();

            this.azureTableOperation.FetchEntityOf1StringStringString<UserProfile>((partitionKey, rowKey, tableName) => userProfile);
            this.azureTableOperation.GetEntityByPartitionKeyOf1String<AuthorityAccess>((partitionKey) => authorityAccessData);

            UserDetails actualUserDetails = this.userManager.GetUserAndAccessDetailsByUserId(userProfile.RowKey);

            CustomAssert.AreEqualByProperties(expectedUserDetails.UserInfo, actualUserDetails.UserInfo, CommonConstants.UserInfoParamList);
            
            for (int i = 0; i < expectedUserDetails.AccessInfo.Count(); i++)
            {
                CustomAssert.AreEqualByProperties(expectedUserDetails.AccessInfo.ElementAt(i), actualUserDetails.AccessInfo.ElementAt(i), CommonConstants.AccessInfoParamList);
            }
        }

        /// <summary>
        /// Test GetUserAndAccessDetailsByUserId method for invalid user id
        /// </summary>
        [TestMethod]
        public void GetUserAndAccessDetailsByUserId_ForInvalidUserId_ReturnsNull()
        {
            this.azureTableOperation.FetchEntityOf1StringStringString<UserProfile>((partitionKey, rowKey, tableName) => null);

            UserDetails userDetails = this.userManager.GetUserAndAccessDetailsByUserId(CommonConstants.InvalidUserId);

            Assert.IsNull(userDetails);
        }

        /// <summary>
        /// Test GetUserDetailsByProviderUserId method for valid provider id
        /// </summary>
        [TestMethod]
        public void GetUserDetailsByProviderUserId_ForValidProviderId_ReturnsCorrectUserDetails()
        {
            WebpagesOauthMembership webpagesOauthMembershipData = CommonData.GetOAuthMembershipData().ElementAt(0);
            UserProfile userProfile = CommonData.GetUserProfiles().ElementAt(0);
            UserDetails userDetails = CommonData.GetUserDetails().ElementAt(0);
            userDetails.MembershipInfo = webpagesOauthMembershipData;

            List<AuthorityAccess> authorityAccessData = CommonData.GetAuthorityAccessData();

            this.azureTableOperation.FetchEntityOf1StringStringString<WebpagesOauthMembership>((partitionKey, rowKey, tableName) => webpagesOauthMembershipData);
            this.azureTableOperation.FetchEntityOf1StringStringString<UserProfile>((partitionKey, rowKey, tableName) => userProfile);
            this.azureTableOperation.GetEntityByPartitionKeyOf1String<AuthorityAccess>((partitionKey) => authorityAccessData);

            UserDetails actualUserDetails = this.userManager.GetUserDetailsByProviderUserId(userDetails.MembershipInfo.RowKey);

            CustomAssert.AreEqualByProperties(userDetails.UserInfo, actualUserDetails.UserInfo, CommonConstants.UserInfoParamList);
            CustomAssert.AreEqualByProperties(userDetails.MembershipInfo, actualUserDetails.MembershipInfo, CommonConstants.MembershipInfoParamList);
            
            for (int i = 0; i < userDetails.AccessInfo.Count(); i++)
            {
                CustomAssert.AreEqualByProperties(userDetails.AccessInfo.ElementAt(i), actualUserDetails.AccessInfo.ElementAt(i), CommonConstants.AccessInfoParamList);
            }
        }

        /// <summary>
        /// Test GetUserDetailsByProviderUserId method for invalid provider id
        /// </summary>
        [TestMethod]
        public void GetUserDetailsByProviderUserId_ForInalidProviderId_ReturnsNull()
        {
            this.azureTableOperation.FetchEntityOf1StringStringString<WebpagesOauthMembership>((partitionKey, rowKey, tableName) => null);

            UserDetails actualUserDetails = this.userManager.GetUserDetailsByProviderUserId(CommonConstants.InvalidProviderId);

            Assert.IsNull(actualUserDetails);
        }
    }
}
