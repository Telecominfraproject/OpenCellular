using System;
using System.Collections.Generic;
using System.Linq;
using System.Data;
using cnip.Models;
using static cnip.Models.gisext;
using static cnip.Models.pgsql;
using static cnip.Models.kruskal;
using static cnip.Models.generic;
using static cnip.Models.settings;
using static cnip.Models.site;

namespace forecast.Models
{
    public static class Links
    {
        public static void Forecast(string puid, string tempfol, string resultid, 
            string sites, bool analysis = false)
        {
            try
            {
                // delete any associated private links to selected sites
                DeleteAnyAssociatedLinks(puid, sites);

                string links = "";

                Coords center = GetExtent(
                    puid, sites, "siteid", "pusersnetwork", Extent.Center);
                Coords bottomLeft = GetExtent(
                    puid, sites, "siteid", "pusersnetwork", Extent.BottomLeft);
                double within50KmsOfSites = Math.Min(
                    gisext.Distance(center, bottomLeft) + 50000, 100000);

                string puidWhere = "uid='" + puid + "' ";
                string NotPuidWhere = "uid<>'" + puid + "' ";

                string sitesWhere = BuildWhere(sites, "siteid");
                string NotSitesWhere = BuildWhere(sites, "siteid", false);

                List<string> sitesList = sites.Split(',').ToList();

                string sql = "";
                sql += "SELECT ta.siteid AS siteida, tb.siteid AS siteidb, ";
                sql += "ROUND(ST_Distance(Geography(ta.wkb_geometry)," +
                    "Geography(tb.wkb_geometry),TRUE)) AS weight ";
                sql += "FROM pusersnetwork ta, pusersnetwork tb WHERE ";
                sql += "ta.uid='" + puid + "' AND tb.uid='" + puid + "' ";
                sql += "AND " + BuildWhere(sites, "ta.siteid");
                sql += "AND " + BuildWhere(sites, "tb.siteid") + ";";

                DataTable dt = GetDataTableFromQuery(sql);

                int V = sitesList.Count; // Number of vertices in graph  
                int E = dt.Rows.Count; // Number of edges in graph  
                Graph graph = new Graph(V, E);
                for (int i = 0; i < dt.Rows.Count; i++)
                {
                    graph.edge[i].src = sitesList.
                        FindIndex(x => x.Equals(dt.Rows[i][0].ToString()));
                    graph.edge[i].dest = sitesList.
                        FindIndex(x => x.Equals(dt.Rows[i][1].ToString()));
                    graph.edge[i].weight = dt.Rows[i][2].ToString().ToInt();
                }
                Edge[] edges = graph.KruskalMST();

                for (int i = 0; i < edges.Length; i++)
                {
                    links += CreateLink(new CreateLinkOptions(
                        puid, sitesList[edges[i].src],
                        puid, sitesList[edges[i].dest], "private"));
                }

                // create private and public link within 50Km of selected sites 
                // to nearest site from selected sites 
                Site privateSite = GetClosestSite(new GetClosestSiteOptions(
                    puidWhere, NotSitesWhere, center, within50KmsOfSites));
                if (privateSite.Siteid.Length > 0)
                {
                    // create link, find nearest selected site to 
                    //(find nearest site from private network within 50Kms of selected sites)
                    Site selectedSite = GetClosestSite(new GetClosestSiteOptions(
                        puidWhere, sitesWhere, privateSite.Location, within50KmsOfSites));
                    links += CreateLink(new CreateLinkOptions(puid, selectedSite.Siteid,
                        puid, privateSite.Siteid, "private"));
                }
                // if there is public link within 50 kms do not create public link
                string closestSiteidTopublicLinkSiteWithin50Kms = "";
                foreach (DataRow row in GetDataTableFromQuery(
                    "SELECT longitude, latitude " +
                    "FROM puserslinks LEFT JOIN " +
                    "pusersnetwork ON " +
                    "puserslinks.uid = pusersnetwork.uid AND " +
                    "puserslinks.siteida = pusersnetwork.siteid " +
                    "WHERE " +
                    "puserslinks.uid='" + puid + "' AND " +
                    "linktype='public'").Rows)
                {
                    Coords publicLinkSiteLocation = new Coords(
                        row[0].ToString().ToDouble(),
                        row[1].ToString().ToDouble()
                        );
                    // closest site from selected sites to public link site
                    Site closestSite = GetClosestSite(new GetClosestSiteOptions(
                        puidWhere, sitesWhere, publicLinkSiteLocation, within50KmsOfSites));
                    if (closestSite.Siteid.Length > 0)
                    {
                        closestSiteidTopublicLinkSiteWithin50Kms = closestSite.Siteid;
                    }
                }
                if (closestSiteidTopublicLinkSiteWithin50Kms.Length == 0)
                {
                    // create link, find nearest selected site to 
                    //(find nearest site from public network within 50Kms of selected sites)
                    Site publicSite = GetClosestSite(new GetClosestSiteOptions(
                        NotPuidWhere, "", center, within50KmsOfSites));
                    if (publicSite.Siteid.Length > 0)
                    {
                        Site selectedSite = GetClosestSite(new GetClosestSiteOptions(
                            puidWhere, sitesWhere, publicSite.Location, within50KmsOfSites));
                        publicSite = GetClosestSite(new GetClosestSiteOptions(
                                                NotPuidWhere, "", selectedSite.Location, within50KmsOfSites));
                        links += CreateLink(new CreateLinkOptions(puid, selectedSite.Siteid,
                            publicSite.Puid, publicSite.Siteid, "public"));
                    }
                }
                links = links.TrimEnd(1);
                // adjust heights to meet fresnel zone clearance
                string resultString = resultid + "@" + links + "@" + AdjustHeightsToMeetLinks(puid, links);
                ExecuteNonQuery("DELETE FROM pusersresults WHERE " +
                    "uid='" + puid + "' AND " +
                    "resultid='0' AND " +
                    "resulttype='links';");
                Coverage.InsertResult(puid, "0", "", resultString, "links");

                if (!analysis)
                {
                    ClearTempPath(tempfol);
                }
            }
            catch (Exception)
            {
                ClearTempPath(tempfol);
            }
        }
        private static string AdjustHeightsToMeetLinks(string puid, string links)
        {
            if (links.Length == 0) { return "@"; }
            List<string> privateSitesModified = new List<string>();
            List<string> publicSitesModified = new List<string>();
            Settings settings = new Settings(puid);
            double xFresnelClearance = settings.Mw_FresnelClearance.ToDouble();
            for (int i = 0; i < links.Split(',').Length; i++)
            {
                string linktype = "";
                string siteida = "";
                string siteidb = "";
                string email = "";
                double locheightAm = 0;
                double locheightBm = 0;
                double heightAm = 0;
                double heightBm = 0;
                double siteheightAm = 0;
                double siteheightBm = 0;
                double deviceheightAm = 0;
                double deviceheightBm = 0;
                double xDistanceKm = 0;
                double xFrequencyGHz = 0;
                string[] xElevations = "".Split('@');
                double xObstm = 0;
                bool linkmeet = false;
                foreach (DataRow row in GetDataTableFromQuery(
                "SELECT " +
                "linktype, siteida, siteidb, " +
                "locheighta, heighta, locheightb, heightb, " +
                "distance, frequencya, elevstr, " +
                "email FROM puserslinks WHERE " +
                "uid='" + puid + "' AND " +
                "linkid ='" + links.Split(',')[i] + "';").Rows)
                {
                    linktype = row[0].ToString();
                    siteida = row[1].ToString();
                    siteidb = row[2].ToString();
                    locheightAm = row[3].ToString().ToDouble();
                    heightAm = row[4].ToString().ToDouble();
                    locheightBm = row[5].ToString().ToDouble();
                    heightBm = row[6].ToString().ToDouble();
                    xDistanceKm = row[7].ToString().ToDouble();
                    xFrequencyGHz = row[8].ToString().ToDouble();
                    xElevations = row[9].ToString().Split('@');
                    email = row[10].ToString();
                }
                foreach (DataRow row in GetDataTableFromQuery(
                    "SELECT height FROM pusersnetwork WHERE " +
                    "uid='" + puid + "' AND " +
                    "siteid ='" + siteida + "';").Rows)
                {
                    siteheightAm = row[0].ToString().ToDouble();
                }
                if (linktype == "private")
                {
                    foreach (DataRow row in GetDataTableFromQuery(
                        "SELECT height FROM pusersnetwork WHERE " +
                        "uid='" + puid + "' AND " +
                        "siteid ='" + siteidb + "';").Rows)
                    {
                        siteheightBm = row[0].ToString().ToDouble();
                    }
                }
                else
                {
                    foreach (DataRow row in GetDataTableFromQuery(
                        "SELECT height FROM pusersnetwork " +
                        "FULL JOIN pusers ON " +
                        "pusers.uid=pusersnetwork.uid WHERE " +
                        "email='" + email + "' AND " +
                        "siteid = '" + siteidb + "';").Rows)
                    {
                        siteheightBm = row[0].ToString().ToDouble();
                    }
                }
                while (!linkmeet)
                {
                    linkmeet = true;
                    deviceheightAm = heightAm + locheightAm;
                    deviceheightBm = heightBm + locheightBm;
                    xObstm = 0;
                    if (xElevations.Length > 1)
                    {
                        for (int x = 0; x < xElevations.Length; x++)
                        {
                            double xElevationm = xElevations[x].Split('&')[3].ToDouble();
                            double xCdistanceKm = xElevations[x].Split('&')[2].ToDouble();
                            double xLoS = ((x * (deviceheightBm - deviceheightAm)) /
                                (xElevations.Length)) + deviceheightAm;
                            double xFresnelFactor = 17.3 * Math.Sqrt((xCdistanceKm *
                                (xDistanceKm - xCdistanceKm)) / (xDistanceKm * xFrequencyGHz));
                            xFresnelFactor = xFresnelFactor *
                                xFresnelClearance / 100;
                            double xFresnelZone = xLoS - xFresnelFactor;
                            xObstm = (xFresnelZone - xElevationm) >= 0 ?
                                0 : xFresnelFactor + xElevationm;
                            if (xObstm > 0) break;
                        }
                    }
                    if (xObstm > 0)
                    {
                        linkmeet = false;
                        heightAm = heightAm + 0.5;
                        heightBm = heightBm + 0.5;
                        if (linktype == "public")
                        {
                            if (heightAm > siteheightAm)
                            {
                                siteheightAm = heightAm;
                                publicSitesModified.Add(siteida);
                                SetSiteHeight(puid, siteida, siteheightAm);
                            }
                            if (heightBm > siteheightBm)
                            {
                                heightBm = siteheightBm;
                            }
                        }
                        else
                        {
                            if (heightAm > siteheightAm)
                            {
                                siteheightAm = heightAm;
                                privateSitesModified.Add(siteida);
                                SetSiteHeight(puid, siteida, siteheightAm);
                            }
                            if (heightBm > siteheightBm)
                            {
                                siteheightBm = heightBm;
                                privateSitesModified.Add(siteidb);
                                SetSiteHeight(puid, siteidb, siteheightBm);
                            }
                        }
                        SetDeviceHeight(puid, links.Split(',')[i], heightAm, heightBm);
                    }
                }
            }
            return String.Join(",",
                privateSitesModified.Distinct().OrderBy(x => x).ToArray()) +
                "@" + String.Join(",",
                publicSitesModified.Distinct().OrderBy(x => x).ToArray());
        }
        private static void SetDeviceHeight(
            string puid, string linkid, double heightAm, double heightBm)
        {
            ExecuteNonQuery("UPDATE puserslinks SET " +
                "heighta='" + heightAm.ToString() + "', " +
                "heightb='" + heightBm.ToString() + "' WHERE " +
                "uid='" + puid + "' AND " +
                "linkid='" + linkid + "'");
        }
        private static void SetSiteHeight(
            string puid, string siteid, double height)
        {
            ExecuteNonQuery("UPDATE pusersnetwork SET " +
                "height='" + height.ToString() + "' WHERE " +
                "uid='" + puid + "' AND " +
                "siteid='" + siteid + "'");
        }
        private struct GetClosestSiteOptions
        {
            public string PuidWhere { get; set; }
            public string SitesWhere { get; set; }
            public Coords Location;
            public double WithinDistance { get; set; }
            public GetClosestSiteOptions(
                string puidWhere,
                string sitesWhere,
                Coords location,
                double withinDistance)
            {
                PuidWhere = puidWhere;
                SitesWhere = sitesWhere;
                Location = location;
                WithinDistance = withinDistance;
            }
        }
        private static Site GetClosestSite(GetClosestSiteOptions options)
        {
            Site cs = new Site("", "", 0, 0);
            foreach (DataRow row in GetDataTableFromQuery(
                "SELECT uid, siteid, longitude, latitude " +
                "FROM pusersnetwork WHERE " +
                 options.PuidWhere +
                 (options.SitesWhere.Length > 0 ?
                 "AND " + options.SitesWhere : "") +
                "AND ST_Distance(" +
                "Geography(ST_GeomFromText('POINT(" +
                options.Location.X.ToString() + " " +
                options.Location.Y.ToString() + ")',4326))," +
                "Geography(wkb_geometry),TRUE) <= " +
                options.WithinDistance.ToString() + " " +
                "ORDER BY ST_Distance(" +
                "Geography(ST_GeomFromText('POINT(" +
                options.Location.X.ToString() + " " +
                options.Location.Y.ToString() + ")',4326))," +
                "Geography(wkb_geometry),TRUE) LIMIT 1;").Rows)
            {
                cs.Puid = row[0].ToString();
                cs.Siteid = row[1].ToString();
                cs.Location.X = row[2].ToString().ToDouble();
                cs.Location.Y = row[3].ToString().ToDouble();
            }
            return cs;
        }
        private struct CreateLinkOptions
        {
            public string PuidA { get; set; }
            public string SiteidA { get; set; }
            public string PuidB { get; set; }
            public string SiteidB { get; set; }
            public string LinkType { get; set; }
            public CreateLinkOptions(
                string puidA,
                string siteidA,
                string puidB,
                string siteidB,
                string linkType)
            {
                PuidA = puidA;
                SiteidA = siteidA;
                PuidB = puidB;
                SiteidB = siteidB;
                LinkType = linkType;
            }
        }
        private static string CreateLink(
            CreateLinkOptions options)
        {
            // check if arguments are valid
            if (options.SiteidA == "" ||
                options.SiteidB == "" ||
                options.PuidA == "" ||
                options.PuidB == "") { return ""; }
            // check if arguments are valid
            if (options.SiteidA == options.SiteidB &&
                options.PuidA == options.PuidB) { return ""; }
            // check if link already exits than exit
            foreach (DataRow row in GetDataTableFromQuery(
                "SELECT linkid FROM puserslinks WHERE " +
                "uid='" + options.PuidA + "' AND " +
                "linktype='" + options.LinkType + "' AND ((" +
                "siteida='" + options.SiteidA + "' AND " +
                "siteidb='" + options.SiteidB + "') OR (" +
                "siteida='" + options.SiteidB + "' AND " +
                "siteidb='" + options.SiteidA + "'));").Rows)
            {
                return "";
            }
            // get new link id for user
            string linkid = GetNewLinkId(options.PuidA);
            // load settings to get microwave paramters
            Settings settings = new Settings(options.PuidA);
            string frequency = settings.Mw_Frequency;
            string channelwidth = settings.Mw_Channelwidth;
            string outputpower = settings.Mw_OutputPower;
            string antennagain = settings.Mw_AntennaGain;
            string losses = settings.Mw_Losses;
            // load name and email for public link
            string name = "";
            string email = "";
            if (options.LinkType == "public")
            {
                foreach (DataRow row in GetDataTableFromQuery(
                    "SELECT name, email FROM pusers WHERE " +
                    "uid='" + options.PuidB + "';").Rows)
                {
                    name = row[0].ToString();
                    email = row[1].ToString();
                }
            }
            string sql = "INSERT INTO puserslinks (" +
                "uid, linkid, linkname, linktype, " +
                "siteida, locheighta, heighta, bearinga, channelwidtha, frequencya, " +
                "outputpowera, antennagaina, lossesa, " +
                "siteidb, locheightb, heightb, bearingb, channelwidthb, frequencyb, " +
                "outputpowerb, antennagainb, lossesb, " +
                "distance, name, email, wkb_geometry) ";
            sql += "SELECT ";
            sql += "'" + options.PuidA + "' AS uid, ";
            sql += "'" + linkid + "' AS linkid, ";
            sql += "'new link " + linkid + "' AS linkname, ";
            sql += "'" + options.LinkType + "' AS linktype, ";
            sql += "sitea.siteid, ";
            // temporarily save the location as text in locheighta column
            // to avoid additional queries
            sql += "ST_AsText(sitea.wkb_geometry) AS locaheighta, '0.5', ";
            sql += "CASE WHEN " +
                "degrees(ST_Azimuth(Geography(sitea.wkb_geometry)," +
                "Geography(siteb.wkb_geometry))) < 0 THEN 360 + " +
                "degrees(ST_Azimuth(Geography(sitea.wkb_geometry)," +
                "Geography(siteb.wkb_geometry))) ELSE " +
                "degrees(ST_Azimuth(Geography(sitea.wkb_geometry)," +
                "Geography(siteb.wkb_geometry))) END AS bearinga, ";
            sql += "'" + channelwidth + "' AS channelwidtha, ";
            sql += "'" + frequency + "' AS frequencya, ";
            sql += "'" + outputpower + "' AS outputpowera, ";
            sql += "'" + antennagain + "' AS antennagaina, ";
            sql += "'" + losses + "' AS lossesa, ";
            sql += "siteb.siteid, ";
            // temporarily save the location as text in locheightb column
            // to avoid additional queries
            sql += "ST_AsText(siteb.wkb_geometry) AS locaheightb, '0.5', ";
            sql += "CASE WHEN " +
                "degrees(ST_Azimuth(Geography(siteb.wkb_geometry)," +
                "Geography(sitea.wkb_geometry))) < 0 THEN 360 + " +
                "degrees(ST_Azimuth(Geography(siteb.wkb_geometry)," +
                "Geography(sitea.wkb_geometry))) ELSE " +
                "degrees(ST_Azimuth(Geography(siteb.wkb_geometry)," +
                "Geography(sitea.wkb_geometry))) END AS bearinga, ";
            sql += "'" + channelwidth + "' AS channelwidthb, ";
            sql += "'" + frequency + "' AS frequencyb, ";
            sql += "'" + outputpower + "' AS outputpowerb, ";
            sql += "'" + antennagain + "' AS antennagainb, ";
            sql += "'" + losses + "' AS lossesb, ";
            sql += "ST_Distance(Geography(sitea.wkb_geometry)," +
                "Geography(siteb.wkb_geometry),TRUE)/1000 AS distance, ";
            sql += "'" + name + "' AS name, ";
            sql += "'" + email + "' AS email, ";
            sql += "ST_MakeLine(" +
                "sitea.wkb_geometry,siteb.wkb_geometry) AS wkb_geometry ";
            sql += "FROM ";
            sql += "pusersnetwork sitea, ";
            sql += "pusersnetwork siteb ";
            sql += "WHERE ";
            sql += "sitea.uid='" + options.PuidA + "' AND " +
                "sitea.siteid = '" + options.SiteidA + "' AND ";
            sql += "siteb.uid='" + options.PuidB + "' AND " +
                "siteb.siteid = '" + options.SiteidB + "';";
            ExecuteNonQuery(sql);

            // update location heights and elevation string
            // use temporarily saved locations in locheight columns
            foreach (DataRow row in GetDataTableFromQuery(
                "SELECT linkid, " +
                "ST_X(ST_GeomFromText(locheighta,4326)) AS x1, " +
                "ST_Y(ST_GeomFromText(locheighta,4326)) AS y1, " +
                "ST_X(ST_GeomFromText(locheightb,4326)) AS x2, " +
                "ST_Y(ST_GeomFromText(locheightb,4326)) AS y2 " +
                "FROM " +
                "puserslinks WHERE " +
                "uid='" + options.PuidA + "' AND " +
                "linkid='" + linkid + "';").Rows)
            {
                string lineString = "LINESTRING(" +
                    row[1].ToString() + " " + row[2].ToString() + "," +
                    row[3].ToString() + " " + row[4].ToString() + ")";
                string[] elevArray = GetElevationStringFromLineString(
                    lineString, 100).Split('#');
                ExecuteNonQuery("UPDATE puserslinks SET " +
                    "locheighta='" + elevArray[0] + "', " +
                    "locheightb='" + elevArray[1] + "', " +
                    "elevstr='" + elevArray[2] + "' " +
                    "WHERE uid='" + options.PuidA + "' AND " +
                    "linkid='" + row[0] + "';");
            }
            return linkid + ",";
        }
        private static string GetNewLinkId(
            string puid)
        {
            int newid = 1;
            foreach (DataRow row in GetDataTableFromQuery(
                "SELECT " +
                "MAX(CAST(linkid AS INTEGER))+1 FROM " +
                "puserslinks WHERE " +
                "uid='" + puid + "' GROUP BY uid;").Rows)
            {
                newid = row[0].ToString().ToInt();
            }
            return newid.ToString();
        }
        private static void DeleteAnyAssociatedLinks(
            string puid, string sites)
        {
            foreach (string site in sites.Split(','))
            {
                ExecuteNonQuery("DELETE FROM puserslinks WHERE " +
                    "uid='" + puid + "' AND " +
                    "linktype='private' AND (" +
                    "siteida='" + site + "' OR " +
                    "siteidb='" + site + "');");
            }
        }
    }
}
