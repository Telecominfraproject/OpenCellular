// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.AzureTableAccess
{
    using System;
    using Microsoft.WindowsAzure.Storage.Table;

    /// <summary>
    /// Type representing azure table <see cref="UserProfile"/>, user id is set to row key
    /// </summary>
    public class UserProfile : TableEntity
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="UserProfile"/> class
        /// </summary>
        public UserProfile()
        {
            this.PartitionKey = "1";
        }

        /// <summary>
        /// Gets or sets user name
        /// </summary>
        public string UserName { get; set; }

        /// <summary>
        /// Gets or sets First name
        /// </summary>
        public string FirstName { get; set; }

        /// <summary>
        /// Gets or sets Last name
        /// </summary>
        public string LastName { get; set; }

        /// <summary>
        /// Gets or sets country
        /// </summary>
        public string State { get; set; }

        /// <summary>
        /// Gets or sets location
        /// </summary>
        public string City { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether the user is a super admin
        /// </summary>
        public bool IsSuperAdmin { get; set; }

        public string Link { get; set; }

        public string Gender { get; set; }

        public string UpdatedTime { get; set; }

        public string CreatedTime { get; set; }

        public string PreferredEmail { get; set; }

        public string AccountEmail { get; set; }

        public string TimeZone { get; set; }

        public string Address1 { get; set; }

        public string Address2 { get; set; }

        public string Phone { get; set; }

        public string Country { get; set; }

        public string ZipCode { get; set; }

        public string PhoneCountryCode { get; set; }
    }
}
