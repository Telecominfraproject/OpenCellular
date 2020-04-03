// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.AzureTableAccess
{
    using Microsoft.WindowsAzure.Storage.Table;

    /// <summary>
    /// Type representing azure table AccessLevel, Access Level Id as Row key
    /// </summary>
    public class AccessLevel : TableEntity
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="AccessLevel" /> class.
        /// </summary>
        public AccessLevel()
        {
            this.PartitionKey = "1";
        }

        /// <summary>
        /// Gets or sets Access Level Name
        /// </summary>
        public string AccessLevelName { get; set; }
    }
}
