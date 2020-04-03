// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common
{
    using System;
    using System.Collections.Generic;
    using System.Threading.Tasks;
    using Microsoft.Whitespace.Entities;
    using Microsoft.Whitespace.Common.Utilities;
    using System.Text;
    using Microsoft.Practices.Unity;
    using System.Diagnostics;

    /// <summary>
    /// Helper class to get portal contours from different entities
    /// </summary>
    public static class PortalContourHelper
    {
        static object rawContourLock = new object();
        static object portalContourLock = new object();

        /// <summary>Gets or sets the sync logger.</summary>
        /// <value>The sync logger.</value>        
        private static ILogger AzureLogger { get; set; }

        static PortalContourHelper()
        {
            IUnityContainer container = Utils.Configuration.CurrentContainer;
            AzureLogger = container.Resolve<ILogger>();
        }

        /// <summary>
        /// Get list of PortalContours from list of CDBSTvEngData
        /// </summary>
        /// <param name="cdbsData">list of CDBSTvEngData</param>
        /// <param name="incumbentType">incumbent type</param>
        /// <returns>list of portal contours</returns>
        public static List<PortalContour> GetContoursFromTVStations(List<CDBSTvEngData> cdbsData, IncumbentType incumbentType)
        {
            List<PortalContour> sourceContours = new List<PortalContour>();
            object lockObject = new object();
            int type = (int)incumbentType;

            Parallel.ForEach(cdbsData, cdbsTvEngData =>
            {
                try
                {
                    PortalContour portalContour = new PortalContour();
                    portalContour.CallSign = cdbsTvEngData.CallSign;
                    portalContour.Channel = cdbsTvEngData.Channel;
                    portalContour.Type = type;
                    portalContour.Latitude = cdbsTvEngData.Latitude;
                    portalContour.Longitude = cdbsTvEngData.Longitude;
                    portalContour.RowKey = (int)incumbentType + "-" + cdbsTvEngData.RowKey;
                    portalContour.PartitionKey = "RGN1-" + cdbsTvEngData.Channel.ToString();

                    if (incumbentType == IncumbentType.TV_TRANSLATOR)
                    {
                        portalContour.ParentLatitude = cdbsTvEngData.ParentLatitude;
                        portalContour.ParentLongitude = cdbsTvEngData.ParentLongitude;
                        portalContour.Contour = GetContourPoints(portalContour);
                    }
                    else
                    {
                        portalContour.Contour = GetContourPoints(cdbsTvEngData.Contour);
                    }

                    lock (lockObject)
                    {
                        sourceContours.Add(portalContour);
                    }
                }
                catch (Exception ex)
                {
                    AzureLogger.Log(TraceEventType.Error, LoggingMessageId.RegistrationSyncManagerGenericMessage, "Error while processing " + incumbentType + " with latitude:" + cdbsTvEngData.Latitude + " longitude: " + cdbsTvEngData.Longitude + " " + ex.ToString());
                }
            });

            return sourceContours;
        }

        /// <summary>
        /// Get list of PortalContours from list of MVPDRegistration
        /// </summary>
        /// <param name="cdbsData">list of MVPDRegistration</param>
        /// <param name="incumbentType">incumbent type</param>
        /// <returns>list of portal contours</returns>
        public static List<PortalContour> GetContoursFromMvpd(List<MVPDRegistration> registrations)
        {
            List<PortalContour> sourceContours = new List<PortalContour>();
            object lockObject = new object();
            int type = (int)IncumbentType.MVPD;

            Parallel.ForEach(registrations, mvpdRegistration =>
            {
                try
                {
                    PortalContour portalContour = new PortalContour();
                    portalContour.Type = type;
                    portalContour.Latitude = mvpdRegistration.Latitude;
                    portalContour.Longitude = mvpdRegistration.Longitude;
                    portalContour.ParentLatitude = mvpdRegistration.TxLatitude;
                    portalContour.ParentLongitude = mvpdRegistration.TxLongitude;
                    portalContour.RowKey = type + "-" + mvpdRegistration.RowKey;

                    if (!string.IsNullOrEmpty(mvpdRegistration.MVPDChannel))
                    {
                        TvSpectrum spectrum = JsonSerialization.DeserializeString<TvSpectrum>(mvpdRegistration.MVPDChannel);
                        portalContour.CallSign = spectrum.CallSign;
                        portalContour.Channel = Convert.ToInt16(spectrum.Channel);
                        portalContour.PartitionKey = "RGN1-" + portalContour.Channel.ToString();
                        portalContour.Contour = GetContourPoints(portalContour);

                        lock (lockObject)
                        {
                            sourceContours.Add(portalContour);
                        }
                    }
                }
                catch (Exception ex)
                {
                    AzureLogger.Log(TraceEventType.Error, LoggingMessageId.RegistrationSyncManagerGenericMessage, "Error while processing MVPD with latitude:" + mvpdRegistration.Latitude + " longitude: " + mvpdRegistration.Longitude + " " + ex.ToString());
                }
            });

            return sourceContours;
        }

        /// <summary>
        /// Get contours data from raw contours info
        /// </summary>
        /// <param name="rawContour">raw contours info</param>
        /// <returns>contour data</returns>
        private static string GetContourPoints(string rawContour)
        {
            lock (rawContourLock)
            {
                List<Position> contourPoints = new List<Position>();
                StringBuilder contourBuilder = new StringBuilder();

                if (!string.IsNullOrWhiteSpace(rawContour))
                {
                    Contour contour = JsonSerialization.DeserializeString<Contour>(rawContour);

                    if (contour.ContourPoints != null)
                    {
                        foreach (Location location in contour.ContourPoints)
                        {
                            contourBuilder.Append(location.Latitude);
                            contourBuilder.Append(" ");
                            contourBuilder.Append(location.Longitude);
                            contourBuilder.Append(" ");
                        }
                    }
                }

                return contourBuilder.ToString().Trim();
            }
        }

        /// <summary>
        /// Calculate contours from given incumbent and parent incumbent locations
        /// </summary>
        /// <param name="sourceContour">portal contours</param>
        /// <returns>contours data</returns>
        private static string GetContourPoints(PortalContour sourceContour)
        {
            lock (portalContourLock)
            {
                Location stationLocation = new Location(sourceContour.Latitude, sourceContour.Longitude);
                Distance sameChannelKeyHoleDistance = new Distance(80, DistanceUnit.KM); // 80 km (outer keyhole)
                Distance sameChannelDistance = new Distance(8, DistanceUnit.KM); // 8 km (inner keyhole)
                Location parentLocation = new Location(sourceContour.ParentLatitude, sourceContour.ParentLongitude);

                var stationToParentTxBearing = GeoCalculations.CalculateBearing(stationLocation, parentLocation);

                // keyhole calculations
                var keyHoleArcStarting = stationToParentTxBearing - 30;
                var keyHoleArcEnding = stationToParentTxBearing + 30;

                if (keyHoleArcStarting > 360)
                {
                    keyHoleArcStarting = keyHoleArcStarting - 360;
                }

                if (keyHoleArcEnding > 360)
                {
                    keyHoleArcEnding = keyHoleArcEnding - 360;
                }

                if (keyHoleArcStarting < 0)
                {
                    keyHoleArcStarting = keyHoleArcStarting + 360;
                }

                if (keyHoleArcEnding < 0)
                {
                    keyHoleArcEnding = keyHoleArcEnding + 360;
                }

                List<Location> contourPoints = CalculateRadialContour(stationLocation, sameChannelDistance);
                List<Location> keyHolePoints = CalculateRadialContour(stationLocation, sameChannelKeyHoleDistance);

                if (keyHoleArcStarting < keyHoleArcEnding)
                {
                    for (int i = (int)keyHoleArcStarting; i < (int)keyHoleArcEnding; i++)
                    {
                        contourPoints[i] = keyHolePoints[i];
                    }
                }
                else
                {
                    for (int i = (int)keyHoleArcStarting; i <= 360; i++)
                    {
                        contourPoints[i] = keyHolePoints[i];
                    }
                    for (int i = (int)0; i <= (int)keyHoleArcEnding; i++)
                    {
                        contourPoints[i] = keyHolePoints[i];
                    }
                }

                StringBuilder contourBuilder = new StringBuilder();
                foreach (Location point in contourPoints)
                {
                    contourBuilder.Append(point.Latitude);
                    contourBuilder.Append(" ");
                    contourBuilder.Append(point.Longitude);
                    contourBuilder.Append(" ");
                }

                return contourBuilder.ToString().Trim();
            }
        }

        /// <summary>
        /// Calculates the radial contour.
        /// </summary>
        /// <param name="startLocation">The start location.</param>
        /// <param name="distance">The distance.</param>
        /// <returns>returns List{Location}.</returns>
        private static List<Location> CalculateRadialContour(Location startLocation, Distance distance)
        {
            // rotating azimuth at 1 degree intervals
            List<Location> deltaLocations = new List<Location>();

            for (int azimuth = 0; azimuth <= 360; azimuth++)
            {
                var deltaCoord = GeoCalculations.GetLocationTowardsBearing(startLocation, distance, azimuth);
                deltaLocations.Add(deltaCoord);
            }

            return deltaLocations;
        }
    }
}
