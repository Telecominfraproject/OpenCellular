// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Terrain
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.Text;
    using Common.Utilities;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Entities;

    /// <summary>This class reads USGS1ArcReader, 1arc second interval data</summary>
    public class OrdnanceSurveyOSReader : ITerrainElevation
    {
        /// <summary>The elevation cache</summary>
        private ICache elevationCache;

        /// <summary>
        /// Initializes a new instance of the <see cref="OrdnanceSurveyOSReader"/> class.
        /// </summary>
        public OrdnanceSurveyOSReader()
        {
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
                    this.elevationCache = new ElevationCache(this.Logger, this.TerrainDalc, this.GetCacheElementSize());
                }

                return this.elevationCache;
            }
        }

        /// <summary>Calculates the elevation.</summary>
        /// <param name="location">The location.</param>
        /// <returns>returns elevation.</returns>
        public Distance CalculateElevation(Location location)
        {
            throw new NotImplementedException();
        }

        /// <summary>Gets the size of the single cache.</summary>
        /// <returns>returns cache size.</returns>
        public int GetCacheElementSize()
        {
            return 512 * 1024;
        }

        /// <summary>Returns a <see cref="System.String" /> that represents this instance.</summary>
        /// <returns>A <see cref="System.String" /> that represents this instance.</returns>
        public override string ToString()
        {
            return "Ordnance Survey Reader";
        }

        /// <summary>
        /// Calculates the elevation.
        /// </summary>
        /// <param name="paramEasting">The parameter easting.</param>
        /// <param name="paramNorthing">The parameter northing.</param>
        /// <returns>returns the clutter dataset value.</returns>
        public double CalculateElevation(double paramEasting, double paramNorthing)
        {
            string logMethodName = string.Format("ClutterDatasetReader.CalculateElevation- Easting -> {0}, Northing -> {1}", paramEasting, paramNorthing);

            // Begin Log transaction
            this.Logger.Log(TraceEventType.Information, LoggingMessageId.ElevationMessage, "Enter " + logMethodName);

            int easting = (int)paramEasting;
            int northing = (int)paramNorthing;

            string fileInitial = GeoCalculations.GetNationalGridTileName(easting, northing);

            string eastingSuffix = ((easting / 10000) % 10).ToString();
            string northingSuffix = ((northing / 10000) % 10).ToString();
            string format = ".asc";

            string fileName = string.Concat(fileInitial, eastingSuffix, northingSuffix, format);
           
            double elevation = 0.0;

            try
            {
                byte[] fileData = this.ElevationDataCache.ReadCachedData(fileName) as byte[];
                string terrainRawData = Encoding.ASCII.GetString(fileData);
                string[] terrainData = terrainRawData.Split(new[] { Environment.NewLine }, StringSplitOptions.RemoveEmptyEntries);

                terrainData = terrainData.Skip(5).ToArray();

                int terrainEastingCellIndex = ((int)easting % 10000) / 50;

                // calculate from end of file
                int terrainNorthingCellIndex = 199 - (((int)northing % 10000) / 50);
               
                string row = terrainData[terrainNorthingCellIndex];

                var rowEastings = row.Split(new[] { " " }, StringSplitOptions.None);

                elevation = Convert.ToDouble(rowEastings[terrainEastingCellIndex]);

                if (elevation == -0.8)
                {
                    elevation = 0;
                }
            }
            catch (Exception ex)
            {
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.ElevationGlobUnsupportedTileId, ex.ToString());

                return 0;
            }

            return elevation;
        }
    }
}
