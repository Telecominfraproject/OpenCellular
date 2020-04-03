// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Terrain
{
    using System;
    using System.Collections.Generic;
    using System.Configuration;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.Net;
    using System.Text;
    using System.Threading.Tasks;
    using Microsoft.ApplicationServer.Caching;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Common.Utilities;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Blob;

    /// <summary>
    /// Elevation Cache
    /// </summary>
    public class ElevationCache : ICache
    {
        /// <summary>
        /// Contains a reference to a local in-memory cache.
        /// </summary>
        private Dictionary<string, ElevationCacheEntry> localCache = new Dictionary<string, ElevationCacheEntry>();

        /// <summary>
        /// Contains a reference to azure's DataCache (can disable Azure DataCache by setting the "UseAzureCache" app setting to "False").
        /// </summary>
        private DataCache azureCache;

        /// <summary>
        /// Maximum cache size in local cache in MB.
        /// </summary>
        private int maxLocalCacheSize;

        /// <summary>The time to evict cache</summary>
        private int timeToEvictCache;

        /// <summary>The maximum cache expiry time</summary>
        private int maxCacheExpiryTime;

        /// <summary>The cache element size</summary>
        private int cacheElementSize;

        /// <summary>
        /// Initializes a new instance of the ElevationCache class.
        /// </summary>
        /// <param name="logger">The logger.</param>
        /// <param name="terrainDalc">The terrain DALC.</param>
        /// <param name="cacheElementSizeInBytes">Size of the cache element in bytes.</param>
        public ElevationCache(ILogger logger, IDalcTerrain terrainDalc, int cacheElementSizeInBytes)
        {
            this.Logger = logger;
            this.TerrainDalc = terrainDalc;
            this.cacheElementSize = cacheElementSizeInBytes;

            this.maxLocalCacheSize = Utils.Configuration["maxLocalCacheSize_MB"].ToInt32();
            this.timeToEvictCache = Utils.Configuration["TimeToEvictCache_Min"].ToInt32();
            this.maxCacheExpiryTime = Utils.Configuration["MaxCacheExpiryTime_Hour"].ToInt32();

            //// Do not use Azure Cahcing if the "UseAzureCache" variable is set to fale in the web.config or app.config
            //// Not enabled due to limitation of SingleElementCache size in azure which is less than the single file size of terrain
            try
            {
                this.azureCache = Utils.Configuration["UseAzureCache"].ToBool() ? new DataCacheFactory().GetDefaultCache() : null;
            }
            catch (Exception)
            {
                this.azureCache = null;
            }
        }

        /// <summary>
        /// Gets or sets ILogger Interface
        /// </summary>
        public ILogger Logger { get; set; }

        /// <summary>
        /// Gets or sets the terrain DALC.
        /// </summary>
        /// <value>The terrain DALC.</value>
        public IDalcTerrain TerrainDalc { get; set; }

        /// <summary>
        /// Adds the specified file to the cache (in memory)
        /// </summary>
        /// <param name="filename">File to be cached in memory</param>
        public void Add(string filename)
        {
            string logMethodName = "ElevationCache.Add (filename -> " + filename;

            bool cacheSizeExceeded = false;
            string dataDirectory = Utils.Configuration["dataDirectory"];
            
            try
            {
                //// Check to see if the cache size has exceeded
                cacheSizeExceeded = this.GetMemoryUsage() >= this.maxLocalCacheSize;

                if (cacheSizeExceeded)
                {
                    this.CleanupLocalCache();
                }

                byte[] fileContents = null;

                // Insert file contents into the cache
                if (!this.localCache.ContainsKey(filename))
                {
                    try
                    {
                        fileContents = (byte[])this.TerrainDalc.LoadTerrainFile(filename, dataDirectory, this.cacheElementSize);
                    }
                    catch (Exception ex)
                    {
                        this.Logger.Log(TraceEventType.Critical, LoggingMessageId.PropagationTileNotFound, string.Format("Tile Not Found for filename -> {0}", filename));
                        this.Logger.Log(TraceEventType.Critical, LoggingMessageId.PropagationException, ex.ToString());
                        fileContents = new byte[this.cacheElementSize];
                    }

                    ElevationCacheEntry newEntry = new ElevationCacheEntry(fileContents);
                    this.localCache.Add(filename, newEntry);

                    // Also add to azure cache.
                    if (this.azureCache != null)
                    {
                        // Since we just read this value from blog storage, lets also put it in azure cache.
                        this.azureCache.Put(filename, newEntry);
                    }
                }
            }
            catch (Exception ex)
            {
                // We've hit a tile in Canada. We don't care about such a tile
                // We hit a tile that does not exist for any Country in the system.
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.ElevationGlobUnsupportedTileId, logMethodName);
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.ElevationGlobUnsupportedTileId, ex.ToString());
            }
        }

        /// <summary>
        /// Returns memory usage in bytes
        /// </summary>
        /// <returns>Returns memory usage</returns>
        public double GetMemoryUsage()
        {
            return (this.cacheElementSize * this.localCache.Count) / (1024 * 1024);
        }

        /// <summary>
        /// Clears the cache contents
        /// </summary>
        public void Clear()
        {
            this.localCache.Clear();
        }

        /// <summary>
        /// Removes old and untouched entries from the cache
        /// </summary>
        public void CleanupLocalCache()
        {
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.TerrainCacheGenericMessage, "Cache Cleanup Called");

            TimeSpan timeOutPeriod = TimeSpan.FromMinutes(this.timeToEvictCache);

            ////Evict the oldest entry
            DateTime now = DateTime.Now;
            List<string> toBeDeleted = new List<string>();

            ////Number of items removed from the cache
            int numberCleaned = 0;

            var sortedCaches = this.localCache.OrderBy(obj => obj.Value.LastAccessed).ThenBy(obj => obj.Value.AccessCount);
            foreach (var cacheItem in sortedCaches)
            {
                if (cacheItem.Value.LastAccessed > DateTime.Now.AddHours(this.maxCacheExpiryTime))
                {
                    toBeDeleted.Add(cacheItem.Key);
                }
                else if (now.Subtract(cacheItem.Value.LastAccessed).Ticks > timeOutPeriod.Ticks)
                {
                    ////Mark up entries to be deleted
                    toBeDeleted.Add(cacheItem.Key);
                }
            }

            if (toBeDeleted.Count > 0)
            {
                foreach (string key in toBeDeleted)
                {
                    numberCleaned++;
                    this.localCache.Remove(key);
                }
            }
            else
            {
                // if no entries found to delete then delete the oldest first entry
                this.localCache.Remove(sortedCaches.ElementAt(0).Key);
            }
        }

        /// <summary>
        /// Returns bytes at the specified indices
        /// </summary>
        /// <param name="fileName">File Name</param>
        /// <returns>cache object</returns>
        public object ReadCachedData(string fileName)
        {
            ElevationCacheEntry entry = this.GetElevationEntry(fileName);

            return entry.Data;
        }

        /// <summary>
        /// Returns the elevation cache entry (first searches the local cache, then the azure cache and finally loads the file from blob storage if it was not cached).
        /// </summary>
        /// <param name="fileName">Name of the file that is being cached.</param>
        /// <returns>Returns the cache entry (null if no entry was found)</returns>
        private ElevationCacheEntry GetElevationEntry(string fileName)
        {
            ElevationCacheEntry entry = null;

            // First check local cache
            if (this.localCache.ContainsKey(fileName))
            {
                entry = this.localCache[fileName];
                entry.RefreshCacheDetails();

                return entry;
            }

            if (this.azureCache != null)
            {
                // Not in local cache, so check azure cache
                entry = this.azureCache.Get(fileName) as ElevationCacheEntry;
                if (entry != null)
                {
                    this.localCache[fileName] = entry;
                    entry.RefreshCacheDetails();
                }
            }

            // Not in local or azure cache, so read from azure storage
            this.Add(fileName);

            if (this.localCache.ContainsKey(fileName))
            {
                entry = this.localCache[fileName];
            }

            return entry;
        }
    }
}
