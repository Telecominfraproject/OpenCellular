using System;
using System.IO;
using System.Data;
using System.Xml;
using cnip.Models;
using static cnip.Models.gisext;
using static cnip.Models.gdalext;
using static cnip.Models.pgsql;
using static cnip.Models.generic;
using static cnip.Models.settings;
using static cnip.Models.site;

namespace forecast.Models
{
    class Coverage
    {
        public static string Forecast(string puid, string tempfol, string resultfol,
            string resultid, string sites, string polygons, string measurementType = "",
            bool analysis = false, bool saveResult = false)
        {
            string resultString = "";
            // try run forecast
            try
            {
                Settings settings = new Settings(puid);
                if (measurementType.Length > 0)
                {
                    settings.SetPl_MeasurementType(measurementType);
                }

                string cpuid = "c" + puid;
                ExecuteNonQuery(
                    "DROP TABLE IF EXISTS " + cpuid + "grid;" +
                    "CREATE TABLE " + cpuid + "grid (" +
                    "wkb_geometry_txt text, rate integer);");

                string siteString = "";

                foreach (string siteid in sites.Split(','))
                {
                    // load site
                    Site site = new Site(puid, siteid);
                    // add site to siteString
                    siteString += site.Siteid + "," + site.SiteName + "&";

                    // set prediction radius
                    string predictionRadius = settings.Pl_Radius;
                    if (sites.Split(',').Length > 1)
                    {
                        if (measurementType.Length > 0)
                        {
                            predictionRadius = GetPredictionRadiusForMultipleSitesRun(
                                puid, sites, site.Siteid,
                                (settings.Rp_CellRadius.ToDouble() + 1.5).ToString());
                        }
                        else
                        {
                            predictionRadius = GetPredictionRadiusForMultipleSitesRun(
                                puid, sites, site.Siteid, settings.Pl_Radius);
                        }
                    }
                    else
                    {
                        if (measurementType.Length > 0)
                        {
                            predictionRadius = (settings.Rp_CellRadius.ToDouble() + 1.5).ToString();
                        }
                    }

                    // create temp site id and site table id
                    string tsiteid = "s" + site.Siteid;
                    string spuid = tsiteid + puid;

                    // write site qth file 
                    File.WriteAllText(tempfol + tsiteid + ".qth",
                      site.SiteName + "\r\n"
                    + site.Latitude + "\r\n"
                    + site.Longitude + "\r\n"
                    + site.HBA + "\r\n"
                    );

                    // write site lrp file
                    File.WriteAllText(tempfol + tsiteid + ".lrp",
                      settings.EarthDielectricConstant +
                      "\t; Earth Dielectric Constant(Relative permittivity)" + "\r\n"
                    + settings.EarthConductivity +
                      "\t; Earth Conductivity(Siemens per meter)" + "\r\n"
                    + settings.AtmosphericBendingConstant +
                      "\t; Atmospheric Bending Constant(N - Units)" + "\r\n"
                    + String.Format("{0:0.0}", site.Frequency.ToDouble()) +
                      "\t; Frequency in MHz(20 MHz to 20 GHz)" + "\r\n"
                    + settings.RadioClimateNumber +
                      "\t; Radio Climate" + "\r\n"
                    + site.PolarizationNumber +
                      "\t; Polarization(0 = Horizontal, 1 = Vertical)" + "\r\n"
                    + String.Format("{0:0.00}", settings.Pl_ReliabilityS.ToDouble() / 100) +
                      "\t; Fraction of situations" + "\r\n"
                    + String.Format("{0:0.00}", settings.Pl_ReliabilityT.ToDouble() / 100) +
                      "\t; Fraction of time" + "\r\n"
                    + String.Format("{0:0.00}", site.EIRP.ToDouble()) +
                      "\t; Transmitter Effective Radiated Power in Watts(optional)" + "\r\n"
                    );

                    //// write antenna horizontal and vertical beam files
                    //string bearing = (site.AntennaType == "omni" ? "0.0" :
                    //    String.Format("{0:0.0}", site.Azimuth.ToDouble()));

                    //// write horizontal beam file
                    //File.WriteAllText(tempfol + tsiteid + ".az",
                    //  bearing + "\n" + File.ReadAllText(@"C:/SPLAT/antenna/" + site.AntennaModel + ".az")
                    //);

                    //// write vertical beam file
                    //File.WriteAllText(tempfol + tsiteid + ".el",
                    //" " + String.Format("{0:0.0}", site.DownTilt.ToDouble()) + "\t" +
                    //  bearing + "\n" + File.ReadAllText(@"C:/SPLAT/antenna/" + site.AntennaModel + ".el")
                    //);

                    // write pathloss thematic file
                    File.WriteAllText(tempfol + tsiteid + ".lcf",
                      File.ReadAllText(@"C:/SPLAT/pathloss/" + settings.Pl_Thematic + ".lcf")
                    );

                    // write received power thematic file
                    File.WriteAllText(tempfol + tsiteid + ".dcf",
                      File.ReadAllText(@"C:/SPLAT/receivedpower/" + settings.Pl_Thematic + ".dcf")
                    );

                    // write field strength thematic file
                    File.WriteAllText(tempfol + tsiteid + ".scm",
                      File.ReadAllText(@"C:/SPLAT/fieldstrength/" + settings.Pl_Thematic + ".scm")
                    );

                    // run splat
                    string args = "-d C:/SPLAT/sdf-" + settings.Resolution + "/  -t " +
                                tempfol + tsiteid + ".qth  -R " + predictionRadius + " -L " +
                                settings.Pl_ReceiverHeight + " -m 1.333 -kml  -ngs  -metric  -erp " +
                                (settings.Pl_MeasurementType == "Received Power (dBm)" ? site.EIRP + " -dbm" :
                                settings.Pl_MeasurementType == "Field Strength (dBµV/m)" ? site.EIRP : "0")
                                + "  -o " + tempfol + tsiteid + ".ppm" +
                                (settings.Pl_PropagationModel == "ITM / Longley-Rice (< 20GHz)" ? " -olditm" : "");
                    if (settings.Resolution == "sd")
                    {
                        StartProcess("C:/SPLAT/Splat-1-3-1-SD-mx64.exe", args);
                    }
                    else
                    {
                        StartProcess("C:/SPLAT/Splat-1-3-1-HD-mx36.exe", args);
                    }

                    //get extent from kml created by splat
                    XmlDocument xmldoc = new XmlDocument();
                    XmlNodeList xmlnode;
                    xmldoc.Load(tempfol + tsiteid + ".kml");
                    xmlnode = xmldoc.GetElementsByTagName("LatLonBox");

                    string north = xmlnode[0].ChildNodes.Item(0).InnerText.Trim();
                    string south = xmlnode[0].ChildNodes.Item(1).InnerText.Trim();
                    string east = xmlnode[0].ChildNodes.Item(2).InnerText.Trim();
                    string west = xmlnode[0].ChildNodes.Item(3).InnerText.Trim();

                    GdalInfo gdalinfo = new GdalInfo();
                    var options = new[] { "" };

                    // convert ppm created by splat to tif
                    options = new[]{
                        "-of", "Gtiff",
                        "-a_ullr", west, north, east, south,
                        "-a_srs", "EPSG:4326"
                    };
                    Translate(tempfol + tsiteid + ".ppm", tempfol + tsiteid + ".tif", options);

                    // set pixel size for tif
                    SetGdalInfo(tempfol + tsiteid + ".tif",
                        new GdalInfo(west.ToDouble(), settings.XPixelSize,
                        north.ToDouble(), settings.YPixelSize));// pixel size x, y
                    gdalinfo = GetGdalInfo(tempfol + tsiteid + ".tif");

                    // clip tif to site raidus
                    // /////get bounds for site with  prediction radius
                    Bounds b = new Bounds();
                    b = gisext.MBR(site.Location, predictionRadius.ToDouble() * 1000);

                    // /////align bounds to raster grid
                    int col = (int)((b.XMin - gdalinfo.XMin) / gdalinfo.XPixelSize) - 10;
                    int row = (int)((gdalinfo.YMax - b.YMax) / -gdalinfo.YPixelSize) - 10;
                    b.XMin = gdalinfo.XMin + gdalinfo.XPixelSize * col + gdalinfo.XCoefficient * row;
                    b.YMax = gdalinfo.YMax + gdalinfo.YCoefficient * col + gdalinfo.YPixelSize * row;

                    col = (int)((b.XMax - gdalinfo.XMin) / gdalinfo.XPixelSize) + 10;
                    row = (int)((gdalinfo.YMax - b.YMin) / -gdalinfo.YPixelSize) + 10;
                    b.XMax = gdalinfo.XMin + gdalinfo.XPixelSize * col + gdalinfo.XCoefficient * row;
                    b.YMin = gdalinfo.YMax + gdalinfo.YCoefficient * col + gdalinfo.YPixelSize * row;

                    options = new[]{
                        "-of", "Gtiff",
                        "-projwin", b.XMin.ToString(), b.YMax.ToString(), b.XMax.ToString(), b.YMin.ToString()
                    };
                    Translate(tempfol + tsiteid + ".tif", tempfol + spuid + ".tif", options);

                    string sql = "";
                    sql += "DROP TABLE IF EXISTS " + spuid + "grid;" + "\r\n";
                    sql += "CREATE TABLE " + spuid + "grid AS  " + "\r\n";
                    sql += "SELECT " + "\r\n";
                    sql += "CAST(ST_Value(rast, 1, x, y) AS int) As b1, " + "\r\n";
                    sql += "CAST(ST_Value(rast, 2, x, y) AS int) As b2, " + "\r\n";
                    sql += "CAST(ST_Value(rast, 3, x, y) AS int) As b3, " + "\r\n";
                    sql += "ST_AsText(ST_PixelAsPolygon(rast, x, y)) AS wkb_geometry_txt " + "\r\n";
                    sql += "FROM (SELECT (ST_PixelAsPoints(rast, 1)).* FROM " +
                        spuid + ") tmp, " + spuid + "; " + "\r\n";
                    sql += "ALTER TABLE " + spuid + "grid ADD COLUMN color text, " +
                        "ADD COLUMN rate integer;" + "\r\n";
                    sql += "UPDATE " + spuid + "grid SET color = " +
                        "CONCAT(cast(b1 as character varying),','," +
                        "cast(b2 as character varying),','," +
                        "cast(b3 as character varying));" + "\r\n";
                    sql += "UPDATE " + spuid + "grid SET rate = " +
                        settings.ThematicBaseType + "_" + settings.Pl_Thematic + ".rate FROM " +
                        settings.ThematicBaseType + "_" + settings.Pl_Thematic + " WHERE " +
                        spuid + "grid.color = " +
                        settings.ThematicBaseType + "_" + settings.Pl_Thematic + ".color;" + "\r\n";
                    sql += "ALTER TABLE " + spuid + "grid " +
                        "DROP COLUMN b1, DROP COLUMN b2, DROP COLUMN b3, DROP COLUMN color;" + "\r\n";
                    sql += "DELETE FROM " + spuid + "grid WHERE rate IS NULL;" + "\r\n";
                    if (settings.Pl_MeasurementType == "SNR (dB)")
                    {
                        sql += "UPDATE " + spuid + "grid SET rate = (" +
                            site.SnrFactor + " - rate);" + "\r\n";
                        sql += "UPDATE " + spuid + "grid SET rate = (";
                        sql += "CASE WHEN rate < 0 THEN -5 ELSE ";
                        sql += "CASE WHEN rate >=  0 AND rate < 5 THEN 0 ELSE ";
                        sql += "CASE WHEN rate >=  5 AND rate < 10 THEN 5 ELSE ";
                        sql += "CASE WHEN rate >=  10 AND rate < 15 THEN 10 ELSE ";
                        sql += "CASE WHEN rate >=  15 AND rate < 20 THEN 15 ELSE ";
                        sql += "CASE WHEN rate >=  20 AND rate < 25 THEN 20 ELSE ";
                        sql += "CASE WHEN rate >=  25 AND rate < 30 THEN 25 ELSE ";
                        sql += "CASE WHEN rate >=  30 AND rate < 35 THEN 30 ELSE ";
                        sql += "CASE WHEN rate >=  35 AND rate < 40 THEN 35 ELSE ";
                        sql += "CASE WHEN rate >=  40 AND rate < 45 THEN 40 ELSE ";
                        sql += "CASE WHEN rate >=  45 AND rate < 50 THEN 45 ELSE ";
                        sql += "CASE WHEN rate >=  50 AND rate < 55 THEN 50 ELSE ";
                        sql += "CASE WHEN rate >=  55 AND rate < 60 THEN 55 ELSE ";
                        sql += "CASE WHEN rate >= 60 THEN 60 ELSE 60 END " +
                            "END END END END END END END END END END END END END); " + "\r\n";
                    }
                    else if (settings.Pl_MeasurementType == "Data Rate Score")
                    {
                        sql += "UPDATE " + spuid + "grid SET rate = ("
                            + site.SnrFactor + " - rate);" + "\r\n";
                        sql += "UPDATE " + spuid + "grid SET rate = (";
                        sql += "CASE WHEN rate < 0 THEN 1 ELSE ";
                        sql += "CASE WHEN rate >=  0 AND rate < 5 THEN 2 ELSE ";
                        sql += "CASE WHEN rate >=  5 AND rate < 10 THEN 2 ELSE ";
                        sql += "CASE WHEN rate >=  10 AND rate < 15 THEN 2 ELSE ";
                        sql += "CASE WHEN rate >=  15 AND rate < 20 THEN 3 ELSE ";
                        sql += "CASE WHEN rate >=  20 AND rate < 25 THEN 3 ELSE ";
                        sql += "CASE WHEN rate >=  25 AND rate < 30 THEN 3 ELSE ";
                        sql += "CASE WHEN rate >=  30 AND rate < 35 THEN 4 ELSE ";
                        sql += "CASE WHEN rate >=  35 AND rate < 40 THEN 4 ELSE ";
                        sql += "CASE WHEN rate >=  40 AND rate < 45 THEN 4 ELSE ";
                        sql += "CASE WHEN rate >=  45 AND rate < 50 THEN 5 ELSE ";
                        sql += "CASE WHEN rate >=  50 AND rate < 55 THEN 5 ELSE ";
                        sql += "CASE WHEN rate >=  55 AND rate < 60 THEN 5 ELSE ";
                        sql += "CASE WHEN rate >= 60 THEN 5 ELSE 5 END " +
                            "END END END END END END END END END END END END END); " + "\r\n";
                    }
                    else if (settings.Pl_MeasurementType == "Voice Quality Score")
                    {
                        sql += "UPDATE " + spuid + "grid SET rate = (" +
                            site.SnrFactor + " - rate);" + "\r\n";
                        sql += "UPDATE " + spuid + "grid SET rate = (";
                        sql += "CASE WHEN rate < 0 THEN 1 ELSE ";
                        sql += "CASE WHEN rate >=  0 AND rate < 5 THEN 2 ELSE ";
                        sql += "CASE WHEN rate >=  5 AND rate < 10 THEN 2 ELSE ";
                        sql += "CASE WHEN rate >=  10 AND rate < 15 THEN 3 ELSE ";
                        sql += "CASE WHEN rate >=  15 AND rate < 20 THEN 3 ELSE ";
                        sql += "CASE WHEN rate >=  20 AND rate < 25 THEN 3 ELSE ";
                        sql += "CASE WHEN rate >=  25 AND rate < 30 THEN 4 ELSE ";
                        sql += "CASE WHEN rate >=  30 AND rate < 35 THEN 4 ELSE ";
                        sql += "CASE WHEN rate >=  35 AND rate < 40 THEN 4 ELSE ";
                        sql += "CASE WHEN rate >=  40 AND rate < 45 THEN 5 ELSE ";
                        sql += "CASE WHEN rate >=  45 AND rate < 50 THEN 5 ELSE ";
                        sql += "CASE WHEN rate >=  50 AND rate < 55 THEN 5 ELSE ";
                        sql += "CASE WHEN rate >=  55 AND rate < 60 THEN 5 ELSE ";
                        sql += "CASE WHEN rate >= 60 THEN 5 ELSE 5 END " +
                            "END END END END END END END END END END END END END); " + "\r\n";
                    }
                    sql += "CREATE TABLE " + cpuid + "gridtmp AS " +
                        "SELECT wkb_geometry_txt, " +
                        (settings.ThematicExtension == "tpl" ? "Min" : "Max") +
                        "(rate) AS rate FROM (SELECT * FROM " +
                        spuid + "grid UNION SELECT * FROM " +
                        cpuid + "grid) tmp GROUP BY wkb_geometry_txt;" + "\r\n";
                    sql += "DROP TABLE IF EXISTS " + cpuid + "grid;" + "\r\n";
                    sql += "ALTER TABLE " + cpuid + "gridtmp RENAME TO " + cpuid + "grid;" + "\r\n";
                    // write sql to file
                    File.WriteAllText(tempfol + tsiteid + "mysql.sql", sql);
                    // load raster to pgsql and run grid query
                    File.WriteAllText(tempfol + tsiteid + ".bat",
                        "\"C:/Program Files/PostgreSQL/9.5/bin/raster2pgsql\" -s 4326 -d -F -I -C \"" +
                        tempfol + spuid + ".tif\" > " + tempfol + tsiteid + ".sql" + "\r\n" +
                        "SET PGPASSWORD=root" + "\r\n" +
                        "\"C:/Program Files/PostgreSQL/9.5/bin/psql\" -f \"" +
                        tempfol + tsiteid + ".sql\" cnip postgres" + "\r\n" +
                        "SET PGPASSWORD=root" + "\r\n" +
                        "\"C:/Program Files/PostgreSQL/9.5/bin/psql\" -f \"" +
                        tempfol + tsiteid + "mysql.sql\" cnip postgres"
                        );
                    StartProcess(tempfol + tsiteid + ".bat");
                    // drop table spuid grid created for single site
                    DropTable(spuid + "grid");
                    // spuid table created by raster2pgsql 
                    DropTable(spuid);
                }

                // fix siteString remove extra & from the end
                if (siteString.Length > 0)
                {
                    siteString = siteString.TrimEnd(1);

                    string sql = "";
                    sql += "ALTER TABLE " + cpuid + "grid " +
                        "ADD COLUMN color text, " +
                        "ADD COLUMN b1 integer, ADD COLUMN b2 integer, ADD COLUMN b3 integer, " +
                        "ADD COLUMN rast raster, ADD COLUMN wkb_geometry geometry;" + "\r\n";
                    sql += "UPDATE " + cpuid + "grid SET color = " +
                        settings.ThematicExtension + "_" + settings.Pl_Thematic + ".color FROM " +
                        settings.ThematicExtension + "_" + settings.Pl_Thematic + " WHERE " +
                        cpuid + "grid.rate = " +
                        settings.ThematicExtension + "_" + settings.Pl_Thematic + ".rate;" + "\r\n";
                    sql += "UPDATE " + cpuid + "grid SET " +
                        "wkb_geometry = ST_GeomFromText(wkb_geometry_txt,4326), " +
                        "b1 = cast(split_part(color, ',', 1) as integer), " +
                        "b2 = cast(split_part(color, ',', 2) as integer), " +
                        "b3 = cast(split_part(color, ',', 3) as integer);" + "\r\n";
                    sql += "UPDATE " + cpuid + "grid SET " +
                        "rast = ST_AsRaster(wkb_geometry, " +
                        settings.XPixelSize.ToString() + "," +
                        settings.YPixelSize.ToString().Replace("-", "") +
                        ", '8BUI'::text, b1, NULL);" + "\r\n"; // pixel size x, y
                    sql += "UPDATE " + cpuid + "grid SET " +
                        "rast = ST_AddBand(rast, '8BUI'::text, b2, NULL);" + "\r\n";
                    sql += "UPDATE " + cpuid + "grid SET " +
                        "rast = ST_AddBand(rast, '8BUI'::text, b3, NULL);" + "\r\n";
                    sql += "UPDATE " + cpuid + "grid SET " +
                        "rast = ST_AddBand(rast, '8BUI'::text, 255, NULL);" + "\r\n";
                    sql += "SELECT AddRasterConstraints('" + cpuid + "grid'::name, 'rast'::name);" + "\r\n";

                    File.WriteAllText(tempfol + puid + "mysql.sql", sql);

                    File.WriteAllText(tempfol + puid + ".bat",
                        "SET PGPASSWORD=root" + "\r\n" +
                        "\"C:/Program Files/PostgreSQL/9.5/bin/psql\" -f \"" +
                        tempfol + puid + "mysql.sql\" cnip postgres"
                        );
                    StartProcess(tempfol + puid + ".bat");

                    if (TableHasData(cpuid + "grid"))
                    {
                        GdalInfo gdalinfo = new GdalInfo();
                        var options = new[] { "" };

                        options = new[]{
                            "-of", "Gtiff",
                            "-a_srs", "EPSG:4326"
                        };
                        Translate("PG:dbname=cnip host=localhost " +
                            "user=postgres password=root port=5432 " +
                            "mode=2 schema=public column=rast table=" +
                            cpuid + "grid",
                            tempfol + cpuid + "grid.tif", options);

                        options = new[]{
                            "-of", "Gtiff",
                            "-s_srs", "EPSG:4326",
                            "-t_srs", "EPSG:4326",
                            "-r", "near"
                        };
                        Warp(tempfol + cpuid + "grid.tif",
                            tempfol + cpuid + "gridf.tif", options);

                        options = new[]{
                            "-of", "PNG"
                        };
                        Translate(tempfol + cpuid + "gridf.tif",
                            tempfol + cpuid + "gridf.png", options);

                        /// build thematic String 
                        // ,&$#@>
                        // result : thematicString>pngString>polygonsString@polygonsString
                        // thematicString
                        // rate&r,g,b$rate&r,g,b
                        // pngString
                        // png,1,2,3,4
                        // polygonsString
                        // polygonid,polygonname#siteid,sitename#png,1,2,3,4#cttrue,ctext1234
                        // #rate&value&r,g,b$rate&value&r,g,b#bestcandidate
                        string thematicString = "";
                        if (settings.Pl_MeasurementType.Contains("Score"))
                        {
                            sql = "SELECT ratestring, color FROM " +
                                settings.ThematicExtension + "_" + settings.Pl_Thematic +
                                " ORDER BY weight desc;";
                        }
                        else
                        {
                            sql = "SELECT rate, color FROM " +
                                settings.ThematicExtension + "_" + settings.Pl_Thematic +
                                " ORDER BY weight desc;";
                        }
                        foreach (DataRow thematic in GetDataTableFromQuery(sql).Rows)
                        {

                            thematicString += thematic[0].ToString() + "&" +
                                thematic[1].ToString() + "$";
                        }
                        thematicString = thematicString.TrimEnd(1);
                        /// build png String 
                        // ,&$#@>
                        // result : thematicString>pngString>polygonsString@polygonsString
                        // thematicString
                        // rate&r,g,b$rate&r,g,b
                        // pngString
                        // png,1,2,3,4
                        // polygonsString
                        // polygonid,polygonname#siteid,sitename#png,1,2,3,4#cttrue,ctext1234
                        // #rate&value&r,g,b$rate&value&r,g,b#bestcandidate
                        gdalinfo = GetGdalInfo(tempfol + cpuid + "gridf.tif");
                        string pngname = "resultid" + resultid + "r" +
                            RandomNumber(0, 1000).ToString() + ".png";
                        string pngString = pngname + "," +
                            gdalinfo.XMin.ToString() + "," + gdalinfo.YMin.ToString() + "," +
                            gdalinfo.XMax.ToString() + "," + gdalinfo.YMax.ToString();
                        File.WriteAllBytes(resultfol + pngname,
                            File.ReadAllBytes(tempfol + cpuid + "gridf.png"));

                        // check if user has polygons
                        string coverageTest = "";
                        string polygonsString = "";

                        string polygonsWhere = BuildWhere(polygons, "polygonid");
                        foreach (DataRow polygon in GetDataTableFromQuery(
                            "SELECT polygonid, polygonname FROM " +
                            "puserspolygons WHERE uid='" + puid + "'" +
                            (polygonsWhere.Length > 0 ? " AND " + polygonsWhere : "") + ";").Rows)
                        {
                            DataTable dt;
                            if (settings.Pl_MeasurementType.Contains("Score"))
                            {
                                dt = GetDataTableFromQuery(
                                "SELECT ratestring, covered, tmp.color FROM (" +
                                "SELECT rate , round(100*(ratecount*" +
                                (settings.Resolution == "sd" ? "90*90" : "30*30") +
                                "/ST_Area(wkb_geometry,TRUE))/0.01)*0.01 as covered, color FROM " +
                                "(SELECT rate, count(rate) AS ratecount, color FROM (" +
                                "SELECT rate, color FROM " + cpuid + "grid, " +
                                "puserspolygons WHERE ST_Intersects(" + cpuid + "grid.wkb_geometry," +
                                "puserspolygons.wkb_geometry) AND " +
                                "puserspolygons.polygonid = '" + polygon[0].ToString() + "' AND " +
                                "puserspolygons.uid='" + puid + "')tmp " +
                                "GROUP BY rate, color ORDER BY rate)tmp, " +
                                "puserspolygons WHERE " +
                                "puserspolygons.polygonid = '" + polygon[0].ToString() + "' AND " +
                                "puserspolygons.uid='" + puid + "')tmp " +
                                ", " + settings.ThematicExtension + "_" + settings.Pl_Thematic +
                                " WHERE " + settings.ThematicExtension + "_" + settings.Pl_Thematic +
                                ".rate = tmp.rate;");
                            }
                            else
                            {
                                dt = GetDataTableFromQuery(
                                "SELECT rate , round(100*(ratecount*" +
                                (settings.Resolution == "sd" ? "90*90" : "30*30") +
                                "/ST_Area(wkb_geometry,TRUE))/0.01)*0.01, color FROM " +
                                "(SELECT rate, count(rate) AS ratecount, color FROM (" +
                                "SELECT rate, color FROM " + cpuid + "grid, " +
                                "puserspolygons WHERE ST_Intersects(" + cpuid + "grid.wkb_geometry," +
                                "puserspolygons.wkb_geometry) AND " +
                                "puserspolygons.polygonid = '" + polygon[0].ToString() + "' AND " +
                                "puserspolygons.uid='" + puid + "') tmp " +
                                "GROUP BY rate, color ORDER BY rate)tmp, " +
                                "puserspolygons WHERE " +
                                "puserspolygons.polygonid = '" + polygon[0].ToString() + "' AND " +
                                "puserspolygons.uid='" + puid + "';");
                            }
                            if (dt.Rows.Count > 0)
                            {
                                // coverage test
                                if (settings.Pl_MeasurementType == "Received Power (dBm)")
                                {
                                    coverageTest = GetDataTableFromQuery(
                                        "SELECT sum(covered) > 90 FROM (" +
                                        "SELECT rate , round(100*(ratecount*" +
                                        (settings.Resolution == "sd" ? "90*90" : "30*30") +
                                        "/ST_Area(wkb_geometry,TRUE))/0.01)*0.01 as covered FROM " +
                                        "(SELECT rate, count(rate) AS ratecount FROM (" +
                                        "SELECT rate FROM " + cpuid + "grid, " +
                                        "puserspolygons WHERE ST_Intersects(" + cpuid + "grid.wkb_geometry," +
                                        "puserspolygons.wkb_geometry) AND " +
                                        "puserspolygons.polygonid = '" + polygon[0].ToString() + "' AND " +
                                        "puserspolygons.uid='" + puid + "') tmp " +
                                        "GROUP BY rate ORDER BY rate)tmp, " +
                                        "puserspolygons WHERE " +
                                        "puserspolygons.polygonid = '" + polygon[0].ToString() + "' AND " +
                                        "puserspolygons.uid='" + puid + "'" +
                                        ")tmp WHERE rate >= -90").Rows[0][0].ToString();
                                    coverageTest += "," + GetDataTableFromQuery(
                                        "SELECT sum(covered) FROM (" +
                                        "SELECT tmp.rate, covered*weight as covered FROM " +
                                        settings.ThematicBaseType + "_" + settings.Pl_Thematic + ",(" +
                                        "SELECT rate , round(100*(ratecount*" +
                                        (settings.Resolution == "sd" ? "90*90" : "30*30") +
                                        "/ST_Area(wkb_geometry,TRUE))/0.01)*0.01 as covered FROM " +
                                        "(SELECT rate, count(rate) AS ratecount FROM (" +
                                        "SELECT rate FROM " + cpuid + "grid, " +
                                        "puserspolygons WHERE ST_Intersects(" + cpuid + "grid.wkb_geometry," +
                                        "puserspolygons.wkb_geometry) AND " +
                                        "puserspolygons.polygonid = '" + polygon[0].ToString() + "' AND " +
                                        "puserspolygons.uid='" + puid + "') tmp " +
                                        "GROUP BY rate ORDER BY rate)tmp, " +
                                        "puserspolygons WHERE " +
                                        "puserspolygons.polygonid = '" + polygon[0].ToString() + "' AND " +
                                        "puserspolygons.uid='" + puid + "')tmp WHERE " +
                                        settings.ThematicBaseType + "_" + settings.Pl_Thematic +
                                        ".rate = tmp.rate)tmp WHERE rate >= -90").Rows[0][0].ToString();
                                }
                                else
                                { coverageTest = "N/A"; }

                                // ,&$#@>
                                // result : thematicString>pngString>polygonsString@polygonsString
                                // thematicString
                                // rate&r,g,b$rate&r,g,b
                                // pngString
                                // png,1,2,3,4
                                // polygonsString
                                // polygonid,polygonname#siteid,sitename#png,1,2,3,4#cttrue,ctext1234
                                // #rate&value&r,g,b$rate&value&r,g,b#bestcandidate
                                polygonsString += polygon[0].ToString() + "," + polygon[1].ToString() + "#"
                                               + siteString + "#" + pngString + "#" + coverageTest + "#";
                                for (int j = 0; j < dt.Rows.Count; j++)
                                {
                                    polygonsString += dt.Rows[j][0].ToString() + "&" +
                                        dt.Rows[j][1].ToString() + "&" + dt.Rows[j][2].ToString() + "$";
                                }
                                polygonsString = polygonsString.TrimEnd(1) + "#" + "@";
                            }
                        }
                        polygonsString = polygonsString.TrimEnd(1);
                        // ,&$#@>
                        // result : thematicString>pngString>polygonsString@polygonsString
                        // thematicString
                        // rate&r,g,b$rate&r,g,b
                        // pngString
                        // png,1,2,3,4
                        // polygonsString
                        // polygonid,polygonname#siteid,sitename#png,1,2,3,4#cttrue,ctext1234
                        // #rate&value&r,g,b$rate&value&r,g,b#bestcandidate
                        // insert result
                        resultString = thematicString + ">" + pngString + ">" + polygonsString;
                        if (saveResult)
                        {
                            InsertResult(puid, resultid, "P" + resultid + ": " +
                                settings.Pl_MeasurementType, resultString);
                        }
                        if (!analysis)
                        {
                            // clear temp folder
                            ClearTempPath(tempfol);
                        }
                        // clear temp queries
                        DropTable(cpuid + "grid");
                    }
                }
            }
            catch (Exception)
            {
                ClearTempPath(tempfol);
            }
            return resultString;
        }
        public static void InsertResult(
            string puid, string resultid, string resultname, string resultstring, string resulttype = "coverage")
        {
            ExecuteNonQuery("INSERT INTO pusersresults " +
                "(uid, resultid, resultname, resultstring, resulttype) " +
                "VALUES " +
                "('" + puid + "','" + resultid + "','" +
                resultname + "','" + resultstring + "','" + resulttype + "');");
        }
        private static string GetPredictionRadiusForMultipleSitesRun(
            string puid, string sites, string site, string predictionRadius)
        {
            string sql = "";
            sql += "SELECT LEAST(Round(Max(dist/1000)/0.1)*0.1,10) FROM ";
            sql += "( ";
            sql += "SELECT ";
            sql += "ta.siteid AS siteid, ";
            sql += "ST_Distance(Geography(ta.wkb_geometry),";
            sql += "Geography(tb.wkb_geometry),TRUE) AS dist ";
            sql += "FROM ";
            sql += "pusersnetwork ta, ";
            sql += "pusersnetwork tb ";
            sql += "WHERE ";
            sql += "ta.siteid = '" + site + "' AND ";
            sql += "ta.uid = '" + puid + "' AND ";
            sql += "tb.uid = '" + puid + "' AND ";
            sql += BuildWhere(sites, "tb.siteid");
            sql += ")tmp GROUP BY siteid;";
            foreach (DataRow row in GetDataTableFromQuery(sql).Rows)
            {
                predictionRadius = Math.Max(
                    row[0].ToString().ToDouble(),
                    predictionRadius.ToDouble()
                    ).ToString();
            }
            return predictionRadius;
        }
    }
}
