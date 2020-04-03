// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;

    /// <summary>
    /// Represents Request Converter.
    /// </summary>
    public class RequestConverter : BaseJsonConverter
    {
        /// <summary>
        /// ReadJson method.
        /// </summary>
        /// <param name="reader">reader information</param>
        /// <param name="objectType">objectType information</param>
        /// <param name="existingValue">existingValue information</param>
        /// <param name="serializer">serializer information</param>
        /// <returns>Reads the Json data</returns>
        public override object ReadJson(JsonReader reader, Type objectType, object existingValue, JsonSerializer serializer)
        {
                var request = existingValue as Request ?? new Request();

                JObject jsonObject = JObject.Load(reader);
                var properties = jsonObject.Properties().ToList();

                for (int i = 0; i < properties.Count; i++)
                {
                    var jsonProp = properties[i];
                    if (!jsonProp.HasValues)
                    {
                        continue;
                    }

                    switch (jsonProp.Name)
                    {
                        case Constants.PropertyNameJsonRpc:
                            request.JsonRpc = jsonProp.ToObject<string>();
                            break;

                        case Constants.PropertyNameMethod:
                            request.Method = jsonProp.ToObject<string>();
                            break;

                        case Constants.PropertyNameId:
                            request.Id = jsonProp.ToObject<string>();
                            break;

                        case Constants.PropertyNameParams:
                            request.Params = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<Parameters>() : null;
                            break;

                        ////default:
                        ////    request.UnKnownTypes.Add(jsonProp.Name, jsonProp.ToObject<string>());
                        ////    break;
                    }
                }          
 
            return request;
        }
    }
}
