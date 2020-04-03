// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.AzureOAuthProvider.Test.Integration
{
    using System.Collections.Generic;
    using System.Configuration;
    using System.Linq;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.WhiteSpaces.Test.Common;
    using Microsoft.WindowsAzure.ServiceRuntime;
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Table;
    using Microsoft.WhiteSpaces.AzureTableAccess;

    /// <summary>
    /// Test cases for Azure Table Operation class methods
    /// </summary>
    [TestClass]
    public class AzureTableOperationTests
    {
        /// <summary>
        /// variable to hold <see cref="CloudStorageAccount"/> value
        /// </summary>
        private readonly CloudStorageAccount cloudStorageAccount;

        /// <summary>
        /// variable to hold <see cref="CloudTableClient"/> value
        /// </summary>
        private readonly CloudTableClient cloudTableClient;

        /// <summary>
        /// variable to hold <see cref="IAzureTableOperation"/> value
        /// </summary>
        private readonly IAzureTableOperation azureTableOperation;

        /// <summary>
        /// variable to hold <see cref="CloudTable"/> value
        /// </summary>
        private CloudTable cloudTable;

        /// <summary>
        /// Initializes a new instance of the <see cref="AzureTableOperationTests"/> class
        /// </summary>
        public AzureTableOperationTests() 
        {
            this.cloudStorageAccount = CloudStorageAccount.Parse(this.StorageConnectionString);
            this.cloudTableClient = this.cloudStorageAccount.CreateCloudTableClient();
            this.azureTableOperation = new AzureTableOperation();
        }

        /// <summary>
        /// Gets storage connection string value from configuration
        /// </summary>
        public string StorageConnectionString
        {
            get
            {
                return RoleEnvironment.IsAvailable
                            ? RoleEnvironment.GetConfigurationSettingValue("AzureStorageAccountConnectionString")
                            : ConfigurationManager.ConnectionStrings["AzureStorageAccountConnectionString"].ConnectionString;
            }
        }

        /// <summary>
        /// Test InsertEntity method which insert record in to azure table
        /// </summary>
        [TestMethod]
        public void Test_InsertEntity_method()
        {
            UserProfile entity = CommonData.GetUserProfiles().ElementAt(0);
            this.azureTableOperation.InsertEntity(entity);

            TableOperation retrieveOperation = TableOperation.Retrieve<UserProfile>(entity.PartitionKey, entity.RowKey);

            this.cloudTable = this.cloudTableClient.GetTableReference(CommonConstants.UserProfileTable);
            TableResult retrievedResult = this.cloudTable.Execute(retrieveOperation);

            CustomAssert.AreEqualByProperties(entity, retrievedResult.Result as UserProfile, CommonConstants.UserInfoParamList);

            this.DeleteEntity(CommonConstants.UserProfileTable, entity);
        }

        /// <summary>
        /// Test FetchEntity method which fetches record from an azure table
        /// </summary>
        [TestMethod]
        public void Test_FetchEntity_method() 
        {
            UserProfile entity = CommonData.GetUserProfiles().ElementAt(0);
            this.cloudTable = this.cloudTableClient.GetTableReference(CommonConstants.UserProfileTable);
            TableOperation insertOperation = TableOperation.InsertOrReplace(entity);
            this.cloudTable.Execute(insertOperation);

            UserProfile actualTableEntity = this.azureTableOperation.FetchEntity<UserProfile>(entity.PartitionKey, entity.RowKey);
            CustomAssert.AreEqualByProperties(entity, actualTableEntity, CommonConstants.UserInfoParamList);

            this.DeleteEntity(CommonConstants.UserProfileTable, entity);
        }

        /// <summary>
        /// Test GetEntityByPartitionKey method which fetches records from an azure table
        /// </summary>
        [TestMethod]
        public void Test_GetEntityByPartitionKey_method()
        {
            UserProfile entity = CommonData.GetUserProfiles().ElementAt(0);
            this.cloudTable = this.cloudTableClient.GetTableReference(CommonConstants.UserProfileTable);
            TableOperation insertOperation = TableOperation.InsertOrReplace(entity);
            this.cloudTable.Execute(insertOperation);

            IEnumerable<UserProfile> actualEntities = this.azureTableOperation.GetEntityByPartitionKey<UserProfile>(entity.PartitionKey);

            foreach (UserProfile user in actualEntities)
            {
                if (string.Compare(entity.RowKey, user.RowKey, true) == 0)
                {
                    CustomAssert.AreEqualByProperties(entity, user);
                    break;
                }
            }

            this.DeleteEntity(CommonConstants.UserProfileTable, entity);
        }

        /// <summary>
        /// Deletes an entity from an azure table
        /// </summary>
        /// <param name="tableName">Name of the azure table</param>
        /// <param name="entity">entity that needs to be deleted</param>
        private void DeleteEntity(string tableName, TableEntity entity)
        {
            this.cloudTable = this.cloudTableClient.GetTableReference(tableName);
            TableOperation deleteOperation = TableOperation.Delete(entity);
            this.cloudTable.Execute(deleteOperation);
        }
    }
}
