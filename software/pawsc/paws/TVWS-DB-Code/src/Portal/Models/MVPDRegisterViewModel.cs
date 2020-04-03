// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Portal
{
    using System.Collections.Generic;
    using System.ComponentModel.DataAnnotations;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Common.Utilities;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WhiteSpaces.Common;
    using Microsoft.WhiteSpaces.Common.Entities;

    ///Validation failing. TODO find out the root cause
    public class MVPDRegisterViewModel //: IValidatableObject
    {        
        public string FriendlyName { get; set; }

        public string Description { get; set; }

        [Required(ErrorMessage = "Latitude is Required")]
        public string MVPDLocationLatittude { get; set; }

        [Required(ErrorMessage = "Longitude is Required")]
        public string MVPDLocationLongitude { get; set; }

        [Required(ErrorMessage = "Latitude is Required")]
        public string TransmitterLatittude { get; set; }

        [Required(ErrorMessage = "Longitude is Required")]
        public string TransmitterLongitude { get; set; }

        [Required(ErrorMessage = "MVPD Company is Required")]
        public string CableCompanyName { get; set; }

        public string Channel { get; set; }

        public string CallSign { get; set; }

        public string StartDate { get; set; }

        public string StartTime { get; set; }

        public string EndDate { get; set; }

        public string EndTime { get; set; }

        [Required(ErrorMessage = "Name is Required")]
        public string Name { get; set; }

        [Required(ErrorMessage = "Address1 is Required")]
        public string Address1 { get; set; }

        public string Address2 { get; set; }

        [Required(ErrorMessage = "City is Required")]
        public string City { get; set; }

        public string State { get; set; }

        [Required(ErrorMessage = "Country is Required")]
        public string Country { get; set; }

        [Required(ErrorMessage = "Email is Required")]
        public string Email { get; set; }

        public string Phone { get; set; }

        // TODO: Investigate should we need to validate device location on UX side or should it be done in back-end ?
        public IEnumerable<ValidationResult> Validate(ValidationContext validationContext)
        {
            double latitude = double.Parse(this.MVPDLocationLatittude);
            double longitude = double.Parse(this.MVPDLocationLongitude);

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
