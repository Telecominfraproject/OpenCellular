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
    using System.Web.Caching;
    using ApplicationServer.Caching;
    using Entities;
    using Microsoft.WindowsAzure.ServiceRuntime;
    using Practices.Unity;
    using Utilities;
    using WindowsAzure.Storage.Table;

    /// <summary>
    /// Represents Class OFCOMServiceCacheHelper.
    /// </summary>
    public class OFCOMServiceCacheHelper : IServiceCacheHelper
    {
        /// <summary>The cache objects</summary>
        private static List<PmseAssignment> pmseAssignmentCache;

        /// <summary>The synchronize object</summary>
        private static object syncObject = new object();

        /// <summary>The synchronize timer</summary>
        private static Timer syncTimer;

        /// <summary>
        /// Last modified timestamp of pmseAssignmentCache object.
        /// </summary>
        private static DateTimeOffset cacheModifiedOn = DateTimeOffset.MinValue;

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
            this.FillPMSEAssignmentDataCache();
        }

        /// <summary>
        /// Gets the service cache objects.
        /// </summary>
        /// <param name="cacheObjectType">Type of the cache object.</param>
        /// <param name="serviceCacheRequestParameters">The service cache request parameters.</param>
        /// <returns>returns List of cache data.</returns>
        public object GetServiceCacheObjects(ServiceCacheObjectType cacheObjectType, ServiceCacheRequestParameters serviceCacheRequestParameters)
        {
            if (cacheObjectType == ServiceCacheObjectType.PMSEAssignment)
            {
                return this.GetPMSEAssignment(serviceCacheRequestParameters);
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
            const string LogMethodName = "OFCOMServiceCacheHelper.SearchCacheObjects";

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DatabaseCacheMessage, "Enter " + LogMethodName);
            object results = null;
            try
            {
                this.LoadCacheData<PmseAssignment>(ServiceCacheObjectType.PMSEAssignment);

                if (((int)searchCacheRequestType & (int)SearchCacheRequestType.ByEastingNorthing) == (int)SearchCacheRequestType.ByEastingNorthing)
                {
                    results = pmseAssignmentCache.Where(obj => obj.Easting == serviceCacheRequestParameters.Easting && obj.Northing == serviceCacheRequestParameters.Northing);
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
        /// Fills the CDBS TVENG data cache.
        /// </summary>
        private void FillPMSEAssignmentDataCache()
        {
            const string LogMethodName = "OFCOMServiceCacheHelper.FillPMSEAssignmentDataCache";

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DatabaseCacheMessage, "Enter " + LogMethodName);

            try
            {
                Stopwatch watch = new Stopwatch();
                //// List<PmseAssignment> pmseAssignment = null;
                bool refreshCache = false;
                var pmsesyncStatus = this.DalcCommon.FetchEntity<DynamicTableEntity>(Utils.GetRegionalTableName(Constants.PmseSyncStatusTableName), new { RowKey = Constants.PMSEAssignmentsTable });

                if (pmsesyncStatus.Count > 0 && pmsesyncStatus[0].Properties.Keys.Contains("status") && cacheModifiedOn != DateTimeOffset.MinValue)
                {
                    var status = pmsesyncStatus[0].Properties["status"].Int32Value;
                    var timestamp = pmsesyncStatus[0].Timestamp;
                    if (status == 1)
                    {
                        if (cacheModifiedOn < timestamp)
                        {
                            refreshCache = true;
                        }
                    }
                }
                else
                {
                    refreshCache = true;
                }

                if (refreshCache)
                {
                    watch.Start();

                    lock (syncObject)
                    {
                        if (pmseAssignmentCache != null)
                        {
                            pmseAssignmentCache.Clear();
                        }

                        pmseAssignmentCache = this.DalcServiceCache.FetchPmseAssignments(Constants.PMSEAssignmentsTable);
                        cacheModifiedOn = DateTimeOffset.UtcNow;
                    }

                    watch.Stop();

                    string logMessage = string.Empty;
                    TraceEventType traceEventType = TraceEventType.Information;

                    if ((pmseAssignmentCache == null) || (pmseAssignmentCache != null && !pmseAssignmentCache.Any()))
                    {
                        logMessage = "No PMSE updates found in WSDB. Hence, PMSE Assignment Cache has been refreshed to null or empty list";
                        traceEventType = TraceEventType.Warning;
                    }
                    else
                    {
                        logMessage = string.Format("PMSE Assignment Cache has been refreshed with the PMSE Updates {0}| Time taken to refresh cache {1} seconds", pmseAssignmentCache.FirstOrDefault().PartitionKey, watch.Elapsed.Seconds);
                    }

                    this.Logger.Log(traceEventType, LoggingMessageId.DatabaseCacheMessage, logMessage);
                }

                int timeInterval = (int)TimeSpan.FromMinutes(5).TotalMilliseconds;
                if (Utils.Configuration.HasSetting("PmseAssignment_CacheIntervalMin"))
                {
                    timeInterval = (int)TimeSpan.FromMinutes(Utils.Configuration["PmseAssignment_CacheIntervalMin"].ToInt32()).TotalMilliseconds;
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
        /// Runs the cache.
        /// </summary>
        /// <param name="sender">The sender.</param>
        /// <param name="e">The <see cref="ElapsedEventArgs"/> instance containing the event data.</param>
        private void UpdateCache(object sender, ElapsedEventArgs e)
        {
            this.FillPMSEAssignmentDataCache();
        }

        /// <summary>
        /// Gets the incumbents.
        /// </summary>
        /// <param name="requestParameters">The request parameters.</param>
        /// <returns>returns Incumbent[][].</returns>
        private List<PmseAssignment> GetPMSEAssignment(ServiceCacheRequestParameters requestParameters)
        {
            const string LogMethodName = "OFCOMServiceCacheHelper.GetPMSEAssignment";

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DatabaseCacheMessage, "Enter " + LogMethodName);
            try
            {
                this.LoadCacheData<PmseAssignment>(ServiceCacheObjectType.PMSEAssignment);
                return pmseAssignmentCache;
            }
            catch (Exception ex)
            {
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.DatabaseCacheMessage, string.Format("Error in {0}: Message {1}", LogMethodName, ex.ToString()));
            }

            this.Logger.Log(TraceEventType.Information, LoggingMessageId.DatabaseCacheMessage, "Exit " + LogMethodName);

            return null;
        }

        /// <summary>
        /// Combines the and get cache.
        /// </summary>
        /// <typeparam name="T">type of object</typeparam>
        /// <param name="cacheObjectType">Type of the cache object.</param>
        private void LoadCacheData<T>(ServiceCacheObjectType cacheObjectType)
        {
            if (cacheObjectType == ServiceCacheObjectType.PMSEAssignment)
            {
                if (pmseAssignmentCache == null)
                {
                    pmseAssignmentCache = this.DalcServiceCache.FetchPmseAssignments(Constants.PMSEAssignmentsTable);
                }
            }
        }

        #endregion
    }
}
