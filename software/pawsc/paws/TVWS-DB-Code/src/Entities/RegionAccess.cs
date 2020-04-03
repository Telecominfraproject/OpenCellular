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
    public class RegionAccess : TableEntity
    {
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
        [JsonIgnore]
        public AccessLevel AccessLevel { get; set; }

        /// <summary>
        /// Gets or sets the User access level.
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameAccessLevel)]
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
        /// Gets or sets the requested access level for this region.
        /// </summary>
        [JsonProperty(PropertyName = Constants.PropertyNameRequestedAccessLevel)]
        public AccessLevel RequestedAccessLevel { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether user delete flag.
        /// </summary>
        public bool Deleted { get; set; }
    }
}
