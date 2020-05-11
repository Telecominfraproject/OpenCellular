using System;
using System.Data;
using cnip.Models;
using static cnip.Models.pgsql;
using static cnip.Models.generic;
using static cnip.Models.settings;
using static cnip.Models.rfantenna;
using static cnip.Models.site;

namespace forecast.Models
{
    public static class Sites
    {
        public static string PredictSites(string puid, string polygonid, string technology)
        {

            foreach (DataRow row in GetDataTableFromQuery(
                "SELECT ta.siteid FROM " +
                "pusersnetwork ta, puserspolygons tb WHERE " +
                "ta.uid='" + puid + "' AND tb.polygonid='" + polygonid + "' AND " +
                "ST_Intersects(ta.wkb_geometry,tb.wkb_geometry);").Rows)

            {
                string siteid = row[0].ToString();
                ExecuteNonQuery("DELETE FROM puserslinks WHERE uid='" +
                    puid + "' AND (siteida='" + siteid +
                    "' OR siteidb='" + siteid + "');");
                ExecuteNonQuery("DELETE FROM pusersnetwork WHERE uid='" +
                    puid + "' AND siteid='" + siteid + "';");
            }

            Settings settings = new Settings(puid);
            double cellRadiusMeters = settings.Rp_CellRadius.ToDouble() * 1000;

            string siteCount = GetDataTableFromQuery(
                "SELECT Least(Greatest(Round(ST_Area(wkb_geometry,TRUE)/(" +
                cellRadiusMeters + "*" + cellRadiusMeters + ")/1.00)*1.00,1),50) " +
                "FROM puserspolygons WHERE polygonid='" + polygonid +
                "' AND uid='" + puid + "';").Rows[0][0].ToString();

            double polygonLength = Math.Sqrt(GetDataTableFromQuery(
                "SELECT Round(ST_Area(wkb_geometry,TRUE)/1.00)*1.00 " +
                "FROM puserspolygons WHERE polygonid='" + polygonid +
                "' AND uid='" + puid + "';").Rows[0][0].ToString().ToDouble());

            string sql = "DROP TABLE IF EXISTS s" + puid + "poly;";
            sql += "DROP TABLE IF EXISTS s" + puid + "poly_pts;";
            sql += "DROP TABLE IF EXISTS s" + puid + "poly_pts_clus;";
            sql += "DROP TABLE IF EXISTS s" + puid + "poly_centers;";
            sql += "CREATE TABLE s" + puid + "poly AS " +
                "SELECT * FROM puserspolygons WHERE " +
                "polygonid='" + polygonid + "' AND uid='" + puid + "';";
            sql += "CREATE TABLE s" + puid + "poly_pts AS " +
                "SELECT (ST_Dump(ST_GeneratePoints(wkb_geometry," +
                siteCount + "*600))).geom AS wkb_geometry FROM s" + puid + "poly;";
            sql += "CREATE TABLE s" + puid + "poly_pts_clus AS " +
                "SELECT wkb_geometry, ST_ClusterKmeans(wkb_geometry," +
                siteCount + ") OVER () AS clusterid FROM s" + puid + "poly_pts;";
            sql += "CREATE TABLE s" + puid + "poly_centers AS " +
                "SELECT clusterid, ST_Centroid(ST_collect(wkb_geometry)) AS " +
                "wkb_geometry FROM s" + puid + "poly_pts_clus GROUP BY clusterid;";
            ExecuteNonQuery(sql);

            int baseid = 1;
            foreach (DataRow row in GetDataTableFromQuery(
                "SELECT MAX(CAST(siteid AS INTEGER))+1 FROM " +
                "pusersnetwork WHERE uid='" + puid + "' GROUP BY uid").Rows)
            { baseid = row[0].ToString().ToInt(); }

            RFAntenna rfAntenna = GetRFAntennaParameters(settings.Rp_AntennaModel);
            string sites = "";
            string clusterids = "";
            foreach (DataRow row in GetDataTableFromQuery(
                "SELECT ST_AsText(wkb_geometry), clusterid FROM " +
                "s" + puid + "poly_centers;").Rows)
            {
                string[] location = row[0].ToString().
                    Replace("POINT(", "").Replace(")", "").Split(' ');
                ExecuteNonQuery("INSERT INTO pusersnetwork (" +
                    "uid, siteid, sitename, " +
                    "longitude, latitude, height, " +
                    "region, country, city, district, province, address, comments, " +
                    "technology, band, bandwidth, " +
                    "cellid, lac, " +
                    "rfcn, rfid, dlfrequency, ulfrequency, " +
                    "rfpower, hba, azimuth, antennamodel, antennatype, " +
                    "polarization, vbeamwidth, hbeamwidth, downtilt, antennagain, " +
                    "feederloss, wkb_geometry) VALUES " +
                    "('" + puid + "','" + (baseid).ToString() + "','" +
                    "new site " + (baseid).ToString() + "',"
                    + location[0] + "," + location[1] + ",'','','','','','','','','" +
                    technology + "','" +
                    (technology == "2G" ?
                    settings.Rp_GSMBand : settings.Rp_LTEBand) + "','" +
                    (technology == "2G" ?
                    settings.Rp_GSMBandwidth : settings.Rp_LTEBandwidth) + "','" +
                    (10000 + baseid).ToString() + "','" + puid + "','','','','','','','0','" +
                    rfAntenna.AntennaModel + "','" + rfAntenna.AntennaType + "','" +
                    rfAntenna.Polarization + "','" + rfAntenna.VBeamWidth + "','" +
                    rfAntenna.HBeamWidth + "','" + rfAntenna.DownTilt + "','" +
                    rfAntenna.AntennaGain + "','0'," +
                    "ST_GeomFromText('POINT(" + location[0] + " " + location[1] + ")',4326));");
                sites += (baseid).ToString() + ","; baseid++;
                clusterids += row[1].ToString() + ",";
            }
            sites = sites.TrimEnd(1);
            clusterids = clusterids.TrimEnd(1);
            PredictHeightAndPower(puid, sites, clusterids, polygonLength);
            sql = "DROP TABLE IF EXISTS s" + puid + "poly;";
            sql += "DROP TABLE IF EXISTS s" + puid + "poly_pts;";
            sql += "DROP TABLE IF EXISTS s" + puid + "poly_pts_clus;";
            sql += "DROP TABLE IF EXISTS s" + puid + "poly_centers;";
            ExecuteNonQuery(sql);
            return sites;
        }
        public static void PredictHeightAndPower(string puid, string sites, string clusterids = "", double polygonLengthMeters = 0)
        {
            Settings settings = new Settings(puid);
            double cellRadiusMeters = settings.Rp_CellRadius.ToDouble() * 1000;

            if (clusterids.Length > 0)
            {
                if (polygonLengthMeters < cellRadiusMeters)
                {
                    cellRadiusMeters = polygonLengthMeters;
                }
            }

            int cluster = 0;
            foreach (string siteid in sites.Split(','))
            {
                Site site = new Site(puid, siteid);
                if (site.RfPower == 0 || site.HBA == " m " || site.Height == "")
                {
                    /* average cluster elevation */
                    double averageClusterElevation = 0;
                    if (clusterids.Length > 0)
                    {
                        string clusterid = clusterids.Split(',')[cluster]; cluster++;
                        averageClusterElevation = AverageElevationFromCluster(puid, clusterid);
                    }
                    else
                    {
                        averageClusterElevation = generic.AverageElevationFromCluster(
                            site.Location, cellRadiusMeters);
                    }
                    /* average coverage elevation fcc method */
                    double averageCircularElevation =
                        AverageElevationFromCenter(site.Location, cellRadiusMeters);
                    /* average elevation */
                    double averageElevation = (averageClusterElevation + averageCircularElevation) / 2;
                    /* site elevation  */
                    double siteElevation = GetElevationFromLonLat(site.Location.X, site.Location.Y);
                    /* 5 meter clutter for villages, will be implemented from myClutter in future */
                    double averageClutterHeight = 5;
                    /* los height meters = receiver height */
                    double lineOfSightHeight = settings.Pl_ReceiverHeight.ToDouble();
                    /* transmitter height  */
                    double transmitterHeight = (averageElevation > siteElevation ?
                        (averageElevation - siteElevation) : 0)
                        + Math.Max(lineOfSightHeight, averageClutterHeight) + 1;
                    /* Tx = 20Log(d) + 20Log(f) + 32.44 + Rxs - Txg - Rxg + losses + fadeMargin */
                    double transmitterPowerWatt = Math.Ceiling(DbmToWatt(
                        20 * Math.Log10(cellRadiusMeters / 1000)
                        + 20 * Math.Log10(site.Technology == "2G" ? 947 : 942) /* f center 947 for gsm 900 band */
                        + 32.44
                        + settings.Pl_ReceiverSensitivity.ToDouble() /* Rx Sensitivity */
                        - site.AntennaGain /* antennagain */
                        - settings.Pl_ReceiverGain.ToDouble() /* Rx Gain */
                        + site.FeederLoss /* feeder losses etc */
                        + 20 /* fadeMargin 20dB to 30dB */));
                    ExecuteNonQuery("UPDATE pusersnetwork SET " +
                       "height='" + (site.Height.ToDouble() < transmitterHeight ?
                       transmitterHeight.ToString() : site.Height) + "', " +
                       "hba='" + transmitterHeight.ToString() + "', " +
                       "rfpower='" + transmitterPowerWatt.ToString() + "' " +
                       "WHERE siteid='" + siteid + "' AND uid='" + puid + "';");
                }
            }
        }
        private static double AverageElevationFromCluster(string puid, string clusterid)
        {
            string pointsArray = "";
            foreach (DataRow point in GetDataTableFromQuery(
                "SELECT ST_AsText(wkb_geometry) FROM " +
                "s" + puid + "poly_pts_clus WHERE " +
                "clusterid=" + clusterid + ";").Rows)
            {
                string[] location = point[0].ToString().
                    Replace("POINT(", "").Replace(")", "").Split(' ');
                pointsArray += location[0] + "&" + location[1] + "@";
            }
            pointsArray = pointsArray.TrimEnd(1);
            return GetAverageElevationFromPointsArray(pointsArray);
        }
    }
}
