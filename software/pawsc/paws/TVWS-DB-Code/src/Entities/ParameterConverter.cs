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
    internal class ParameterConverter : BaseJsonConverter
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
            Parameters parameters = existingValue as Parameters ?? new Parameters();

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

                    case Constants.PropertyNameDeviceDescriptor:
                        parameters.DeviceDescriptor = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<DeviceDescriptor>() : null;
                        break;

                    case Constants.PropertyNameLocation:
                        parameters.Location = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<GeoLocation>() : null;
                        break;

                    case Constants.PropertyNameMasterDeviceLocation:
                        parameters.MasterDeviceLocation = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<GeoLocation>() : null;
                        break;

                    case Constants.PropertyNameDeviceOwner:
                        parameters.DeviceOwner = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<DeviceOwner>() : null;
                        break;

                    case Constants.PropertyNameAntenna:
                        parameters.Antenna = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<AntennaCharacteristics>() : null;
                        break;

                    case Constants.PropertyNameOwner:
                        parameters.Owner = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<DeviceOwner>() : null;
                        break;

                    case Constants.PropertyNameCapabilities:
                        parameters.Capabilities = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<DeviceCapabilities>() : null;
                        break;

                    case Constants.PropertyNameMasterDeviceDescriptors:
                        parameters.MasterDeviceDescriptors = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<DeviceDescriptor>() : null;
                        break;

                    case Constants.PropertyNameRequestType:
                        parameters.RequestType = jsonProp.Value.ToObject<string>();
                        break;

                    case Constants.PropertyNameLocations:
                        parameters.Locations = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<GeoLocation[]>() : null;
                        break;

                    case Constants.PropertyNameSpectra:
                        parameters.Spectra = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<Spectrum[]>() : null;
                        break;

                    case Constants.PropertyNameDeviceDescriptors:
                        parameters.DevicesDescriptors = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<DeviceDescriptor[]>() : null;
                        break;

                    case Constants.PropertyNameRequestor:
                        parameters.Requestor = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<DeviceOwner>() : null;
                        break;

                    case Constants.PropertyNameTvSpectrum:
                        parameters.TvSpectrum = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<TvSpectrum>() : null;
                        break;

                    case Constants.PropertyNameTvSpectra:
                        parameters.TvSpectra = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<TvSpectrum[]>() : null;
                        break;

                    case Constants.PropertyNameIncumbentType:
                        parameters.IncumbentType = jsonProp.Value.ToObject<string>();
                        break;

                    case Constants.PropertyNameRegistrationId:
                        parameters.RegistrationId = jsonProp.Value.ToObject<string>();
                        break;

                    case Constants.PropertyNameRegistrationDisposition:
                        parameters.RegistrationDisposition = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<RegistrationDisposition>() : null;
                        break;

                    case Constants.PropertyNameRegistrant:
                        parameters.Registrant = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<Vcard>() : null;
                        break;

                    case Constants.PropertyNameLPAuxRegistrant:
                        parameters.LPAuxRegistrant = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<Versitcard.VCard>() : null;
                        break;

                    case Constants.PropertyNameTempBASRegistrant:
                        parameters.TempBASRegistrant = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<Versitcard.VCard>() : null;
                        break;

                    case Constants.PropertyNameContact:
                        parameters.Contact = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<Versitcard.VCard>() : null;
                        break;

                    case Constants.PropertyNameTransmitLocation:
                        parameters.TransmitLocation = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<Location>() : null;
                        break;

                    case Constants.PropertyNamePointsArea:
                        parameters.PointsArea = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<Point[]>() : null;
                        break;

                    case Constants.PropertyNameQuadrilateralArea:
                        parameters.QuadrilateralArea = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<QuadrilateralArea[]>() : null;
                        break;

                    case Constants.PropertyNameMVPDLocation:
                        parameters.MVPDLocation = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<Location>() : null;
                        break;

                    case Constants.PropertyNameTempBasLocation:
                        parameters.TempBasLocation = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<Location>() : null;
                        break;

                    case Constants.PropertyNameEvent:
                        parameters.Event = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<Event>() : null;
                        break;

                    case Constants.PropertyNameUserId:
                        parameters.UserId = jsonProp.Value.ToObject<string>();
                        break;
                    case Constants.PropertyNameLPAUXId:
                        parameters.LPAUXRegId = jsonProp.Value.ToObject<string>();
                        break;

                    case Constants.PropertyNameVenue:
                        parameters.Venue = jsonProp.Value.ToObject<string>();
                        break;

                    case Constants.PropertyNameDeviceId:
                        parameters.DeviceId = jsonProp.Value.ToObject<string>();
                        break;

                    case Constants.PropertyNameSerialNumber:
                        parameters.SerialNumber = jsonProp.Value.ToObject<string>();
                        break;

                    case Constants.PropertyNameTimeStamp:
                        parameters.TimeStamp = jsonProp.Value.ToObject<string>();
                        break;

                    case Constants.PropertyNameInterferenceQueryStartTime:
                        parameters.StartTime = jsonProp.Value.ToObject<string>();
                        break;

                    case Constants.PropertyNameReferenceSensitivity:
                        parameters.Prefsens = jsonProp.Value.ToObject<double>();
                        break;

                    case Constants.PropertyNameMaxMasterEirp:
                        parameters.MaxMasterEIRP = jsonProp.Value.ToObject<double>();
                        break;

                    case Constants.PropertyNameInterferenceQueryEndTime:
                        parameters.EndTime = jsonProp.Value.ToObject<string>();
                        break;

                    case "mvpdRegistrant":
                        parameters.MVPDRegistrant = jsonProp.Value.HasValues ? jsonProp.Value.ToObject<Versitcard.VCard>() : null;
                        break;

                    case "ulsFileNumber":
                        parameters.ULSFileNumber = jsonProp.Value.ToObject<string>();
                        break;

                    case "uniqueId":
                        parameters.UniqueId = jsonProp.Value.ToObject<string>();
                        break;

                    case "contourRequestCallSign":
                        parameters.ContourRequestCallSign = jsonProp.Value.ToObject<string>();
                        break;

                    case "testingStage":
                        parameters.TestingStage = jsonProp.Value.ToObject<int>();
                        break;

                    case "pmseAssignmentTableName":
                        parameters.PMSEAssignmentTableName = jsonProp.Value.ToObject<string>();
                        break;

                    default:
                        parameters.UnKnownTypes.Add(jsonProp.Name, jsonProp.ToObject<string>());
                        break;
                }
            }

            return parameters;
        }
    }
}
