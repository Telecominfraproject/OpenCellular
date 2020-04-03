// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.AzureOAuthProvider
{
    using Microsoft.WhiteSpaces.AzureTableAccess;

    // This is a class used to create infrastructure tables at starting of the application
    public static class Infrastructure
    {
        private static IAzureTableOperation azureTableOperations;

        static Infrastructure()
        {
            azureTableOperations = new AzureTableOperation();
        }

        // These access levels need to change as per the current requirement
        public static void CreateAccessLevels()
        {
            AccessLevel level = new AccessLevel()
            {
                RowKey = "0",
                AccessLevelName = "None"
            };
            azureTableOperations.InsertEntity(level);

            level = new AccessLevel
            {
                RowKey = "1",
                AccessLevelName = "PortalUser"
            };
            azureTableOperations.InsertEntity(level);

            level = new AccessLevel
            {
                RowKey = "2",
                AccessLevelName = "DeviceVendor"
            };

            azureTableOperations.InsertEntity(level);
            level = new AccessLevel
            {
                RowKey = "3",
                AccessLevelName = "Licensee"
            };
            azureTableOperations.InsertEntity(level);

            level = new AccessLevel
            {
                RowKey = "4",
                AccessLevelName = "Admin"
            };
            azureTableOperations.InsertEntity(level);

            level = new AccessLevel
            {
                RowKey = "5",
                AccessLevelName = "SuperAdmin"
            };
            azureTableOperations.InsertEntity(level);
        }

        public static void CreateAuthorities()
        {
            Authority authority = new Authority()
            {
                RowKey = "0",
                AuthorityName = "fcc"
            };
            azureTableOperations.InsertEntity(authority);

            authority = new Authority()
            {
                RowKey = "1",
                AuthorityName = "ofcom"
            };
            azureTableOperations.InsertEntity(authority);
        }
    }
}
