// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common
{
    using System.Collections.Generic;
    using System.ComponentModel.DataAnnotations;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Common.Utilities;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WhiteSpaces.Common.Entities;

    ///Validation failing. TODO find out the root cause
    public class LicensedLpAuxRegistration //: IValidatableObject
    {
        public string CallSign { get; set; }

        [Required(ErrorMessage = "Required")]
        public string Latitude { get; set; }

        [Required(ErrorMessage = "Required")]
        public string Longitude { get; set; }

        public int[] Channels { get; set; }

        public string StartTime { get; set; }

        public string EndTime { get; set; }

        public string StartDate { get; set; }

        public string EndDate { get; set; }

        public bool IsRecurred { get; set; }

        public string[] WeekDays { get; set; }

        public string ReoccurrenceEndDate { get; set; }

        public bool IsReoccurenceDaily { get; set; }

        public bool IsReoccurenceWeekly { get; set; }

        public bool IsReOccurenceMonthly { get; set; }

        public string Name { get; set; }

        public string Address1 { get; set; }

        public string Address2 { get; set; }

        public string Phone { get; set; }

        public string Email { get; set; }

        public string City { get; set; }

        public string Country { get; set; }

        public string State { get; set; }

        public string FriendlyName { get; set; }

        public string Description { get; set; }

        public string TimeZone { get; set; }

        public string IncumbentType { get; set; }

        public string UlsFileNumber { get; set; }

        public ChannelInfo[] Channellist { get; set; }

        public string ChannelListString { get; set; }

        public string WeekDaysString { get; set; }

        public int ReoccurenceInstance { get; set; }

        public string GrantDate { get; set; }

        public string ExpireDate { get; set; }

        public string VenueName { get; set; }

        public string ResponsibleParty { get; set; }

        public string ContactPhone { get; set; }

        public string OrgName { get; set; }

        // TODO: Investigate should we need to validate device location on UX side or should it be done in back-end ?
        public IEnumerable<ValidationResult> Validate(ValidationContext validationContext)
        {
            double latitude = double.Parse(this.Latitude);
            double longitude = double.Parse(this.Longitude);

            bool isValidLocation = false;

            Region region = CommonUtility.GetRegionByName(this.Country);
            string regionCode = region.Regulatory.RegionCode;

            Dictionary<string, List<RegionPolygonsCache>> regionPolygons = (Dictionary<string, List<RegionPolygonsCache>>)DatabaseCache.ServiceCacheHelper.GetServiceCacheObjects(ServiceCacheObjectType.RegionPolygons, null);

            if (region != null && regionPolygons.ContainsKey(regionCode))
            {
                isValidLocation = GeoCalculations.IsPointInRegionPolygons(regionPolygons[regionCode], latitude, longitude);
            }

            if (!isValidLocation)
            {
                yield return new ValidationResult(string.Format("{0},{1} is outside the boundaries of supported CountryRegions", latitude, longitude), new List<string> { "City" });
            }
        }
    }
}
