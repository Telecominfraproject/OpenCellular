// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using Newtonsoft.Json;

    [Serializable]
    public class ClientUserData
    {
        [JsonProperty("first_name")]
        public string FirstName { get; set; }

        [JsonProperty("gender")]
        public string Gender { get; set; }

        [JsonProperty("id")]        
        public string ProviderUserId { get; set; }

        [JsonProperty("last_name")]
        public string LastName { get; set; }

        [JsonProperty("link")]
        public Uri Link { get; set; }

        [JsonProperty("name")]
        public string UserName { get; set; }

        [JsonProperty("locale")]
        public string Locale { get; set; }

        [JsonProperty("updated_time")]
        public string UpdatedTime { get; set; }

        [JsonProperty("emails")]
        public Emails Emails { get; set; }

        [JsonProperty("personal")]
        public Address Personal { get; set; }

        [JsonProperty("business")]
        public Address Business { get; set; }

        public string AccessToken { get; set; }      
    }
}
