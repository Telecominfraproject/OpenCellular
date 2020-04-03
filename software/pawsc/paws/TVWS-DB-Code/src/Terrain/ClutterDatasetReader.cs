// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Terrain
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Drawing;
    using System.IO;
    using System.IO.Compression;
    using System.Linq;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Entities;

    /// <summary>This class reads ClutterDatasetReader, 1arc second interval data</summary>
    public class ClutterDatasetReader : IClutterDatasetReader
    {
        /// <summary>
        /// Initializes a new instance of the ClutterDatasetReader class.
        /// </summary>
        public ClutterDatasetReader()
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
        /// Gets or sets the terrain image.
        /// </summary>
        /// <value>The terrain image.</value>
        private static Bitmap TerrainImage { get; set; }
    
        /// <summary>
        /// Calculates the elevation.
        /// </summary>
        /// <param name="easting">The easting.</param>
        /// <param name="northing">The northing.</param>
        /// <returns>returns the clutter dataset value.</returns>
        public int CalculateClutter(double easting, double northing)
        {
            string logMethodName = string.Format("ClutterDatasetReader.CalculateElevation- Easting -> {0}, Northing -> {1}", easting, northing);

            // Begin Log transaction
            ////this.Logger.Log(TraceEventType.Information, LoggingMessageId.ElevationMessage, "Enter " + logMethodName);

            ////const int Nrows = 52000;
            ////const int NCols = 28000;
            const double CellSize = 25;
            const int UpperLeftEasting = 0;
            const int UpperLeftNorthing = 1300000;

            int clutterValue = 0;

            try
            {
                int x = (int)(Math.Abs(UpperLeftEasting - easting) / CellSize);
                int y = (int)(Math.Abs(UpperLeftNorthing - northing) / CellSize);

                if (TerrainImage == null)
                {
                    this.LoadTerrainImage();
                }

                var color = TerrainImage.GetPixel(x, y);
                clutterValue = color.G;
            }
            catch (Exception ex)
            {
                this.Logger.Log(TraceEventType.Error, LoggingMessageId.ElevationGlobUnsupportedTileId, ex.ToString());

                return clutterValue;
            }

            // End Log transaction
            ////this.Logger.Log(TraceEventType.Information, LoggingMessageId.ElevationMessage, "Exit " + logMethodName);

            return clutterValue;
        }

        /// <summary>
        /// Loads the terrain image.
        /// </summary>
        private void LoadTerrainImage()
        {
            string fileName = Utils.Configuration["ClutterDataFileName"];
            Stream dataStream = this.TerrainDalc.LoadTerrainFile(fileName, Utils.Configuration["landCoverDataDirectory"]);
            ////Stream dataStream = File.OpenRead(@"D:\Whitespace\TerrainData\lcm2007\lcm2007_25m_gb.zip");

            if (Path.GetExtension(fileName) == ".zip")
            {
                using (ZipArchive zipFile = new ZipArchive(dataStream, ZipArchiveMode.Read))
                {
                    var zipFileEntry = zipFile.Entries.FirstOrDefault(obj => obj.FullName.Contains("lcm2007_25m_gb.tif"));
                    BinaryReader rdr = new BinaryReader(zipFileEntry.Open());
                    MemoryStream stream = new MemoryStream(rdr.ReadBytes((int)zipFileEntry.Length));
                    TerrainImage = new Bitmap(stream);
                }
            }
            else if (Path.GetExtension(fileName) == ".tif")
            {
                TerrainImage = new Bitmap(dataStream);
            }
        }
    }
}
