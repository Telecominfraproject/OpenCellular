// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Xml.Serialization;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;

    /// <summary>
    /// Json converter for Interference Query.
    /// </summary>
    internal class InterferenceQueryConverter : BaseJsonConverter
    {
        /// <summary>
        /// Logic called while de-serializing json string to object
        /// </summary>
        /// <param name="reader">The serialized json reader data.</param>
        /// <param name="objectType">The type of object being de-serialized.</param>
        /// <param name="existingValue">The current value.</param>
        /// <param name="serializer">Controls how the object is de-serialized.</param>
        /// <returns>Returns the de-serialized object.</returns>
        public override object ReadJson(JsonReader reader, Type objectType, object existingValue, JsonSerializer serializer)
        {
            InterferenceQueryRequest interferenceQuery = existingValue as InterferenceQueryRequest ?? new InterferenceQueryRequest();

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
                    case Constants.PropertyNameTimeStamp:
                        interferenceQuery.TimeStamp = jsonProp.ToObject<string>();
                        break;

                    case Constants.PropertyNameInterferenceQueryStartTime:
                        interferenceQuery.StartTime = jsonProp.ToObject<string>();
                        break;

                    case Constants.PropertyNameInterferenceQueryEndTime:
                        interferenceQuery.EndTime = jsonProp.ToObject<string>();
                        break;

                    case Constants.PropertyNameLocation:
                        interferenceQuery.Location = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<GeoLocation>() : null;
                        break;

                    case Constants.PropertyNameRequestor:
                        interferenceQuery.Requestor = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<DeviceOwner>() : null;
                        break;

                    case Constants.PropertyNameInterferenceQueryRequestType:
                        interferenceQuery.RequestType = reader.ReadAsString();
                        break;
                }
            }

            return interferenceQuery;
        }
    }
}
