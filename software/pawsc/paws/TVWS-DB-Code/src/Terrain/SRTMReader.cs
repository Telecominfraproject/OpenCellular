// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Terrain
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Entities;

    /// <summary>This class reads SRTM version 2, 1arc second interval data</summary>
    public class SRTMReader : ITerrainElevation
    {
        /// <summary>The default size (width and height) for each Version 4 SRTM
        /// (all but two files were 6001x6001 -- the "n30w090.RGT" and "s10e025.RGT" files are only 6000x6000).
        /// </summary>
        private const int DefaultSRTMWidthHeight = 6001;

        /// <summary>The four megabytes</summary>
        private const int FourMB = 4194304;

        /// <summary>The version </summary>
        private readonly int version;

        /// <summary>The Version 4 SRTM files that have a custom width/height value. </summary>
        private readonly Dictionary<string, int> srtmFileWidthHeight = new Dictionary<string, int> { { "n30w090.rgt", 6000 }, { "s10e025.rgt", 6000 } };

        /// <summary>The elevation cache</summary>
        private ICache elevationCache;

        /// <summary>
        /// Initializes a new instance of the <see cref="SRTMReader"/> class.
        /// </summary>
        public SRTMReader()
        {
            this.version = 4;
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

        /// <summary>
        /// Gets the elevation data cache.
        /// </summary>
        /// <value>The elevation data cache.</value>
        public ICache ElevationDataCache
        {
            get
            {
                if (this.elevationCache == null)
                {
                    this.elevationCache = new ElevationCache(this.Logger, this.TerrainDalc, this.GetSingleCacheSize());
                }

                return this.elevationCache;
            }
        }

        /// <summary>Gets the memory usage.</summary>
        /// <returns>returns memory usage</returns>
        public double GetMemoryUsage()
        {
            return this.ElevationDataCache.GetMemoryUsage();
        }

        /// <summary>Calculates the elevation.</summary>
        /// <param name="location">The location.</param>
        /// <returns>returns elevation.</returns>
        public Distance CalculateElevation(Location location)
        {
            string logMethodName = string.Format("SRTMReader.CalculateElevation- Lat -> {0}, Lon -> {1}", location.Latitude, location.Longitude);

            // Begin Log transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.ElevationMessage, "Enter " + logMethodName);

            // Determine the bottom latitude of the grid
            // which encompasses the location coordinates
            double bottomLatitude = (int)location.Latitude;

            // Determine the left longitude of the grid
            // which encompasses the location coordinates
            double leftLongitude = 0;

            // Determine which elevation file we need for the specified location
            // Also, determine the lower left lat, long
            string requiredFile = this.GetFileName(location, ref bottomLatitude, ref leftLongitude);

            // Stores the elevation for the actual location
            double elevation = 0;

            int rows = 0;
            int columns = 0;

            try
            {
                // For performance reason, the SRTM file width/height are hard-coded  instead of reading
                // from the actual files.
                int value;

                if (this.srtmFileWidthHeight.TryGetValue(requiredFile.ToLower(), out value))
                {
                    rows = value;
                    columns = value;
                }
                else
                {
                    rows = DefaultSRTMWidthHeight;
                    columns = DefaultSRTMWidthHeight;
                }

                if (rows == 0 || columns == 0)
                {
                    // This means the corresponding cache entry was swapped out
                    // by the time we get here i.e. made this call.
                    return new Distance(0, DistanceUnit.Feet);
                }

                // This is the index where the data can be fetched from
                int seek = this.Translate(bottomLatitude, leftLongitude, location, rows, columns);
                int subCacheFirstByteIndex = (seek + 1) % FourMB;
                int subCacheSecondByteIndex = seek % FourMB;
                int subCacheRowId = (seek + 1) - subCacheFirstByteIndex;
                string subcacheIndex = requiredFile + subCacheRowId;

                // Lookup cache for the two bytes corresponding to the elevation data
                byte[] converterBuffer = this.ReadBytes(subcacheIndex, subCacheFirstByteIndex, subCacheSecondByteIndex);

                if (converterBuffer == null)
                {
                    elevation = double.NegativeInfinity;
                }
                else
                {
                    // Convert to little endian
                    elevation = BitConverter.ToInt16(converterBuffer, 0);

                    if (elevation == -9999)
                    {
                        elevation = 0;
                    }
                }
            }
            catch (Exception ex)
            {
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.ElevationGlobUnsupportedTileId, ex.ToString());

                return new Distance(0, DistanceUnit.Feet);
            }

            // End Log transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.ElevationMessage, "Exit " + logMethodName);

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

        /// <summary>Gets the size of the single cache.</summary>
        /// <returns>returns cache size.</returns>
        public int GetSingleCacheSize()
        {
            return 6001 * 6001 * 2;
        }

        /// <summary>Returns a <see cref="System.String" /> that represents this instance.</summary>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public override string ToString()
        {
            return "SRTM4.1";
        }

        /// <summary>Gets the name of the file.</summary>
        /// <param name="location">The location.</param>
        /// <param name="bottomLatitude">The bottom latitude.</param>
        /// <param name="leftLongitude">The left longitude.</param>
        /// <returns>returns filename.</returns>
        private string GetFileName(Location location, ref double bottomLatitude, ref double leftLongitude)
        {
            const string LogMethodName = "SRTMReader.GetFileName(Location location, ref double bottomLatitude, ref double leftLongitude)";

            // Begin Log transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.ElevationMessage, "Enter " + LogMethodName);

            int longitude = (int)location.Longitude;

            // For negative longitude, need to round down (specifically the case between 0.5 and -0.5 will get the same value if truncated).
            if (location.Longitude < 0)
            {
                longitude = (int)Math.Floor(location.Longitude);
            }

            int latitude = (int)location.Latitude;

            // For negative Latitude, need to round down (specifically the case between 0.5 and -0.5 will get the same value if truncated).
            if (location.Latitude < 0)
            {
                latitude = (int)Math.Floor(location.Latitude);
            }

            if (this.version == 4)
            {
                latitude = (int)Math.Floor((double)latitude / 5.0) * 5;
                longitude = (int)Math.Floor((double)longitude / 5.0) * 5;
            }
            else
            {
                if (location.Longitude < 0)
                {
                    // We want the lower left part of the cell
                    longitude -= 1;
                }
            }

            // We're going to return these values to the caller
            bottomLatitude = latitude;
            leftLongitude = longitude;

            string filename = string.Empty;

            if (latitude >= 0)
            {
                filename += "N" + latitude;
            }
            else
            {
                latitude = Math.Abs(latitude);
                filename += "S" + latitude;
            }

            if (longitude >= 0)
            {
                if (longitude < 10)
                {
                    filename += "E00" + longitude;
                }
                else if (longitude < 100)
                {
                    filename += "E0" + longitude;
                }
                else
                {
                    filename += "E" + longitude;
                }
            }
            else
            {
                longitude = Math.Abs(longitude);

                if (longitude < 10)
                {
                    filename += "W00" + longitude;
                }
                else if (longitude < 100)
                {
                    filename += "W0" + longitude;
                }
                else
                {
                    filename += "W" + longitude;
                }
            }

            if (this.version == 4)
            {
                filename += ".rgt";
            }
            else
            {
                filename += ".hgt";
            }

            // End Log transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.ElevationMessage, "Exit " + LogMethodName);

            return filename;
        }

        /// <summary>Translates the specified bottom latitude.</summary>
        /// <param name="bottomLatitude">The bottom latitude.</param>
        /// <param name="leftLongitude">The left longitude.</param>
        /// <param name="location">The location.</param>
        /// <param name="rows">The rows.</param>
        /// <param name="columns">The columns.</param>
        /// <returns>returns translated points</returns>
        private int Translate(double bottomLatitude, double leftLongitude, Location location, int rows, int columns)
        {
            int x, y;

            y = (rows - 1) - (int)(rows * Math.Abs(location.Latitude - bottomLatitude) / 5.0);
            x = (int)(columns * Math.Abs(leftLongitude - location.Longitude) / 5.0);

            return 4 + (((y * columns) + x) * 2);
        }

        /// <summary>
        /// Returns bytes at the specified indices
        /// </summary>
        /// <param name="fileName">File Name</param>
        /// <param name="byteIndices">The byte indices.</param>
        /// <returns>Array of Bytes</returns>
        private byte[] ReadBytes(string fileName, params int[] byteIndices)
        {
            var cacheData = this.ElevationDataCache.ReadCachedData(fileName) as byte[];

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
    }
}
