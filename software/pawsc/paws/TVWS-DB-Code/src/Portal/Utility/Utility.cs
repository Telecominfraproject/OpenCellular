// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Portal
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Web;
    using System.Xml.Linq;
    using Microsoft.Practices.Unity;       
    using Microsoft.WhiteSpaces.AzureOAuthProvider;
    using Microsoft.WhiteSpaces.AzureTableAccess;
    using Microsoft.WhiteSpaces.Common;         
    using Microsoft.WindowsAzure.ServiceRuntime;

    public class Utility
    {
        public static IEnumerable<string> GetCounties()
        {
            List<string> countryList = new List<string>();

            // Iterate the Framework Cultures...
            foreach (CultureInfo ci in CultureInfo.GetCultures(System.Globalization.CultureTypes.AllCultures))
            {
                RegionInfo ri = null;
                try
                {
                    ri = new RegionInfo(ci.Name);
                }
                catch
                {
                    // If a RegionInfo object could not be created we don't want to use the CultureInfo
                    //    for the country list.
                    continue;
                }

                if (!countryList.Contains(ri.EnglishName))
                {
                    countryList.Add(ri.EnglishName);
                }
            }

            countryList.Sort();
            countryList.MoveItemAtIndexToFront(countryList.IndexOf("Kenya"));
            countryList.MoveItemAtIndexToFront(countryList.IndexOf("Singapore"));
            countryList.MoveItemAtIndexToFront(countryList.IndexOf("United Kingdom"));
            countryList.MoveItemAtIndexToFront(countryList.IndexOf("United States"));

            return countryList;
        }

        public static IEnumerable<string> GetTimeZones()
        {
            List<string> timezoneList = new List<string>();

            var timeZones = TimeZoneInfo.GetSystemTimeZones();
            foreach (var timezone in timeZones)
            {
                timezoneList.Add(timezone.DisplayName);
            }

            return timezoneList;
        }

        public static List<AccessLevel> GetAllAccessLevels()
        {
            var userManager = ConfigHelper.CurrentContainer.Resolve<IUserManager>();

            return userManager.GetAllAccessLevels().Where(x => (Convert.ToInt32(x.RowKey) > 1 && (Convert.ToInt32(x.RowKey) < 5))).ToList();
        }

        public static List<Authority> GetAllAuthorities()
        {
            var userManager = ConfigHelper.CurrentContainer.Resolve<IUserManager>();
            var regionSource = ConfigHelper.CurrentContainer.Resolve<IRegionSource>();
            var regions = regionSource.GetAvailableRegions();
            List<Authority> authorities = new List<Authority>();

            foreach (var region in regions)
            {
               var authority = new Authority
                {
                    RowKey = region.RegionInformation.Id,
                    AuthorityName = ((Authorities)Convert.ToInt32(region.RegionInformation.Id)).ToString()
                };

               authorities.Add(authority);
            }

            return authorities;
        }

        public static Microsoft.WhiteSpaces.Common.Role[] GetUpgradableAccessLevels(int currentAccessLevel, string regulatory)
        {
            return AppConfigRegionSource.GetRoles().Where(x => x.Id > currentAccessLevel && x.ApplicableRegulatories.Contains(regulatory)).ToArray();
        }

        public static IEnumerable<string> GetCountryPhoneCodes()
        {
            List<string> codeDictionary = new List<string>();
            string filePath = string.Empty;

            if (RoleEnvironment.IsAvailable)
            {
                filePath = Environment.GetEnvironmentVariable("RoleRoot") + "\\approot\\bin\\CountryCodes.config";
                if (!File.Exists(filePath))
                {
                    filePath = Environment.GetEnvironmentVariable("RoleRoot") + "\\approot\\CountryCodes.config";
                }
            }
            else if (HttpContext.Current != null && HttpContext.Current.Server != null)
            {
                filePath = HttpContext.Current.Server.MapPath(@"~\bin") + "\\CountryCodes.config";
            }

            XDocument xmlDoc = XDocument.Load(filePath);           
            var countryCodes = xmlDoc.Descendants("data-set").Elements("countrycode");

            foreach (var countryCode in countryCodes)
            {
                var country = countryCode.Elements("country").Select(r => r.Value).FirstOrDefault();
                var code = countryCode.Elements("code").Select(r => r.Value).FirstOrDefault();

                codeDictionary.Add(country + "(+" + code + ")");
            }

            return codeDictionary;
        }
    }
}
