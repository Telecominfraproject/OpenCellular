// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.AzureOAuthProvider
{
    using Microsoft.WindowsAzure.Storage.Table;

    /// <summary>
    /// Type representing azure table <see cref="WebpagesOauthMembership"/>, Provider user id is assigned to row key
    /// </summary>
    public class WebpagesOauthMembership : TableEntity
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="WebpagesOauthMembership"/> class
        /// </summary>
        public WebpagesOauthMembership()
        {
            this.PartitionKey = "1";
        }

        /// <summary>
        /// Gets or sets provider name
        /// </summary>
        public string Provider { get; set; }

        /// <summary>
        /// Gets or sets user id
        /// </summary>
        public int UserId { get; set; }
    }
}
