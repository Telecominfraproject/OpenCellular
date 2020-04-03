// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Xml.Serialization;
    using Microsoft.Practices.EnterpriseLibrary.Validation.Validators;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;

    /// <summary>
    /// The PolygonConverter is used to convert a Json reader into an Polygon instance.
    /// </summary>
    internal class PolygonConverter : BaseJsonConverter
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
            Polygon polygon = existingValue as Polygon ?? new Polygon();

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
                    case Constants.PropertyNameExterior:
                        polygon.Exterior = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<Point[]>() : null;
                        break;

                    ////default:
                    ////    polygon.UnKnownTypes.Add(value, reader.ReadAsString());
                    ////    break;
                }
            }

            return polygon;
        }
    }
}
