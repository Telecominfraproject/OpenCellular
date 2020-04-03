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
    public class TBasLinkRegistration //: IValidatableObject
    {
        [Required(ErrorMessage = "Required")]
        public string CallSign { get; set; }

        [Required(ErrorMessage = "Required")]
        public string TransmitterLatitude { get; set; }

        [Required(ErrorMessage = "Required")]
        public string TransmitterLongitude { get; set; }

        [Required(ErrorMessage = "Required")]
        public string RecieverLatitude { get; set; }

        [Required(ErrorMessage = "Required")]
        public string RecieverLongitude { get; set; }

        [Required(ErrorMessage = "Required")]
        public int[] Channels { get; set; }

        [Required(ErrorMessage = "Required")]
        public string StartDate { get; set; }

        [Required(ErrorMessage = "Required")]
        public string EndDate { get; set; }

        [Required(ErrorMessage = "Required")]
        public string StartTime { get; set; }

        [Required(ErrorMessage = "Required")]
        public string EndTime { get; set; }

        [Required(ErrorMessage = "Required")]
        public string Address1 { get; set; }

        public string Address2 { get; set; }

        public string Phone { get; set; }

        [Required(ErrorMessage = "Required")]
        public string Email { get; set; }

        public string City { get; set; }

        [Required(ErrorMessage = "Required")]
        public string Country { get; set; }

        public string State { get; set; }

        [Required(ErrorMessage = "Required")]
        public string FriendlyName { get; set; }

        public string Description { get; set; }

        public string TimeZone { get; set; }

        public string Name { get; set; }

        // TODO: Investigate should we need to validate device location on UX side or should it be done in back-end ?
        public IEnumerable<ValidationResult> Validate(ValidationContext validationContext)
        {
            double latitude = double.Parse(this.RecieverLatitude);
            double longitude = double.Parse(this.RecieverLongitude);

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
