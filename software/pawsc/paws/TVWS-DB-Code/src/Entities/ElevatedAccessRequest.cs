// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel.DataAnnotations;
    using System.Linq;
    using System.Text;
    using Microsoft.WindowsAzure.Storage.Table;
    using Newtonsoft.Json;

    /// <summary>
    /// Represents Region Access.
    /// </summary>
    public class ElevatedAccessRequest : TableEntity
    {
        /// <summary>
        /// Gets or sets user id.
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameUserId)]
        [Required(ErrorMessage = Constants.ErrorMessageUserIdRequired)]
        public string UserId { get; set; }

        /// <summary>
        /// Gets or sets the region (Kenya, Singapore, ...).
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameRegion)]
        [Required(ErrorMessage = Constants.ErrorMessageRegionIdRequired)]
        public string Region { get; set; }

        /// <summary>
        /// Gets or sets User access level.
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameAccessLevel)]
        [Required(ErrorMessage = Constants.ErrorMessageAccessLevelRequired)]
        public AccessLevel AccessLevel { get; set; }

        /// <summary>
        /// Gets or sets the User access level.
        /// </summary>
        public string AccessLevelString 
        {
            get 
            {
                return this.AccessLevel.ToString();
            }

            set 
            {
                    AccessLevel al = AccessLevel.None;
                    Enum.TryParse<AccessLevel>(value, out al);
                    this.AccessLevel = al;
            } 
        }

        /// <summary>
        /// Gets or sets the Justification string set by the user when requesting admin access.
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameJustification)]
        public string Justification { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether user delete flag.
        /// </summary>
        public bool Deleted { get; set; }

        /// <summary>
        /// Gets or sets the status.
        /// </summary>
        /// <value>The status.</value>
        public string Status { get; set; }

        /// <summary>
        /// Gets or sets user registered date.
        /// </summary>
        public string CreationDate { get; set; }

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
    }
}
