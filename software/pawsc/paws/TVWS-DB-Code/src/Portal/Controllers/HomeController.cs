// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Portal
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Web.Mvc;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WhiteSpaces.BusinessManager;
    using Microsoft.WhiteSpaces.Common;
    using Microsoft.WhiteSpaces.Common.Entities;
    using Microsoft.WhiteSpaces.Common.Enums;
    using Microsoft.WhiteSpaces.Portal.Models;

    /// <summary>
    /// Home page in the portal
    /// </summary>
    [AllowAnonymous]
    public class HomeController : Controller
    {
        private readonly IWhitespacesManager whitespacesManager;
        private readonly IRegionSource regionSource;

        public HomeController(IWhitespacesManager whitespacesManager, IRegionSource regionSource)
        {
            if (whitespacesManager == null)
            {
                throw new ArgumentNullException("whitespacesManager");
            }

            if (regionSource == null)
            {
                throw new ArgumentNullException("regionSource");
            }

            this.whitespacesManager = whitespacesManager;
            this.regionSource = regionSource;
        }

        /// <summary>
        /// Returns Index view 
        /// </summary>
        /// <returns>Index view</returns>
        public ActionResult Index()
        {
            this.ViewData["availableRegions"] = this.regionSource.GetAvailableRegions().Select(region => region.RegionInformation).ToList();

            return this.View();
        }

        public ActionResult GetAvailableRegions()
        {
            List<RegionInfo> regionInfo = this.GetAllRegions();

            if (this.Request.IsAjaxRequest())
            {
                return this.Json(regionInfo);
            }

            this.ViewData["availableRegions"] = regionInfo;

            return this.View("AvailableRegions");
        }

        public JsonResult GetAvailableFeaturesforRegion(string regionName)
        {
            var availableFeatures = this.regionSource.GetAvailableRegions().Where(x => string.Compare(x.RegionInformation.Name, regionName, System.StringComparison.OrdinalIgnoreCase) == 0).FirstOrDefault().Regulatory.SupportedFeatures;

            return this.Json(availableFeatures, JsonRequestBehavior.AllowGet);
        }

        public ActionResult GetRegionChannelsDetailView(string regionName)
        {
            Region countryRegion = CommonUtility.GetRegionByName(regionName);

            if (countryRegion == null)
            {
                System.Diagnostics.Trace.Write(string.Format("The country region {0} is not supported", regionName));

                throw new NotSupportedException(string.Format("The country region {0} is not supported", regionName));
            }

            return this.PartialView("InfoboxPartial", countryRegion);
        }

        /// <summary>
        /// Returns Privacy view
        /// </summary>
        /// <returns>Privacy view</returns>
        public ActionResult Privacy()
        {
            return this.View();
        }

        /// <summary>
        /// Returns Terms of Use view
        /// </summary>
        /// <returns>TermsOfUse view</returns>
        public ActionResult TermsOfUse()
        {
            return this.View();
        }

        /// <summary>
        /// Returns Contact Us view
        /// </summary>
        /// <returns>ContactUs view</returns>
        public ActionResult ContactUs()
        {
            return this.View();
        }

        /// <summary>
        /// Returns About view
        /// </summary>
        /// <returns>About view</returns>
        public ActionResult About()
        {
            return this.View();
        }

        public ActionResult Error()
        {
            return this.View();
        }

        public ViewResult FAQ()
        {
            return this.View("FAQ");
        }

        public ViewResult HowItWorks()
        {
            return this.View("HowItWorks");
        }

        private List<RegionInfo> GetAllRegions()
        {
            return this.regionSource.GetAvailableRegions().Select(region => region.RegionInformation).OrderBy(region => region.Name).ToList();
        }

        private RegionChannelsDetail GetRegionChannelsDetail(List<ChannelInformation> channelInfoList, double latitude, double longitude, string regionName)
        {
            int highPowerChannels = 0;
            int lowPowerChannels = 0;
            int unavailableChannels = 0;

            if (channelInfoList != null)
            {
                highPowerChannels = channelInfoList.Where(channelInfo => channelInfo.OperationMode == (int)ChannelOperationMode.HighPower).Count();
                lowPowerChannels = channelInfoList.Where(channelInfo => channelInfo.OperationMode == (int)ChannelOperationMode.LowPower).Count();
                unavailableChannels = channelInfoList.Where(channelInfo => channelInfo.OperationMode == (int)ChannelOperationMode.None).Count();
            }

            return new RegionChannelsDetail
            {
                Latitude = latitude,
                Longitude = longitude,
                RegionName = regionName,
                LowPowerChannelsCount = lowPowerChannels,
                HighPowerChannelsCount = highPowerChannels,
                UnAvailableChannelsCount = unavailableChannels
            };
        }

        private IEnumerable<ChannelInformation> GetChannelInfoList(double latitude, double longitude, string incumbentType, string countryRegion, int channelStartId, int channelEndId, double powerDBmTransitionPoint)
        {
            ChannelInfo[] channelInfoList = null;
            List<ChannelInformation> channels = new List<ChannelInformation>();

            try
            {
                channelInfoList = this.whitespacesManager.GetChannelList(incumbentType, latitude, longitude, countryRegion);
            }
            catch (Exception ex)
            {
                System.Diagnostics.Trace.TraceError(ex.ToString());
            }

            bool anyFreeChannels = channelInfoList != null && channelInfoList.Any();
            //// TODO: Little bit of refactoring required below. First get all the channels details and then Free Channels, then create a list of ChannelInformation list.
            //// As of now, still there is no support for GetAllChannels from the region management api.
            for (int channelId = channelStartId; channelId <= channelEndId; channelId++)
            {
                ChannelInformation channelInformation = new ChannelInformation();
                ChannelInfo channel = anyFreeChannels ? channelInfoList.FirstOrDefault(channelInfo => channelInfo.ChannelId == channelId) : null;

                if (channel == null)
                {
                    // TODO: channel should be obtained from the GetAllChannels method from the region management api, which is not implemented yet.
                    channel = new ChannelInfo { ChannelId = channelId };
                    channelInformation.OperationMode = (int)ChannelOperationMode.None;
                }
                else
                {
                    channelInformation.OperationMode = (int)CommonUtility.GetChannelOperationMode(channel.MaxPowerDBm, powerDBmTransitionPoint);
                }

                channelInformation.Channel = channel;

                channels.Add(channelInformation);
            }

            return channels;
        }
    }
}
