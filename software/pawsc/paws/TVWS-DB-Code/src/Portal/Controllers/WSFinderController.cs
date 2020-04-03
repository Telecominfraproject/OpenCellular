// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Portal.Controllers
{
    using System;
    using System.Collections.Generic;
    using System.Device.Location;
    using System.IO;
    using System.Linq;
    using System.Threading.Tasks;
    using System.Web.Mvc;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WhiteSpaces.BusinessManager;
    using Microsoft.WhiteSpaces.Common;
    using Microsoft.WhiteSpaces.Common.Entities;
    using Microsoft.WhiteSpaces.Common.Enums;
    using Microsoft.WhiteSpaces.Common.UnitConvertor;
    using Microsoft.WhiteSpaces.Common.Utilities;
    using Microsoft.WhiteSpaces.Portal.Models;
    using INCU = Microsoft.WhiteSpaces.Common.Incumbent;

    [AllowAnonymous]
    public class WSFinderController : Controller
    {
        private const string RegionCode = "RegionCode";

        private readonly IWhitespacesManager whitespacesManager;
        private readonly IRegionSource regionSource;

        public WSFinderController(IWhitespacesManager whitespacesManager, IRegionSource regionSource)
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

        [Dependency]
        public IAuditor WhitespaceFinderAuditor { get; set; }

        [Dependency]
        public ILogger WhitespaceFinderLogger { get; set; }

        // GET: /WSFinder/
        public ActionResult Index()
        {
            this.ViewData["availableRegions"] = this.regionSource.GetAvailableRegions().Select(region => region.RegionInformation).ToList();

            return this.View();
        }

        public ViewResult GetWhitespacesFinderView(string regulatoryName)
        {
            Region currentRegion = this.regionSource.GetAvailableRegions()
                                                    .Where(region => string.Compare(regulatoryName, region.Regulatory.Name, StringComparison.OrdinalIgnoreCase) == 0)
                                                    .FirstOrDefault();

            this.ViewData["availableRegions"] = new List<RegionInfo> { currentRegion.RegionInformation };

            RegionWhitespaceDetails whitespaceDetails = this.GetRegionWhitespaceDetails(currentRegion.Regulatory.RegionCode, null);

            return this.View("RegionSpecificFinder", whitespaceDetails);
        }

        public PartialViewResult GetCountryRegionSpecificControls(RegionSearchParams model)
        {
            RegionWhitespaceDetails whitespaceDetails = this.GetRegionWhitespaceDetails(model.RegionCode, model.AntennaHeight);

            return this.PartialView("WhitespaceDetailsPartial", whitespaceDetails);
        }

        public PartialViewResult FindNearByIncumbents(RegionSearchParams model)
        {
            RegionWhitespaceDetails whitespaceDetails = this.GetRegionWhitespaceDetails(model, this.ModelState.IsValid);

            return this.PartialView("WhitespaceDetailsPartial", whitespaceDetails);
        }

        public JsonResult GetProtectedIncumbents(string incumbentType, string countryRegion, int channel)
        {
            Region region = this.regionSource.GetAvailableRegions().FirstOrDefault(country => string.Compare(country.RegionInformation.Name, countryRegion, StringComparison.OrdinalIgnoreCase) == 0);

            if (this.ModelState.IsValid && region != null)
            {
                List<int> channelList = new List<int>() { channel };
                List<INCU> incumbents = this.whitespacesManager.GetIncumbents(incumbentType, countryRegion, channelList);

                JsonResult jsonResult = this.Json(incumbents, JsonRequestBehavior.AllowGet);

                jsonResult.MaxJsonLength = int.MaxValue;

                return jsonResult;
            }

            return null;
        }

        public ActionResult GetIncumbents(RegionSearchParams searchParams, int[] channels)
        {
            List<INCU> incumbents = new List<INCU>();
            Region region = this.regionSource.GetAvailableRegions().FirstOrDefault(rgn => string.Compare(rgn.RegionInformation.Name, searchParams.CountryRegion, StringComparison.OrdinalIgnoreCase) == 0);

            if (region == null)
            {
                //// TODO: Logic to handle Invalid CountryRegion as argument.
            }

            incumbents = this.whitespacesManager.GetIncumbents(searchParams.IncumbentType, searchParams.Latitude, searchParams.Longitude, searchParams.CountryRegion, channels);

            // TODO: May be we need to have condition check below to make sure following filter logic executes only for FCC. 

            // [Temporary fix]: Commenting following search location based filtering, because for incumbent type such as "TV_US" is not return TransmitLocation,
            // which plays a major role in filtering the incumbents for the given search location.
            // incumbents = this.FilterIncumbentsByRegion(searchParams.Latitude, searchParams.Longitude, incumbents);
            incumbents = this.FilterIncumbentsByChannels(channels, incumbents);

            return this.Json(incumbents, JsonRequestBehavior.AllowGet);
        }

        private static IEnumerable<ProtectedDevice> FilterPortectedDevicesByChannel(IEnumerable<ProtectedDevice> protectedDevices, int channel)
        {
            if (protectedDevices != null)
            {
                protectedDevices = protectedDevices.Where(device => device.Channels.Contains(channel));
            }

            return protectedDevices;
        }

        /// <summary>
        /// Code to extract all the incumbents filtered by Channel Number across the country region.
        /// </summary>
        /// <param name="incumbents">list of incumbents</param>
        /// <param name="tvbdType">type of incumbents</param>
        private static void WriteIncumbentsToAFile(IEnumerable<INCU> incumbents, string tvbdType)
        {
            string filePath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments), "Incumbent.txt");
            FileStream stream = System.IO.File.OpenWrite(filePath);
            if (stream != null)
            {
                stream.Close();
                List<string> contents = incumbents.Select(incumbent =>
                {
                    bool contourAvailability = incumbent.ContourPoints != null && incumbent.ContourPoints.Any();

                    return string.Format(
                        "Incumbent Type: {0}{1} Channel No:{2}, Contours Availability Status:{3} {4} Latitude:{5}, Longitude:{6}{7} -----------^^^^^",
                        incumbent.Type.ToString(),
                        Environment.NewLine,
                        incumbent.Channel,
                        contourAvailability,
                        Environment.NewLine,
                        incumbent.TransmitLocation.Latitude,
                        incumbent.TransmitLocation.Longitude,
                        Environment.NewLine);
                }).ToList();

                contents.Insert(0, string.Format("TVBD Type: {0}{1}^^^^^^^---------------", tvbdType, Environment.NewLine));

                System.IO.File.AppendAllLines(filePath, contents);
            }
        }

        private RegionWhitespaceDetails GetRegionWhitespaceDetails(string regionCode, double? antennaHeight)
        {
            IEnumerable<Region> supportedCountries = this.regionSource.GetAvailableRegions();
            Region region = supportedCountries.FirstOrDefault(country => string.Compare(country.Regulatory.RegionCode, regionCode, StringComparison.OrdinalIgnoreCase) == 0);

            RegionSearchParams defaultSearchParams = new RegionSearchParams
            {
                CountryRegion = region != null ? region.RegionInformation.Name : string.Empty
            };

            if (region != null)
            {
                defaultSearchParams.RegionCode = region.Regulatory.RegionCode;
                defaultSearchParams.Latitude = region.RegionInformation.Latitude;
                defaultSearchParams.Longitude = region.RegionInformation.Longitude;
                defaultSearchParams.IncumbentType = region.Regulatory.SupportedDeviceTypes.FirstOrDefault().Type;
                defaultSearchParams.AntennaHeight = antennaHeight;
            }

            return this.GetRegionWhitespaceDetails(defaultSearchParams, false);
        }

        private RegionWhitespaceDetails GetRegionWhitespaceDetails(RegionSearchParams searchParams, bool getChannelInfoList)
        {
            Region region = this.regionSource.GetAvailableRegions().FirstOrDefault(rgn => string.Compare(rgn.Regulatory.RegionCode, searchParams.RegionCode, StringComparison.OrdinalIgnoreCase) == 0);

            RegionWhitespaceDetails whitespaceDetails = new RegionWhitespaceDetails
            {
                AntennaHeight = searchParams.AntennaHeight,
                Location = searchParams.Location,
                EnableProtectedAreas = region != null ? region.Regulatory.AllowAccessToProtedtedArea : false
            };

            if (region != null)
            {
                whitespaceDetails.StartChannelNo = region.Regulatory.WSChannelsInfo.StartChannel;
                whitespaceDetails.EndChannelNo = region.Regulatory.WSChannelsInfo.EndChannel;

                if (region.Regulatory.SupportedDeviceTypes != null)
                {
                    whitespaceDetails.DeviceTypes = region.Regulatory.SupportedDeviceTypes.ToList();
                    whitespaceDetails.SelectedIncumbentType = string.IsNullOrEmpty(searchParams.IncumbentType) ? whitespaceDetails.DeviceTypes.FirstOrDefault().Type : searchParams.IncumbentType;
                    whitespaceDetails.PowerDBmTransitionPoint = region.Regulatory.WSChannelsInfo.PowerDBmTransitionPoint;
                }
            }

            if (getChannelInfoList)
            {
                string[] deviceTypes = region.Regulatory.SupportedDeviceTypes.Select(device => device.Type).ToArray();

                Task getAnalysisResult = new Task(() =>
                    {
                        this.GetAvailableChannelsLookup(searchParams, whitespaceDetails, region);
                    });

                getAnalysisResult.Start();

                try
                {
                    if (region.Regulatory.AllowAccessToProtedtedArea)
                    {
                        Task getIncumbents = new Task(() =>
                        {
                            this.GetOccupiedChannels(searchParams, whitespaceDetails, region.RegionInformation.Name);
                        });

                        getIncumbents.Start();
                        Task.WaitAll(getAnalysisResult, getIncumbents);
                    }
                    else
                    {
                        Task.WaitAll(getAnalysisResult);
                    }
                }
                catch (AggregateException ex)
                {
                    // TODO: Is it required to initialize following view model lists to empty list or leave them as null ?
                    // Swallow the exception, initialize lists to empty list and return results.
                    whitespaceDetails.AnalysisResult = new WhitespaceAnalysisResult(new List<WhitespaceSummary>());
                    whitespaceDetails.Channels = new List<ChannelInformation>();
                    whitespaceDetails.OccupiedChannes = new List<INCU>();

                    System.Diagnostics.Trace.Write(ex.ToString());

                    this.WhitespaceFinderAuditor.RegionCode = Convert.ToInt16(region.RegionInformation.Id);
                    this.WhitespaceFinderAuditor.TransactionId = this.WhitespaceFinderLogger.TransactionId;
                    this.WhitespaceFinderAuditor.Audit(AuditId.WhitespaceFinder, AuditStatus.Failure, default(int), ex.ToString());
                }
            }

            whitespaceDetails.RegionChannelsDetail = this.GetRegionChannelsDetail(whitespaceDetails.Channels, searchParams.Latitude, searchParams.Longitude, searchParams.CountryRegion);

            return whitespaceDetails;
        }

        private void GetAvailableChannelsLookup(RegionSearchParams searchParams, RegionWhitespaceDetails whitespaceDetails, Region region)
        {
            Dictionary<string, ChannelInfo[]> availableChannelsLookup = new Dictionary<string, ChannelInfo[]>();
            ChannelInfo[] channels = null;

            string[] deviceTypes = region.Regulatory.SupportedDeviceTypes.Select(device => device.Type).ToArray();

            whitespaceDetails.AnalysisResult = this.GetWhitespaceAnalysisResult(deviceTypes, searchParams.Latitude, searchParams.Longitude, region, availableChannelsLookup);

            if (availableChannelsLookup.Any())
            {
                channels = availableChannelsLookup[searchParams.IncumbentType];
            }

            whitespaceDetails.Channels = this.GetChannelAvailabilityInfo(channels, region.Regulatory.WSChannelsInfo.StartChannel, region.Regulatory.WSChannelsInfo.EndChannel, region.Regulatory.WSChannelsInfo.PowerDBmTransitionPoint).ToList();

            whitespaceDetails.AvailableChannelsLookup = availableChannelsLookup;
        }

        private void GetOccupiedChannels(RegionSearchParams searchParams, RegionWhitespaceDetails whitespaceDetails, string countryRegion)
        {
            IEnumerable<INCU> incumbents = this.GetReseveredChannelsFromIncumbentRequest(searchParams.IncumbentType, searchParams.Latitude, searchParams.Longitude, countryRegion, null);

            if (incumbents != null)
            {
                List<INCU> reservedIncumbents = incumbents.ToList();
                whitespaceDetails.OccupiedChannes = this.FilterIncumbentsByRegion(searchParams.Latitude, searchParams.Longitude, reservedIncumbents).OrderBy(inc => inc.Channel).ToList();
            }
        }

        private WhitespaceAnalysisResult GetWhitespaceAnalysisResult(string[] incumbentTypes, double latitude, double longitude, Region region, Dictionary<string, ChannelInfo[]> channelsLookup)
        {
            string countryRegion = region.RegionInformation.Name;
            int channelStartId = region.Regulatory.WSChannelsInfo.StartChannel;
            int channelEndId = region.Regulatory.WSChannelsInfo.EndChannel;

            int bandwidth = region.Regulatory.WSChannelsInfo.ChannelBandwidth;

            Task<ChannelInfo[]>[] tasks = new Task<ChannelInfo[]>[incumbentTypes.Length];
            List<WhitespaceSummary> whitespaceSummaryCollection = new List<WhitespaceSummary>();

            for (int taskIndex = 0; taskIndex < incumbentTypes.Length; taskIndex++)
            {
                int index = taskIndex;
                tasks[index] = new Task<ChannelInfo[]>(() =>
                {
                    return this.whitespacesManager.GetChannelList(incumbentTypes[index], latitude, longitude, countryRegion);
                });

                tasks[index].Start();
            }

            try
            {
                Task.WaitAll(tasks);

                for (int taskIndex = 0; taskIndex < tasks.Length; taskIndex++)
                {
                    ChannelInfo[] channelInfo = tasks[taskIndex].Result;

                    if (tasks[taskIndex].Result == null || (tasks[taskIndex].Result != null && tasks[taskIndex].Result.Length == 0))
                    {
                        this.WhitespaceFinderAuditor.RegionCode = Convert.ToInt16(CommonUtility.GetRegionByName(countryRegion).RegionInformation.Id);
                        this.WhitespaceFinderAuditor.TransactionId = this.WhitespaceFinderLogger.TransactionId;
                        this.WhitespaceFinderAuditor.Audit(AuditId.WhitespaceFinder, AuditStatus.Failure, default(int), "No records found for region " + countryRegion + " having device type " + incumbentTypes[taskIndex]);
                    }

                    // [TODO: Associated Bug Id:#253262] This is  only a temporary fix for Ofcom Free Channels, as OFcom region management api returning 
                    // duplicated channels. The Channels are duplicated twice one for each bandwidth 8 MHz and 0.1 MHz. We should consider only 8 MHz channels 
                    // on Ux.
                    if (string.Compare(region.RegionInformation.Id, "1", StringComparison.OrdinalIgnoreCase) == 0)
                    {
                        channelInfo = channelInfo.Where(channel => (int)channel.Bandwidth == bandwidth).ToArray();
                    }

                    channelsLookup.Add(incumbentTypes[taskIndex], channelInfo);

                    WhitespaceSummary summary = WhitespaceAnalysis.AnalyseWhitespaceSummary(channelInfo.AsEnumerable(), incumbentTypes[taskIndex], channelStartId, channelEndId);
                    whitespaceSummaryCollection.Add(summary);
                }
            }
            catch (AggregateException ex)
            {
                System.Diagnostics.Trace.TraceError(ex.ToString());
            }

            var collection = new WhitespaceAnalysisResult(whitespaceSummaryCollection);

            return new WhitespaceAnalysisResult(whitespaceSummaryCollection);
        }

        private IEnumerable<ChannelInformation> GetChannelAvailabilityInfo(ChannelInfo[] channelInfoList, int channelStartId, int channelEndId, double powerDBmTransitionPoint)
        {
            List<ChannelInformation> channels = new List<ChannelInformation>();

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

        private List<INCU> FilterIncumbentsByChannels(int[] channels, List<INCU> incumbents)
        {
            List<INCU> incumbentList = new List<INCU>();

            foreach (INCU incumbent in incumbents)
            {
                if (channels.Contains(incumbent.Channel))
                {
                    incumbentList.Add(incumbent);
                }
            }

            return incumbentList;
        }

        // [Temporary Fix:] As back-end is not filtering the incumbents for a given search location, this is a work around for filtering the incumbents for a given search location.
        private IEnumerable<INCU> FilterIncumbentsByRegion(double latitude, double longitude, IEnumerable<INCU> incumbents)
        {
            List<INCU> incumbentList = new List<INCU>();
            GeoCoordinate startCoordinate = new GeoCoordinate(latitude, longitude);

            foreach (INCU incumbent in incumbents)
            {
                double longitueDistance = startCoordinate.GetDistanceTo(new GeoCoordinate(latitude, incumbent.TransmitLocation.Longitude)).ToDistance(DistanceUnit.Meter, DistanceUnit.Miles);

                double latitueDistance = startCoordinate.GetDistanceTo(new GeoCoordinate(incumbent.TransmitLocation.Latitude, longitude)).ToDistance(DistanceUnit.Meter, DistanceUnit.Miles);

                if (latitueDistance <= 69 && longitueDistance <= 53)
                {
                    incumbentList.Add(incumbent);
                }
            }

            return incumbentList;
        }

        private ProtectedDevice[] GetProtectedDevices(string incumbentType, double latitude, double longitude, string countryRegion)
        {
            return this.whitespacesManager.GetDeviceList(incumbentType, latitude, longitude, countryRegion);
        }

        // [Temporary Fix:] Until the back-end provides complete support for GetDeviceList method in the RegionManagment api.
        private IEnumerable<INCU> GetReseveredChannelsFromIncumbentRequest(string incumbentType, double latitude, double longitude, string countryRegion, IEnumerable<int> channels)
        {
            // Currently supported protected device types for FCC
            string[] incumbentTypes = new string[] 
            {
                IncumbentType.MVPD.ToString(),
                IncumbentType.TBAS.ToString(),
                IncumbentType.TV_US.ToString(),
                IncumbentType.LPAux.ToString(),
                IncumbentType.UnlicensedLPAux.ToString()
            };

            Task<List<INCU>>[] incumbentRequests = new Task<List<INCU>>[incumbentTypes.Length];
            List<INCU> incumbents = new List<INCU>();

            for (int index = 0; index < incumbentTypes.Length; index++)
            {
                int incumbentIndex = index;

                incumbentRequests[incumbentIndex] = new Task<List<INCU>>(() =>
                {
                    return this.whitespacesManager.GetIncumbents(incumbentTypes[incumbentIndex], latitude, longitude, countryRegion, channels);
                });

                incumbentRequests[incumbentIndex].Start();
            }

            try
            {
                Task.WaitAll(incumbentRequests);

                for (int taskIndex = 0; taskIndex < incumbentRequests.Length; taskIndex++)
                {
                    if (incumbentRequests[taskIndex].Result != null)
                    {
                        incumbents.InsertRange(incumbents.Count, incumbentRequests[taskIndex].Result);
                    }
                }
            }
            catch (AggregateException ex)
            {
                // Swallow the exception and return empty list.
                System.Diagnostics.Trace.TraceError(ex.ToString());
            }

            return incumbents;
        }
    }
}
