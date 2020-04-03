// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.AzureOAuthProvider.Test.Integration
{
    using System.Collections.Generic;
    using System.Configuration;
    using System.Linq;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.WindowsAzure.ServiceRuntime;
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Table;
    using WhiteSpaces.Common;
    using WhiteSpaces.Test.Common;
    using Microsoft.WhiteSpaces.AzureTableAccess;

    /// <summary>
    /// Integration Test cases for UserManager class methods
    /// </summary>
    [TestClass]
    public class UserManagerTests
    {
        /// <summary>
        /// variable to hold <see cref="CloudStorageAccount"/> value
        /// </summary>
        private static readonly CloudStorageAccount CloudStorageAccount;

        /// <summary>
        /// variable to hold <see cref="CloudTableClient"/> value
        /// </summary>
        private static readonly CloudTableClient CloudTableClient;

        /// <summary>
        /// variable to hold <see cref="IAzureTableOperation"/> value
        /// </summary>
        private static readonly IAzureTableOperation AzureTableOperation;

        /// <summary>
        /// variable to hold <see cref="UserManager"/> value
        /// </summary>
        private static readonly UserManager UserManager;

        /// <summary>
        /// variable to hold <see cref="CloudTable"/> value
        /// </summary>
        private static CloudTable cloudTable;

        /// <summary>
        /// variable to hold <see cref="UserProfile"/> collection
        /// </summary>
        private static List<UserProfile> userProfileEntities;

        /// <summary>
        /// variable to hold <see cref="WebpagesOauthMembership"/> collection
        /// </summary>
        private static List<WebpagesOauthMembership> webpagesOauthMembershipEntities;

        /// <summary>
        /// variable to hold <see cref="AuthorityAccess"/> collection
        /// </summary>
        private static List<AuthorityAccess> authorityAccessEntities;

        /// <summary>
        /// Initializes static members of the<see cref="UserManagerTests"/> class
        /// </summary>
        static UserManagerTests()
        {
            CloudStorageAccount = CloudStorageAccount.Parse(StorageConnectionString);
            CloudTableClient = CloudStorageAccount.CreateCloudTableClient();
            AzureTableOperation = new AzureTableOperation();
            UserManager = new UserManager();
        }

        /// <summary>
        /// Gets storage connection string value from configuration
        /// </summary>
        public static string StorageConnectionString
        {
            get
            {
                return RoleEnvironment.IsAvailable
                            ? RoleEnvironment.GetConfigurationSettingValue("AzureStorageAccountConnectionString")
                            : ConfigurationManager.ConnectionStrings["AzureStorageAccountConnectionString"].ConnectionString;
            }
        }

        /// <summary>
        /// Class set up method
        /// </summary>
        /// <param name="context">Test context</param>
        [ClassInitialize]
        public static void InitializeClass(TestContext context)
        {
            userProfileEntities = CommonData.GetUserProfiles();
            webpagesOauthMembershipEntities = CommonData.GetOAuthMembershipData();
            authorityAccessEntities = CommonData.GetAuthorityAccessData();
            
            foreach (UserProfile userProfileEntity in userProfileEntities)
            {
                AzureTableOperation.InsertEntity(userProfileEntity);
            }

            foreach (WebpagesOauthMembership webpagesOauthMembershipEntity in webpagesOauthMembershipEntities)
            {
                AzureTableOperation.InsertEntity(webpagesOauthMembershipEntity);
            }

            foreach (AuthorityAccess authorityAccessEntity in authorityAccessEntities)
            {
                AzureTableOperation.InsertEntity(authorityAccessEntity);
            }
        }

        /// <summary>
        /// Class clean up method
        /// </summary>
        [ClassCleanup]
        public static void CleanClass()
        {
            foreach (AuthorityAccess authorityAccessEntity in authorityAccessEntities)
            {
                DeleteEntity(Constants.AuthorityAccessTable, authorityAccessEntity);
            }

            foreach (WebpagesOauthMembership webpagesOauthMembershipEntity in webpagesOauthMembershipEntities)
            {
                DeleteEntity(Constants.WebpagesOauthMembershipTable, webpagesOauthMembershipEntity);
            }

            foreach (UserProfile userProfileEntity in userProfileEntities)
            {
                DeleteEntity(Constants.UserProfileTable, userProfileEntity);
            }
        }

        /// <summary>
        /// Test GetUserProfileFromUserId method on providing valid user id
        /// </summary>
        [TestMethod]
        public void GetUserProfileFromUserId_ForValidUserId_ReturnsCorrectUserProfile()
        {
            UserProfile actualUserProfile = UserManager.GetUserProfileFromUserId(userProfileEntities.ElementAt(0).RowKey);
            CustomAssert.AreEqualByProperties(userProfileEntities.ElementAt(0), actualUserProfile, CommonConstants.UserInfoParamList);
        }

        /// <summary>
        /// Test GetUserProfileFromUserId method on providing invalid user id
        /// </summary>
        [TestMethod]
        public void GetUserProfileFromUserId_ForInvalidUserId_ReturnsNull()
        {
            UserProfile actualUserProfile = UserManager.GetUserProfileFromUserId(CommonConstants.InvalidUserId);
            Assert.IsNull(actualUserProfile);
        }

        /// <summary>
        /// Test GetUserProfileFromProvider method on providing correct provider name
        /// </summary>
        [TestMethod]
        public void GetUserProfileFromProvider_ForCorrectProvider_ReturnsCorrectUserProfile()
        {
            UserProfile actualUserProfile = UserManager.GetUserProfileFromProvider(CommonConstants.Provider, userProfileEntities.ElementAt(0).RowKey);
            CustomAssert.AreEqualByProperties(userProfileEntities.ElementAt(0), actualUserProfile, CommonConstants.UserInfoParamList);
        }

        /// <summary>
        /// Test GetUserProfileFromProvider method on providing incorrect provider name
        /// </summary>
        [TestMethod]
        public void GetUserProfileFromProvider_ForIncorrectProvider_ReturnsNull()
        {
            UserProfile actualUserProfile = UserManager.GetUserProfileFromProvider(CommonConstants.InvalidProvider, userProfileEntities.ElementAt(0).RowKey);
            Assert.IsNull(actualUserProfile);
        }

        /// <summary>
        /// Test GetAllUserProfiles method
        /// </summary>
        [TestMethod]
        public void GetAllUserProfiles()
        {
            IEnumerable<UserProfile> actualUserProfileList = UserManager.GetAllUserProfiles();
            Assert.IsTrue(actualUserProfileList.Count() >= userProfileEntities.Count());
        }

        /// <summary>
        /// Test GetMembershipDataOfAllUsers method
        /// </summary>
        [TestMethod]
        public void GetMemshipDataofAllUsers()
        {
            IEnumerable<WebpagesOauthMembership> actualWebpagesOauthMembershipList = UserManager.GetMemshipDataofAllUsers();
            Assert.IsTrue(actualWebpagesOauthMembershipList.Count() >= webpagesOauthMembershipEntities.Count());
        }

        /// <summary>
        /// Test GetAllAuthorities method 
        /// </summary>
        [TestMethod]
        public void GetAllAuthorities()
        {
            IEnumerable<Authority> authorityList = UserManager.GetAllAuthorities();
            Assert.IsTrue(CommonData.GetListOfAllAuthorities().Count() == authorityList.Count(), "Expected and actual authorities count is not equal");
            
            for (int i = 0; i < authorityList.Count(); i++)
            {
                Assert.AreEqual(CommonData.GetListOfAllAuthorities().ElementAt(i).AuthorityName, authorityList.ElementAt(i).AuthorityName);
            }
        }

        /// <summary>
        /// Test GetAllAccessLevels method 
        /// </summary>
        [TestMethod]
        public void GetAllAccessLevels()
        {
            IEnumerable<AccessLevel> accessLevelList = UserManager.GetAllAccessLevels();
            Assert.IsTrue(CommonData.GetListOfAllAccessLevels().Count() == accessLevelList.Count(), "Expected and actual authorities count is not equal");
            
            for (int i = 0; i < accessLevelList.Count(); i++)
            {
                Assert.AreEqual(CommonData.GetListOfAllAccessLevels().ElementAt(i).AccessLevelName, accessLevelList.ElementAt(i).AccessLevelName);
            }
        }

        /// <summary>
        /// Test GetUserIdByName method for valid user name
        /// </summary>
        [TestMethod]
        public void GetUserIdByName_ForValidUserName_ReturnsCorrectUserId()
        {
            int actualUserId = UserManager.GetUserIdByName(userProfileEntities.ElementAt(0).UserName);
            Assert.AreEqual(int.Parse(userProfileEntities.ElementAt(0).RowKey), actualUserId);
        }

        /// <summary>
        /// Test GetUserIdByName method for invalid user name
        /// </summary>
        [TestMethod]
        public void GetUserIdByName_ForInvalidUserName_ReturnsZero()
        {
            int actualUserId = UserManager.GetUserIdByName(CommonConstants.InvalidUser);
            Assert.AreEqual(0, actualUserId);
        }

        /// <summary>
        /// Test GetUserAccessDetails method 
        /// </summary>
        [TestMethod]
        public void GetUserAccessDetails()
        {
            IEnumerable<AccessDetails> actualAccessDetails = UserManager.GetUserAccessDetails(userProfileEntities.ElementAt(0).RowKey);     
            
            for (int i = 0; i < actualAccessDetails.Count(); i++)
            {
                Assert.AreEqual(CommonData.GetAccessDetails().ElementAt(i).AccessLevel, actualAccessDetails.ElementAt(i).AccessLevel);
                Assert.AreEqual(CommonData.GetAccessDetails().ElementAt(i).Authority, actualAccessDetails.ElementAt(i).Authority);
                Assert.AreEqual(CommonData.GetAccessDetails().ElementAt(i).RequestedAccessLevel, actualAccessDetails.ElementAt(i).RequestedAccessLevel);
            }
        }

        /// <summary>
        /// Test GetUserAccessDetails method 
        /// </summary>
        [TestMethod]
        public void GetUserAccessDetails_ForInvalidUserId_ReturnsCountZero()
        {
            IEnumerable<AccessDetails> actualAccessDetails = UserManager.GetUserAccessDetails(CommonConstants.InvalidUserId);
            Assert.AreEqual(0, actualAccessDetails.Count());
        }

        /// <summary>
        /// Test GetAccessLevelForAuthority method for valid user
        /// </summary>
        [TestMethod]
        public void GetAccessLevelForAuthority_ForValidUser_ReturnsAccessLevel()
        {
            AuthorityAccess actualAuthorityAccessData = UserManager.GetAccessLevelForAuthority(authorityAccessEntities.ElementAt(0).PartitionKey, authorityAccessEntities.ElementAt(0).RowKey);

            Assert.AreEqual(authorityAccessEntities.ElementAt(0).AccessLevel, actualAuthorityAccessData.AccessLevel);
        }

        /// <summary>
        /// Test GetUserAndAccessDetailsByUserId method for valid user id
        /// </summary>
        [TestMethod]
        public void GetUserAndAccessDetailsByUserId_ForValidUserId_ReturnsCorrectUserDetails()
        {
            List<UserDetails> userDetailsList = CommonData.GetUserDetails();
            UserDetails actualUserDetails = UserManager.GetUserAndAccessDetailsByUserId(userProfileEntities.ElementAt(0).RowKey);

            CustomAssert.AreEqualByProperties(userDetailsList.ElementAt(0).UserInfo, actualUserDetails.UserInfo, CommonConstants.UserInfoParamList);
            
            for (int i = 0; i < actualUserDetails.AccessInfo.Count(); i++)
            {
                CustomAssert.AreEqualByProperties(userDetailsList.ElementAt(0).AccessInfo.ElementAt(i), actualUserDetails.AccessInfo.ElementAt(i), CommonConstants.AccessInfoParamList);
            }
        }

        /// <summary>
        /// Test GetUserAndAccessDetailsByUserId method for invalid user id
        /// </summary>
        [TestMethod]
        public void GetUserAndAccessDetailsByUserId_ForInvalidUserId_ReturnsNull()
        {
            List<UserDetails> userDetailsList = CommonData.GetUserDetails();
            UserDetails actualUserDetails = UserManager.GetUserAndAccessDetailsByUserId(CommonConstants.InvalidUserId);

            Assert.IsNull(actualUserDetails);
        }

        /// <summary>
        /// Test GetMembershipFromProvider method on providing correct provider name
        /// </summary>
        [TestMethod]
        public void GetMembershipFromProvider_ForCorrectProvider_ReturnsCorrectMembershipData()
        {
            WebpagesOauthMembership expectedWebpagesOauthMembershipData = CommonData.GetOAuthMembershipData().ElementAt(0);

            WebpagesOauthMembership actualWebpagesOauthMembershipData = UserManager.GetMembershipFromProvider(expectedWebpagesOauthMembershipData.Provider, expectedWebpagesOauthMembershipData.UserId.ToString());

            CustomAssert.AreEqualByProperties(expectedWebpagesOauthMembershipData, actualWebpagesOauthMembershipData, CommonConstants.MembershipInfoParamList);
        }

        /// <summary>
        /// Test GetMembershipFromProvider method on providing incorrect provider name
        /// </summary>
        [TestMethod]
        public void GetMembershipFromProvider_ForIncorrectProvider_ReturnsNull()
        {
            WebpagesOauthMembership expectedWebpagesOauthMembershipData = CommonData.GetOAuthMembershipData().ElementAt(0);

            WebpagesOauthMembership actualWebpagesOauthMembershipData = UserManager.GetMembershipFromProvider(CommonConstants.InvalidProvider, CommonConstants.InvalidProviderId);

            Assert.IsNull(actualWebpagesOauthMembershipData, "Membership data should be NULL for incorrect provider");
        }

        /// <summary>
        /// Test GetUserDetailsByProviderUserId method for valid provider id
        /// </summary>
        [TestMethod]
        public void GetUserDetailsByProviderUserId_ForValidProviderId_ReturnsCorrectUserDetails()
        {
            WebpagesOauthMembership webpagesOauthMembershipData = CommonData.GetOAuthMembershipData().ElementAt(0);
            List<UserDetails> userDetailsWithoutMembershipInfo = CommonData.GetUserDetails();

            foreach (UserDetails detail in userDetailsWithoutMembershipInfo)
            {
                detail.MembershipInfo = webpagesOauthMembershipData;
            }

            UserDetails expectedUserDetails = userDetailsWithoutMembershipInfo.ElementAt(0);
            UserDetails actualUserDetails = UserManager.GetUserDetailsByProviderUserId(expectedUserDetails.MembershipInfo.UserId.ToString());

            CustomAssert.AreEqualByProperties(expectedUserDetails.UserInfo, actualUserDetails.UserInfo, CommonConstants.UserInfoParamList);
            CustomAssert.AreEqualByProperties(expectedUserDetails.MembershipInfo, actualUserDetails.MembershipInfo, CommonConstants.MembershipInfoParamList);
            
            for (int i = 0; i < actualUserDetails.AccessInfo.Count(); i++)
            {
                CustomAssert.AreEqualByProperties(expectedUserDetails.AccessInfo.ElementAt(i), actualUserDetails.AccessInfo.ElementAt(i), CommonConstants.AccessInfoParamList);
            }
        }

        /// <summary>
        /// Test GetUserDetailsByProviderUserId method for invalid provider id
        /// </summary>
        [TestMethod]
        public void GetUserDetailsByProviderUserId_ForInalidProviderId_ReturnsNull()
        {
            WebpagesOauthMembership webpagesOauthMembershipData = CommonData.GetOAuthMembershipData().ElementAt(0);
            List<UserDetails> userDetailsWithoutMembershipInfo = CommonData.GetUserDetails();

            foreach (UserDetails detail in userDetailsWithoutMembershipInfo)
            {
                detail.MembershipInfo = webpagesOauthMembershipData;
            }

            UserDetails expectedUserDetails = userDetailsWithoutMembershipInfo.ElementAt(0);

            UserDetails actualUserDetails = UserManager.GetUserDetailsByProviderUserId(CommonConstants.InvalidProviderId);

            Assert.IsNull(actualUserDetails);
        }

        /// <summary>
        /// Deletes an entity from an azure table
        /// </summary>
        /// <param name="tableName">Name of the azure table</param>
        /// <param name="entity">entity that needs to be deleted</param>
        private static void DeleteEntity(string tableName, TableEntity entity)
        {
            cloudTable = CloudTableClient.GetTableReference(tableName);
            TableOperation deleteOperation = TableOperation.Delete(entity);
            cloudTable.Execute(deleteOperation);
        }
    }
}
