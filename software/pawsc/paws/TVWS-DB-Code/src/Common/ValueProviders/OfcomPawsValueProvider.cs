// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common.ValueProviders
{
    using System;
    using System.Collections.Generic;
    using Entities;
    using Utilities;

    /// <summary>
    ///     Represents PawsOfComValueProvider
    /// </summary>
    public class OfcomPawsValueProvider : PawsRegionalValueProvider
    {
        /// <summary>
        ///     Gets the device id.
        /// </summary>
        /// <param name="objData">The object data.</param>
        /// <returns>returns System.String.</returns>
        public override string GetDeviceId(dynamic objData)
        {
            DeviceDescriptor deviceData = objData as DeviceDescriptor;

            if (deviceData == null)
            {
                deviceData = this.GetDeviceDescriptor(objData);
            }

            return deviceData.SerialNumber + deviceData.ManufacturerId + deviceData.ModelId;
        }

        /// <summary>
        ///     Gets the device id.
        /// </summary>
        /// <param name="deviceDescriptor">The Device Descriptor.</param>
        /// <returns>returns rule set information.</returns>
        public override List<RulesetInfo> GetRuleSetInfo(DeviceDescriptor deviceDescriptor)
        {
            List<RulesetInfo> ruleSetList = new List<RulesetInfo>();
            if (deviceDescriptor.EtsiEnDeviceType.ToLower() == Constants.PropertyNameEtsiDeviceTypeA.ToLower())
            {
                List<RuleSetInfoEntity> ruleSet = this.PawsDalc.GetRulesetInfo(Utils.GetRegionalTableName(Constants.RuleSetInformationTable));
                foreach (RuleSetInfoEntity rs in ruleSet)
                {
                    rs.MaxEirpHz = null;
                    rs.MaxLocationChange = null;
                    rs.MaxPollingSecs = null;
                    rs.RowKey = null;
                    rs.PartitionKey = null;
                    ruleSetList.Add(rs.ToRuleSetInfo());
                }
            }
            else
            {
                List<RuleSetInfoEntity> ruleSet = this.PawsDalc.GetRulesetInfo(Utils.GetRegionalTableName(Constants.RuleSetInformationTable));
                foreach (RuleSetInfoEntity rs in ruleSet)
                {
                    rs.PartitionKey = null;
                    rs.RowKey = null;
                    ruleSetList.Add(rs.ToRuleSetInfo());
                }
            }

            return ruleSetList;
        }

        /// <summary>
        /// Gets the WSD information.
        /// </summary>
        /// <typeparam name="T">type of request</typeparam>
        /// <param name="request">The request.</param>
        /// <returns>returns Incumbent.</returns>
        public override Incumbent[] GetWSDInfo<T>(T request)
        {
            List<Incumbent> wsdInfo = new List<Incumbent>();
            Type requestType = typeof(T);
            if (requestType == typeof(IAvailableSpectrumRequest))
            {
                var spectrumRequest = request as IAvailableSpectrumRequest;
                Incumbent currentIncumbent = new Incumbent()
                                          {
                                              Location = spectrumRequest.MasterDeviceLocation != null ? spectrumRequest.MasterDeviceLocation.ToLocation() : spectrumRequest.Location.ToLocation(),
                                              DeviceCategory = spectrumRequest.MasterDeviceDescriptors != null ? spectrumRequest.MasterDeviceDescriptors.EtsiDeviceCategory : spectrumRequest.DeviceDescriptor.EtsiDeviceCategory,
                                              EmissionClass = spectrumRequest.MasterDeviceDescriptors != null ? spectrumRequest.MasterDeviceDescriptors.EtsiEnDeviceEmissionsClass : spectrumRequest.DeviceDescriptor.EtsiEnDeviceEmissionsClass,
                                              Capabilities = spectrumRequest.Capabilities,
                                              MasterDeviceDescriptor = spectrumRequest.MasterDeviceDescriptors,
                                              DeviceDescriptor = spectrumRequest.MasterDeviceDescriptors != null ? spectrumRequest.MasterDeviceDescriptors : spectrumRequest.DeviceDescriptor,
                                              IncumbentType = Conversion.ToIncumbentType(spectrumRequest.MasterDeviceDescriptors != null ? spectrumRequest.MasterDeviceDescriptors.EtsiEnDeviceType : spectrumRequest.DeviceDescriptor.EtsiEnDeviceType),
                                          };

                if (spectrumRequest.Antenna != null)
                {
                    currentIncumbent.Height = spectrumRequest.Antenna.Height;
                    currentIncumbent.HeightType = spectrumRequest.Antenna.HeightType;
                }

                wsdInfo.Add(currentIncumbent);
            }
            else if (requestType == typeof(IBatchAvailableSpectrumRequest))
            {
                var spectrumRequest = request as IBatchAvailableSpectrumRequest;
                foreach (var receiverLocation in spectrumRequest.Locations)
                {
                    var currentIncumbent = new Incumbent()
                    {
                        Location = receiverLocation.ToLocation(),
                        DeviceCategory = spectrumRequest.MasterDeviceDescriptors != null ? spectrumRequest.MasterDeviceDescriptors.EtsiDeviceCategory : spectrumRequest.DeviceDescriptor.EtsiDeviceCategory,
                        EmissionClass = spectrumRequest.MasterDeviceDescriptors != null ? spectrumRequest.MasterDeviceDescriptors.EtsiEnDeviceEmissionsClass : spectrumRequest.DeviceDescriptor.EtsiEnDeviceEmissionsClass,
                        Capabilities = spectrumRequest.Capabilities,
                        MasterDeviceDescriptor = spectrumRequest.MasterDeviceDescriptors,
                        DeviceDescriptor = spectrumRequest.MasterDeviceDescriptors != null ? spectrumRequest.MasterDeviceDescriptors : spectrumRequest.DeviceDescriptor,
                        IncumbentType = Conversion.ToIncumbentType(spectrumRequest.MasterDeviceDescriptors != null ? spectrumRequest.MasterDeviceDescriptors.EtsiEnDeviceType : spectrumRequest.DeviceDescriptor.EtsiEnDeviceType)
                    };

                    if (spectrumRequest.Antenna != null)
                    {
                        currentIncumbent.Height = spectrumRequest.Antenna.Height;
                        currentIncumbent.HeightType = spectrumRequest.Antenna.HeightType;
                    }

                    wsdInfo.Add(currentIncumbent);
                }

                if (spectrumRequest.MasterDeviceLocation != null)
                {
                    var currentIncumbent = new Incumbent()
                    {
                        Location = spectrumRequest.MasterDeviceLocation.ToLocation(),
                        DeviceCategory = spectrumRequest.MasterDeviceDescriptors != null ? spectrumRequest.MasterDeviceDescriptors.EtsiDeviceCategory : spectrumRequest.DeviceDescriptor.EtsiDeviceCategory,
                        EmissionClass = spectrumRequest.MasterDeviceDescriptors != null ? spectrumRequest.MasterDeviceDescriptors.EtsiEnDeviceEmissionsClass : spectrumRequest.DeviceDescriptor.EtsiEnDeviceEmissionsClass,
                        Capabilities = spectrumRequest.Capabilities,
                        MasterDeviceDescriptor = spectrumRequest.MasterDeviceDescriptors,
                        DeviceDescriptor = spectrumRequest.MasterDeviceDescriptors != null ? spectrumRequest.MasterDeviceDescriptors : spectrumRequest.DeviceDescriptor,
                        IncumbentType = Conversion.ToIncumbentType(spectrumRequest.MasterDeviceDescriptors != null ? spectrumRequest.MasterDeviceDescriptors.EtsiEnDeviceType : spectrumRequest.DeviceDescriptor.EtsiEnDeviceType)
                    };

                    if (spectrumRequest.Antenna != null)
                    {
                        currentIncumbent.Height = spectrumRequest.Antenna.Height;
                        currentIncumbent.HeightType = spectrumRequest.Antenna.HeightType;
                    }

                    wsdInfo.Add(currentIncumbent);
                }
            }

            return wsdInfo.ToArray();
        }

        /// <summary>
        /// Gets the WSD information.
        /// </summary>
        /// <param name="parameters">The parameters.</param>
        /// <returns>returns Incumbent.</returns>
        public override Incumbent IncumbentInfoForGetChannelListRequest(Parameters parameters)
        {
            Incumbent incumbent = new Incumbent();
            incumbent.Location = parameters.Location.ToLocation();
            //// incumbent.IncumbentType = Conversion.ToIncumbentType(parameters.IncumbentType);
            incumbent.DeviceId = parameters.DeviceId;
            incumbent.RequestType = parameters.RequestType;
            incumbent.MaxMasterEIRP = parameters.MaxMasterEIRP;
            incumbent.PREFSENS = parameters.Prefsens;
            incumbent.IsTestingStage = false;
            if (parameters.TestingStage > 0)
            {
                incumbent.TestingStage = parameters.TestingStage;
                incumbent.IsTestingStage = true;
            }

            incumbent.PMSEAssignmentTable = parameters.PMSEAssignmentTableName;

            incumbent.StartTime = parameters.StartTime.ToDateTime("dd/MM/yyyy HH:mm");
            incumbent.UniqueId = parameters.UniqueId;

            if (parameters.Antenna != null)
            {
                incumbent.Height = parameters.Antenna.Height;
                incumbent.HeightType = parameters.Antenna.HeightType;
            }

            if (parameters.DeviceDescriptor != null)
            {
                incumbent.DeviceCategory = parameters.DeviceDescriptor.EtsiDeviceCategory;
                incumbent.EmissionClass = parameters.DeviceDescriptor.EtsiEnDeviceEmissionsClass;
                incumbent.IncumbentType = Conversion.ToIncumbentType(parameters.DeviceDescriptor.EtsiEnDeviceType);
            }

            return incumbent;
        }
    }
}
