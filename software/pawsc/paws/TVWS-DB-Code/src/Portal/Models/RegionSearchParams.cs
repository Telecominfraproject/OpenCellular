// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Portal.Models
{
    using System.Collections.Generic;
    using System.ComponentModel.DataAnnotations;
    using Microsoft.WhiteSpaces.Common;
    using Microsoft.WhiteSpaces.Common.Entities;    

    public class RegionSearchParams : IValidatableObject
    {
        [Display(Name = "Location")]
        [Required]
        public string Location { get; set; }

        [Display(Name = "CountryRegion")]
        [Required]
        public string CountryRegion { get; set; }

        [Display(Name = "Latitude")]
        [Required]
        [Range(-90, 90)]
        public double Latitude { get; set; }

        [Display(Name = "Longitude")]
        [Required]
        [Range(-180, 180)]
        public double Longitude { get; set; }

        [Display(Name = "AntennaHeight")]
        [Range(0, 30)]
        public double? AntennaHeight { get; set; }

        public string IncumbentType { get; set; }

        public string RegionCode { get; set; }

        public IEnumerable<ValidationResult> Validate(ValidationContext validationContext)
        {
            string regionCode;

            if (!CommonUtility.IsPointInRegulatoryBodyBoundary(this.Latitude, this.Longitude, out regionCode))
            {
                Region region = CommonUtility.GetRegionByName(this.CountryRegion);

                if (region == null)
                {
                    yield return new ValidationResult(string.Format("{0},{1} is outside the boundaries of supported CountryRegions", this.Location, this.CountryRegion), new List<string> { "Location" });
                }
                else
                {
                    regionCode = region.Regulatory.RegionCode;
                }
            }

            this.RegionCode = regionCode;
        }
    }
}
