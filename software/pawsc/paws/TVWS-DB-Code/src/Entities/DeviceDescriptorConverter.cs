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
    /// Represents a Device Descriptor Json Converter.
    /// </summary>
    internal class DeviceDescriptorConverter : BaseJsonConverter
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
            DeviceDescriptor objData = existingValue as DeviceDescriptor ?? new DeviceDescriptor();

            JObject jsonObject = JObject.Load(reader);
            var properties = jsonObject.Properties().ToList();

            for (int i = 0; i < properties.Count; i++)
            {
                var jsonProp = properties[i];
                switch (jsonProp.Name)
                {
                    case Constants.PropertyNameSerialNumber:
                        objData.SerialNumber = jsonProp.ToObject<string>();
                        break;

                    case Constants.PropertyNameManufacturerId:
                        objData.ManufacturerId = jsonProp.ToObject<string>();
                        break;

                    case Constants.PropertyNameModelId:
                        objData.ModelId = jsonProp.ToObject<string>();
                        break;

                    case Constants.PropertyNameFccId:
                        objData.FccId = jsonProp.ToObject<string>();
                        break;                  

                    case Constants.PropertyNameFccTvbdDeviceType:
                        objData.FccTvbdDeviceType = jsonProp.ToObject<string>();
                        break;

                    case Constants.PropertyNameRulesetIds:
                        objData.RulesetIds = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<string[]>() : null;
                        break;

                    case Constants.PropertyNameEtsiDeviceType:
                        objData.EtsiEnDeviceType = jsonProp.ToObject<string>();
                        break;
                    case Constants.PropertyNameEtsiDeviceCategory:
                        objData.EtsiDeviceCategory = jsonProp.ToObject<string>();
                        break;
                    case Constants.PropertyNameEtsiEnTechnologyId:
                        objData.EtsiEnTechnologyId = jsonProp.ToObject<string>();
                        break;
                    case Constants.PropertyNameEtsiEnDeviceEmissionsClass:
                        objData.EtsiEnDeviceEmissionsClass = jsonProp.ToObject<string>();
                        break;
                    ////default:
                    ////    if (reader.Value != null)
                    ////    {
                    ////        objData.UnKnownTypes.Add(jsonProp.Name, reader.Value.ToString());
                    ////    }
                    //// break;
                }
            }

            return objData;
        }
    }
}
