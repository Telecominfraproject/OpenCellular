using System;
using System.Data;
using System.IO;
using System.Web;
using System.Web.UI;
using System.Web.Services;
using cnip.Models;
using static cnip.Models.gdalext;
using static cnip.Models.gisext;
using static cnip.Models.generic;
using static cnip.Models.pgsql;
using static cnip.Models.strext;

namespace cnip.pages
{
    public partial class webui : Page
    {
        //protected void Page_PreRender(object sender, EventArgs e)
        //{
        //    if (Request.IsSecureConnection == false /*&& !Request.Url.Host.Contains("localhost")*/)
        //    {
        //        Response.Redirect(Request.Url.AbsoluteUri.Replace("http://", "https://"));
        //    }
        //}
        /***************************************************************/
        /**************************Page Load**************************/
        /***************************************************************/
        protected void Page_Load(object sender, EventArgs e)
        {
            IsPukey();
            InitializeUser();
            GetNetworkFromDB();
            GetPolygonsFromDB();
            GetLinksFromDB();
            GetNotesFromDB();
            GetResultsFromDB();
            GetPublicNetworkFromDB();
            GetPublicNotesFromDB();
        }
        [WebMethod]
        public static void GetVectorsFromDB()
        {
            GetNetworkFromDB();
            GetPolygonsFromDB();
            GetLinksFromDB();
            GetNotesFromDB();
            GetResultsFromDB();
        }
        [WebMethod]
        public static string GetElevationStringFromLineString(string lineString)
        {
            return generic.GetElevationStringFromLineString(lineString);
        }
        [WebMethod]
        public static int GetElevationFromLonLat(double longitude, double latitude)
        {
            return generic.GetElevationFromLonLat(longitude, latitude);
        }
        [WebMethod]
        public static double Distance(double longitude1, double latitude1, double longitude2, double latitude2)
        {
            return pgsql.Distance(new Coords(longitude1, latitude1), new Coords(longitude2, latitude2));
        }
        /***************************************************************/
        /**************************User Key Validation**************************/
        /***************************************************************/
        public void IsPukey()
        {
            string puid = (string)HttpContext.Current.Session["uid"];
            string pukey = (string)HttpContext.Current.Session["ukey"];
            if (Is(puid) && Is(pukey))
            {
                DataTable dt = GetDataTableFromQuery(
                    "SELECT ukey FROM pusers WHERE uid = '" + puid + "';");
                if (dt.Rows.Count > 0)
                {
                    if ((string)dt.Rows[0][0] != pukey)
                    {
                        Response.Redirect("~/Glavni/Index"); return;
                    }
                }
                else
                {
                    Response.Redirect("~/Glavni/Index"); return;
                }
            }
            else
            {
                Response.Redirect("~/Glavni/Index"); return;
            }
        }
        public static bool _IsPukey()
        {
            string puid = (string)HttpContext.Current.Session["uid"];
            string pukey = (string)HttpContext.Current.Session["ukey"];
            bool test = false;
            if (Is(puid) && Is(pukey))
            {
                DataTable dt = GetDataTableFromQuery(
                    "SELECT ukey FROM pusers WHERE uid = '" + puid + "';");
                if (dt.Rows.Count > 0)
                {
                    if ((string)dt.Rows[0][0] == pukey)
                    {
                        test = true;
                    }
                }
            }
            return test;
        }
        public static bool Is(string test)
        {
            return (!(test is null) && test != "") ? true : false;
        }
        /***************************************************************/
        /**************************Result Methods**************************/
        /***************************************************************/
        [WebMethod]
        public static void GetResultsFromDB()
        {
            if (_IsPukey())
            {
                string puid = (string)HttpContext.Current.Session["uid"];
                string vectorLayers = GetVectorLayersPath();
                if (File.Exists(vectorLayers + "results.geojson"))
                {
                    File.Delete(vectorLayers + "results.geojson");
                }
                var options = new[] {
                    "-f", "GEOJSON",
                    "-sql", "SELECT " +
                    "resultid, resultname, resultstring, wkb_geometry " +
                    "FROM pusersresults WHERE uid ='" + puid + "' AND resulttype='coverage';"};
                bool test = Pgsql2Json(
                    "PG:dbname=cnip host=localhost user=postgres password=root port=5432",
                    vectorLayers + "results.geojson", options);
                if (!File.Exists(vectorLayers + "results.geojson"))
                {
                    File.WriteAllText(vectorLayers + "results.geojson",
                        "{\"type\":\"FeatureCollection\",\"features\":[]}");
                }
            }
        }
        [WebMethod]
        public static string GetNewResultId()
        {
            string puid = (string)HttpContext.Current.Session["uid"];
            DataTable ni = GetDataTableFromQuery("SELECT MAX(CAST(resultid AS INTEGER))+1 FROM " +
                "pusersresults WHERE uid = '" + puid + "' GROUP BY uid");
            int newid = 1; if (ni.Rows.Count > 0) { newid = ni.Rows[0][0].ToString().ToInt(); }
            return newid.ToString();
        }
        [WebMethod]
        public static string GetResultString(string resultid)
        {
            string resultstring = "";
            string puid = (string)HttpContext.Current.Session["uid"];
            DataTable dt = GetDataTableFromQuery("SELECT resultstring FROM pusersresults WHERE " +
                "uid='" + puid + "' AND resultid='" + resultid + "';");
            if (dt.Rows.Count > 0) { resultstring = dt.Rows[0][0].ToString(); }
            return resultstring;
        }
        [WebMethod]
        public static void DeleteResult(string resultid, string pngs)
        {
            string puid = (string)HttpContext.Current.Session["uid"];
            ExecuteNonQuery("DELETE FROM pusersresults WHERE " +
                "uid='" + puid + "' AND resultid='" + resultid + "';");
            foreach (string png in pngs.Split(','))
            {
                try { File.Delete(GetResultsPath() + png); }
                catch (Exception) { }
                finally { }
            }
        }
        [WebMethod]
        public static string GetLinksResult()
        {
            string msg = "";
            string puid = (string)HttpContext.Current.Session["uid"];
            DataTable dt = GetDataTableFromQuery("SELECT resultstring FROM pusersresults WHERE " +
                "uid='" + puid + "' AND resultid='0' AND resulttype='links';");
            if (dt.Rows.Count > 0) { msg = dt.Rows[0][0].ToString(); }
            return msg;
        }
        /***************************************************************/
        /**************************User Initialization And Paths **************************/
        /***************************************************************/
        public static string GetVectorLayersPath()
        {
            string puid = (string)HttpContext.Current.Session["uid"];
            string path = null;
            path = HttpContext.Current.Server.MapPath("~").ToString();
            path = path.Replace("\\", "/") + (path.Right(1) == "\\" ? "" : "/")
                + "users/" + puid + "/vectorLayers/";
            return path;
        }
        public static string GetResultsPath()
        {
            string puid = (string)HttpContext.Current.Session["uid"];
            string path = null;
            path = HttpContext.Current.Server.MapPath("~").ToString();
            path = path.Replace("\\", "/") + (path.Right(1) == "\\" ? "" : "/")
                + "users/" + puid + "/results/";
            return path;
        }
        public static string GetTempPath()
        {
            string puid = (string)HttpContext.Current.Session["uid"];
            string path = null;
            path = HttpContext.Current.Server.MapPath("~").ToString();
            path = path.Replace("\\", "/") + (path.Right(1) == "\\" ? "" : "/")
                + "users/" + puid + "/temp/";
            return path;
        }
        public static void InitializeUser()
        {
            string puid = (string)HttpContext.Current.Session["uid"];
            if (GetDataTableFromQuery("SELECT uid FROM puserssettings WHERE uid = '"
                + puid + "'").Rows.Count == 0)
            {
                ExecuteNonQuery("INSERT INTO puserssettings " +
                    "(uid, pl_measurementtype, pl_terrainresolution, pl_thematic, " +
                    "pl_radius, pl_propagationmodel, pl_reliabilitys, pl_reliabilityt, " +
                    "pl_terrainconductivity, pl_radioclimate , pl_receiverheight, " +
                    "pl_receivergain , pl_receiversensitivity, rp_antennamodel, " +
                    "rp_cellradius, rp_gsmband, rp_gsmbandwidth, rp_lteband, " +
                    "rp_ltebandwidth, mw_frequency, mw_channelwidth, mw_outputpower, " +
                    "mw_antennagain, mw_losses, mw_fresnelclearance) " +
                    "VALUES " +
                    "('" + puid + "', 'Received Power (dBm)', '90m', 'default', 1.5, " +
                    "'ITWOM 3.0 (< 20GHz)', 50, 90, 'Average ground', " +
                    "'Maritime Temperate (Land)', 1.5, 2.14, -90, 'LT OD9-5 890-950 MHz', " +
                    "1.5, 'GSM900 PGSM', '0.2', 'B08_FDD_900 E-GSM', '10', 5, " +
                    "20, 47, 22.5, 0, 60);");
            }
            string path = HttpContext.Current.Server.MapPath("~").ToString();
            path = path.Replace("\\", "/") + (path.Right(1) == "\\" ? "" : "/") + "users/";
            string vectorLayers = path + puid + "/vectorLayers/";
            string temp = path + puid + "/temp/";
            string results = path + puid + "/results/";
            if (!Directory.Exists(path + puid + "/"))
            {
                Directory.CreateDirectory(path + puid + "/");
            }
            if (!Directory.Exists(vectorLayers))
            {
                Directory.CreateDirectory(vectorLayers);
            }
            if (!Directory.Exists(temp))
            {
                Directory.CreateDirectory(temp);
            }
            if (!Directory.Exists(results))
            {
                Directory.CreateDirectory(results);
            }
            if (!File.Exists(vectorLayers + "mynetwork.geojson"))
            {
                File.WriteAllText(vectorLayers + "mynetwork.geojson",
                    "{\"type\":\"FeatureCollection\",\"features\":[]}");
            }
            if (!File.Exists(vectorLayers + "mypolygons.geojson"))
            {
                File.WriteAllText(vectorLayers + "mypolygons.geojson",
                    "{\"type\":\"FeatureCollection\",\"features\":[]}");
            }
            if (!File.Exists(vectorLayers + "mylinks.geojson"))
            {
                File.WriteAllText(vectorLayers + "mylinks.geojson",
                    "{\"type\":\"FeatureCollection\",\"features\":[]}");
            }
            if (!File.Exists(vectorLayers + "myclutter.geojson"))
            {
                File.WriteAllText(vectorLayers + "myclutter.geojson",
                    "{\"type\":\"FeatureCollection\",\"features\":[]}");
            }
            if (!File.Exists(vectorLayers + "publicnetwork.geojson"))
            {
                File.WriteAllText(vectorLayers + "publicnetwork.geojson",
                    "{\"type\":\"FeatureCollection\",\"features\":[]}");
            }
            if (!File.Exists(vectorLayers + "results.geojson"))
            {
                File.WriteAllText(vectorLayers + "results.geojson",
                    "{\"type\":\"FeatureCollection\",\"features\":[]}");
            }
            if (!File.Exists(vectorLayers + "publicnotes.geojson"))
            {
                File.WriteAllText(vectorLayers + "publicnotes.geojson",
                    "{\"type\":\"FeatureCollection\",\"features\":[]}");
            }
            if (!File.Exists(vectorLayers + "mynotes.geojson"))
            {
                File.WriteAllText(vectorLayers + "mynotes.geojson",
                    "{\"type\":\"FeatureCollection\",\"features\":[]}");
            }
        }
        public static void GetNetworkFromDB()
        {
            string puid = (string)HttpContext.Current.Session["uid"];
            string vectorLayers = GetVectorLayersPath();
            if (File.Exists(vectorLayers + "mynetwork.geojson"))
            {
                File.Delete(vectorLayers + "mynetwork.geojson");
            }
            var options = new[] {
                    "-f", "GEOJSON",
                    "-sql", "SELECT " +
                    "siteid, sitename, longitude, latitude, height, region, " +
                    "country, city, district, province, address, comments, technology, " +
                    "band, bandwidth, cellid, lac, rfcn, rfid, dlfrequency, ulfrequency, " +
                    "rfpower, hba, azimuth, antennamodel, antennatype, polarization, " +
                    "vbeamwidth, hbeamwidth, downtilt, antennagain, feederloss, wkb_geometry " +
                    "FROM pusersnetwork WHERE uid ='" + puid + "';"};
            bool test = Pgsql2Json(
                "PG:dbname=cnip host=localhost user=postgres password=root port=5432",
                vectorLayers + "mynetwork.geojson", options);
            if (!File.Exists(vectorLayers + "mynetwork.geojson"))
            {
                File.WriteAllText(vectorLayers + "mynetwork.geojson",
                    "{\"type\":\"FeatureCollection\",\"features\":[]}");
            }
        }
        public static void GetPublicNotesFromDB()
        {
            string puid = (string)HttpContext.Current.Session["uid"];
            string vectorLayers = GetVectorLayersPath();
            if (File.Exists(vectorLayers + "publicnotes.geojson"))
            {
                File.Delete(vectorLayers + "publicnotes.geojson");
            }
            var options = new[] {
                    "-f", "GEOJSON",
                    "-sql", "SELECT " +
                    "noteid, notename, notetype, description, name, email, wkb_geometry " +
                    "FROM pusersnotes INNER JOIN pusers On pusers.uid = pusersnotes.uid " +
                    "WHERE pusersnotes.uid <> '" + puid + "' AND notetype='Public';"};
            bool test = Pgsql2Json(
                "PG:dbname=cnip host=localhost user=postgres password=root port=5432",
                vectorLayers + "publicnotes.geojson", options);
            if (!File.Exists(vectorLayers + "publicnotes.geojson"))
            {
                File.WriteAllText(vectorLayers + "publicnotes.geojson",
                    "{\"type\":\"FeatureCollection\",\"features\":[]}");
            }
        }
        public static void GetPublicNetworkFromDB()
        {
            string puid = (string)HttpContext.Current.Session["uid"];
            string vectorLayers = GetVectorLayersPath();
            if (File.Exists(vectorLayers + "publicnetwork.geojson"))
            {
                File.Delete(vectorLayers + "publicnetwork.geojson");
            }
            var options = new[] {
                    "-f", "GEOJSON",
                    "-sql", "SELECT " +
                    "siteid, sitename, longitude, latitude, height, region, " +
                    "country, city, district, province, address, comments, " +
                    "technology, band, bandwidth, cellid, lac, rfcn, rfid, " +
                    "dlfrequency, ulfrequency, rfpower, hba, azimuth, antennamodel, " +
                    "antennatype, polarization, vbeamwidth, hbeamwidth, downtilt, " +
                    "antennagain, feederloss, name, email, wkb_geometry " +
                    "FROM pusersnetwork INNER JOIN pusers On pusers.uid = pusersnetwork.uid " +
                    "WHERE pusersnetwork.uid <> '" + puid + "';"};
            bool test = Pgsql2Json(
                    "PG:dbname=cnip host=localhost user=postgres password=root port=5432",
                    vectorLayers + "publicnetwork.geojson", options);
            if (!File.Exists(vectorLayers + "publicnetwork.geojson"))
            {
                File.WriteAllText(vectorLayers + "publicnetwork.geojson",
                    "{\"type\":\"FeatureCollection\",\"features\":[]}");
            }
        }
        public static void GetNotesFromDB()
        {
            string puid = (string)HttpContext.Current.Session["uid"];
            string vectorLayers = GetVectorLayersPath();
            if (File.Exists(vectorLayers + "mynotes.geojson"))
            {
                File.Delete(vectorLayers + "mynotes.geojson");
            }
            var options = new[] {
                    "-f", "GEOJSON",
                    "-sql", "SELECT " +
                    "noteid, notename, notetype, description, wkb_geometry " +
                    "FROM pusersnotes WHERE uid='" + puid + "';"};
            bool test = Pgsql2Json(
                "PG:dbname=cnip host=localhost user=postgres password=root port=5432",
                vectorLayers + "mynotes.geojson", options);
            if (!File.Exists(vectorLayers + "mynotes.geojson"))
            {
                File.WriteAllText(vectorLayers + "mynotes.geojson",
                    "{\"type\":\"FeatureCollection\",\"features\":[]}");
            }
        }
        [WebMethod]
        public static void GetPolygonsFromDB()
        {
            string puid = (string)HttpContext.Current.Session["uid"];
            string vectorLayers = GetVectorLayersPath();
            if (File.Exists(vectorLayers + "mypolygons.geojson"))
            {
                File.Delete(vectorLayers + "mypolygons.geojson");
            }
            var options = new[] {
                    "-f", "GEOJSON",
                    "-sql", "SELECT " +
                    "polygonid, polygonname, wkb_geometry " +
                    "FROM puserspolygons WHERE uid ='" + puid + "';"};
            bool test = Pgsql2Json(
                "PG:dbname=cnip host=localhost user=postgres password=root port=5432",
                vectorLayers + "mypolygons.geojson", options);
            if (!File.Exists(vectorLayers + "mypolygons.geojson"))
            {
                File.WriteAllText(vectorLayers + "mypolygons.geojson",
                    "{\"type\":\"FeatureCollection\",\"features\":[]}");
            }
        }
        public static void GetLinksFromDB()
        {
            string puid = (string)HttpContext.Current.Session["uid"];
            string vectorLayers = GetVectorLayersPath();
            if (File.Exists(vectorLayers + "mylinks.geojson"))
            {
                File.Delete(vectorLayers + "mylinks.geojson");
            }
            var options = new[] {
                    "-f", "GEOJSON",
                    "-sql", "SELECT " +
                    "linkid, linkname, linktype, siteida, locheighta, bearinga, " +
                    "channelwidtha, frequencya, outputpowera, antennagaina, lossesa, " +
                    "siteidb, locheightb, bearingb, channelwidthb, frequencyb, " +
                    "outputpowerb, antennagainb, lossesb, distance, elevstr, name, email, wkb_geometry " +
                    "FROM puserslinks WHERE uid ='" + puid + "';"};
            bool test = Pgsql2Json(
                "PG:dbname=cnip host=localhost user=postgres password=root port=5432",
                vectorLayers + "mylinks.geojson", options);
            if (!File.Exists(vectorLayers + "mylinks.geojson"))
            {
                File.WriteAllText(vectorLayers + "mylinks.geojson",
                    "{\"type\":\"FeatureCollection\",\"features\":[]}");
            }
        }
        /***************************************************************/
        /**************************Web Methods**************************/
        /***************************************************************/
        public static string GetUsername()
        {
            return (string)HttpContext.Current.Session["name"];
        }
        public static string GetPuid()
        {
            return (string)HttpContext.Current.Session["uid"];
        }
        public static string GetEmail()
        {
            return (string)HttpContext.Current.Session["email"];
        }
        [WebMethod]
        public static void Logoff()
        {
            if (_IsPukey())
            {
                string puid = (string)HttpContext.Current.Session["uid"];
                ExecuteNonQuery("UPDATE pusers SET ukey = '' WHERE uid = '" + puid + "';");
            }
            HttpContext.Current.Session["name"] = "";
            HttpContext.Current.Session["email"] = "";
            HttpContext.Current.Session["uid"] = "";
            HttpContext.Current.Session["ukey"] = "";
        }
        [WebMethod]
        public static string GetSettings()
        {
            if (_IsPukey())
            {
                string puid = (string)HttpContext.Current.Session["uid"];
                DataTable dt = GetDataTableFromQuery("SELECT * " +
                    "FROM puserssettings WHERE uid = '" + puid + "';");
                if (dt.Rows.Count > 0)
                {
                    string y = "";
                    for (int x = 0; x < dt.Columns.Count; x++)
                    {
                        y += dt.Rows[0][x].ToString() + '\t';
                    }
                    y = y.TrimEnd(1);
                    return y;
                }
            }
            return "";
        }
        [WebMethod]
        public static void SetSettings(string options)
        {
            if (_IsPukey())
            {
                string puid = (string)HttpContext.Current.Session["uid"];
                string[] settings = options.Split('\t');
                if (settings.Length == 24)
                {
                    string pl_measurementtype = settings[0];
                    string pl_terrainresolution = settings[1];
                    string pl_thematic = settings[2];
                    string pl_radius = settings[3];
                    string pl_propagationmodel = settings[4];
                    string pl_reliabilitys = settings[5];
                    string pl_reliabilityt = settings[6];
                    string pl_terrainconductivity = settings[7];
                    string pl_radioclimate = settings[8];
                    string pl_receiverheight = settings[9];
                    string pl_receivergain = settings[10];
                    string pl_receiversensitivity = settings[11];
                    string rp_antennamodel = settings[12];
                    string rp_cellradius = settings[13];
                    string rp_gsmband = settings[14];
                    string rp_gsmbandwidth = settings[15];
                    string rp_lteband = settings[16];
                    string rp_ltebandwidth = settings[17];
                    string mw_frequency = settings[18];
                    string mw_channelwidth = settings[19];
                    string mw_outputpower = settings[20];
                    string mw_antennagain = settings[21];
                    string mw_losses = settings[22];
                    string mw_fresnelclearance = settings[23];
                    ExecuteNonQuery("UPDATE puserssettings SET " +
                        "pl_measurementtype = '" + pl_measurementtype + "', " +
                        "pl_terrainresolution = '" + pl_terrainresolution + "', " +
                        "pl_thematic = '" + pl_thematic + "', " +
                        "pl_radius = " + pl_radius + ", " +
                        "pl_propagationmodel = '" + pl_propagationmodel + "', " +
                        "pl_reliabilitys = " + pl_reliabilitys + ", " +
                        "pl_reliabilityt = " + pl_reliabilityt + ", " +
                        "pl_terrainconductivity = '" + pl_terrainconductivity + "', " +
                        "pl_radioclimate = '" + pl_radioclimate + "', " +
                        "pl_receiverheight = " + pl_receiverheight + ", " +
                        "pl_receivergain = " + pl_receivergain + ", " +
                        "pl_receiversensitivity = " + pl_receiversensitivity + ", " +
                        "rp_antennamodel = '" + rp_antennamodel + "', " +
                        "rp_cellradius = '" + rp_cellradius + "', " +
                        "rp_gsmband = '" + rp_gsmband + "', " +
                        "rp_gsmbandwidth = '" + rp_gsmbandwidth + "', " +
                        "rp_lteband = '" + rp_lteband + "', " +
                        "rp_ltebandwidth = '" + rp_ltebandwidth + "', " +
                        "mw_frequency = " + mw_frequency + ", " +
                        "mw_channelwidth = " + mw_channelwidth + ", " +
                        "mw_outputpower = " + mw_outputpower + ", " +
                        "mw_antennagain = " + mw_antennagain + ", " +
                        "mw_losses = " + mw_losses + ", " +
                        "mw_fresnelclearance = " + mw_fresnelclearance + " " +
                        "WHERE uid = '" + puid + "' " + ";");
                }
            }
        }
        [WebMethod]
        public static void UploadNetworkToDB()
        {
            if (_IsPukey())
            {
                string puid = (string)HttpContext.Current.Session["uid"];
                var options = new[] {
                    "-overwrite",
                    "-f", "PostgreSQL",
                    "-nln", "mynetwork_" + puid,
                    "-nlt","POINT"};
                Json2Pgsql(GetVectorLayersPath() + "mynetwork.geojson",
                    "PG:dbname=cnip host=localhost user=postgres password=root port=5432", options);
                if (TableExists("mynetwork_" + puid))
                {
                    ExecuteNonQuery("DELETE FROM pusersnetwork WHERE uid = '" + puid + "';");
                    if (TableHasData("mynetwork_" + puid))
                    {
                        ExecuteNonQuery("INSERT INTO pusersnetwork " +
                        "(uid, siteid, sitename, longitude, latitude, height, region, " +
                        "country, city, district, province, address, comments, technology, " +
                        "band, bandwidth, cellid, lac, rfcn, rfid, dlfrequency, ulfrequency, " +
                        "rfpower, hba, azimuth, antennamodel, antennatype, polarization, " +
                        "vbeamwidth, hbeamwidth, downtilt, antennagain, feederloss, wkb_geometry) " +
                        "SELECT '" + puid + "', " +
                        "siteid, sitename, " +
                        "cast(longitude as double precision), " +
                        "cast(latitude as double precision), height, region, country, city, " +
                        "district, province, address, comments, technology, band, bandwidth, " +
                        "cellid, lac, rfcn, rfid, dlfrequency, ulfrequency, rfpower, hba, " +
                        "azimuth, antennamodel, antennatype, polarization, vbeamwidth, hbeamwidth, " +
                        "downtilt, antennagain, feederloss, wkb_geometry " +
                        "FROM mynetwork_" + puid + ";");
                    }
                    DropTable("mynetwork_" + puid);
                }
            }
        }
        [WebMethod]
        public static string GetNewPolygonId()
        {
            string puid = (string)HttpContext.Current.Session["uid"];
            DataTable ni = GetDataTableFromQuery("SELECT MAX(CAST(polygonid AS INTEGER))+1 FROM " +
                "puserspolygons WHERE uid = '" + puid + "' GROUP BY uid");
            int newid = 1; if (ni.Rows.Count > 0) { newid = ni.Rows[0][0].ToString().ToInt(); }
            return newid.ToString();
        }
        [WebMethod]
        public static void UploadUserPolygonsToDB(string polygonname)
        {
            if (_IsPukey())
            {
                string puid = (string)HttpContext.Current.Session["uid"];
                var options = new[] {
                    "-overwrite",
                    "-f", "PostgreSQL",
                    "-nln", "myuserpolygons_" + puid,
                    "-nlt","POLYGON"};
                Json2Pgsql(GetVectorLayersPath() + polygonname,
                    "PG:dbname=cnip host=localhost user=postgres password=root port=5432", options);
                if (TableExists("myuserpolygons_" + puid))
                {
                    if (TableHasData("myuserpolygons_" + puid))
                    {
                        string polygonid = GetNewPolygonId();
                        ExecuteNonQuery("INSERT INTO puserspolygons " +
                            "(uid, polygonid, polygonname, wkb_geometry) " +
                            "SELECT '" + puid + "', " +
                            "CAST(" + polygonid + " + row_number() over () AS character varying), " +
                            "CONCAT('new polygon ',CAST(" + polygonid
                            + " + row_number() over () AS character varying)), wkb_geometry " +
                            "FROM myuserpolygons_" + puid + ";");
                    }
                    DropTable("myuserpolygons_" + puid);
                    File.Delete(GetVectorLayersPath() + polygonname);
                }
            }
        }
        [WebMethod]
        public static void UploadPolygonsToDB()
        {
            if (_IsPukey())
            {
                string puid = (string)HttpContext.Current.Session["uid"];
                var options = new[] {
                    "-overwrite",
                    "-f", "PostgreSQL",
                    "-nln", "mypolygons_" + puid,
                    "-nlt","POLYGON"};
                Json2Pgsql(GetVectorLayersPath() + "mypolygons.geojson",
                    "PG:dbname=cnip host=localhost user=postgres password=root port=5432", options);
                if (TableExists("mypolygons_" + puid))
                {
                    ExecuteNonQuery("DELETE FROM puserspolygons WHERE uid = '" + puid + "';");
                    if (TableHasData("mypolygons_" + puid))
                    {
                        ExecuteNonQuery("INSERT INTO puserspolygons " +
                            "(uid, polygonid, polygonname, wkb_geometry) " +
                            "SELECT '" + puid + "', " +
                            "polygonid, polygonname, wkb_geometry " +
                            "FROM mypolygons_" + puid + ";");
                    }
                    DropTable("mypolygons_" + puid);
                }
            }
        }
        [WebMethod]
        public static void UploadNotesToDB()
        {
            if (_IsPukey())
            {
                string puid = (string)HttpContext.Current.Session["uid"];
                var options = new[] {
                    "-overwrite",
                    "-f", "PostgreSQL",
                    "-nln", "mynotes_" + puid,
                    "-nlt","POINT"};
                Json2Pgsql(GetVectorLayersPath() + "mynotes.geojson",
                    "PG:dbname=cnip host=localhost user=postgres password=root port=5432", options);
                if (TableExists("mynotes_" + puid))
                {
                    ExecuteNonQuery("DELETE FROM pusersnotes WHERE uid = '" + puid + "';");
                    if (TableHasData("mynotes_" + puid))
                    {
                        ExecuteNonQuery("INSERT INTO pusersnotes " +
                            "(uid, noteid, notename, notetype, description, wkb_geometry) " +
                            "SELECT '" + puid + "', " +
                            "noteid, notename, notetype, description, wkb_geometry " +
                            "FROM mynotes_" + puid + ";");
                    }
                    DropTable("mynotes_" + puid);
                }
            }
        }
        [WebMethod]
        public static void UploadLinksToDB()
        {
            if (_IsPukey())
            {
                string puid = (string)HttpContext.Current.Session["uid"];
                var options = new[] {
                    "-overwrite",
                    "-f", "PostgreSQL",
                    "-nln", "mylinks_" + puid,
                    "-nlt","LINESTRING"};
                Json2Pgsql(GetVectorLayersPath() + "mylinks.geojson",
                    "PG:dbname=cnip host=localhost user=postgres password=root port=5432", options);
                if (TableExists("mylinks_" + puid))
                {
                    ExecuteNonQuery("DELETE FROM puserslinks WHERE uid = '" + puid + "';");
                    if (TableHasData("mylinks_" + puid))
                    {
                        ExecuteNonQuery("INSERT INTO puserslinks " +
                            "(uid, linkid, linkname, linktype, siteida, locheighta, bearinga, " +
                            "channelwidtha, frequencya, outputpowera, antennagaina, lossesa, " +
                            "siteidb, locheightb, bearingb, channelwidthb, frequencyb, " +
                            "outputpowerb, antennagainb, lossesb, distance, elevstr, name, email, wkb_geometry) " +
                            "SELECT '" + puid + "', " +
                            "linkid, linkname, linktype, siteida, locheighta, bearinga, " +
                            "channelwidtha, frequencya, outputpowera, antennagaina, lossesa, " +
                            "siteidb, locheightb, bearingb, channelwidthb, frequencyb, " +
                            "outputpowerb, antennagainb, lossesb, distance, elevstr, name, email, wkb_geometry " +
                            "FROM mylinks_" + puid + ";");
                    }
                    DropTable("mylinks_" + puid);
                }
            }
        }
        [WebMethod]
        public static void UploadDataToUrl(string data, string url)
        {
            if (_IsPukey())
            {
                string puid = (string)HttpContext.Current.Session["uid"];
                string path = HttpContext.Current.Server.MapPath("~").ToString();
                path = path.Replace("\\", "/") + (path.Right(1) == "\\" ? "" : "/") + url;
                File.WriteAllText(path, data);
            }
        }
        [WebMethod]
        public static string GetForecastRunning()
        {
            return File.Exists(GetTempPath() + "running.txt").ToString().ToLower();
        }
        [WebMethod]
        public static void PredictCoverage(string resultid, string sites)
        {
            if (_IsPukey())
            {
                string args = "";
                string puid = (string)HttpContext.Current.Session["uid"];
                string tempfol = GetTempPath();
                string resultfol = GetResultsPath();

                if (File.Exists(tempfol + "running.txt")) { return; }
                // clear temp folder
                ClearTempPath(tempfol);
                // create prediction running file
                File.WriteAllText(tempfol + "running.txt", "running");
                args += "PredictCoverage " +
                        puid + " " +
                        "\"" + tempfol + "\" " +
                        "\"" + resultfol + "\" " +
                        "\"" + resultid + "\" " +
                        "\"" + sites + "\"";
                StartProcess("C:/TelecomInfraProject/OpenCellular/forecast/forecast/bin/Debug/forecast.exe", args, false);
            }
        }
        [WebMethod]
        public static void RadioPlan(string sites)
        {
            if (_IsPukey())
            {
                string args = "";
                string puid = (string)HttpContext.Current.Session["uid"];
                string tempfol = GetTempPath();

                if (File.Exists(tempfol + "running.txt")) { return; }
                // clear temp folder
                ClearTempPath(tempfol);
                // create prediction running file
                File.WriteAllText(tempfol + "running.txt", "running");
                args += "RadioPlan " +
                        puid + " " +
                        "\"" + tempfol + "\" " +
                        "\"" + sites + "\"";
                StartProcess("C:/TelecomInfraProject/OpenCellular/forecast/forecast/bin/Debug/forecast.exe", args, false);
            }
        }
        [WebMethod]
        public static void Links(string sites)
        {
            if (_IsPukey())
            {
                string args = "";
                string puid = (string)HttpContext.Current.Session["uid"];
                string tempfol = GetTempPath();

                if (File.Exists(tempfol + "running.txt")) { return; }
                // clear temp folder
                ClearTempPath(tempfol);
                // create prediction running file
                File.WriteAllText(tempfol + "running.txt", "running");
                args += "Links " +
                        puid + " " +
                        "\"" + tempfol + "\" " +
                        "\"" + sites + "\"";
                StartProcess("C:/TelecomInfraProject/OpenCellular/forecast/forecast/bin/Debug/forecast.exe", args, false);
            }
        }
        [WebMethod]
        public static void PredictSites(string resultid, string polygonid, string technology)
        {
            if (_IsPukey())
            {
                string args = "";
                string puid = (string)HttpContext.Current.Session["uid"];
                string tempfol = GetTempPath();
                string resultfol = GetResultsPath();

                if (File.Exists(tempfol + "running.txt")) { return; }
                // clear temp folder
                ClearTempPath(tempfol);
                // create prediction running file
                File.WriteAllText(tempfol + "running.txt", "running");
                args += "PredictSites " +
                        puid + " " +
                        "\"" + tempfol + "\" " +
                        "\"" + resultfol + "\" " +
                        "\"" + resultid + "\" " +
                        "\"" + polygonid + "\" " +
                        "\"" + technology + "\"";
                StartProcess("C:/TelecomInfraProject/OpenCellular/forecast/forecast/bin/Debug/forecast.exe", args, false);
            }
        }
        [WebMethod]
        public static void BestCandidate(string resultid, string sites, string polygonid)
        {
            if (_IsPukey())
            {
                string args = "";
                string puid = (string)HttpContext.Current.Session["uid"];
                string tempfol = GetTempPath();
                string resultfol = GetResultsPath();

                if (File.Exists(tempfol + "running.txt")) { return; }
                // clear temp folder
                ClearTempPath(tempfol);
                // create prediction running file
                File.WriteAllText(tempfol + "running.txt", "running");
                args += "BestCandidate " +
                        puid + " " +
                        "\"" + tempfol + "\" " +
                        "\"" + resultfol + "\" " +
                        "\"" + resultid + "\" " +
                        "\"" + sites + "\" " +
                        "\"" + polygonid + "\"";
                StartProcess("C:/TelecomInfraProject/OpenCellular/forecast/forecast/bin/Debug/forecast.exe", args, false);
            }
        }
    }
}