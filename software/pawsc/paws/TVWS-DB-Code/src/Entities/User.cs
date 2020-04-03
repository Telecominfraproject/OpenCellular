// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel.DataAnnotations;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using Microsoft.WindowsAzure.Storage.Table;
    using Newtonsoft.Json;

    /// <summary>
    /// Represents Access Level.
    /// </summary>
    public enum AccessLevel
    {
        /// <summary>
        /// Portal User Default Access Level.
        /// </summary>
        None = 0,

        /// <summary>
        /// Portal User Default Access Level.
        /// </summary>
        PortalUser = 1,

        /// <summary>
        /// Device Vendor Can Query for device information.
        /// </summary>
        DeviceVendor = 2,

        /// <summary>
        /// Licensee can add or delete incumbents they have added. 
        /// </summary>
        Licensee = 3,

        /// <summary>
        /// Admin has access to all methods but only for a specific region.
        /// </summary>
        Admin = 4,

        /// <summary>
        /// Super Admin has access to all methods across all regions.
        /// </summary>
        SuperAdmin = 5
    }
 
    /// <summary>
    /// Represents a registered User.
    /// </summary>
    public class User : TableEntity
    {
        /// <summary>
        /// Gets or sets user id.
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameUserId)]
        [Required(ErrorMessage = Constants.ErrorMessageUserIdRequired)]
        public string UserId { get; set; }
        
        /// <summary>
        /// Gets or sets user id.
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameUserFirstName)]
        [Required(ErrorMessage = Constants.ErrorMessageUserFirstNameRequired)]
        public string UserFirstName { get; set; }

        /// <summary>
        /// Gets or sets user last name.
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameUserLastName)]
        [Required(ErrorMessage = Constants.ErrorMessageUserLastNameRequired)]
        public string UserLastName { get; set; }

        /// <summary>
        /// Gets or sets user country.
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameCountry)]
        [Required(ErrorMessage = Constants.ErrorMessageCountryRequired)]
        public string Country { get; set; }

        /// <summary>
        /// Gets or sets user City or state.
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameCity)]
        [Required(ErrorMessage = Constants.ErrorMessageCountryRequired)]
        public string City { get; set; }

        /// <summary>
        /// Gets or sets the value for Admin Access Requested.
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameAccess)]
        public RegionAccess[] Access { get; set; }

        /// <summary>
        /// Gets or sets user registered date.
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameCreateDate)]
        public DateTime CreationDate { get; set; }

        /// <summary>
        /// Gets or sets Create By.
        /// </summary>
        public string CreateBy { get; set; }

        /// <summary>
        /// Gets or sets the modified on.
        /// </summary>
        /// <value>The modified on.</value>
        public DateTime UpdatedOn { get; set; }

        /// <summary>
        /// Gets or sets the modified by.
        /// </summary>
        /// <value>The modified by.</value>
        public string UpdatedBy { get; set; }

        /// <summary>
        /// Gets or sets the value Justification string set by the user when requesting admin access.
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameJustification)]
        public string Justification { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether user delete flag.
        /// </summary>
        public bool Deleted { get; set; }
    }
}
