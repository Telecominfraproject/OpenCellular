// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Linq;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;

    /// <summary>
    ///     Json converter for the Paws Batch Available Spectrum request.
    /// </summary>
    internal class BatchAvailableSpectrumConverter : BaseJsonConverter
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
            AvailableSpectrumRequest availableSpectrum = existingValue as AvailableSpectrumRequest ?? new AvailableSpectrumRequest();

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
                    case Constants.PropertyNameDeviceDescriptor:
                        availableSpectrum.DeviceDescriptor = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<DeviceDescriptor>() : null;
                        break;

                    case Constants.PropertyNameLocation:
                        availableSpectrum.Location = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<GeoLocation>() : null;
                        break;

                    case Constants.PropertyNameAntenna:
                        availableSpectrum.Antenna = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<AntennaCharacteristics>() : null;
                        break;

                    case Constants.PropertyNameOwner:
                        availableSpectrum.Owner = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<DeviceOwner>() : null;
                        break;

                    case Constants.PropertyNameCapabilities:
                        availableSpectrum.Capabilities = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<DeviceCapabilities>() : null;
                        break;

                    case Constants.PropertyNameMasterDeviceDescriptors:
                        availableSpectrum.MasterDeviceDescriptors = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<DeviceDescriptor>() : null;
                        break;

                    case Constants.PropertyNameRequestType:
                        availableSpectrum.RequestType = reader.ReadAsString();
                        break;

                        ////default:
                        ////    availableSpectrum.UnKnownTypes.Add(value, reader.ReadAsString());
                        ////    break;
                }
            }

            return availableSpectrum;
        }
    }
}
