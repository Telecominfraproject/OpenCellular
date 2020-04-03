// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common
{
    using System.Configuration;
    using Microsoft.WhiteSpaces.Common.Configurations;

    public class RegionConfigurationElement : ConfigurationElement
    {
        [ConfigurationProperty("name", IsRequired = true, IsKey = true)]
        public string Name
        {
            get { return (string)this["name"]; }
        }

        [ConfigurationProperty("id", IsRequired = true)]
        public string Id
        {
            get { return (string)this["id"]; }
        }

        [ConfigurationProperty("latitude", IsRequired = true)]
        public double Latitude
        {
            get { return (double)this["latitude"]; }
        }

        [ConfigurationProperty("longitude", IsRequired = true)]
        public double Longitude
        {
            get { return (double)this["longitude"]; }
        }

        [ConfigurationProperty("imagePath", IsRequired = true)]
        public string ImagePath
        {
            get { return (string)this["imagePath"]; }
        }

        [ConfigurationProperty("Api", IsRequired = true)]
        public string ApiUrl
        {
            get { return (string)this["Api"]; }
        }

        [ConfigurationProperty("ChannelStart", DefaultValue = 2)]
        public int ChannelStart
        {
            get { return (int)this["ChannelStart"]; }
        }

        [ConfigurationProperty("ChannelEnd", DefaultValue = 51)]
        public int ChannelEnd
        {
            get { return (int)this["ChannelEnd"]; }
        }

        [ConfigurationProperty("PowerDBmTransitionPoint", IsRequired = true)]
        public double PowerDBmTransitionPoint
        {
            get { return (double)this["PowerDBmTransitionPoint"]; }
        }

        [ConfigurationProperty("EnableProtectedAreas", DefaultValue = false)]
        public bool EnableProtectedAreas
        {
            get { return (bool)this["EnableProtectedAreas"]; }
        }

        [ConfigurationProperty("deviceTypes", DefaultValue = null)]
        public DeviceTypesConfigurationElementCollection DeviceTypes
        {
            get { return (DeviceTypesConfigurationElementCollection)this["deviceTypes"]; }
        }

        [ConfigurationProperty("features", DefaultValue = null)]
        public FeaturesConfigurationElementCollection Features
        {
            get { return (FeaturesConfigurationElementCollection)this["features"]; }
        }

        [ConfigurationProperty("PropagationModel", IsRequired = true)]
        public string PropagationModel
        {
            get { return (string)this["PropagationModel"]; }
        }

        [ConfigurationProperty("ChannelBandwidth", IsRequired = true)]
        public int ChannelBandwidth
        {
            get { return (int)this["ChannelBandwidth"]; }
        }

        [ConfigurationProperty("RegulatoryName", IsRequired = true)]
        public string RegulatoryBodyName
        {
            get { return (string)this["RegulatoryName"]; }
        }

        [ConfigurationProperty("RegionCode")]
        public string RegionCode
        {
            get { return (string)this["RegionCode"]; }
        }
    }
}
