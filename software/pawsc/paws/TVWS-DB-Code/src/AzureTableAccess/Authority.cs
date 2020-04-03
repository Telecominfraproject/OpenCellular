// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.AzureTableAccess
{
    using Microsoft.WindowsAzure.Storage.Table;

    /// <summary>
    /// Type representing azure table Authority, Authority Id as RowKey
    /// </summary>
    public class Authority : TableEntity
    {
        /// <summary>
        /// Initializes a new instance of the Authority class
        /// </summary>
        public Authority()
        {
            this.PartitionKey = "1";
        }

        /// <summary>
        /// Gets or sets Authority Name
        /// </summary>
        public string AuthorityName { get; set; }
    }
}
