// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Linq;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;

    /// <summary>
    ///     The GeoLocationConverter is used to convert a Json reader into an GeoLocation instance.
    /// </summary>
    internal class PawsResponseConverter : BaseJsonConverter
    {
        /// <summary>
        ///     Logic called while de-serializing json string to object
        /// </summary>
        /// <param name="reader">The serialized json reader data.</param>
        /// <param name="objectType">The type of object being de-serialized.</param>
        /// <param name="existingValue">The current value.</param>
        /// <param name="serializer">Controls how the object is de-serialized.</param>
        /// <returns>Returns the de-serialized object.</returns>
        public override object ReadJson(JsonReader reader, Type objectType, object existingValue, JsonSerializer serializer)
        {
            PawsResponse location = existingValue as PawsResponse ?? new PawsResponse();

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
                    case Constants.PropertyNameResult:
                        location.Result = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<Result>() : null;
                        break;

                    case Constants.PropertyNameError:
                        location.Error = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<Result>() : null;
                        break;

                    case Constants.PropertyNameJsonRpc:
                        location.JsonRpc = jsonProp.ToObject<string>();
                        break;

                    case Constants.PropertyNameId:
                        location.Id = jsonProp.ToObject<string>();
                        break;

                    ////default:
                    ////    location.UnKnownTypes.Add(jsonProp.Name, jsonProp.Value.ToObject<string>());
                    ////    break;
                }
            }

            return location;
        }
    }
}
