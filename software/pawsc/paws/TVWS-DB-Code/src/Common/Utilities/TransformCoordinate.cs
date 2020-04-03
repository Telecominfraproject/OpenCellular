// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common.Utilities
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.IO;
    using System.IO.Compression;
    using System.Linq;
    using System.Text;
    using Entities;
    using Utilities;

    /// <summary>
    /// Represents Class TransformCoordinate.
    /// </summary>
    public static class TransformCoordinate
    {
        /// <summary>The synchronize object</summary>
        private static object syncObject = new object();

        /// <summary>The initialized</summary>
        private static bool isInitialized = false;

        /// <summary>The grids</summary>
        private static Dictionary<int, List<double[]>> grids = new Dictionary<int, List<double[]>>();

        /// <summary>The OSG file grid</summary>
        private static List<double[]> osgFileGrid = new List<double[]>();

        /// <summary>The file keys</summary>
        private static Dictionary<int, string> fileKeys = new Dictionary<int, string>();

        /// <summary>The DX</summary>
        private static FortranDoubleArray dx;

        /// <summary>The DY</summary>
        private static FortranDoubleArray dy;

        /// <summary>The XMAX</summary>
        private static FortranDoubleArray xmaxs;

        /// <summary>The XMIN</summary>
        private static FortranDoubleArray xmins;

        /// <summary>The YMAX</summary>
        private static FortranDoubleArray ymaxs;

        /// <summary>The YMIN</summary>
        private static FortranDoubleArray ymins;

        /// <summary>The NC</summary>
        private static FortranDoubleArray nc;

        /// <summary>
        /// Initializes this instance.
        /// </summary>
        public static void Initialize()
        {
            lock (syncObject)
            {
                if (isInitialized)
                {
                    return;
                }

                const int INITIALSIZE = 14;
                dx = new FortranDoubleArray(INITIALSIZE);
                dy = new FortranDoubleArray(INITIALSIZE);
                xmaxs = new FortranDoubleArray(INITIALSIZE);
                xmins = new FortranDoubleArray(INITIALSIZE);
                ymaxs = new FortranDoubleArray(INITIALSIZE);
                ymins = new FortranDoubleArray(INITIALSIZE);
                nc = new FortranDoubleArray(INITIALSIZE);

                fileKeys.Add(1, "conus.las");
                fileKeys.Add(2, "conus.los");

                fileKeys.Add(3, "alaska.las");
                fileKeys.Add(4, "alaska.los");

                fileKeys.Add(5, "stlrnc.las");
                fileKeys.Add(6, "stlrnc.los");

                fileKeys.Add(7, "stgeorge.las");
                fileKeys.Add(8, "stgeorge.los");

                fileKeys.Add(9, "stpaul.las");
                fileKeys.Add(10, "stpaul.los");

                fileKeys.Add(11, "prvi.las");
                fileKeys.Add(12, "prvi.los");

                fileKeys.Add(13, "hawaii.las");
                fileKeys.Add(14, "hawaii.los");

                LoadNadFiles();

                LoadOSGFiles();

                isInitialized = true;
            }
        }

        /// <summary>
        /// To the na D83.
        /// </summary>
        /// <param name="sourceLocation">The source location in NAD 27 format.</param>
        /// <returns>returns Location.</returns>
        public static Location ToNAD83(Location sourceLocation)
        {
            if (!isInitialized)
            {
                Initialize();
            }

            // CHANGE THE LONGITUDE TO POSITIVE EAST FOR INTERPOLATION
            double xpt = sourceLocation.Longitude;
            double ypt = sourceLocation.Latitude;
            double ypt2 = 0.0;
            double xpt2 = 0.0;

            int irow = 0;
            int jcol = 0;
            double xgrid = 0.0;
            double ygrid = 0.0;
            double dlas = 0.0;
            double dlos = 0.0;

            var fileKey = 0;
            for (int i = 1; i <= fileKeys.Count; i++)
            {
                fileKey = i;

                // Check to see it the point is outside the area of the gridded data 
                if (xpt >= xmaxs[fileKey] || xpt <= xmins[fileKey] || ypt >= ymaxs[fileKey] || ypt <= ymins[fileKey])
                {
                    continue;
                }

                xgrid = ((xpt - xmins[fileKey]) / dx[fileKey]) + 1.0;
                ygrid = ((ypt - ymins[fileKey]) / dy[fileKey]) + 1.0;

                irow = (int)ygrid;
                jcol = (int)xgrid;

                break;
            }

            if (irow == 0 || jcol == 0)
            {
                return null;
            }

            double ay, bee, cee, dee, zee;
            var dataValues = grids[fileKey];
            try
            {
                double tee1 = dataValues[irow][jcol];
                double tee3 = dataValues[irow][jcol + 1];

                double tee2 = dataValues[irow + 1][jcol];
                double tee4 = dataValues[irow + 1][jcol + 1];

                COEFF(tee1, tee2, tee3, tee4, out ay, out bee, out cee, out dee);

                SURF(xgrid, ygrid, out zee, ay, bee, cee, dee, irow, jcol);

                dlas = zee;

                // increment for LOS file. every LOS file is at +1 position from same region LAS File.
                fileKey = fileKey + 1;

                dataValues = grids[fileKey];
                tee1 = dataValues[irow][jcol];
                tee3 = dataValues[irow][jcol + 1];

                tee2 = dataValues[irow + 1][jcol];
                tee4 = dataValues[irow + 1][jcol + 1];

                COEFF(tee1, tee2, tee3, tee4, out ay, out bee, out cee, out dee);

                SURF(xgrid, ygrid, out zee, ay, bee, cee, dee, irow, jcol);

                dlos = zee;

                ypt2 = ypt + (dlas / 3600.0);
                xpt2 = xpt - (dlos / 3600.0);

                return new Location(ypt2, xpt2);
            }
            catch (Exception)
            {
                return null;
            }
        }

        /// <summary>
        /// To the easting northing.
        /// </summary>
        /// <param name="latitude">The latitude.</param>
        /// <param name="longitude">The longitude.</param>
        /// <returns>returns OSGLocation.</returns>
        public static Tuple<double, double> ToEastingNorthing(double latitude, double longitude)
        {
            if (!isInitialized)
            {
                Initialize();
            }

            latitude = GeoCalculations.ToRad(latitude);
            longitude = GeoCalculations.ToRad(longitude);

            double cosLat = Math.Cos(latitude), sinLat = Math.Sin(latitude);

            ////airy 180
            ////double a = 6377563.39600, b = 6356256.909; // Airy 1830 major & minor semi-axes
            ////double f0 = 0.9996012717; // NatGrid scale factor on central meridian

            //////// grs 180
            double a = 6378137, b = 6356752.314; // GRS 180
            double f0 = 0.9996012717; // NatGrid scale factor on central meridian
            double e2 = 6.69437999e-3; // eccentricity squared
            double no = -100000; // northing & eastings of true origin, metres
            double e0 = 400000; // northing & eastings of true origin, metres

            double lat0 = GeoCalculations.ToRad(49.0);
            double lon0 = GeoCalculations.ToRad(-2.0); // NatGrid true origin

            double n = (a - b) / (a + b);
            double nu = (a * f0) / Math.Sqrt(1.0 - (e2 * Math.Pow(sinLat, 2))); // transverse radius of curvature
            double ro = (a * f0 * (1 - e2)) / Math.Pow(1 - (e2 * sinLat * sinLat), 1.5); // meridional radius of curvature

            double η2 = (nu / ro) - 1;
            double nsquare = n * n;
            double ncube = n * n * n;

            double ma = (1 + n + ((5.0 / 4.0) * nsquare) + ((5.0 / 4.0) * ncube)) * (latitude - lat0);
            double mb = ((3.0 * n) + (3.0 * nsquare) + ((21.0 / 8.0) * ncube)) * Math.Sin(latitude - lat0) * Math.Cos(latitude + lat0);
            double mc = (((15.0 / 8.0) * nsquare) + ((15.0 / 8.0) * ncube)) * Math.Sin(2 * (latitude - lat0)) * Math.Cos(2 * (latitude + lat0));
            double md = (35.0 / 24.0) * ncube * Math.Sin(3.0 * (latitude - lat0)) * Math.Cos(3.0 * (latitude + lat0));
            double m = b * f0 * (ma - mb + mc - md); // meridional arc

            double cos3lat = cosLat * cosLat * cosLat;
            double cos5lat = cos3lat * cosLat * cosLat;
            double tan2lat = Math.Tan(latitude) * Math.Tan(latitude);
            double tan4lat = tan2lat * tan2lat;

            double i = m + no;
            double ii = (nu / 2.0) * sinLat * cosLat;
            double iii = (nu / 24.0) * sinLat * cos3lat * (5 - tan2lat + (9 * η2));
            double iiia = (nu / 720.0) * sinLat * cos5lat * (61 - (58 * tan2lat) + tan4lat);
            double iv = nu * cosLat;
            double v = (nu / 6.0) * cos3lat * ((nu / ro) - tan2lat);
            double vi = (nu / 120.0) * cos5lat * (5 - (18 * tan2lat) + tan4lat + (14 * η2) - (58 * tan2lat * η2));

            double lon1 = longitude - lon0;
            double lon2 = lon1 * lon1, lon3 = lon2 * lon1, lon4 = lon3 * lon1, lon5 = lon4 * lon1, lon6 = lon5 * lon1;

            double northings = i + (ii * lon2) + (iii * lon4) + (iiia * lon6);
            double eastings = e0 + (iv * lon1) + (v * lon3) + (vi * lon5);

            ////var eastIndex = (int)eastings / 1000;
            ////var northIndex = (int)northings / 1000;
            ////var x0 = eastIndex * 1000;
            ////var y0 = northIndex * 1000;
            ////var shiftloc_se0sn0sg0 = eastIndex + (northIndex * 701) + 1;
            ////var shiftloc_se1sn1sg1 = (eastIndex + 1) + (northIndex * 701) + 1;
            ////var shiftloc_se2sn2sg2 = (eastIndex + 1) + ((northIndex + 1) * 701) + 1;
            ////var shiftloc_se3sn3sg3 = (eastIndex) + ((northIndex + 1) * 701) + 1;

            ////double[] shift_se0sn0sg0 = OSGFileGrid[shiftloc_se0sn0sg0 - 1];
            ////double[] shift_se1sn1sg1 = OSGFileGrid[shiftloc_se1sn1sg1 - 1];
            ////double[] shift_se2sn2sg2 = OSGFileGrid[shiftloc_se2sn2sg2 - 1];
            ////double[] shift_se3sn3sg3 = OSGFileGrid[shiftloc_se3sn3sg3 - 1];

            ////double dx = eastings - x0;
            ////double dy = northings - y0;
            ////double t = dx / 1000;
            ////double u = dy / 1000;

            ////double se = ((1 - t) * (1 - u) * shift_se0sn0sg0[3]) + (t * (1 - u) * shift_se1sn1sg1[3]) + (t * u * shift_se2sn2sg2[3]) + ((1 - t) * u * shift_se3sn3sg3[3]);
            ////double sn = ((1 - t) * (1 - u) * shift_se0sn0sg0[4]) + (t * (1 - u) * shift_se1sn1sg1[4]) + (t * u * shift_se2sn2sg2[4]) + ((1 - t) * u * shift_se3sn3sg3[4]);
            ////////double sg = ((1 - t) * (1 - u) * shift_se0sn0sg0[5]) + (t * (1 - u) * shift_se1sn1sg1[5]) + (t * u * shift_se2sn2sg2[5]) + ((1 - t) * u * shift_se3sn3sg3[5]);
            var shifts = CalculateShifts(eastings, northings);

            double trueEasting = eastings + shifts.se;
            double trueNorthing = northings + shifts.sn;

            return new Tuple<double, double>(trueEasting, trueNorthing);
        }

        /// <summary>
        /// To the latitude longitude.
        /// </summary>
        /// <param name="eastings">The easting.</param>
        /// <param name="northings">The northing.</param>
        /// <returns>returns Location.</returns>
        public static Location ToLatitudeLongitude(int eastings, int northings)
        {
            ////if (!IsInitialized)
            ////{
            ////    Initialize();
            ////}

            double easting = eastings;
            double northing = northings;
            var shifts = CalculateShifts(easting, northing);
            var initialEasting = easting - shifts.se;
            var initialNorthing = northing - shifts.sn;
            var continousShift = CalculateShifts(initialEasting, initialNorthing);
            var secondEasting = easting - continousShift.se;
            var secondNorthing = northing - continousShift.sn;
            var diff1 = Math.Abs(initialEasting - secondEasting);
            var diff2 = Math.Abs(initialNorthing - secondNorthing);
            while (diff1 > .0001 || diff2 > .0001)
            {
                initialEasting = secondEasting;
                initialNorthing = secondNorthing;
                continousShift = CalculateShifts(initialEasting, initialNorthing);
                secondEasting = easting - continousShift.se;
                secondNorthing = northing - continousShift.sn;
                diff1 = Math.Abs(secondEasting - initialEasting);
                diff2 = Math.Abs(secondNorthing - initialNorthing);
            }

            easting = secondEasting;
            northing = secondNorthing;

            ////airy 180
            ////const double a = 6377563.39600; // Airy 1830 major & minor semi-axes
            ////const double b = 6356256.909; // Airy 1830 major & minor semi-axes
            ////const double f0 = 0.9996012717; // NatGrid scale factor on central meridian
            ////double e2 = (Math.Pow(a, 2) - Math.Pow(b, 2)) / Math.Pow(a, 2); // eccentricity squared

            //// grs 180
            double a = 6378137, b = 6356752.314; // GRS 180
            double f0 = 0.9996012717; // NatGrid scale factor on central meridian
            double e2 = 6.69437999e-3; // eccentricity squared

            double lat0 = GeoCalculations.ToRad(49.0);
            double lon0 = GeoCalculations.ToRad(-2.0); // NatGrid true origin
            double n0 = -100000.0; // northing & eastings of true origin, metres
            double e0 = 400000.0; // northing & eastings of true origin, metres
            double n = (a - b) / (a + b);
            double nsquare = n * n;
            double ncube = n * n * n;

            Func<double, double> calculateMeridonalArc = (lat01) =>
            {
                double ma = (1 + n + ((5.0 / 4.0) * nsquare) + ((5.0 / 4.0) * ncube)) * (lat01 - lat0);
                double mb = ((3.0 * n) + (3.0 * nsquare) + ((21.0 / 8.0) * ncube)) * Math.Sin(lat01 - lat0) * Math.Cos(lat01 + lat0);
                double mc = (((15.0 / 8.0) * nsquare) + ((15.0 / 8.0) * ncube)) * Math.Sin(2 * (lat01 - lat0)) * Math.Cos(2 * (lat01 + lat0));
                double md = (35.0 / 24.0) * ncube * Math.Sin(3.0 * (lat01 - lat0)) * Math.Cos(3.0 * (lat01 + lat0));
                double m = b * f0 * (ma - mb + mc - md); // meridional arc

                return m;
            };

            double initialLat = ((northing - n0) / (a * f0)) + lat0;
            double m1 = calculateMeridonalArc(initialLat);
            double projection = Math.Abs(northing - n0 - m1);
            while (projection >= 0.001)
            {
                initialLat = ((northing - n0 - m1) / (a * f0)) + initialLat;
                m1 = calculateMeridonalArc(initialLat);
                projection = Math.Abs(northing - n0 - m1);
            }

            double cosLat = Math.Cos(initialLat), sinLat = Math.Sin(initialLat);
            double nu = (a * f0) / Math.Sqrt(1.0 - (e2 * Math.Pow(sinLat, 2.0))); // transverse radius of curvature
            double ro = (a * f0 * (1.0 - e2)) / Math.Pow(1.0 - (e2 * sinLat * sinLat), 1.5); // meridional radius of curvature
            double η2 = (nu / ro) - 1.0;
            double tanLat = Math.Tan(initialLat), tan2lat = Math.Pow(tanLat, 2), tan4lat = Math.Pow(tanLat, 4), tan6lat = Math.Pow(tanLat, 6);
            double seclat = 1.0 / cosLat;
            double nu3 = Math.Pow(nu, 3), nu5 = Math.Pow(nu, 5), nu7 = Math.Pow(nu, 7);

            double vii = tanLat / (2.0 * ro * nu);
            double viii = (tanLat / (24.0 * ro * nu3)) * (5.0 + (3.0 * tan2lat) + η2 - (9.0 * tan2lat * η2));
            double ix = (tanLat / (720.0 * ro * nu5)) * (61.0 + (90.0 * tan2lat) + (45.0 * tan4lat));
            double x = seclat / nu;
            double xi = (seclat / (6.0 * nu3)) * ((nu / ro) + (2.0 * tan2lat));
            double xii = (seclat / (120.0 * nu5)) * (5.0 + (28.0 * tan2lat) + (24.0 * tan4lat));
            double xiia = (seclat / (504.0 * nu7)) * (61.0 + (662.0 * tan2lat) + (1320.0 * tan4lat) + (720.0 * tan6lat));

            double lat1 = initialLat - (vii * Math.Pow(easting - e0, 2)) + (viii * Math.Pow(easting - e0, 4)) - (ix * Math.Pow(easting - e0, 6));
            double lon1 = lon0 + (x * (easting - e0)) - (xi * Math.Pow(easting - e0, 3)) + (xii * Math.Pow(easting - e0, 5)) - (xiia * Math.Pow(easting - e0, 7));

            double latitude = GeoCalculations.Rad2Deg(lat1);
            double longitude = GeoCalculations.Rad2Deg(lon1);

            return new Location(latitude, longitude);
        }

        /// <summary>
        /// Loads the files.
        /// </summary>
        private static void LoadNadFiles()
        {
            for (int fileKey = 1; fileKey <= fileKeys.Count; fileKey++)
            {
                var data = File.ReadAllBytes(Path.Combine(Utils.GetValidAssemblyPath(), @"TransformationData\nad\" + fileKeys[fileKey]));
                var dt1 = Encoding.ASCII.GetString(data.Take(56).ToArray());
                var dt2 = Encoding.ASCII.GetString(data.Take(64).ToArray());
                var ncols = BitConverter.ToInt32(data, 64);
                var nrows = BitConverter.ToInt32(data, 68);
                var nz1 = BitConverter.ToInt32(data, 72);
                var xmin = BitConverter.ToSingle(data, 76);
                var dx1 = BitConverter.ToSingle(data, 80);
                var ymin = BitConverter.ToSingle(data, 84);
                var dy1 = BitConverter.ToSingle(data, 88);
                var angle1 = BitConverter.ToSingle(data, 92);

                var actualCols = ncols + 1;
                var singleRowByteCount = (ncols + 1) * 4;

                var dataRows = new List<byte[]>();
                for (int i = 0; i < data.Length; i += singleRowByteCount)
                {
                    dataRows.Add(data.Skip(i).Take(singleRowByteCount).ToArray());
                }

                var dataValues = new List<double[]>();
                foreach (var item in dataRows)
                {
                    var curRowValues = new List<double>();
                    for (int i = 0; i < item.Length; i += 4)
                    {
                        curRowValues.Add(BitConverter.ToSingle(item, i));
                    }

                    dataValues.Add(curRowValues.ToArray());
                }

                grids.Add(fileKey, dataValues);
                xmins[fileKey] = xmin;
                ymins[fileKey] = ymin;
                xmaxs[fileKey] = xmin + ((ncols - 1) * dx1);
                ymaxs[fileKey] = ymin + ((nrows - 1) * dy1);
                dx[fileKey] = Math.Abs(dx1);
                dy[fileKey] = Math.Abs(dy1);
                nc[fileKey] = ncols;
            }
        }

        /// <summary>
        /// Loads the OSG files.
        /// </summary>
        private static void LoadOSGFiles()
        {
            using (var fileStream = File.OpenRead(Path.Combine(Utils.GetValidAssemblyPath(), "TransformationData\\OSG\\OSG_DATA.zip")))
            {
                ZipArchive zipFile = new ZipArchive(fileStream, ZipArchiveMode.Read);
                var osgStream = zipFile.Entries[0].Open();

                string dataLine = string.Empty;
                using (TextReader txtRdr = new StreamReader(osgStream))
                {
                    while ((dataLine = txtRdr.ReadLine()) != null)
                    {
                        var splitData = dataLine.Split(',');
                        double[] rowdata = new double[splitData.Length];
                        for (int k = 0; k < splitData.Length; k++)
                        {
                            rowdata[k] = Convert.ToDouble(splitData[k]);
                        }

                        osgFileGrid.Add(rowdata);
                    }
                }
            }
        }

        /// <summary>
        /// Surfs the specified XGRID.
        /// </summary>
        /// <param name="xgrid">The XGRID.</param>
        /// <param name="ygrid">The YGRID.</param>
        /// <param name="zee">The ZEE.</param>
        /// <param name="ay">The AY.</param>
        /// <param name="bee">The BEE.</param>
        /// <param name="cee">The CEE.</param>
        /// <param name="dee">The DEE.</param>
        /// <param name="irow">The IROW.</param>
        /// <param name="jcol">The JCOL.</param>
        private static void SURF(double xgrid, double ygrid, out double zee, double ay, double bee, double cee, double dee, int irow, int jcol)
        {
            double zee1, zee2, zee3, zee4;
            zee1 = ay;
            zee2 = bee * (xgrid - jcol);
            zee3 = cee * (ygrid - irow);
            zee4 = dee * (xgrid - jcol) * (ygrid - irow);
            zee = zee1 + zee2 + zee3 + zee4;
        }

        /// <summary>
        /// Calculate COEFF
        /// </summary>
        /// <param name="tee1">The TEE1.</param>
        /// <param name="tee2">The TEE2.</param>
        /// <param name="tee3">The TEE3.</param>
        /// <param name="tee4">The TEE4.</param>
        /// <param name="ay">The AY.</param>
        /// <param name="bee">The BEE.</param>
        /// <param name="cee">The CEE.</param>
        /// <param name="dee">The DEE.</param>
        private static void COEFF(double tee1, double tee2, double tee3, double tee4, out double ay, out double bee, out double cee, out double dee)
        {
            ay = tee1;
            bee = tee3 - tee1;
            cee = tee2 - tee1;
            dee = tee4 - tee3 - tee2 + tee1;
        }

        /// <summary>
        /// Calculates the shifts.
        /// </summary>
        /// <param name="easting">The easting.</param>
        /// <param name="northing">The northing.</param>
        /// <returns>returns dynamic.</returns>
        private static dynamic CalculateShifts(double easting, double northing)
        {
            var eastIndex = (int)easting / 1000;
            var northIndex = (int)northing / 1000;
            var x0 = eastIndex * 1000;
            var y0 = northIndex * 1000;
            var shiftloc_se0sn0sg0 = eastIndex + (northIndex * 701) + 1;
            var shiftloc_se1sn1sg1 = (eastIndex + 1) + (northIndex * 701) + 1;
            var shiftloc_se2sn2sg2 = (eastIndex + 1) + ((northIndex + 1) * 701) + 1;
            var shiftloc_se3sn3sg3 = eastIndex + ((northIndex + 1) * 701) + 1;

            double[] shift_se0sn0sg0 = osgFileGrid[shiftloc_se0sn0sg0 - 1];
            double[] shift_se1sn1sg1 = osgFileGrid[shiftloc_se1sn1sg1 - 1];
            double[] shift_se2sn2sg2 = osgFileGrid[shiftloc_se2sn2sg2 - 1];
            double[] shift_se3sn3sg3 = osgFileGrid[shiftloc_se3sn3sg3 - 1];

            double dx = easting - x0;
            double dy = northing - y0;
            double t = dx / 1000;
            double u = dy / 1000;

            double se = ((1 - t) * (1 - u) * shift_se0sn0sg0[3]) + (t * (1 - u) * shift_se1sn1sg1[3]) + (t * u * shift_se2sn2sg2[3]) + ((1 - t) * u * shift_se3sn3sg3[3]);
            double sn = ((1 - t) * (1 - u) * shift_se0sn0sg0[4]) + (t * (1 - u) * shift_se1sn1sg1[4]) + (t * u * shift_se2sn2sg2[4]) + ((1 - t) * u * shift_se3sn3sg3[4]);

            return new { se = se, sn = sn };
        }
    }
}
