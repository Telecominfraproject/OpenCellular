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
    /// Represents a Spectrum Json Converter.
    /// </summary>
    internal class FrequencyRangeConverter : BaseJsonConverter
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
            FrequencyRange frequencyRange = existingValue as FrequencyRange ?? new FrequencyRange();

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
                    case Constants.PropertyNameStartHz:
                        frequencyRange.StartHz = jsonProp.ToObject<float>();
                        break;

                    case Constants.PropertyNameStopHz:
                        frequencyRange.StopHz = jsonProp.ToObject<float>();
                        break;

                    case Constants.PropertyNameMaxPowerDBm:
                        frequencyRange.MaxPowerDBm = jsonProp.ToObject<float>();
                        break;

                    case Constants.PropertyNameChannelId:
                        frequencyRange.ChannelId = jsonProp.ToObject<string>();
                        break;

                    ////default:
                    ////    device.UnKnownTypes.Add(value, reader.ReadAsString());
                    ////    break;
                }
            }

            return frequencyRange;
        }
    }
}
