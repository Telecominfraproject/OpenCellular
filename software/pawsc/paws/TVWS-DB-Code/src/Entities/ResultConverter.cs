// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Linq;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;

    /// <summary>
    ///     JSon converter for paws parameters.
    /// </summary>
    internal class ResultConverter : BaseJsonConverter
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
            Result parameters = existingValue as Result ?? new Result();
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
                    case Constants.PropertyNameType:
                        parameters.Type = jsonProp.ToObject<string>();
                        break;

                    case Constants.PawsPropertyNameCode:
                        parameters.Code = jsonProp.ToObject<string>();
                        break;

                    case Constants.PropertyNameMessage:
                        parameters.Message = jsonProp.ToObject<string>();
                        break;

                    case Constants.PropertyNameData:
                        parameters.Data = jsonProp.ToObject<string>();
                        break;

                    case Constants.PropertyNameVersion:
                        parameters.Version = jsonProp.ToObject<string>();
                        break;

                    case Constants.PropertyNameRulesetInfo:
                        parameters.RulesetInfo = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<RulesetInfo[]>() : null;
                        break;

                    case Constants.PropertyNameDatabaseChange:
                        parameters.DatabaseChange = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<DbUpdateSpec>() : null;
                        break;

                    case Constants.PropertyNameDeviceDescriptor:
                        parameters.DeviceDescriptor = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<DeviceDescriptor>() : null;
                        break;

                    case Constants.PropertyNameLocation:
                        parameters.Location = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<GeoLocation>() : null;
                        break;

                    case Constants.PropertyNameTimeStamp:
                        parameters.TimeStamp = jsonProp.ToObject<string>();
                        break;

                    case Constants.PropertyNameSpectrumSchedules:
                        parameters.SpectrumSchedules = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<SpectrumSchedule[]>() : null;
                        break;

                    case Constants.PropertyNameNeedsSpectrumReport:
                        parameters.NeedsSpectrumReport = jsonProp.ToObject<bool>();
                        break;

                    case Constants.PropertyNameMaxTotalBwHz:
                        parameters.MaxTotalBwHz = jsonProp.ToObject<float?>();
                        break;

                    case Constants.PropertyNameMaxContiguousBwHz:
                        parameters.MaxContiguousBwHz = jsonProp.ToObject<float?>();
                        break;

                    case Constants.PropertyNameGeoSpectrumSchedules:
                        parameters.GeoSpectrumSchedules = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<GeoSpectrumSchedule[]>() : null;
                        break;

                    case Constants.PropertyNameSpectrumSpecs:
                        parameters.SpectrumSpecs = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<SpectrumSpec[]>() : null;
                        break;

                    case Constants.PropertyNameDeviveValidities:
                        parameters.DeviceValidities = jsonProp.Value.ToObject<DeviceValidity[]>();
                        break;

                    case Constants.PropertyNameIncumbentList:
                        parameters.IncumbentList = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<object[]>() : null;
                        break;

                    case Constants.PropertyNameRegionManagementUniqueId:
                        parameters.UniqueId = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<string>() : null;
                        break;

                    case Constants.PropertyNameRegionManagementRequestType:
                        parameters.RequestType = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<string>() : null;
                        break;

                    case Constants.PropertyNameRegionManagementStartTime:
                        parameters.StartTime = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<string>() : null;
                        break;

                    case Constants.PropertyNameRegionManagementEndTime:
                        parameters.EndTime = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<string>() : null;
                        break;

                    case Constants.PropertyNameRegionManagementChannels:
                        parameters.Channels = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<ChannelInfo[]>() : null;
                        break;

                    case Constants.PropertyNameChannelInfo:
                        parameters.ChannelInfo = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<ChannelInfo[]>() : null;
                        break;

                    case "lpAuxLicenses":
                        parameters.LpAuxLicenses = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<LpAuxLicenseInfo[]>() : null;
                        break;

                    case "searchMVPDCallSigns":
                        parameters.SearchMVPDCallSigns = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<MVPDCallSignsInfo[]>() : null;
                        break;

                    case Constants.PropertyNameCallsignInfo:
                        parameters.CallsignInfo = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<MVPDCallSignsInfo>() : null;
                        break;

                    case "channelsInCSV":
                        parameters.ChannelsInCSV = jsonProp.Value.ToObject<string>();
                        break;

                    case Constants.PropertyNameDeviceList:
                        parameters.DeviceList = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<ProtectedDevice[]>() : null;
                        break;

                    case "geoSpectrumSpecs":
                        parameters.GeoSpectrumSpecs = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<GeoSpectrumSpec[]>() : null;
                        break;

                    case "contourData":
                        parameters.ContourData = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<ContourDetailsInfo[]>() : null;
                        break;

                    case "intermediateResults1":
                        parameters.IntermediateResults1 = jsonProp.Value.ToObject<string>();
                        break;

                    case "masterOperationParameters":
                        parameters.MasterOperationParameters = jsonProp.Value.ToObject<string>();
                        break;

                    ////default:
                    ////    parameters.UnKnownTypes.Add(value, reader.ReadAsString());
                    ////    break;
                }
            }

            return parameters;
        }
    }
}
