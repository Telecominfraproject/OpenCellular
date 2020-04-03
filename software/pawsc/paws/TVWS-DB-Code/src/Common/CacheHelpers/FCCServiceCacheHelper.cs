// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common.CacheHelpers
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.Timers;
    using ApplicationServer.Caching;
    using Entities;
    using Microsoft.WindowsAzure.ServiceRuntime;
    using Practices.Unity;
    using Utilities;
    using WindowsAzure.Storage.Table;

    /// <summary>
    /// Represents Class FCCServiceCacheHelper.
    /// </summary>
    public class FCCServiceCacheHelper : IServiceCacheHelper
    {
        /// <summary>The cache objects</summary>
        private static List<CacheObjectTvEngdata> tvengCache;

        /// <summary>The LPAUX Cache</summary>
        private static SynchronizedCollection<LPAuxRegistration> lpauxCache;

        private static List<RegionPolygonsCache> regionPolygonsCache;

        private static DateTime regionPolygonsLastModifiedTime = DateTime.MinValue;

        /// <summary>The synchronize object</summary>
        private static object syncObject = new object();

        /// <summary>The synchronize timer</summary>
        private static Timer syncTimer;

        /// <summary>
        /// The regionPolygons synchronization timer.
        /// </summary>
        private static Timer regionPolygonTimer;

        /// <summary>
        /// Gets or sets the DALC database cache.
        /// </summary>
        /// <value>The DALC database cache.</value>
        [Dependency]
        public IDalcCommon DalcCommon { get; set; }

        /// <summary>
        /// Gets or sets the DALC service cache.
        /// </summary>
        /// <value>The DALC service cache.</value>
        [Dependency]
        public IDalcServiceCacheHelper DalcServiceCache { get; set; }

        /// <summary>
        /// Gets or sets the logger.
        /// </summary>
        /// <value>The logger.</value>
        [Dependency]
        public ILogger Logger { get; set; }

        #region Implementation of IServiceCacheHelper

        /// <summary>
        /// Downloads the objects for service cache.
        /// </summary>
        public void DownloadObjectsForServiceCache()
        {
            this.FillCdbsTvEngDataCache();
            this.FillLpAuxCache();
            this.FillRegionPolygonsCache();
        }

        /// <summary>
        /// Gets the service cache objects.
        /// </summary>
        /// <param name="cacheObjectType">Type of the cache object.</param>
        /// <param name="serviceCacheRequestParameters">The service cache request parameters.</param>
        /// <returns>returns List of cache data.</returns>
        public object GetServiceCacheObjects(ServiceCacheObjectType cacheObjectType, ServiceCacheRequestParameters serviceCacheRequestParameters)
        {
            if (cacheObjectType == ServiceCacheObjectType.TvEngData)
            {
                return this.GetIncumbents(serviceCacheRequestParameters);
            }

            if (cacheObjectType == ServiceCacheObjectType.LPAUX)
            {
                return this.GetLPAuxRegistrations(serviceCacheRequestParameters);
            }

            if (cacheObjectType == ServiceCacheObjectType.RegionPolygons)
            {
                this.FillRegionPolygonsCache();
                return regionPolygonsCache;
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
            const string LogMethodName = "FCCServiceCacheHelper.SearchCacheObjects";

            List<Incumbent> incumbents = new List<Incumbent>();

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DatabaseCacheMessage, "Enter " + LogMethodName);
            object results = null;
            try
            {
                if (cacheObjectType == ServiceCacheObjectType.TvEngData)
                {
                    this.LoadCacheData<CacheObjectTvEngdata>(ServiceCacheObjectType.TvEngData);

                    if (((int)searchCacheRequestType & (int)SearchCacheRequestType.ByCallSign) == (int)SearchCacheRequestType.ByCallSign)
                    {
                        results = tvengCache.Where(obj => obj.CallSign == serviceCacheRequestParameters.CallSign).ToList();
                    }
                    else if (((int)searchCacheRequestType & (int)SearchCacheRequestType.ByVsdService) == (int)SearchCacheRequestType.ByVsdService)
                    {
                        results = tvengCache.Where(obj => obj.VsdService == serviceCacheRequestParameters.VsdService);
                    }
                }
                else if (cacheObjectType == ServiceCacheObjectType.LPAUX)
                {
                    this.LoadCacheData<LPAuxRegistration>(ServiceCacheObjectType.LPAUX);
                    if (((int)searchCacheRequestType & (int)SearchCacheRequestType.None) == (int)SearchCacheRequestType.None)
                    {
                        results = lpauxCache;
                    }
                }
                else if (cacheObjectType == ServiceCacheObjectType.RegionPolygons)
                {
                    this.LoadCacheData<RegionPolygonsCache>(ServiceCacheObjectType.RegionPolygons);
                    results = regionPolygonsCache;
                }
            }
            catch (Exception ex)
            {
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DatabaseCacheMessage, string.Format("Error in {0}: Message {1}", LogMethodName, ex.ToString()));
            }

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DatabaseCacheMessage, "Exit " + LogMethodName);

            return results;
        }

        /// <summary>
        /// Updates the cache objects.
        /// </summary>
        /// <param name="cacheObjectType">Type of the cache object.</param>
        /// <param name="updateType">Type of the update.</param>
        /// <param name="cacheItem">The cache item.</param>
        public void UpdateCacheObjects(ServiceCacheObjectType cacheObjectType, ServiceCacheUpdateType updateType, object cacheItem)
        {
            const string LogMethodName = "FCCServiceCacheHelper.UpdateCacheObjects";

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DatabaseCacheMessage, "Enter " + LogMethodName);
            try
            {
                if (cacheObjectType == ServiceCacheObjectType.LPAUX)
                {
                    if (updateType == ServiceCacheUpdateType.Add)
                    {
                        lpauxCache.Add((LPAuxRegistration)cacheItem);
                    }
                }
            }
            catch (Exception ex)
            {
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DatabaseCacheMessage, string.Format("Error in {0}: Message {1}", LogMethodName, ex.ToString()));
            }

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DatabaseCacheMessage, "Exit " + LogMethodName);
        }

        /// <summary>
        /// Fills the CDBS TVENG data cache.
        /// </summary>
        private void FillCdbsTvEngDataCache()
        {
            const string LogMethodName = "FCCServiceCacheHelper.FillCdbsTvEngDataCache";

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DatabaseCacheMessage, "Enter " + LogMethodName);

            try
            {
                List<DynamicTableEntity> regionSyncStatusDetails;
                regionSyncStatusDetails = this.DalcCommon.FetchEntity<DynamicTableEntity>(Utils.GetRegionalTableName(Constants.RegionSyncStatusTableName), new { RowKey = Constants.CDBSUSMexicoTVEngDataTableName });

                Stopwatch watch = new Stopwatch();

                List<CacheObjectTvEngdata> cdbsUsMexicoData = null;
                List<CacheObjectTvEngdata> cdbsCanadaData = null;

                string fileName = ServiceCacheObjectType.TvEngData.ToString();
                bool refreshCache = true;
                if (Utils.CheckFileExists(fileName))
                {
                    FileInfo fileInfo = new FileInfo(Utils.GetLocalStorCacheFileName(fileName));
                    if (fileInfo.LastWriteTime.Date >= regionSyncStatusDetails[0].Timestamp.Date || fileInfo.CreationTime.Date >= regionSyncStatusDetails[0].Timestamp.Date)
                    {
                        refreshCache = false;
                    }
                }

                if (refreshCache)
                {
                    watch.Start();

                    cdbsUsMexicoData = this.DalcServiceCache.FetchTvEngData(Constants.CDBSUSMexicoTVEngDataTableName);
                    tvengCache = cdbsUsMexicoData;

                    cdbsCanadaData = this.DalcServiceCache.FetchTvEngData(Constants.CDBSCanadaTvEngDataTableName);
                    tvengCache.AddRange(cdbsCanadaData);

                    watch.Stop();
                    Utils.WriteCacheFile<CacheObjectTvEngdata>(tvengCache, fileName);
                }

                this.UpdateDataCacheStatus("ServiceAPI", Constants.CDBSUSMexicoTVEngDataTableName, watch.Elapsed);

                int timeInterval = (int)TimeSpan.FromHours(1).TotalMilliseconds;
                if (Utils.Configuration.HasSetting("CDBSUSMexicoTvEngData_CacheInterval"))
                {
                    timeInterval = Utils.Configuration["CDBSUSMexicoTvEngData_CacheInterval"].ToInt32();
                }

                if (syncTimer == null)
                {
                    syncTimer = new Timer(timeInterval);
                    syncTimer.AutoReset = false;
                    syncTimer.Elapsed += this.UpdateCache;
                    syncTimer.Start();
                }
                else
                {
                    // NB: Following is a fix to have syncTimer.Elapsed as Multicast delegate and to ensure there exists only one registration with UpdateCache 
                    // event handler.[ As syncTimer instance has been used to execute FillCdbsTvEngDataCache and FillLpAuxCache]                    
                    syncTimer.Elapsed -= this.UpdateCache;
                    syncTimer.Elapsed += this.UpdateCache;

                    syncTimer.Interval = timeInterval;
                }
            }
            catch (Exception ex)
            {
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DatabaseCacheMessage, string.Format("Error in {0}: Message {1}", LogMethodName, ex.ToString()));
            }

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DatabaseCacheMessage, "Exit " + LogMethodName);
        }

        /// <summary>
        /// Fills the LPAUX cache.
        /// </summary>
        private void FillLpAuxCache()
        {
            const string LogMethodName = "FCCServiceCacheHelper.FillLpAuxCache";

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DatabaseCacheMessage, "Enter " + LogMethodName);

            try
            {
                List<DynamicTableEntity> regionSyncStatusDetails;
                regionSyncStatusDetails = this.DalcCommon.FetchEntity<DynamicTableEntity>(Utils.GetRegionalTableName(Constants.DataCacheUpdateStatusTableName), new { RowKey = Constants.LPAuxRegistrationTable });
                Stopwatch watch = new Stopwatch();

                List<LPAuxRegistration> lpauxRegistrations = null;
                bool refreshCache = lpauxCache == null || regionSyncStatusDetails.Count == 0 || regionSyncStatusDetails[0]["NeedsUpdate"].BooleanValue.Value;

                if (refreshCache)
                {
                    watch.Start();
                    lpauxRegistrations = this.DalcServiceCache.FetchLpAuxAregistrations(Constants.LPAuxRegistrationTable);
                    foreach (var lpauxRegistration in lpauxRegistrations)
                    {
                        lpauxRegistration.DeSerializeAndClearObjectsFromJson();
                    }

                    lpauxCache = new SynchronizedCollection<LPAuxRegistration>(syncObject, lpauxRegistrations);

                    watch.Stop();
                }

                this.UpdateDataCacheStatus("LpAuxReg", Constants.LPAuxRegistrationTable, watch.Elapsed);

                int timeInterval = (int)TimeSpan.FromHours(1).TotalMilliseconds;
                if (syncTimer == null)
                {
                    syncTimer = new Timer(timeInterval);
                    syncTimer.AutoReset = false;
                    syncTimer.Elapsed += this.UpdateLpAuxCache;
                    syncTimer.Start();
                }
                else
                {
                    // NB: Following is a fix to have syncTimer.Elapsed as Multicast delegate and to ensure there exists only one registration with 
                    // UpdateLpAuxCache event handler.[ As syncTimer instance has been used to execute FillCdbsTvEngDataCache and FillLpAuxCache]   
                    syncTimer.Elapsed -= this.UpdateLpAuxCache;
                    syncTimer.Elapsed += this.UpdateLpAuxCache;

                    syncTimer.Interval = timeInterval;
                }
            }
            catch (Exception ex)
            {
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DatabaseCacheMessage, string.Format("Error in {0}: Message {1}", LogMethodName, ex.ToString()));
            }

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DatabaseCacheMessage, "Exit " + LogMethodName);
        }

        /// <summary>
        /// Fills the RegionPolygons cache.
        /// </summary>
        private void FillRegionPolygonsCache()
        {
            const string LogMethodName = "FCCServiceCacheHelper.FillRegionPolygonsCache";

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DatabaseCacheMessage, "Enter " + LogMethodName);

            try
            {
                // TODO : Will have to find a better solution to read the latest record timestamp,instead of fetching all the records and finding last written/updated record timestamp.
                List<RegionPolygon> regionPolygonsCollection = this.DalcCommon.FetchEntity<RegionPolygon>(Constants.RegionContourTableName, new { PartitionKey = Constants.FccRegionCode });

                bool refreshCache = false;
                Stopwatch stopWatch = new Stopwatch();

                if ((regionPolygonsCache != null && regionPolygonsCache.Any()) && (regionPolygonsCollection != null && regionPolygonsCollection.Any()) && (regionPolygonsLastModifiedTime != DateTime.MinValue))
                {
                    // Recently modified record timestamp.
                    DateTime lastModifiedTime = regionPolygonsCollection.Max(regionPolygons => regionPolygons.Timestamp.DateTime);

                    if (regionPolygonsLastModifiedTime < lastModifiedTime)
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
                    stopWatch.Start();

                    lock (syncObject)
                    {
                        if (regionPolygonsCache != null)
                        {
                            regionPolygonsCache.Clear();
                        }

                        regionPolygonsCache = this.DalcServiceCache.FetchRegionPolygons(Constants.RegionContourTableName, Constants.FccRegionCode);

                        if (regionPolygonsCache != null)
                        {
                            bool ascending = false;

                            // Sort polygon collection of each region from larger to smaller size (in terms of Bounding box area.).
                            foreach (RegionPolygonsCache regionPolygonCache in regionPolygonsCache)
                            {
                                regionPolygonCache.OrderPolygonCollectionBy(ascending);
                            }
                        }

                        regionPolygonsLastModifiedTime = DateTime.UtcNow;
                    }

                    stopWatch.Stop();

                    TraceEventType eventType = TraceEventType.Information;
                    string logMessage = string.Empty;

                    if ((regionPolygonsCache == null) || (regionPolygonsCache != null && regionPolygonsCache.Count == 0))
                    {
                        logMessage = string.Format("Could not find RegionPolygons entries in {0} table for the PartitionKey {1}", Constants.RegionContourTableName, Constants.FccRegionCode);
                        eventType = TraceEventType.Warning;
                    }
                    else
                    {
                        logMessage = string.Format("RegionPolygonsCache has been successfully refreshed for the Region Key {0} | Total Records: {1} | Time taken:{2} seconds", Constants.FccRegionCode, regionPolygonsCache.Count, stopWatch.Elapsed.Seconds);
                    }

                    this.Logger.Log(eventType, LoggingMessageId.DatabaseCacheMessage, logMessage);

                    int timeInterval = (int)TimeSpan.FromHours(12).TotalMilliseconds;

                    if (Utils.Configuration.HasSetting("RegionPolygons_CacheIntervalMin"))
                    {
                        timeInterval = (int)TimeSpan.FromMinutes(Utils.Configuration["RegionPolygons_CacheIntervalMin"].ToInt32()).TotalMilliseconds;
                    }

                    if (regionPolygonTimer == null)
                    {
                        regionPolygonTimer = new Timer(timeInterval);
                        regionPolygonTimer.AutoReset = false;
                        regionPolygonTimer.Elapsed += this.UpdateRegionPolygonsCache;
                        regionPolygonTimer.Start();
                    }
                    else
                    {
                        regionPolygonTimer.Interval = timeInterval;
                    }
                }
            }
            catch (Exception ex)
            {
                string logMessage = string.Format("Region code: {0}| Error Details: {1}", Constants.FccRegionCode, ex.ToString());
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DatabaseCacheMessage, logMessage);
            }

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DatabaseCacheMessage, "Exit " + LogMethodName);
        }

        /// <summary>
        /// Runs the cache.
        /// </summary>
        /// <param name="sender">The sender.</param>
        /// <param name="e">The <see cref="ElapsedEventArgs"/> instance containing the event data.</param>
        private void UpdateCache(object sender, ElapsedEventArgs e)
        {
            this.FillCdbsTvEngDataCache();
        }

        /// <summary>
        /// Updates the LPAUX cache.
        /// </summary>
        /// <param name="sender">The sender.</param>
        /// <param name="e">The <see cref="ElapsedEventArgs"/> instance containing the event data.</param>
        private void UpdateLpAuxCache(object sender, ElapsedEventArgs e)
        {
            this.FillLpAuxCache();
        }

        /// <summary>
        /// Updates the RegionPolygons cache.
        /// </summary>
        /// <param name="sender">The sender.</param>
        /// <param name="e">The <see cref="ElapsedEventArgs"/> instance containing the event data.</param>
        private void UpdateRegionPolygonsCache(object sender, ElapsedEventArgs e)
        {
            this.FillRegionPolygonsCache();
        }

        /// <summary>
        /// Gets the incumbents.
        /// </summary>
        /// <param name="requestParameters">The request parameters.</param>
        /// <returns>returns Incumbent[][].</returns>
        private Incumbent[] GetIncumbents(ServiceCacheRequestParameters requestParameters)
        {
            const string LogMethodName = "FCCServiceCacheHelper.GetIncumbents";

            List<Incumbent> incumbents = new List<Incumbent>();

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DatabaseCacheMessage, "Enter " + LogMethodName);
            try
            {
                var squareArea = requestParameters.SearchArea;

                this.LoadCacheData<CacheObjectTvEngdata>(ServiceCacheObjectType.TvEngData);
                if (tvengCache != null)
                {
                    IEnumerable<CacheObjectTvEngdata> engDataList = tvengCache.Where(p => (p.Latitude <= squareArea.TopLeftPoint.Latitude && p.Latitude >= squareArea.BottomRightPoint.Latitude
                                                                                         && p.Longitude <= squareArea.BottomRightPoint.Longitude && p.Longitude >= squareArea.TopLeftPoint.Longitude));

                    foreach (CacheObjectTvEngdata engData in engDataList)
                    {
                        Incumbent incumbent = new Incumbent();
                        incumbent.Channel = engData.Channel;
                        incumbent.CallSign = engData.CallSign;
                        incumbent.Latitude = engData.Latitude;
                        incumbent.Longitude = engData.Longitude;
                        incumbent.Contour = engData.Contour;
                        incumbent.VsdService = engData.VsdService;
                        incumbents.Add(incumbent);
                    }
                }
            }
            catch (Exception ex)
            {
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DatabaseCacheMessage, string.Format("Error in {0}: Message {1}", LogMethodName, ex.ToString()));
            }

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DatabaseCacheMessage, "Exit " + LogMethodName);

            return incumbents.ToArray();
        }

        /// <summary>
        /// Gets the LPAUX registrations.
        /// </summary>
        /// <param name="requestParameters">The request parameters.</param>
        /// <returns>returns Incumbent[].</returns>
        private List<LPAuxRegistration> GetLPAuxRegistrations(ServiceCacheRequestParameters requestParameters)
        {
            const string LogMethodName = "FCCServiceCacheHelper.GetLPAuxRegistrations";

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DatabaseCacheMessage, "Enter " + LogMethodName);
            try
            {
                if (lpauxCache == null)
                {
                    this.FillLpAuxCache();
                }

                return lpauxCache.ToList();
            }
            catch (Exception ex)
            {
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DatabaseCacheMessage, string.Format("Error in {0}: Message {1}", LogMethodName, ex.ToString()));
            }

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DatabaseCacheMessage, "Exit " + LogMethodName);

            return null;
        }

        /// <summary>
        /// Updates the data cache status.
        /// </summary>
        /// <param name="partitionKey">The partition key.</param>
        /// <param name="rowKey">The row key.</param>
        /// <param name="timeTaken">The time taken.</param>
        private void UpdateDataCacheStatus(string partitionKey, string rowKey, TimeSpan timeTaken)
        {
            DynamicTableEntity dataCacheDetail = new DynamicTableEntity();
            dataCacheDetail.PartitionKey = partitionKey;
            dataCacheDetail.RowKey = rowKey;
            dataCacheDetail.Properties.Add("status", EntityProperty.GeneratePropertyForInt(1));
            dataCacheDetail.Properties.Add("TotalTime_InSec", EntityProperty.GeneratePropertyForDouble(timeTaken.TotalSeconds));

            //true value will make sure cached data will be updated in specified time
            dataCacheDetail.Properties.Add("NeedsUpdate", EntityProperty.GeneratePropertyForBool(true));

            this.DalcCommon.InsertEntity(Utils.GetRegionalTableName(Constants.DataCacheUpdateStatusTableName), dataCacheDetail);
        }

        /// <summary>
        /// Combines the and get cache.
        /// </summary>
        /// <typeparam name="T">type of object</typeparam>
        /// <param name="cacheObjectType">Type of the cache object.</param>
        private void LoadCacheData<T>(ServiceCacheObjectType cacheObjectType)
        {
            if (cacheObjectType == ServiceCacheObjectType.TvEngData)
            {
                if (tvengCache == null)
                {
                    tvengCache = Utils.ReadDumpedFile<CacheObjectTvEngdata>(cacheObjectType.ToString());
                }
            }
            else if (cacheObjectType == ServiceCacheObjectType.LPAUX)
            {
                if (lpauxCache == null)
                {
                    this.FillLpAuxCache();
                }
            }
            else if (cacheObjectType == ServiceCacheObjectType.RegionPolygons)
            {
                if (regionPolygonsCache == null)
                {
                    this.FillRegionPolygonsCache();
                }
            }
        }

        #endregion
    }
}
