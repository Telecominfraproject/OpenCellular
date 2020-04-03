// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.AzureTableAccess
{
    using Microsoft.WindowsAzure.Storage.Table;

    /// <summary>
    /// Type representing azure table <see cref="AuthorityAccess"/>, UserId  as PartitionKey and AuthorityId as RowKey
    /// </summary>
    public class AuthorityAccess : TableEntity
    {
        public int AccessLevel { get; set; }       
    }
}
