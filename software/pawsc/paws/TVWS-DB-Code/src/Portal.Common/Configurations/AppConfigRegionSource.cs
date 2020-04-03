// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common
{
    using System;
    using System.Collections.Generic;
    using System.Configuration;
    using Microsoft.WhiteSpaces.Common.Configurations;
    using Microsoft.WhiteSpaces.Common.Entities;

    public class AppConfigRegionSource : IRegionSource
    {
        private readonly Lazy<List<Region>> regionConfig = new Lazy<List<Region>>(() =>
            {
                List<Region> regions = new List<Region>();
                string configSectionName = "availableRegions";

                AvailableRegionsConfigurationSection configSection = ConfigurationManager.GetSection(configSectionName) as AvailableRegionsConfigurationSection;

                if (configSection == null)
                {
                    string message = string.Format("The .exe.config file does not contain a {0} section.", configSectionName);
                    throw new InvalidOperationException(message);
                }

                if (configSection.AvailableRegions != null)
                {
                    for (int index = 0; index < configSection.AvailableRegions.Count; index++)
                    {
                        RegionConfigurationElement regionConfigurationElement = configSection.AvailableRegions[index];

                        RegionInfo regionInfo = new RegionInfo(
                            regionConfigurationElement.Name,
                            regionConfigurationElement.Id,
                            regionConfigurationElement.Latitude,
                            regionConfigurationElement.Longitude,
                            regionConfigurationElement.ImagePath);

                        WhitespaceChannelsInfo wsChannelsInfo = new WhitespaceChannelsInfo(
                            regionConfigurationElement.ChannelStart,
                            regionConfigurationElement.ChannelEnd,
                            regionConfigurationElement.ChannelBandwidth,
                            regionConfigurationElement.PropagationModel,
                            regionConfigurationElement.PowerDBmTransitionPoint);

                        List<DeviceType> deviceTypes = new List<DeviceType>();

                        for (int deviceIndex = 0; deviceIndex < regionConfigurationElement.DeviceTypes.Count; deviceIndex++)
                        {
                            DeviceTypeConfigurationElement deviceConfigurationElement = regionConfigurationElement.DeviceTypes[deviceIndex];

                            deviceTypes.Add(new DeviceType(deviceConfigurationElement.FriendlyName, deviceConfigurationElement.Type));
                        }

                        List<string> features = new List<string>();

                        for (int featureIndex = 0; featureIndex < regionConfigurationElement.Features.Count; featureIndex++)
                        {
                            FeatureConfigurationElement featureConfigurationElement = regionConfigurationElement.Features[featureIndex];

                            features.Add(featureConfigurationElement.FriendlyName);
                        }

                        RegulatoryBody regulatoryBody = new RegulatoryBody(
                            regionConfigurationElement.RegulatoryBodyName,
                            wsChannelsInfo,
                            regionConfigurationElement.ApiUrl,
                            deviceTypes,
                            features,
                            regionConfigurationElement.EnableProtectedAreas,
                            regionConfigurationElement.RegionCode);

                        regions.Add(new Region(regionInfo, regulatoryBody));
                    }
                }

                return regions;
            });

        public static IEnumerable<Role> GetRoles()
        {
            List<Role> accessLevels = new List<Role>();
            string configSectionName = "accessLevels";

            AccessLevelsConfigurationSection configSection = ConfigurationManager.GetSection(configSectionName) as AccessLevelsConfigurationSection;

            if (configSection == null)
            {
                string message = string.Format("The .exe.config file does not contain a {0} section.", configSectionName);
                throw new InvalidOperationException(message);
            }

            if (configSection.AccessLevels != null)
            {
                for (int index = 0; index < configSection.AccessLevels.Count; index++)
                {
                    AceesLevelConfigurationElement element = configSection.AccessLevels[index];
                    accessLevels.Add(new Role(element.Name, element.Id, element.FriendlyName, element.ApplicableRegulatories.Split(',')));
                }
            }

            return accessLevels;
        }

        public List<Region> GetAvailableRegions()
        {
            return this.regionConfig.Value;
        }        
    }
}
