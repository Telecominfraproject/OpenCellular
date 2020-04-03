// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Terrain
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using ApplicationServer.Caching;
    using Common.Utilities;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Entities;

    /// <summary>This class reads USGS1ArcReader, 1arc second interval data</summary>
    public class USGS1ArcReader : ITerrainElevation
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

        /// <summary>
        /// Initializes a new instance of the <see cref="USGS1ArcReader"/> class.
        /// </summary>
        public USGS1ArcReader()
        {
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

        /// <summary>Gets or sets the logger.</summary>
        /// <value>The logger.</value>
        [Dependency]
        public ILogger Logger { get; set; }

        /// <summary>Gets or sets the auditor.</summary>
        /// <value>The auditor.</value>
        [Dependency]
        public IAuditor Auditor { get; set; }

        /// <summary>
        /// Gets or sets the terrain DALC.
        /// </summary>
        /// <value>The terrain DALC.</value>
        [Dependency]
        public IDalcTerrain TerrainDalc { get; set; }

        /// <summary>Calculates the elevation.</summary>
        /// <param name="location">The location.</param>
        /// <returns>returns elevation.</returns>
        public Distance CalculateElevation(Location location)
        {
            ////string logMethodName = string.Format("USGS1ArcReader.CalculateElevation- Lat -> {0}, Lon -> {1}", location.Latitude, location.Longitude);

            //// Begin Log transaction
            ////this.Logger.Log(TraceEventType.Information, LoggingMessageId.ElevationMessage, "Enter " + logMethodName);

            string arc1dataDirectory = Utils.Configuration[Constants.ConfigSettingArc1DataDirectoryName];
            string arc2dataDirectory = Utils.Configuration[Constants.ConfigSettingArc2DataDirectoryName];

            Location lowerLeftLocation = new Location();
            bool isArc2File = false;

            // Determine which elevation file we need for the specified location
            // Also, determine the lower left lat, long
            string fileNameArc1 = this.GetFileName(location, ref lowerLeftLocation);
            string fileNameArc2 = this.GetFileName(location, ref lowerLeftLocation, true);

            bool fileExists = false;
            if (this.localCache.ContainsKey(fileNameArc1))
            {
                fileExists = true;
                isArc2File = false;
            }
            else if (this.localCache.ContainsKey(fileNameArc2))
            {
                fileExists = true;
                isArc2File = true;
            }

            if (!fileExists)
            {
                fileExists = this.TerrainDalc.CheckTerrainFileExists(fileNameArc1, arc1dataDirectory);

                if (!fileExists)
                {
                    fileExists = this.TerrainDalc.CheckTerrainFileExists(fileNameArc2, arc2dataDirectory);
                    if (fileExists)
                    {
                        isArc2File = true;
                    }
                }
            }

            // Stores the elevation for the actual location
            double elevation = 0;

            try
            {
                // if no file exists either in 2 places return 0.
                if (!fileExists)
                {
                    return new Distance(0, DistanceUnit.Meter);
                }

                // This is the index where the data can be fetched from
                int firstByteIndex = 0;
                if (isArc2File)
                {
                    firstByteIndex = this.TranslateForArc2File(lowerLeftLocation, location);
                }
                else
                {
                    firstByteIndex = this.TranslateForArc1File(lowerLeftLocation, location);
                }

                // Lookup cache for the two bytes corresponding to the elevation data
                byte[] converterBuffer = this.ReadBytes(isArc2File ? fileNameArc2 : fileNameArc1, isArc2File, firstByteIndex, firstByteIndex + 1, firstByteIndex + 2, firstByteIndex + 3);

                if (converterBuffer == null)
                {
                    elevation = double.NegativeInfinity;
                }
                else
                {
                    elevation = BitConverter.ToSingle(converterBuffer, 0);

                    if (elevation.CompareTo(-9999) == 0)
                    {
                        elevation = 0;
                    }
                }
            }
            catch (Exception ex)
            {
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.ElevationGlobUnsupportedTileId, string.Format("Error in Tile {0}, {1}, {2}, {3}", fileNameArc1, fileNameArc2, location.ToLocationString(), ex.ToString()));

                return new Distance(0, DistanceUnit.Meter);
            }

            //// End Log transaction
            ////this.Logger.Log(TraceEventType.Information, LoggingMessageId.ElevationMessage, "Exit " + logMethodName);

            return new Distance(elevation, DistanceUnit.Meter);
        }

        /// <summary>
        /// Calculates the elevation.
        /// </summary>
        /// <param name="easting">The easting.</param>
        /// <param name="northing">The northing.</param>
        /// <returns>returns value at specified easting, northing</returns>
        public double CalculateElevation(double easting, double northing)
        {
            throw new NotImplementedException();
        }

        /// <summary>
        /// Gets the name of the file.
        /// </summary>
        /// <param name="location">The location.</param>
        /// <param name="upperLeftLeftLocation">The lower left location.</param>
        /// <param name="isArc2File">if set to <c>true</c> [is arc2 file].</param>
        /// <returns>returns filename.</returns>
        public string GetFileName(Location location, ref Location upperLeftLeftLocation, bool isArc2File = false)
        {
            int longitude = (int)Math.Abs(location.Longitude) + 1;

            int latitude = Math.Abs((int)location.Latitude) + 1;

            string filename = "float";

            if (location.Latitude >= 0)
            {
                filename += "n" + latitude;
            }
            else
            {
                filename += "s" + latitude;
            }

            if (location.Longitude >= 0)
            {
                if (longitude < 10)
                {
                    filename += "e00" + longitude;
                }
                else if (longitude < 100)
                {
                    filename += "e0" + longitude;
                }
                else
                {
                    filename += "e" + longitude;
                }
            }
            else
            {
                longitude = Math.Abs(longitude);

                if (longitude < 10)
                {
                    filename += "w00" + longitude;
                }
                else if (longitude < 100)
                {
                    filename += "w0" + longitude;
                }
                else
                {
                    filename += "w" + longitude;
                }
            }

            if (isArc2File)
            {
                filename += "_2.flt";
            }
            else
            {
                filename += "_1.flt";
            }

            upperLeftLeftLocation.Latitude = latitude;
            upperLeftLeftLocation.Longitude = longitude;

            return filename;
        }

        /// <summary>
        /// Translates the specified bottom latitude.
        /// </summary>
        /// <param name="upperLeftLocation">The lower left location.</param>
        /// <param name="point">The point.</param>
        /// <returns>returns translated points</returns>
        private int TranslateForArc1File(Location upperLeftLocation, Location point)
        {
            ////const double lowerLocationMantissa = 0.99833333333;
            const double UpperLocationMantissa = 0.0016666667;
            const int Nrows = 3612;
            const double CellSize = 0.000277777777778;
            double upperLeftLatitude = upperLeftLocation.Latitude + UpperLocationMantissa;
            double upperLeftLongitude = upperLeftLocation.Longitude + UpperLocationMantissa;

            int y = (int)Math.Round((upperLeftLatitude - Math.Abs(point.Latitude)) / CellSize);
            int usedY = y - 1;
            int x = (int)Math.Round((upperLeftLongitude - Math.Abs(point.Longitude)) / CellSize);
            int usedX = x - 1;
            int offset = (usedY * Nrows * 4) + (usedX * 4);

            return offset;
        }

        /// <summary>
        /// Translates for ARC2 file.
        /// </summary>
        /// <param name="upperLeftLocation">The upper left location.</param>
        /// <param name="point">The point.</param>
        /// <returns>returns translated points.</returns>
        private int TranslateForArc2File(Location upperLeftLocation, Location point)
        {
            ////const double lowerLocationMantissa = 0.99666666667;
            const double UpperLocationMantissa = .0033333333;
            const int Nrows = 1812;
            const double CellSize = 0.000555555555556;
            double upperLeftLatitude = upperLeftLocation.Latitude + UpperLocationMantissa;
            double upperLeftLongitude = upperLeftLocation.Longitude + UpperLocationMantissa;

            int y = (int)Math.Round((upperLeftLatitude - Math.Abs(point.Latitude)) / CellSize);
            int usedY = y - 1;
            int x = (int)Math.Round((upperLeftLongitude - Math.Abs(point.Longitude)) / CellSize);
            int usedX = x - 1;
            int offset = (usedY * Nrows * 4) + (usedX * 4);

            return offset;
        }

        /// <summary>
        /// Returns bytes at the specified indices
        /// </summary>
        /// <param name="fileName">File Name</param>
        /// <param name="isArc2File">if set to <c>true</c> [is arc2 file].</param>
        /// <param name="byteIndices">The byte indices.</param>
        /// <returns>Array of Bytes</returns>
        private byte[] ReadBytes(string fileName, bool isArc2File, params int[] byteIndices)
        {
            var cacheData = this.GetElevationEntry(fileName, isArc2File).Data as byte[];

            byte[] bytes = null;
            if (cacheData != null)
            {
                bytes = new byte[byteIndices.Length];
                for (int i = 0; i < byteIndices.Length; i++)
                {
                    bytes[i] = cacheData[byteIndices[i]];
                }
            }
            else
            {
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.TerrainCacheGenericMessage, "File Not Found.");
            }

            return bytes;
        }

        /// <summary>
        /// Adds the specified file to the cache (in memory)
        /// </summary>
        /// <param name="filename">File to be cached in memory</param>
        /// <param name="isArc2Data">if set to <c>true</c> [is arc2 data].</param>
        private void Add(string filename, bool isArc2Data = false)
        {
            string logMethodName = "ElevationCache.Add (filename -> " + filename;

            string dataDirectory;
            if (isArc2Data)
            {
                dataDirectory = Utils.Configuration[Constants.ConfigSettingArc2DataDirectoryName];
            }
            else
            {
                dataDirectory = Utils.Configuration[Constants.ConfigSettingArc1DataDirectoryName];
            }

            int fileSize = 0;
            if (isArc2Data)
            {
                fileSize = 1812 * 1812 * 4;
            }
            else
            {
                fileSize = 3612 * 3612 * 4;
            }

            try
            {
                //// Check to see if the cache size has exceeded
                bool cacheSizeExceeded = this.localCache.Count > 20;

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
                        fileContents = (byte[])this.TerrainDalc.LoadTerrainFile(filename, dataDirectory, fileSize);
                    }
                    catch (Exception ex)
                    {
                        this.Logger.Log(TraceEventType.Critical, LoggingMessageId.PropagationTileNotFound, string.Format("Tile Not Found for filename -> {0}", filename));
                        this.Logger.Log(TraceEventType.Critical, LoggingMessageId.PropagationException, ex.ToString());
                        fileContents = new byte[fileSize];
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
        /// Removes old and untouched entries from the cache
        /// </summary>
        private void CleanupLocalCache()
        {
            this.localCache.Clear();
        }

        /// <summary>
        /// Returns the elevation cache entry (first searches the local cache, then the azure cache and finally loads the file from blob storage if it was not cached).
        /// </summary>
        /// <param name="fileName">Name of the file that is being cached.</param>
        /// <param name="isArcData">if set to <c>true</c> [is arc data].</param>
        /// <returns>Returns the cache entry (null if no entry was found)</returns>
        private ElevationCacheEntry GetElevationEntry(string fileName, bool isArcData)
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
            this.Add(fileName, isArcData);

            if (this.localCache.ContainsKey(fileName))
            {
                entry = this.localCache[fileName];
            }

            return entry;
        }
    }
}
