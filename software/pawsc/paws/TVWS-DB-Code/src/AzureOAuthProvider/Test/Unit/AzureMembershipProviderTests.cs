// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.AzureOAuthProvider.Test.Unit
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Web.Security;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.WhiteSpaces.AzureOAuthProvider.Fakes;
    using Microsoft.WhiteSpaces.Test.Common;
    using Microsoft.WhiteSpaces.AzureTableAccess;

    /// <summary>
    /// Test cases for AzureMembershipProvider class methods
    /// </summary>
    [TestClass]
    public class AzureMembershipProviderTests
    {
        /// <summary>
        /// StubIUserManager reference variable
        /// </summary>
        private readonly StubIUserManager userManager;

        /// <summary>
        /// AzureMembershipProvider reference variable
        /// </summary>
        private readonly AzureMembershipProvider azureMembershipProvider;

        /// <summary>
        /// Initializes a new instance of the <see cref="AzureMembershipProviderTests"/> class
        /// </summary>
        public AzureMembershipProviderTests()
        {
            this.userManager = new StubIUserManager();
            this.azureMembershipProvider = new AzureMembershipProvider(this.userManager);
        }

        /// <summary>
        /// Test <GetUserIdFromOAuth/> which gets user id for the given provider and provider user id
        /// </summary>
        [TestMethod]
        public void GetUserIdFromOAuth_ForValidProvider_ReturnsUserId()
        {
            WebpagesOauthMembership webpagesOauthMembership = CommonData.GetOAuthMembershipData().ElementAt(0);  
            this.userManager.GetMembershipFromProviderStringString = (partitionKey, rowKey) => webpagesOauthMembership;

            int actualUserId = this.azureMembershipProvider.GetUserIdFromOAuth(webpagesOauthMembership.Provider, webpagesOauthMembership.UserId.ToString());

            Assert.AreEqual(webpagesOauthMembership.UserId, actualUserId);
        }

        /// <summary>
        /// Test <GetUserIdFromOAuth/> when null is returned from azure table
        /// </summary>
        [TestMethod]
        public void GetUserIdFromAuth_WhenThereIsNoDataInWebpagesOauthMembershipTable_ReturnsZero()
        {
            WebpagesOauthMembership webpagesOauthMembership = CommonData.GetOAuthMembershipData().ElementAt(0);
            this.userManager.GetMembershipFromProviderStringString = (partitionKey, rowKey) => null;

            int actualUserId = this.azureMembershipProvider.GetUserIdFromOAuth(webpagesOauthMembership.Provider, webpagesOauthMembership.UserId.ToString());

            Assert.AreEqual(0, actualUserId);
        }

        /// <summary>
        /// Test GetUserNameFromId
        /// </summary>
        [TestMethod]
        public void GetUserNameFromId_ForValidUserId_ReturnsUserName()
        {
            UserProfile userProfile = CommonData.GetUserProfiles().ElementAt(0);
            this.userManager.GetUserProfileFromUserIdString = (a) => userProfile;

            string actualUserName = this.azureMembershipProvider.GetUserNameFromId(int.Parse(userProfile.RowKey));

            Assert.AreEqual(userProfile.UserName, actualUserName);
        }

        /// <summary>
        /// Test GetUserNameFromId when user id is zero
        /// </summary>
        [TestMethod]
        public void GetUserNameFromId_WhenUserIdIsZero_ReturnsEmptyUserName()
        {
            UserProfile userProfile = CommonData.GetUserProfiles().ElementAt(0);
            this.userManager.GetUserProfileFromUserIdString = (a) => userProfile;

            string userName = this.azureMembershipProvider.GetUserNameFromId(0);

            ////UserName should be empty when userId is zero
            Assert.IsTrue(string.IsNullOrEmpty(userName), "UserName is not empty when userId is zero");
        }

        /// <summary>
        /// GetUser returns user membership data for a correct user
        /// </summary>
        [TestMethod]
        public void GetUser_ForValidUser_ReturnsMembershipData()
        {
            List<UserProfile> userProfiles = CommonData.GetUserProfiles();
            List<WebpagesOauthMembership> allMembershipData = CommonData.GetOAuthMembershipData();

            string providerUserKey = allMembershipData.ElementAt(0) != null ? allMembershipData.ElementAt(0).RowKey : null;
            MembershipUser expectedMembershipUserData = new MembershipUser(
                    allMembershipData.ElementAt(0).Provider, 
                    userProfiles.ElementAt(0).UserName, 
                    providerUserKey, 
                    userProfiles.ElementAt(0).PreferredEmail, 
                    string.Empty, 
                    string.Empty, 
                    true, 
                    false, 
                    userProfiles.ElementAt(0).Timestamp.LocalDateTime, 
                    DateTime.MinValue, 
                    DateTime.MinValue, 
                    DateTime.MinValue, 
                    DateTime.MinValue);

            this.userManager.GetAllUserProfiles = () => userProfiles;
            this.userManager.GetMemshipDataofAllUsers = () => allMembershipData;

            MembershipUser actualMembershipUserData = this.azureMembershipProvider.GetUser(userProfiles.ElementAt(0).UserName, false);

            CustomAssert.AreEqualByProperties(expectedMembershipUserData, actualMembershipUserData, CommonConstants.MembershipParametersName);
        }
    }
}
