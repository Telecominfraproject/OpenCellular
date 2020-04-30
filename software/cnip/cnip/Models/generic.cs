using System;
using System.Data;
using System.Diagnostics;
using System.IO;
using OSGeo.GDAL;
using static cnip.Models.gisext;
using static cnip.Models.pgsql;
using static cnip.Models.strext;

namespace cnip.Models
{
    public static class generic
    {
        public static int RandomNumber(int min, int max)
        {
            Random random = new Random(); return random.Next(min, max);

        }
        public static double DbmToWatt(double dbm)
        {
            return Math.Pow(10, dbm / 10) / 1000;
        }
        public static double WattToDbm(double watt)
        {
            return 10 * Math.Log10(watt * 1000);
        }
        public enum Extent
        {
            Center = 0,
            BottomLeft = 1,
            BottomRight = 2,
            TopLeft = 3,
            TopRight = 4
        }
        public static Coords GetExtent(
            string puid,
            string values,
            string columnname,
            string tableName,
            Extent extent)
        {
            Coords c = new Coords(0, 0);
            string sitesWhere = BuildWhere(values, columnname);
            foreach (DataRow row in GetDataTableFromQuery(
                "SELECT " +
                (extent == Extent.Center ?
                "(MAX(longitude)+MIN(longitude))/2 AS X, " +
                "(MAX(latitude)+MIN(latitude))/2 AS Y " :
                extent == Extent.BottomLeft ?
                "MIN(longitude) AS X, " +
                "MIN(latitude) AS Y " :
                extent == Extent.BottomRight ?
                "MAX(longitude) AS X, " +
                "MIN(latitude) AS Y " :
                extent == Extent.TopLeft ?
                "MIN(longitude) AS X, " +
                "MAX(latitude) AS Y " :
                "MAX(longitude) AS X, " +
                "MAX(latitude) AS Y "
                ) +
                "FROM " +
                tableName + " WHERE " +
                "uid='" + puid + "' AND " +
                sitesWhere + "GROUP BY uid;").Rows)
            {
                c.X = row[0].ToString().ToDouble();
                c.Y = row[1].ToString().ToDouble();
            }
            return c;
        }
        public static void StartProcess(string path, string args = "", bool wait = true)
        {
            Process process = new Process();
            process.StartInfo.FileName = path;
            process.StartInfo.UseShellExecute = false;
            process.StartInfo.CreateNoWindow = true;
            process.StartInfo.Arguments = args;
            process.Start();
            if (wait) process.WaitForExit();
            process.Close();
        }
        public static string GetElevationStringFromPointsArray(
            string pointsArray)
        {
            string retval = "";
            int loca = 0, locb = 0;
            double prevLon = 0.000000000001, prevLat = 0.000000000001;
            double longitude = 0.000000000000, latitude = 0.000000000000;
            double[] pGT = new double[6];
            string[] locations = pointsArray.Split('@');
            int col = 0, row = 0, lastElev = 0;
            int[] pRBB = new int[1];
            Dataset ds = Gdal.Open(GetRasterPath(-200, -200),
                Access.GA_ReadOnly);
            Band band = ds.GetRasterBand(1);
            CPLErr error;
            for (int i = 0; i < locations.Length; i++)
            {
                try
                {
                    longitude = locations[i].Split('&')[0].ToDouble();
                    latitude = locations[i].Split('&')[1].ToDouble();
                    if (latitude.ToString().Trim().Split('.')[0] !=
                        prevLat.ToString().Trim().Split('.')[0] &&
                       longitude.ToString().Trim().Split('.')[0] !=
                       prevLon.ToString().Trim().Split('.')[0])
                    {
                        prevLat = latitude; prevLon = longitude;
                        ds.Dispose();
                        ds = Gdal.Open(
                            GetRasterPath(longitude, latitude),
                            Access.GA_ReadOnly);
                        pGT = new double[6];
                        ds.GetGeoTransform(pGT);
                        band = ds.GetRasterBand(1);
                    }
                    col = Math.Abs((int)((longitude - pGT[0]) / pGT[1]));
                    row = Math.Abs((int)((pGT[3] - latitude) / -pGT[5]));
                    error =
                        band.ReadRaster(col, row, 1, 1, pRBB, 1, 1, 0, 0);
                    if (pRBB[0] < 0) { pRBB[0] = lastElev; }
                    lastElev = pRBB[0];
                    if (i == 0) { loca = pRBB[0]; }
                    if (i == locations.Length - 2) { locb = pRBB[0]; }
                    retval += locations[i] + "&" + pRBB[0].ToString() + "@";
                }
                catch (Exception)
                {
                    break;
                }
            }
            if (retval.Length > 0)
            {
                retval = loca.ToString() + "#" + locb.ToString() + "#" + retval.TrimEnd(1);
            }
            else
            {
                retval = "0#0#0&0&0&0";
            }
            band.Dispose();
            ds.Dispose();
            return retval;
        }
        public static double GetAverageElevationFromPointsArray(
            string pointsArray)
        {
            int loca = 0, locb = 0;
            double averageElevation = 0;
            double prevLon = 0.000000000001, prevLat = 0.000000000001;
            double longitude = 0.000000000000, latitude = 0.000000000000;
            double[] pGT = new double[6];
            string[] locations = pointsArray.Split('@');
            int col = 0, row = 0, lastElev = 0;
            int[] pRBB = new int[1];
            Dataset ds = Gdal.Open(GetRasterPath(-200, -200),
                Access.GA_ReadOnly);
            Band band = ds.GetRasterBand(1);
            CPLErr error;
            for (int i = 0; i < locations.Length; i++)
            {
                try
                {
                    longitude = locations[i].Split('&')[0].ToDouble();
                    latitude = locations[i].Split('&')[1].ToDouble();
                    if (latitude.ToString().Trim().Split('.')[0] !=
                        prevLat.ToString().Trim().Split('.')[0] &&
                       longitude.ToString().Trim().Split('.')[0] !=
                       prevLon.ToString().Trim().Split('.')[0])
                    {
                        prevLat = latitude; prevLon = longitude;
                        ds.Dispose();
                        ds = Gdal.Open(
                            GetRasterPath(longitude, latitude),
                            Access.GA_ReadOnly);
                        pGT = new double[6];
                        ds.GetGeoTransform(pGT);
                        band = ds.GetRasterBand(1);
                    }
                    col = Math.Abs((int)((longitude - pGT[0]) / pGT[1]));
                    row = Math.Abs((int)((pGT[3] - latitude) / -pGT[5]));
                    error =
                        band.ReadRaster(col, row, 1, 1, pRBB, 1, 1, 0, 0);
                    if (pRBB[0] < 0) { pRBB[0] = lastElev; }
                    lastElev = pRBB[0];
                    if (i == 0) { loca = pRBB[0]; }
                    if (i == locations.Length - 2) { locb = pRBB[0]; }
                    averageElevation = (averageElevation + pRBB[0]) / 2;
                }
                catch (Exception)
                {
                    break;
                }
            }
            band.Dispose();
            ds.Dispose();
            return averageElevation;
        }
        public static int GetElevationFromLonLat(
            double longitude, double latitude)
        {
            try
            {
                Dataset ds = Gdal.Open(
                    GetRasterPath(longitude, latitude),
                    Access.GA_ReadOnly);
                double[] pGT = new double[6];
                ds.GetGeoTransform(pGT);
                int col = Math.Abs((int)((longitude - pGT[0]) / pGT[1]));
                int row = Math.Abs((int)((pGT[3] - latitude) / -pGT[5]));
                Band band = ds.GetRasterBand(1);
                CPLErr error;
                // p read block byte
                int[] pRBB = new int[1];
                error =
                    band.ReadRaster(col, row, 1, 1, pRBB, 1, 1, 0, 0);
                if (pRBB[0] < 0) { pRBB[0] = 0; }
                band.Dispose();
                ds.Dispose();
                return pRBB[0];
            }
            catch (Exception)
            {
                return 0;
            }
        }
        public static string GetRasterPath(
            double longitude, double latitude)
        {
            string path = "dummy.tif";
            string lon = longitude.ToString().Trim().Split('.')[0];
            string lat = latitude.ToString().Trim().Split('.')[0];
            if (longitude != -200 && latitude != -200)
            {
                if (lat.StartsWith("-"))
                {
                    path = "S" +
                        (lat.Replace("-", "").ToInt() + 1).ToString("00");
                }
                else
                {
                    path = "N" + lat.ToInt().ToString("00");
                }
                if (lon.StartsWith("-"))
                {
                    path += "W" +
                        (lon.Replace("-", "").ToInt() + 1).ToString("000");
                }
                else
                {
                    path += "E" + lon.ToInt().ToString("000") + ".hgt";
                }
            }
            return @"C:/SPLAT/sdf-hd/" + path;
        }
        public static string GetElevationStringFromLineString(
            string lineString, int resolution = 100)
        {
            string pointsArray = ""; double unitLength = 0;
            foreach (DataRow row in GetDataTableFromQuery(
                "SELECT locations, dist FROM (" +
                "SELECT " +
                "ST_AsText(ST_LineSubstring(wkb_geometry, " +
                "length/" + resolution.ToString() + "*n/length," +
                "CASE WHEN length/" + resolution.ToString() + "*(n+1)<length " +
                "THEN length/" + resolution.ToString() + "*(n+1)/length " +
                "ELSE 1 END)) AS locations, " +
                "ST_Length(Geography(ST_LineSubstring(wkb_geometry, " +
                "length/" + resolution.ToString() + "*n/length," +
                "CASE WHEN length/" + resolution.ToString() + "*(n+1)<length " +
                "THEN length/" + resolution.ToString() + "*(n+1)/length " +
                "ELSE 1 END)),TRUE) As dist FROM " +
                "(SELECT " +
                "ST_LineMerge(ST_GeomFromText('" +
                lineString + "',4326)) AS wkb_geometry," +
                "ROUND(ST_Length(Geography(ST_GeomFromText('" +
                lineString + "',4326)),TRUE)) As length" +
                ")tmp " +
                "CROSS JOIN generate_series(0,10000) AS n " +
                "WHERE n*length/" + resolution.ToString() + "/length<1" +
                ")tmp WHERE dist>0;").Rows)
            {
                string[] locations = row[0].ToString().Replace(
                    "LINESTRING(", "").Replace(")", "").Split(',');
                string[] location1 = locations[0].Split(' ');
                string[] location2 = locations[1].Split(' ');
                if (pointsArray.Length == 0)
                {
                    pointsArray += location1[0] + "&" +
                        location1[1] + "&" + unitLength.ToString() + "@";
                    unitLength += row[1].ToString().ToDouble() / 1000;
                    pointsArray += location2[0] + '&' +
                        location2[1] + "&" + unitLength.ToString() + "@";
                }
                else
                {
                    unitLength += row[1].ToString().ToDouble() / 1000;
                    pointsArray += location2[0] + "&" +
                        location2[1] + "&" + unitLength.ToString() + "@";
                }                
            }
            pointsArray = pointsArray.TrimEnd(1);
            return GetElevationStringFromPointsArray(pointsArray);
        }
        public static double GetAverageElevationFromLineString(
            string lineString, int resolution = 100)
        {
            string pointsArray = "";
            foreach (DataRow row in GetDataTableFromQuery(
                "SELECT locations, dist FROM (" +
                "SELECT " +
                "ST_AsText(ST_LineSubstring(wkb_geometry, " +
                "length/" + resolution.ToString() + "*n/length," +
                "CASE WHEN length/" + resolution.ToString() + "*(n+1)<length " +
                "THEN length/" + resolution.ToString() + "*(n+1)/length " +
                "ELSE 1 END)) AS locations, " +
                "ST_Length(Geography(ST_LineSubstring(wkb_geometry, " +
                "length/" + resolution.ToString() + "*n/length," +
                "CASE WHEN length/" + resolution.ToString() + "*(n+1)<length " +
                "THEN length/" + resolution.ToString() + "*(n+1)/length " +
                "ELSE 1 END)),TRUE) As dist FROM " +
                "(SELECT " +
                "ST_LineMerge(ST_GeomFromText('" +
                lineString + "',4326)) AS wkb_geometry," +
                "ROUND(ST_Length(Geography(ST_GeomFromText('" +
                lineString + "',4326)),TRUE)) As length" +
                ")tmp " +
                "CROSS JOIN generate_series(0,10000) AS n " +
                "WHERE n*length/" + resolution.ToString() + "/length<1" +
                ")tmp WHERE dist>0;").Rows)
            {
                string[] locations = row[0].ToString().Replace(
                    "LINESTRING(", "").Replace(")", "").Split(',');
                string[] location1 = locations[0].Split(' ');
                string[] location2 = locations[1].Split(' ');
                if (pointsArray.Length == 0)
                {
                    pointsArray += location1[0] + "&" +
                        location1[1] + "@";
                    pointsArray += location2[0] + '&' +
                        location2[1] + "@";
                }
                else
                {
                    pointsArray += location2[0] + "&" +
                        location2[1] + "@";
                }
            }
            pointsArray = pointsArray.TrimEnd(1);
            return GetAverageElevationFromPointsArray(pointsArray);
        }
        public static double AverageElevationFromCluster(Coords center, double radiusMeters)
        {
            string pointsArray = "";
            foreach (DataRow point in GetDataTableFromQuery(
                "SELECT ST_AsText((ST_Dump(ST_GeneratePoints(" +
                "Geometry(ST_Buffer(Geography(ST_GeomFromText(" +
                "'POINT(" + center.X.ToString() + " " + center.Y.ToString() + ")'" +
                ",4326))," + radiusMeters.ToString() + ")),600))).geom);"
                ).Rows)
            {
                string[] location = point[0].ToString().
                    Replace("POINT(", "").Replace(")", "").Split(' ');
                pointsArray += location[0] + "&" + location[1] + "@";
            }
            pointsArray = pointsArray.TrimEnd(1);
            return GetAverageElevationFromPointsArray(pointsArray);
        }
        public static double AverageElevationFromCenter(Coords center, double radiusMeters)
        {
            Coords locA, locB;
            locA = Destination(center, 0, radiusMeters);
            locB = Destination(center, 180, radiusMeters);
            string lineString1 = "LINESTRING(" +
                locA.X.ToString() + " " + locA.Y.ToString() + "," +
                locB.X.ToString() + " " + locB.Y.ToString() + ")";
            locA = Destination(center, 90, radiusMeters);
            locB = Destination(center, 270, radiusMeters);
            string lineString2 = "LINESTRING(" +
                locA.X.ToString() + " " + locA.Y.ToString() + "," +
                locB.X.ToString() + " " + locB.Y.ToString() + ")";
            locA = Destination(center, 45, radiusMeters);
            locB = Destination(center, 225, radiusMeters);
            string lineString3 = "LINESTRING(" +
                locA.X.ToString() + " " + locA.Y.ToString() + "," +
                locB.X.ToString() + " " + locB.Y.ToString() + ")";
            locA = Destination(center, 135, radiusMeters);
            locB = Destination(center, 315, radiusMeters);
            string lineString4 = "LINESTRING(" +
                locA.X.ToString() + " " + locA.Y.ToString() + "," +
                locB.X.ToString() + " " + locB.Y.ToString() + ")";
            return
                (
                    (
                    GetAverageElevationFromLineString(lineString1) +
                    GetAverageElevationFromLineString(lineString2) +
                    GetAverageElevationFromLineString(lineString3) +
                    GetAverageElevationFromLineString(lineString4)
                    )
                 / 4
                );
        }
        public static void ClearTempPath(string tempfol)
        {
            try
            {
                Array.ForEach(Directory.GetFiles(tempfol),
                    delegate (string path) { File.Delete(path); });
            }
            catch (Exception) { }
        }
    }
}
