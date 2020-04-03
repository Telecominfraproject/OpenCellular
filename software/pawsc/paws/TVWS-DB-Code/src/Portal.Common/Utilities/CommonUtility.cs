// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common
{
    using System.Collections;
    using System.Collections.Generic;
    using System.Linq;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Common.Utilities;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WhiteSpaces.Common.Entities;
    using Microsoft.WhiteSpaces.Common.Enums;   

    public class CommonUtility
    {
        public static Region GetRegionByName(string regionName)
        {
            AppConfigRegionSource source = new AppConfigRegionSource();
            return source.GetAvailableRegions().Where(x => string.Compare(x.RegionInformation.Name, regionName, System.StringComparison.OrdinalIgnoreCase) == 0).FirstOrDefault();
        }

        public static ChannelOperationMode GetChannelOperationMode(double maxPowerDBm, double powerDBmTransitionPoint)
        {
            if (maxPowerDBm >= powerDBmTransitionPoint)
            {
                return ChannelOperationMode.HighPower;
            }

            return ChannelOperationMode.LowPower;
        }

        public static bool IsFeatureAvailableForRegion(string regionId, string featureName)
        {
            AppConfigRegionSource source = new AppConfigRegionSource();
            IEnumerable<string> availableFeatures = source.GetAvailableRegions().Where(x => x.RegionInformation.Id == regionId).FirstOrDefault().Regulatory.SupportedFeatures;

            if (availableFeatures.Count() > 0)
            {
                return availableFeatures.Contains(featureName);
            }

            return false;
        }

        public static bool IsPointInRegulatoryBodyBoundary(double latitude, double longitude, out string regionCode)
        {
            bool isInBoundary = false;
            regionCode = null;

            Dictionary<string, List<RegionPolygonsCache>> regionPolygons = (Dictionary<string, List<RegionPolygonsCache>>)DatabaseCache.ServiceCacheHelper.GetServiceCacheObjects(ServiceCacheObjectType.RegionPolygons, null);

            foreach (var regionPolygon in regionPolygons)
            {
                isInBoundary = GeoCalculations.IsPointInRegionPolygons(regionPolygon.Value, latitude, longitude);

                if (isInBoundary)
                {
                    regionCode = regionPolygon.Key;
                    break;
                }
            }

            return isInBoundary;
        }
    }
}
