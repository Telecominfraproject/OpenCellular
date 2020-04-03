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

    /// <summary>
    /// Base class for json conversion
    /// </summary>
    public class BaseJsonConverter : JsonConverter
    {
        /// <summary>
        /// tells whether json converted will used while serializing object
        /// </summary>
        public override bool CanWrite
        {
            get { return false; }
        }

        /// <summary>
        /// Sets boolean value to tell the convert whether object can be converted or not
        /// </summary>
        /// <param name="objectType">The object that is to be potentially converted.</param>
        /// <returns>Returns true if the object can be converted, false otherwise.</returns>
        public override bool CanConvert(Type objectType)
        {
            return true;
        }

        /// <summary>
        /// Logic called while serializing object to json string
        /// </summary>
        /// <param name="writer">The forward only json generator.</param>
        /// <param name="value">The object that is to be serialized.</param>
        /// <param name="serializer">Controls how the object is serialized.</param>
        public override void WriteJson(JsonWriter writer, object value, JsonSerializer serializer)
        {
            throw new NotImplementedException();
        }

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
            throw new NotImplementedException();
        }
    }
}
