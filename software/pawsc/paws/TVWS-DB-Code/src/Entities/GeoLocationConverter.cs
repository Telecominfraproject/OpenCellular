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
    internal class GeoLocationConverter : BaseJsonConverter
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
            GeoLocation geoLocation = existingValue as GeoLocation ?? new GeoLocation();

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
                    case Constants.PropertyNamePoint:
                        geoLocation.Point = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<Ellipse>() : null;
                        break;

                    case Constants.PropertyNameRegion:
                        geoLocation.Region = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<Polygon>() : null;
                        break;

                    case Constants.PropertyNameConfidence:
                        geoLocation.Confidence = jsonProp.ToObject<int>();
                        break;

                        ////default:
                        ////    location.UnKnownTypes.Add(jsonProp.Name, jsonProp.Value.ToObject<string>());
                        ////    break;
                }
            }

            return geoLocation;
        }
    }
}
