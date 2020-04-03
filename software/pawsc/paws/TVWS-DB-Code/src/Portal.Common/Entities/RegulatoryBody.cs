// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common.Entities
{
    using System.Collections.Generic;

    public class RegulatoryBody
    {
        public RegulatoryBody(string name, WhitespaceChannelsInfo whitespaceChannelsInfo, string serviceApi, IEnumerable<DeviceType> supportedDeviceTypes, IEnumerable<string> supportedFeatures, bool allowAccessToProtectedArea, string regionCode)
        {
            this.Name = name;
            this.WSChannelsInfo = whitespaceChannelsInfo;
            this.ServiceApi = serviceApi;
            this.SupportedDeviceTypes = supportedDeviceTypes;
            this.AllowAccessToProtedtedArea = allowAccessToProtectedArea;
            this.SupportedFeatures = supportedFeatures;
            this.RegionCode = regionCode;
        }

        public string Name { get; private set; }

        public WhitespaceChannelsInfo WSChannelsInfo { get; private set; }

        public string ServiceApi { get; private set; }

        public IEnumerable<DeviceType> SupportedDeviceTypes { get; private set; }

        public bool AllowAccessToProtedtedArea { get; private set; }

        public IEnumerable<string> SupportedFeatures { get; private set; }

        public string RegionCode { get; private set; }
    }
}
