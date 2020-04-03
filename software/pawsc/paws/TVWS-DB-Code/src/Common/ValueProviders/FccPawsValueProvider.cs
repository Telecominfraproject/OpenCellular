// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common.ValueProviders
{
    using System;
    using System.Collections.Generic;
    using Entities;
    using Utilities;

    /// <summary>
    /// Represents FCC PawsValueProvider
    /// </summary>
    public class FccPawsValueProvider : PawsRegionalValueProvider
    {
        /// <summary>
        /// Gets the device id.
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

            return deviceData.FccId;
        }

        /// <summary>
        /// Gets the Rule Set information.
        /// </summary>
        /// <param name="deviceDescriptor">The Device Descriptor.</param>
        /// <returns>returns RULESET</returns>        
        public override List<RulesetInfo> GetRuleSetInfo(DeviceDescriptor deviceDescriptor)
        {
            List<RuleSetInfoEntity> ruleSet = this.PawsDalc.GetRulesetInfo(Utils.GetRegionalTableName(Constants.RuleSetInformationTable));
            List<RulesetInfo> ruleSetList = new List<RulesetInfo>();
            foreach (RuleSetInfoEntity rs in ruleSet)
            {
                rs.PartitionKey = null;
                rs.RowKey = null;
                ruleSetList.Add(rs.ToRuleSetInfo());
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
                Incumbent incumbent = new Incumbent()
                                      {
                                          Location = spectrumRequest.MasterDeviceLocation != null ? spectrumRequest.MasterDeviceLocation.ToLocation() : spectrumRequest.Location.ToLocation(),
                                          IncumbentType = Conversion.ToIncumbentType(spectrumRequest.MasterDeviceDescriptors != null ? spectrumRequest.MasterDeviceDescriptors.FccTvbdDeviceType : spectrumRequest.DeviceDescriptor.FccTvbdDeviceType),
                                      };

                if (spectrumRequest.Antenna != null)
                {
                    incumbent.Height = spectrumRequest.Antenna.Height;
                }

                wsdInfo.Add(incumbent);
            }
            else if (requestType == typeof(IBatchAvailableSpectrumRequest))
            {
                var spectrumRequest = request as IBatchAvailableSpectrumRequest;
                foreach (var receiverLocation in spectrumRequest.Locations)
                {
                    var currentIncumbent = new Incumbent()
                    {
                        Location = receiverLocation.ToLocation(),
                        IncumbentType = Conversion.ToIncumbentType(spectrumRequest.DeviceDescriptor.FccTvbdDeviceType)
                    };

                    if (spectrumRequest.Antenna != null)
                    {
                        currentIncumbent.Height = spectrumRequest.Antenna.Height;
                    }

                    wsdInfo.Add(currentIncumbent);
                }

                if (spectrumRequest.MasterDeviceLocation != null)
                {
                    var currentIncumbent = new Incumbent()
                    {
                        Location = spectrumRequest.MasterDeviceLocation.ToLocation(),
                        IncumbentType = Conversion.ToIncumbentType(spectrumRequest.DeviceDescriptor.FccTvbdDeviceType)
                    };

                    if (spectrumRequest.Antenna != null)
                    {
                        currentIncumbent.Height = spectrumRequest.Antenna.Height;
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
            incumbent.IncumbentType = Conversion.ToIncumbentType(parameters.IncumbentType);
            if (parameters.Location != null)
            {
                incumbent.Location = parameters.Location.ToLocation();
            }

            incumbent.PointsArea = parameters.PointsArea;
            incumbent.QuadrilateralArea = parameters.QuadrilateralArea;

            return incumbent;
        }
    }
}
