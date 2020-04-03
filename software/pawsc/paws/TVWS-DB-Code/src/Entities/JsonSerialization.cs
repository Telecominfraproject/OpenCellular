// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Text;
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Table;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Converters;
    using Newtonsoft.Json.Serialization;

    /// <summary>
    /// Initializes static members of the <see cref="JsonSerialization"/> class.
    /// </summary>
    public static class JsonSerialization
    {
        /// <summary>
        /// Contains the json serialize settings.
        /// </summary>
        private static JsonSerializerSettings jsonSerializerSettings = null;

        /// <summary>
        /// Initializes static members of the JsonSerialization class Serialization .
        /// </summary>
        static JsonSerialization()
        {
            jsonSerializerSettings = new JsonSerializerSettings
            {
                NullValueHandling = NullValueHandling.Ignore,
                DefaultValueHandling = DefaultValueHandling.Ignore,
                ContractResolver = new CamelCasePropertyNamesContractResolver()
            };
        }

        /// <summary>
        /// Gets the json serializer settings.
        /// </summary>
        /// <value>The json serializer settings.</value>
        public static JsonSerializerSettings PawsJsonSerializerSetting
        {
            get
            {
                return jsonSerializerSettings;
            }
        }

        /// <summary>
        /// Serializes object to json string
        /// </summary>
        /// <param name="serializerObject">object to serialize</param>
        /// <returns>json string</returns>
        public static string SerializeObject(object serializerObject)
        {
            return JsonConvert.SerializeObject(serializerObject, jsonSerializerSettings);
        }

        /// <summary>
        /// deserializes string to object
        /// </summary>
        /// <typeparam name="T">type of object</typeparam>
        /// <param name="jsonstring">json string</param>
        /// <returns>A deserialized object.</returns>
        public static T DeserializeString<T>(string jsonstring)
        {
            try
            {
                var obj = JsonConvert.DeserializeObject<T>(jsonstring, jsonSerializerSettings);
                return obj;
            }
            catch (System.Exception)
            {
                throw;
            }
        }
    }
}
