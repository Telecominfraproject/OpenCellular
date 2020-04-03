// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Linq;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;

    /// <summary>
    ///     Represents a Event Time Json Converter.
    /// </summary>
    internal class EventTimeConverter : BaseJsonConverter
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
            EventTime device = existingValue as EventTime ?? new EventTime();

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
                    case Constants.PropertyNameStartTime:
                        device.StartTime = jsonProp.ToObject<string>();
                        break;

                    case Constants.PropertyNameStopTime:
                        device.StopTime = jsonProp.ToObject<string>();
                        break;

                        ////default:
                        ////    device.UnKnownTypes.Add(jsonProp.Name, jsonProp.ToObject<string>());
                        ////    break;
                }
            }

            return device;
        }
    }
}
