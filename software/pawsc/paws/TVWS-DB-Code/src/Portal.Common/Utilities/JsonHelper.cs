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
    using Newtonsoft.Json.Serialization;
    
    public static class JsonHelper
    {
        private static JsonSerializerSettings jsonSerializerSettings;

        static JsonHelper()
        {
            jsonSerializerSettings = new JsonSerializerSettings
            {
                NullValueHandling = NullValueHandling.Ignore,
                DefaultValueHandling = DefaultValueHandling.Ignore,
                ContractResolver = new CamelCasePropertyNamesContractResolver()
            };
        }

        public static T DeserializeObject<T>(string jsonString) where T : class
        {
            return JsonConvert.DeserializeObject<T>(jsonString, jsonSerializerSettings);
        }

        public static string SerializeObject(object value)
        {            
            return JsonConvert.SerializeObject(value, jsonSerializerSettings);
        }
    }
}
