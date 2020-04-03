// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Portal.Controllers
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using System.Web.Mvc;    
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WhiteSpaces.AzureOAuthProvider;
    using Microsoft.WhiteSpaces.BusinessManager;
    using Microsoft.WhiteSpaces.Common;
    using Microsoft.WhiteSpaces.Common.Entities;
    using Microsoft.WhiteSpaces.Common.Enums;

    public class RegionManagementController : Controller
    {
        private const int PageSize = 5;
        private readonly IRegionManager regionManager;
        private readonly IWhitespacesManager whitespacesManager;
        private readonly IRegionSource regionSource;        

        public RegionManagementController(IRegionManager regionManager, IWhitespacesManager whitespacesManager, IRegionSource regionSource)
        {
            Microsoft.WhiteSpaces.Common.Check.IsNotNull(regionManager, "User Manager");
            Microsoft.WhiteSpaces.Common.Check.IsNotNull(whitespacesManager, "Whitespaces Manager");
            Microsoft.WhiteSpaces.Common.Check.IsNotNull(regionSource, "Region Source");

            this.regionManager = regionManager;
            this.whitespacesManager = whitespacesManager;
            this.regionSource = regionSource;
        }

        [Dependency]
        public IAuditor RegionManagementAuditor { get; set; }

        [Dependency]
        public ILogger RegionManagementLogger { get; set; }        

        public ActionResult Index()
        {
            UserPrincipal userPrincipal = (UserPrincipal)User;
            UserDetails userDetails = userPrincipal.UserManager.GetUserDetailsByAccessToken(userPrincipal.AccessToken);

            var regions = this.regionSource.GetAvailableRegions();
            List<SelectListItem> regionList = new List<SelectListItem>();

            foreach (var region in regions)
            {
                if (this.IsAuthorizedForRegion(userDetails, region))
                {
                    regionList.Add(new SelectListItem { Text = region.RegionInformation.Name, Value = region.RegionInformation.Name });
                }
            }

            ViewBag.Regions = regionList;

            return this.View();
        }

        public JsonResult BlockChannel(int[] selectedChannels, string locatioList, string regionName, bool regionBlocked)
        {
            UserPrincipal userPrincipal = (UserPrincipal)User;
            var userDetails = userPrincipal.UserManager.GetUserDetailsByAccessToken(userPrincipal.AccessToken);
            Point[] locations = null;

            if (regionBlocked)
            {
                if (regionName == "United States")
                {
                    locations = new Point[] { new Point { Latitude = "49.457546", Longitude = "-127.807338" }, new Point { Latitude = "47.595189", Longitude = "-69.360073" }, new Point { Latitude = "25.249850", Longitude = "-81.049526" }, new Point { Latitude = "32.588650", Longitude = "-117.480190" } };
                }
                else if (regionName == "United Kingdom")
                {
                    locations = new Point[] { new Point { Latitude = "61.172593", Longitude = "-12.032597" }, new Point { Latitude = "60.949320", Longitude = "3.194453" }, new Point { Latitude = "50.328206", Longitude = "-11.263554" }, new Point { Latitude = "50.941396", Longitude = "1.129024" } };
                }
            }
            else
            {
                locations = JsonHelper.DeserializeObject<Point[]>(locatioList);
            }

            var result = this.whitespacesManager.ExcludeChannel(selectedChannels, locations, userPrincipal.AccessToken, regionName);

            StringBuilder channels = new StringBuilder();
            foreach (int channel in selectedChannels)
            {
                channels.Append(channel.ToString());
                if (selectedChannels[selectedChannels.Length - 1] != channel)
                {
                    channels.Append(", ");
                }
            }

            var region = CommonUtility.GetRegionByName(regionName);
            this.RegionManagementAuditor.UserId = userDetails.UserInfo.RowKey;
            this.RegionManagementAuditor.RegionCode = region != null ? Convert.ToInt32(region.RegionInformation.Id) : 0;
            this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
            this.RegionManagementAuditor.Audit(AuditId.BlockChannel, AuditStatus.Success, default(int), channels.ToString() + " channels has been blocked in " + regionName + " by" + userPrincipal.UserName);

            if (string.Equals(result, Microsoft.WhiteSpaces.Common.Constants.ExcludedIdSuccessfully, StringComparison.OrdinalIgnoreCase))
            {
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.PortalExcludeChannel, channels.ToString() + " channels has been blocked in " + regionName + " by" + userPrincipal.UserName);
            }
            else
            {
                this.RegionManagementLogger.Log(TraceEventType.Error, LoggingMessageId.PortalExcludeChannel, userDetails.UserInfo.UserName + " not able to exclude channels because of error " + result);
            }

            return this.Json(result);
        }

        // region Name is temporary, need to remove
        public JsonResult ExcludeDevice(string deviceId, string serialNumber, string regionName = "United States")
        {
            UserPrincipal userPrincipal = (UserPrincipal)User;
            var userDetails = userPrincipal.UserManager.GetUserDetailsByAccessToken(userPrincipal.AccessToken);
            var result = this.whitespacesManager.ExcludeDevice(userPrincipal.AccessToken, regionName, deviceId, serialNumber);

            var region = CommonUtility.GetRegionByName(regionName);
            this.RegionManagementAuditor.UserId = userDetails.UserInfo.RowKey;
            this.RegionManagementAuditor.RegionCode = region != null ? Convert.ToInt32(region.RegionInformation.Id) : 0;
            this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;

            if (string.IsNullOrEmpty(serialNumber))
            {
                this.RegionManagementAuditor.Audit(AuditId.BlockDevice, AuditStatus.Success, default(int), "Device with Id " + deviceId + " is blocked by " + userDetails.UserInfo.UserName);
            }
            else
            {
                this.RegionManagementAuditor.Audit(AuditId.BlockDevice, AuditStatus.Success, default(int), "Device with Id " + deviceId + " and serial number" + serialNumber + " is blocked by " + userDetails.UserInfo.UserName);
            }

            if (string.Equals(result, Microsoft.WhiteSpaces.Common.Constants.ExcludedIdSuccessfully, StringComparison.OrdinalIgnoreCase))
            {
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.PortalExcludeId, "Device with Id " + deviceId + " and serial number" + serialNumber + " is excluded by " + userDetails.UserInfo.UserName);
            }
            else
            {
                this.RegionManagementLogger.Log(TraceEventType.Error, LoggingMessageId.PortalExcludeId, userDetails.UserInfo.UserName + " not able to exclude id because of error " + result);
            }

            return this.Json(result);
        }

        public PartialViewResult GetMvpdRegistrations(int page = 1)
        {
            IEnumerable<MVPDRegistration> registrations = this.regionManager.GetMvpdRegistrations(string.Empty);

            if (page < 1)
            {
                page = 1;
            }

            var pagedregistrations = registrations.Skip((page - 1) * PageSize).Take(PageSize);
            ViewBag.mvpdRowCount = registrations.Count();

            return this.PartialView("MvpdRegistrationsPartial", pagedregistrations);
        }

        public PartialViewResult GetLpAuxRegistrations(int page = 1)
        {
            IEnumerable<LPAuxRegistration> registrations = this.regionManager.GetLpAuxRegistrations(string.Empty);

            if (page < 1)
            {
                page = 1;
            }

            var pagedregistrations = registrations.Skip((page - 1) * PageSize).Take(PageSize);
            ViewBag.lpauxRowCount = registrations.Count();

            return this.PartialView("LpauxRegistrations", pagedregistrations);
        }

        public PartialViewResult GetTempBasRegistrations(int page = 1)
        {
            IEnumerable<TempBASRegistration> registrations = this.regionManager.GetTempBasRegistrations(string.Empty);

            if (page < 1)
            {
                page = 1;
            }

            var pagedregistrations = registrations.Skip((page - 1) * PageSize).Take(PageSize);
            ViewBag.tbasRowCount = registrations.Count();

            return this.PartialView("TempBasRegistrations", registrations);
        }

        public void DeleteRegistration(string partitionKey, string rowkey, string type, string etag)
        {
            UserPrincipal userPrincipal = (UserPrincipal)User;
            var userDetails = userPrincipal.UserManager.GetUserDetailsByAccessToken(userPrincipal.AccessToken);

            RegistrationType registrationType = (RegistrationType)Enum.Parse(typeof(RegistrationType), type);

            this.regionManager.DeleteRegistration(partitionKey, rowkey, etag, registrationType);

            this.RegionManagementAuditor.UserId = userDetails.UserInfo.RowKey;
            this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
            this.RegionManagementAuditor.Audit(AuditId.DeleteRegistration, AuditStatus.Success, default(int), userPrincipal.UserName + " deleted registration of type " + type);
        }

        public PartialViewResult GetExcludedIds(int page = 1)
        {
            UserPrincipal userPrincipal = (UserPrincipal)User;
            var userDetails = userPrincipal.UserManager.GetUserDetailsByAccessToken(userPrincipal.AccessToken);

            List<ExcludedDevice> allRegionsExcludedIds = new List<ExcludedDevice>();
            List<Region> allRegions = this.regionSource.GetAvailableRegions();

            foreach (var eachRegion in allRegions)
            {
                if (!string.IsNullOrEmpty(eachRegion.Regulatory.RegionCode))
                {
                    if (this.IsAuthorizedForRegion(userDetails, eachRegion))
                    {
                        IEnumerable<ExcludedDevice> excludedIds = this.regionManager.GetExcludedIdsByRegionCode(eachRegion.Regulatory.RegionCode);
                        Parallel.ForEach(
                            excludedIds, 
                            excludedId =>
                            {
                                excludedId.RegionCode = eachRegion.Regulatory.RegionCode;
                                excludedId.Region = eachRegion.RegionInformation.Name;
                            });

                        allRegionsExcludedIds.AddRange(excludedIds);
                    }
                }
            }

            ViewBag.IdsRowCount = allRegionsExcludedIds.Count();

            if (page < 1)
            {
                page = 1;
            }

            return this.PartialView("ExcludedIdsPartial", allRegionsExcludedIds.Skip((page - 1) * PageSize).Take(PageSize));
        }

        public void DeleteExcludedId(string regionCode, string partitionKey, string rowKey)
        {
            UserPrincipal userPrincipal = (UserPrincipal)User;
            var userDetails = userPrincipal.UserManager.GetUserDetailsByAccessToken(userPrincipal.AccessToken);

            this.regionManager.DeleteExcludedId(regionCode, partitionKey, rowKey);

            this.RegionManagementAuditor.UserId = userDetails.UserInfo.RowKey;
            this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
            this.RegionManagementAuditor.Audit(AuditId.BlockDevice, AuditStatus.Success, default(int), userPrincipal.UserName + " deleted Excluded Id");
        }

        public PartialViewResult GetExcludedChannels(int page = 1)
        {
            UserPrincipal userPrincipal = (UserPrincipal)User;
            var userDetails = userPrincipal.UserManager.GetUserDetailsByAccessToken(userPrincipal.AccessToken);

            List<BlockedChannels> allRegionsExcludedChannles = new List<BlockedChannels>();
            List<Region> allRegions = this.regionSource.GetAvailableRegions();

            foreach (var eachRegion in allRegions)
            {
                if (!string.IsNullOrEmpty(eachRegion.Regulatory.RegionCode))
                {
                    if (this.IsAuthorizedForRegion(userDetails, eachRegion))
                    {
                        IEnumerable<BlockedChannels> blockedChannels = this.regionManager.GetExcludedChannelsByRegionCode(eachRegion.Regulatory.RegionCode);
                        Parallel.ForEach(
                            blockedChannels, 
                            excludedChannels =>
                            {
                                excludedChannels.RegionCode = eachRegion.Regulatory.RegionCode;
                                excludedChannels.Region = eachRegion.RegionInformation.Name;
                            });

                        allRegionsExcludedChannles.AddRange(blockedChannels);
                    }
                }
            }

            ViewBag.channelsRowCount = allRegionsExcludedChannles.Count();

            if (page < 1)
            {
                page = 1;
            }

            return this.PartialView("ExcludedChannelsPartial", allRegionsExcludedChannles.Skip((page - 1) * PageSize).Take(PageSize));
        }

        public void DeleteExcludedChannel(string regionCode, string partitionKey, string rowKey)
        {
            UserPrincipal userPrincipal = (UserPrincipal)User;
            var userDetails = userPrincipal.UserManager.GetUserDetailsByAccessToken(userPrincipal.AccessToken);

            this.regionManager.DeleteExcludedChannels(regionCode, partitionKey, rowKey);

            this.RegionManagementAuditor.UserId = userDetails.UserInfo.RowKey;
            this.RegionManagementAuditor.TransactionId = this.RegionManagementLogger.TransactionId;
            this.RegionManagementAuditor.Audit(AuditId.BlockedChannel, AuditStatus.Success, default(int), userPrincipal.UserName + " deleted Excluded Id");
        }

        private bool IsAuthorizedForRegion(UserDetails user, Region region)
        {
            var isValid = false;

            if (user.UserInfo.IsSuperAdmin)
            {
                isValid = true;
            }
            else
            {
                var accessInfo = user.AccessInfo.Where(x => x.Authority == (Authorities)Convert.ToInt16(region.RegionInformation.Id)).FirstOrDefault();

                if (accessInfo.AccessLevel == AccessLevels.Admin)
                {
                    isValid = true;
                }
            }

            return isValid;
        }
    }
}
