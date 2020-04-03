// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common.CacheHelpers
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq;
    using System.Text.RegularExpressions;
    using System.Timers;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Common.Utilities;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WhiteSpaces.Common.Entities;
    using MWEConstants = Microsoft.Whitespace.Entities.Constants;

    public class PortalServiceCacheHelper : IServiceCacheHelper
    {
        private static Dictionary<string, List<RegionPolygonsCache>> regionPolygonsCacheDictionary;

        private static Timer syncTimer;

        private static object syncObject = new object();

        private static DateTime lastSyncTime = DateTime.MinValue;

        [Dependency]
        public IRegionSource RegionSource { get; set; }

        [Dependency]
        public IDalcCommon DalcCommon { get; set; }

        [Dependency]
        public IDalcServiceCacheHelper DalcServiceCacheHelper { get; set; }

        [Dependency]
        public ILogger Logger { get; set; }

        /// <summary>
        /// Downloads the objects for service cache.
        /// </summary>
        public void DownloadObjectsForServiceCache()
        {
            this.FillRegionPolygonsCacheDictionary();
        }

        /// <summary>
        /// Gets the service cache objects.
        /// </summary>
        /// <param name="cacheObjectType">Type of the cache object.</param>
        /// <param name="serviceCacheRequestParameters">The service cache request parameters.</param>
        /// <returns>returns List of cache data.</returns>
        public object GetServiceCacheObjects(ServiceCacheObjectType cacheObjectType, ServiceCacheRequestParameters serviceCacheRequestParameters)
        {
            if (cacheObjectType == ServiceCacheObjectType.RegionPolygons)
            {
                if (regionPolygonsCacheDictionary == null)
                {
                    this.FillRegionPolygonsCacheDictionary();
                }

                return regionPolygonsCacheDictionary;
            }

            return null;
        }

        /// <summary>
        /// Searches the cache objects.
        /// </summary>
        /// <param name="cacheObjectType">Type of the cache object.</param>
        /// <param name="searchCacheRequestType">Type of the search cache request.</param>
        /// <param name="serviceCacheRequestParameters">The service cache request parameters.</param>
        /// <returns>returns System.Object.</returns>
        public object SearchCacheObjects(ServiceCacheObjectType cacheObjectType, SearchCacheRequestType searchCacheRequestType, ServiceCacheRequestParameters serviceCacheRequestParameters)
        {
            const string LogMethodName = "PortalServiceCacheHelper.SearchCacheObjects";

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DatabaseCacheMessage, "Enter " + LogMethodName);

            object results = null;

            try
            {
                if (cacheObjectType == ServiceCacheObjectType.RegionPolygons)
                {
                    this.FillRegionPolygonsCacheDictionary();
                    results = regionPolygonsCacheDictionary;
                }
            }
            catch (Exception ex)
            {
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DatabaseCacheMessage, ex.ToString());
            }

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DatabaseCacheMessage, "Exit " + LogMethodName);

            return results;
        }

        /// <summary>
        /// Fills RegionPolygonCache dictionary.
        /// </summary>
        private void FillRegionPolygonsCacheDictionary()
        {
            const string LogMethodName = "PortalServiceCacheHelper.FillRegionPolygonsCacheDictionary";

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DatabaseCacheMessage, "Enter " + LogMethodName);

            try
            {
                bool regionCacheUpdated = false;

                if (regionPolygonsCacheDictionary == null)
                {
                    this.InitializeRegionPolygonsCacheDictionary();
                }

                for (int i = 0; i < regionPolygonsCacheDictionary.Count; i++)
                {
                    var region = regionPolygonsCacheDictionary.ElementAt(i);

                    string regionCode = region.Key;
                    bool refreshCache = false;

                    Stopwatch stopWatch = new Stopwatch();

                    stopWatch.Start();
                    List<RegionPolygon> subregionPolygons = this.DalcCommon.FetchEntity<RegionPolygon>(Constants.RegionContourTableName, new { PartitionKey = Constants.FccRegionCode });

                    if (region.Value.Any() && (subregionPolygons != null && subregionPolygons.Any()) && lastSyncTime != DateTime.MinValue)
                    {
                        DateTime lastModifiedTime = subregionPolygons.Max(subregion => subregion.Timestamp.DateTime);

                        if (lastSyncTime < lastModifiedTime)
                        {
                            refreshCache = true;
                        }
                    }
                    else
                    {
                        refreshCache = true;
                    }

                    if (refreshCache)
                    {
                        lock (syncObject)
                        {
                            if (regionPolygonsCacheDictionary[regionCode] != null)
                            {
                                regionPolygonsCacheDictionary[regionCode].Clear();
                            }

                            regionPolygonsCacheDictionary[regionCode] = this.DalcServiceCacheHelper.FetchRegionPolygons(MWEConstants.RegionContourTableName, regionCode);

                            if (regionPolygonsCacheDictionary[regionCode] != null)
                            {
                                bool ascending = false;

                                // Sort polygon collection of each subregion from larger to smaller size (in terms of Bounding box area.).
                                foreach (RegionPolygonsCache regionPolygonCache in regionPolygonsCacheDictionary[regionCode])
                                {
                                    regionPolygonCache.OrderPolygonCollectionBy(ascending);
                                }
                            }
                        }
                    }

                    stopWatch.Stop();

                    TraceEventType eventType = TraceEventType.Information;
                    string logMessage = string.Empty;

                    if (!regionPolygonsCacheDictionary[regionCode].Any())
                    {
                        logMessage = string.Format("Could not find RegionPolygons entries in {0} table for the PartitionKey {1}", Constants.RegionContourTableName, Constants.FccRegionCode);
                        eventType = TraceEventType.Warning;
                    }
                    else
                    {
                        logMessage = string.Format("RegionPolygonsCache has been successfully refreshed for the Region Key {0} | Total Records: {1} | Time taken:{2} seconds", Constants.FccRegionCode, regionPolygonsCacheDictionary[regionCode].Count, stopWatch.Elapsed.Seconds);
                    }

                    this.Logger.Log(eventType, LoggingMessageId.DatabaseCacheMessage, logMessage);

                    regionCacheUpdated = refreshCache;
                }

                if (regionCacheUpdated)
                {
                    lastSyncTime = DateTime.UtcNow;
                }

                int interval = (int)TimeSpan.FromHours(12).TotalMilliseconds;

                if (Utils.Configuration.HasSetting("RegionPolygons_CacheIntervalMin"))
                {
                    interval = (int)TimeSpan.FromMinutes(Utils.Configuration["RegionPolygons_CacheIntervalMin"].ToInt32()).TotalMilliseconds;
                }

                if (syncTimer == null)
                {
                    syncTimer = new Timer();
                    syncTimer.AutoReset = false;
                    syncTimer.Elapsed += this.UpdateRegionPolygonsCache;
                    syncTimer.Interval = interval;
                }
                else
                {
                    syncTimer.Interval = interval;
                }
            }
            catch (Exception ex)
            {
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DatabaseCacheMessage, string.Format("Error in {0}: Message {1}", LogMethodName, ex.ToString()));
            }

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DatabaseCacheMessage, "Exit " + LogMethodName);            
        }

        private void UpdateRegionPolygonsCache(object sender, ElapsedEventArgs e)
        {
            this.FillRegionPolygonsCacheDictionary();
        }

        private void InitializeRegionPolygonsCacheDictionary()
        {
            regionPolygonsCacheDictionary = new Dictionary<string, List<RegionPolygonsCache>>();

            List<Region> availableRegions = this.RegionSource.GetAvailableRegions();

            foreach (Region region in availableRegions)
            {                
                Match patternMatch = Regex.Match(region.Regulatory.RegionCode, @"(\d+)");

                try
                {
                    if (patternMatch.Success)
                    {
                        int regionId = int.Parse(patternMatch.Value);
                        string regionName = "RGN";

                        using (AzureConfig azureConfig = new AzureConfig(regionId, regionName))
                        {
                            if (azureConfig.HasSetting(Constants.ValidateWSDLocation))
                            {
                                bool locationValidationEnabled = azureConfig[Constants.ValidateWSDLocation].ToBool();

                                if (locationValidationEnabled)
                                {
                                    regionPolygonsCacheDictionary.Add(region.Regulatory.RegionCode, new List<RegionPolygonsCache>());
                                }
                            }
                        }
                    }
                }
                catch (ArgumentException ex)
                {
                    this.Logger.Log(TraceEventType.Error, LoggingMessageId.DatabaseCacheMessage, ex.ToString());
                }
            }
        }
    }
}
