// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Portal
{    
    using System;
    using System.Collections.Generic;
    using Microsoft.WhiteSpaces.AzureOAuthProvider;

    /// <summary>
    /// serializing model
    /// </summary>
    public class UserPrincipalSerializationModel
    {
        public UserPrincipalSerializationModel()
        {
            // Default value for each token
            this.TokenExpirationTime = 3600;
        }

        /// <summary>
        /// Gets or sets user id
        /// </summary>
        public int UserId { get; set; }
                
        /// <summary>
        /// Gets or sets user name
        /// </summary>
        public string UserName { get; set; }

        public string AccessToken { get; set; }

        /// <summary>
        /// Gets or sets provider name
        /// </summary>
        public DateTime AccessTokenCreatedTime { get; set; }

        /// <summary>
        /// Gets or sets provider user id
        /// </summary>
        public int TokenExpirationTime { get; set; }

        public bool IsSuperAdmin { get; set; }
    }
}
