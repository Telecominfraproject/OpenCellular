using System;
using System.Collections.Generic;
using System.Data;
using cnip.Models;
using static cnip.Models.gisext;
using static cnip.Models.pgsql;
using static cnip.Models.generic;

namespace forecast.Models
{
    class RadioPlan
    {
        public static void Forecast(string puid, string tempfol, string resultid, string sites, bool analysis = false)
        {
            try
            {
                if (analysis)
                {
                    sites = GetPotentialSites(puid, sites);
                    if (sites.Length == 0) { return; }
                }
                CreateNet(puid, sites);
                CreateNeighbors(puid);
                CreateCost(puid);
                CreateSequences(50, puid);
                // assign freq, bsic, pci (list<freq, bsic, pci> - neighbors - cand)
                AssignToSequence(50, puid, "2g", "rfcn", CreateARFCN());
                AssignToSequence(50, puid, "2g", "rfid", CreateBSIC());
                AssignToSequence(50, puid, "4g", "rfid", CreatePCI());
                // build pairs
                BuildPairs(puid);
                // update results    
                Update2GRfcn(puid);
                Update2GRfid(puid);
                Update4GRfid(puid);
                // clear temp tables
                ClearRadioPlanTempTables(puid);

                string resultString = resultid + "@" + sites;
                ExecuteNonQuery("DELETE FROM pusersresults WHERE " +
                    "uid='" + puid + "' AND " +
                    "resultid='0' AND " +
                    "resulttype='radioplan';");
                Coverage.InsertResult(puid, "0", "", resultString, "radioplan");

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
        private static string CreatePCI()
        {
            string pcis = "";
            for (int i = 0; i <= 503; i++)
            {
                pcis += (i * 6 > 503 ?
                        (i * 6) - 503 > 503 ?
                        (i * 6) - 1006 > 503 ?
                        (i * 6) - 1509 > 503 ?
                        (i * 6) - 2012 > 503 ?
                        (i * 6) - 2515 :
                        (i * 6) - 2012 :
                        (i * 6) - 1509 :
                        (i * 6) - 1006 :
                        (i * 6) - 503 :
                        i * 6).ToString() + ",";
            }
            return pcis.TrimEnd(1);
        }
        private static string CreateARFCN()
        {
            string arfcns = "";
            for (int i = 1; i <= 124; i++)
            {
                arfcns += (i * 3 > 124 ?
                          (i * 3) - 124 > 124 ?
                          (i * 3) - 248 :
                          (i * 3) - 124 :
                          i * 3).ToString() + ",";
            }
            return arfcns.TrimEnd(1);
        }
        private static string CreateBSIC()
        {
            string bsics = "";
            for (int i = 0; i <= 63; i++)
            {
                bsics += Convert.ToString(i, 8) + ",";
            }
            return bsics.TrimEnd(1);
        }
        private static void AssignToSequence(
            int numseq, string puid, string technology, string columnname, string values)
        {
            List<string> candids = new List<string>();
            List<string> netids = new List<string>();
            List<string> neighbors = new List<string>();
            foreach (DataRow row in GetDataTableFromQuery(
                "SELECT netid, neighbors FROM neigh_" +
                technology + "_2_" + puid + ";").Rows)
            {
                netids.Add(row[0].ToString());
                neighbors.Add(row[1].ToString());
            }
            foreach (DataRow row in GetDataTableFromQuery(
                "SELECT netid FROM cand_" +
                technology + "_" + puid + ";").Rows)
            {
                candids.Add(row[0].ToString());
            }
            string[] valuesArray = values.Split(',');
            int idx = 0;
            for (int i = 0; i < numseq; i++)
            {
                List<string> seqNetids = new List<string>();
                List<string> seqValues = new List<string>();
                foreach (DataRow row in GetDataTableFromQuery(
                    "SELECT netid, " + columnname + " FROM seq_" +
                    technology + "_" + puid + " WHERE " +
                    "seqid=" + (i + 1).ToString() + " " +
                    "ORDER BY seqid, seqorder;").Rows)
                {
                    seqNetids.Add(row[0].ToString());
                    seqValues.Add(row[1].ToString());
                }
                //int idx = 0; //bool firstattempt = true;
                for (int j = 0; j < seqNetids.Count; j++)
                {
                    if (candids.Contains(seqNetids[j]))
                    {
                        if (idx == valuesArray.Length)
                        {
                            //firstattempt = false;
                            idx = 0;
                        }
                        string newValue = valuesArray[idx]; idx += 1;
                        int neighborIndex = netids.FindIndex(
                            x => x.Equals(seqNetids[j]));
                        //if (firstattempt == false)
                        //{
                        if (neighborIndex > -1)
                        {
                            int check = 0;
                            bool validNewValue = false;
                            while (!validNewValue)
                            {
                                validNewValue = true;
                                foreach (string neigh in
                                    neighbors[neighborIndex].Split(','))
                                {
                                    int neighIndex = seqNetids.FindIndex(
                                        x => x.Equals(neigh));
                                    string neighValue = seqValues[neighIndex];
                                    if (newValue == neighValue)
                                    {
                                        validNewValue = false;
                                    }
                                }
                                if (validNewValue == false)
                                {
                                    if (idx == valuesArray.Length)
                                    {
                                        idx = 0;
                                    }
                                    newValue = valuesArray[idx]; idx += 1;
                                    check += 1;
                                    if (check == valuesArray.Length)
                                    {
                                        break;
                                    }
                                }
                            }
                        }
                        //}
                        seqValues[j] = newValue;
                    }
                }
                for (int j = 0; j < seqNetids.Count; j++)
                {
                    if (candids.Contains(seqNetids[j]))
                    {
                        ExecuteNonQuery("UPDATE " +
                            "seq_" + technology + "_" + puid + " SET " +
                            columnname + "='" + seqValues[j] + "' WHERE " +
                            "seqid=" + (i + 1).ToString() + " AND " +
                            "netid='" + seqNetids[j] + "';");
                    }
                }
            }
        }
        private static void ClearRadioPlanTempTables(string puid)
        {
            string sql = "";
            sql += "DROP TABLE IF EXISTS net_2g_" + puid + ";";
            sql += "DROP TABLE IF EXISTS net_4g_" + puid + ";";
            sql += "DROP TABLE IF EXISTS cand_2g_" + puid + ";";
            sql += "DROP TABLE IF EXISTS cand_4g_" + puid + ";";
            sql += "DROP TABLE IF EXISTS neigh_2g_1_" + puid + ";";
            sql += "DROP TABLE IF EXISTS neigh_2g_2_" + puid + ";";
            sql += "DROP TABLE IF EXISTS neigh_2g_3_" + puid + ";";
            sql += "DROP TABLE IF EXISTS neigh_2g_4_" + puid + ";";
            sql += "DROP TABLE IF EXISTS neigh_4g_1_" + puid + ";";
            sql += "DROP TABLE IF EXISTS neigh_4g_2_" + puid + ";";
            sql += "DROP TABLE IF EXISTS neigh_4g_3_" + puid + ";";
            sql += "DROP TABLE IF EXISTS neigh_4g_4_" + puid + ";";
            sql += "DROP TABLE IF EXISTS cost_2g_" + puid + ";";
            sql += "DROP TABLE IF EXISTS cost_4g_" + puid + ";";
            sql += "DROP TABLE IF EXISTS seq_2g_" + puid + ";";
            sql += "DROP TABLE IF EXISTS seq_4g_" + puid + ";";
            sql += "DROP TABLE IF EXISTS rfcn_2g_" + puid + ";";
            sql += "DROP TABLE IF EXISTS rfid_2g_" + puid + ";";
            sql += "DROP TABLE IF EXISTS rfid_4g_" + puid + ";";
            ExecuteNonQuery(sql);
        }
        private static void Update2GRfcn(string puid)
        {
            string sql = "";
            sql += "UPDATE pusersnetwork SET rfcn=tmp.rfcn, " +
                "dlfrequency=CAST(890 + (0.2 * " +
                "CAST(tmp.rfcn AS INT)) + 45 AS CHARACTER VARYING), " +
                "ulfrequency=CAST(890 + (0.2 * " +
                "CAST(tmp.rfcn AS INT)) AS CHARACTER VARYING) ";
            sql += "FROM ";
            sql += "(";
            sql += "SELECT uid, siteid, rfcn ";
            sql += "FROM ";
            sql += "seq_2g_" + puid + " INNER JOIN cand_2g_" + puid + " ON ";
            sql += "seq_2g_" + puid + ".netid=cand_2g_" + puid + ".netid ";
            sql += "WHERE ";
            sql += "seq_2g_" + puid + ".seqid=" + BestSequence2GRfcn(puid);
            sql += ")tmp ";
            sql += "WHERE ";
            sql += "pusersnetwork.uid=tmp.uid AND ";
            sql += "pusersnetwork.siteid=tmp.siteid ";
            ExecuteNonQuery(sql);
        }
        private static void Update2GRfid(string puid)
        {
            string sql = "";
            sql += "UPDATE pusersnetwork SET rfid=tmp.rfid ";
            sql += "FROM ";
            sql += "(";
            sql += "SELECT uid, siteid, rfid ";
            sql += "FROM ";
            sql += "seq_2g_" + puid + " INNER JOIN cand_2g_" + puid + " ON ";
            sql += "seq_2g_" + puid + ".netid=cand_2g_" + puid + ".netid ";
            sql += "WHERE ";
            sql += "seq_2g_" + puid + ".seqid=" + BestSequence2GRfid(puid);
            sql += ")tmp ";
            sql += "WHERE ";
            sql += "pusersnetwork.uid=tmp.uid AND ";
            sql += "pusersnetwork.siteid=tmp.siteid ";
            ExecuteNonQuery(sql);
        }
        private static void Update4GRfid(string puid)
        {
            string sql = "";
            sql += "UPDATE pusersnetwork SET rfcn='3450', rfid=tmp.rfid, " +
                "dlfrequency='925', ulfrequency='880' ";
            sql += "FROM ";
            sql += "(";
            sql += "SELECT uid, siteid, rfid ";
            sql += "FROM ";
            sql += "seq_4g_" + puid + " INNER JOIN cand_4g_" + puid + " ON ";
            sql += "seq_4g_" + puid + ".netid=cand_4g_" + puid + ".netid ";
            sql += "WHERE ";
            sql += "seq_4g_" + puid + ".seqid=" + BestSequence4GRfid(puid);
            sql += ")tmp ";
            sql += "WHERE ";
            sql += "pusersnetwork.uid=tmp.uid AND ";
            sql += "pusersnetwork.siteid=tmp.siteid ";
            ExecuteNonQuery(sql);
        }
        private static string BestSequence2GRfcn(string puid)
        {
            string bestSeq = "1";
            foreach (DataRow row in GetDataTableFromQuery(
                "SELECT seqid FROM " +
                "(" +
                "SELECT seqid, SUM(cost) AS cost FROM " +
                "rfcn_2g_" + puid + " GROUP BY seqid " +
                ")tmp ORDER BY cost DESC, seqid LIMIT 1;").Rows)
            {
                bestSeq = row[0].ToString();
            }
            return bestSeq;
        }
        private static string BestSequence2GRfid(string puid)
        {
            string bestSeq = "1";
            foreach (DataRow row in GetDataTableFromQuery(
                "SELECT seqid FROM " +
                "(" +
                "SELECT seqid, SUM(cost) AS cost FROM " +
                "rfid_2g_" + puid + " GROUP BY seqid " +
                ")tmp ORDER BY cost DESC, seqid LIMIT 1;").Rows)
            {
                bestSeq = row[0].ToString();
            }
            return bestSeq;
        }
        private static string BestSequence4GRfid(string puid)
        {
            string bestSeq = "1";
            foreach (DataRow row in GetDataTableFromQuery(
                "SELECT seqid FROM " +
                "(" +
                "SELECT seqid, SUM(cost) AS cost FROM " +
                "rfid_4g_" + puid + " GROUP BY seqid " +
                ")tmp ORDER BY cost DESC, seqid LIMIT 1;").Rows)
            {
                bestSeq = row[0].ToString();
            }
            return bestSeq;
        }
        private static void BuildPairs(string puid)
        {
            string sql = "";
            // build 2g rfcn, 2g rfid, 4g rfid pairs
            // 2g rfcn
            sql += "DROP TABLE IF EXISTS rfcn_2g_" + puid + ";";
            sql += "CREATE TABLE rfcn_2g_" + puid + " AS ";
            sql += "SELECT ta.seqid, ta.netid, CASE WHEN ";
            sql += "tb.cost IS NULL THEN 100000000 ";
            sql += "ELSE cost END AS cost FROM seq_2g_" + puid +
                " ta LEFT JOIN ";
            sql += "(";
            sql += "SELECT seqid, tmp.netid, SUM(CASE WHEN ";
            sql += "neigh_2g_3_" + puid + ".netpair IS NULL THEN cost ";
            sql += "ELSE 0 END) AS cost FROM ";
            sql += "(";
            sql += "SELECT seqid, netid, tmp.netpair, cost FROM ";
            sql += "(";
            sql += "SELECT ta.seqid, ta.netid, " +
                "CONCAT(ta.netid,'_',tb.netid) AS netpair FROM ";
            sql += "seq_2g_" + puid + " ta, seq_2g_" + puid + " tb WHERE ";
            sql += "ta.seqid=tb.seqid AND " +
                "ta.rfcn= tb.rfcn AND ta.netid<>tb.netid";
            sql += ")tmp INNER JOIN cost_2g_" + puid + " ON ";
            sql += "tmp.netpair=cost_2g_" + puid + ".netpair " +
                "ORDER BY seqid, cost";
            sql += ")tmp LEFT JOIN neigh_2g_3_" + puid + " ON ";
            sql += "tmp.netpair=neigh_2g_3_" + puid + ".netpair " +
                "GROUP BY seqid, tmp.netid ORDER BY seqid, tmp.netid";
            sql += ")tb ON ta.seqid=tb.seqid AND ta.netid=tb.netid;";
            // 2g rfid
            sql += "DROP TABLE IF EXISTS rfid_2g_" + puid + ";";
            sql += "CREATE TABLE rfid_2g_" + puid + " AS ";
            sql += "SELECT ta.seqid, ta.netid, CASE WHEN ";
            sql += "tb.cost IS NULL THEN 100000000 ";
            sql += "ELSE cost END AS cost FROM seq_2g_" + puid +
                " ta LEFT JOIN ";
            sql += "(";
            sql += "SELECT seqid, tmp.netid, SUM(CASE WHEN ";
            sql += "neigh_2g_3_" + puid + ".netpair IS NULL THEN cost ";
            sql += "ELSE 0 END) AS cost FROM ";
            sql += "(";
            sql += "SELECT seqid, netid, tmp.netpair, cost FROM ";
            sql += "(";
            sql += "SELECT ta.seqid, ta.netid, " +
                "CONCAT(ta.netid,'_',tb.netid) AS netpair FROM ";
            sql += "seq_2g_" + puid + " ta, seq_2g_" + puid + " tb WHERE ";
            sql += "ta.seqid=tb.seqid AND " +
                "ta.rfid= tb.rfid AND ta.netid<>tb.netid";
            sql += ")tmp INNER JOIN cost_2g_" + puid + " ON ";
            sql += "tmp.netpair=cost_2g_" + puid + ".netpair " +
                "ORDER BY seqid, cost";
            sql += ")tmp LEFT JOIN neigh_2g_3_" + puid + " ON ";
            sql += "tmp.netpair=neigh_2g_3_" + puid + ".netpair " +
                "GROUP BY seqid, tmp.netid ORDER BY seqid, tmp.netid";
            sql += ")tb ON ta.seqid=tb.seqid AND ta.netid=tb.netid;";
            // 4g rfid
            sql += "DROP TABLE IF EXISTS rfid_4g_" + puid + ";";
            sql += "CREATE TABLE rfid_4g_" + puid + " AS ";
            sql += "SELECT ta.seqid, ta.netid, CASE WHEN ";
            sql += "tb.cost IS NULL THEN 100000000 ";
            sql += "ELSE cost END AS cost FROM seq_4g_" + puid +
                " ta LEFT JOIN ";
            sql += "(";
            sql += "SELECT seqid, tmp.netid, SUM(CASE WHEN ";
            sql += "neigh_4g_3_" + puid + ".netpair IS NULL THEN cost ";
            sql += "ELSE 0 END) AS cost FROM ";
            sql += "(";
            sql += "SELECT seqid, netid, tmp.netpair, cost FROM ";
            sql += "(";
            sql += "SELECT ta.seqid, ta.netid, " +
                "CONCAT(ta.netid,'_',tb.netid) AS netpair FROM ";
            sql += "seq_4g_" + puid + " ta, seq_4g_" + puid + " tb WHERE ";
            sql += "ta.seqid=tb.seqid AND " +
                "ta.rfid= tb.rfid AND ta.netid<>tb.netid";
            sql += ")tmp INNER JOIN cost_4g_" + puid + " ON ";
            sql += "tmp.netpair=cost_4g_" + puid + ".netpair " +
                "ORDER BY seqid, cost";
            sql += ")tmp LEFT JOIN neigh_4g_3_" + puid + " ON ";
            sql += "tmp.netpair=neigh_4g_3_" + puid + ".netpair " +
                "GROUP BY seqid, tmp.netid ORDER BY seqid, tmp.netid";
            sql += ")tb ON ta.seqid=tb.seqid AND ta.netid=tb.netid;";
            // add neigbor of neigbor cost factor
            // 2g rfcn
            sql += "UPDATE rfcn_2g_" + puid + " SET " +
                "cost=cost/cost_factor FROM (";
            sql += "SELECT ta.seqid, ta.netid, " +
                "COUNT(ta.rfcn)+1 AS cost_factor FROM ";
            sql += "seq_2g_" + puid + " ta LEFT JOIN " +
                "neigh_2g_4_" + puid + " tb ON ";
            sql += "ta.netid=tb.netid ";
            sql += "INNER JOIN seq_2g_" + puid + " tc ON ";
            sql += "tb.neighofneigh=tc.netid AND ";
            sql += "ta.seqid=tc.seqid AND ";
            sql += "ta.rfcn=tc.rfcn ";
            sql += "WHERE tb.neighofneigh <> ta.netid ";
            sql += "GROUP BY ta.seqid, ta.netid";
            sql += ")tmp WHERE rfcn_2g_" + puid + ".netid=tmp.netid AND " +
                "rfcn_2g_" + puid + ".seqid=tmp.seqid;";
            // 2g rfid
            sql += "UPDATE rfid_2g_" + puid + " SET " +
                "cost=cost/cost_factor FROM (";
            sql += "SELECT ta.seqid, ta.netid, " +
                "COUNT(ta.rfid)+1 AS cost_factor FROM ";
            sql += "seq_2g_" + puid + " ta LEFT JOIN " +
                "neigh_2g_4_" + puid + " tb ON ";
            sql += "ta.netid=tb.netid ";
            sql += "INNER JOIN seq_2g_" + puid + " tc ON ";
            sql += "tb.neighofneigh=tc.netid AND ";
            sql += "ta.seqid=tc.seqid AND ";
            sql += "ta.rfid=tc.rfid ";
            sql += "WHERE tb.neighofneigh <> ta.netid ";
            sql += "GROUP BY ta.seqid, ta.netid";
            sql += ")tmp WHERE rfid_2g_" + puid + ".netid=tmp.netid AND " +
                "rfid_2g_" + puid + ".seqid=tmp.seqid;";
            // 4g rfcn
            sql += "UPDATE rfid_4g_" + puid + " SET " +
                "cost=cost/cost_factor FROM (";
            sql += "SELECT ta.seqid, ta.netid, " +
                "COUNT(ta.rfid)+1 AS cost_factor FROM ";
            sql += "seq_4g_" + puid + " ta LEFT JOIN " +
                "neigh_4g_4_" + puid + " tb ON ";
            sql += "ta.netid=tb.netid ";
            sql += "INNER JOIN seq_4g_" + puid + " tc ON ";
            sql += "tb.neighofneigh=tc.netid AND ";
            sql += "ta.seqid=tc.seqid AND ";
            sql += "ta.rfid=tc.rfid ";
            sql += "WHERE tb.neighofneigh <> ta.netid ";
            sql += "GROUP BY ta.seqid, ta.netid";
            sql += ")tmp WHERE rfid_4g_" + puid + ".netid=tmp.netid AND " +
                "rfid_4g_" + puid + ".seqid=tmp.seqid;";
            ExecuteNonQuery(sql);
        }
        private static void CreateSequences(int numseq, string puid)
        {
            string sql = "";
            // 2g
            sql += "DROP TABLE IF EXISTS seq_2g_" + puid + ";";
            sql += "CREATE TABLE seq_2g_" + puid + " AS ";
            sql += "SELECT GENERATE_SERIES(1," + numseq.ToString() + ") AS seqid, " +
                "ROUND(RANDOM()*(2147483648 - 1) + 1)::INT as seqorder," +
                "ta.netid, ta.rfcn, ta.rfid FROM net_2g_" + puid +
                " ta ORDER BY seqid, seqorder;";
            // 4g
            sql += "DROP TABLE IF EXISTS seq_4g_" + puid + ";";
            sql += "CREATE TABLE seq_4g_" + puid + " AS ";
            sql += "SELECT GENERATE_SERIES(1," + numseq.ToString() + ") AS seqid, " +
                "ROUND(RANDOM()*(2147483648 - 1) + 1)::INT as seqorder," +
                "ta.netid, ta.rfcn, ta.rfid FROM net_4g_" + puid +
                " ta ORDER BY seqid, seqorder;";
            ExecuteNonQuery(sql);
        }
        private static void CreateCost(string puid)
        {
            string sql = "";
            // 2g
            sql += "DROP TABLE IF EXISTS cost_2g_" + puid + ";";
            sql += "CREATE TABLE cost_2g_" + puid + " AS ";
            sql += "SELECT CONCAT(ta.netid,'_',tb.netid) AS netpair, ";
            sql += "ST_Distance(Geography(ta.wkb_geometry)," +
                "Geography(tb.wkb_geometry),TRUE) AS cost ";
            sql += "FROM net_2g_" + puid + " ta, net_2g_" + puid + " tb ";
            sql += "WHERE ST_Distance(Geography(ta.wkb_geometry)," +
                "Geography(tb.wkb_geometry),TRUE)>0 ";
            sql += "ORDER BY CONCAT(ta.netid,'_',tb.netid);";
            // 4g
            sql += "DROP TABLE IF EXISTS cost_4g_" + puid + ";";
            sql += "CREATE TABLE cost_4g_" + puid + " AS ";
            sql += "SELECT CONCAT(ta.netid,'_',tb.netid) AS netpair, ";
            sql += "ST_Distance(Geography(ta.wkb_geometry)," +
                "Geography(tb.wkb_geometry),TRUE) AS cost ";
            sql += "FROM net_4g_" + puid + " ta, net_4g_" + puid + " tb ";
            sql += "WHERE ST_Distance(Geography(ta.wkb_geometry)," +
                "Geography(tb.wkb_geometry),TRUE)>0 ";
            sql += "ORDER BY CONCAT(ta.netid,'_',tb.netid);";
            ExecuteNonQuery(sql);
        }
        private static string GetPotentialSites(string puid, string sites)
        {
            string potentialSites = "";
            foreach (DataRow row in GetDataTableFromQuery(
                "SELECT siteid FROM " +
                "pusersnetwork WHERE uid='" + puid + "' AND " +
                "dlfrequency='' AND " +
                BuildWhere(sites, "siteid") + ";").Rows)
            {
                potentialSites += row[0].ToString() + ",";
            }
            potentialSites = potentialSites.TrimEnd(1);
            return potentialSites;
        }
        private static void CreateNet(string puid, string sites)
        {
            string sql = "";
            // build where
            string sitesWhere = BuildWhere(sites, "siteid");
            string NotSitesWhere = BuildWhere(sites, "siteid", false);
            // get center to bottom left distance
            Coords center = GetExtent(
                puid, sites, "siteid", "pusersnetwork", Extent.Center);
            Coords bottomLeft = GetExtent(
                puid, sites, "siteid", "pusersnetwork", Extent.BottomLeft);
            double within50KmsOfSites = Math.Min(
                gisext.Distance(center, bottomLeft) + 50000, 100000);
            // create candidates
            //2g
            sql += "DROP TABLE IF EXISTS cand_2g_" + puid + ";";
            sql += "CREATE TABLE cand_2g_" + puid + " AS ";
            sql += "SELECT CONCAT(uid,'_',siteid) AS netid, uid, siteid FROM ";
            sql += "pusersnetwork WHERE uid='" + puid +
                "' AND technology='2G' AND " + sitesWhere + ";";
            //4g
            sql += "DROP TABLE IF EXISTS cand_4g_" + puid + ";";
            sql += "CREATE TABLE cand_4g_" + puid + " AS ";
            sql += "SELECT CONCAT(uid,'_',siteid) AS netid, uid, siteid FROM ";
            sql += "pusersnetwork WHERE uid='" + puid +
                "' AND technology='4G' AND " + sitesWhere + ";";
            ExecuteNonQuery(sql);
            // clear candidates
            foreach (DataRow row in GetDataTableFromQuery(
                "SELECT uid, siteid FROM cand_2g_" + puid + ";").Rows)
            {
                ExecuteNonQuery("UPDATE " +
                    "pusersnetwork SET " +
                    "rfid='', rfcn='' WHERE " +
                    "uid='" + row[0].ToString() + "' AND " +
                    "siteid='" + row[1].ToString() + "';");
            }
            foreach (DataRow row in GetDataTableFromQuery(
                "SELECT uid, siteid FROM cand_4g_" + puid + ";").Rows)
            {
                ExecuteNonQuery("UPDATE " +
                    "pusersnetwork SET " +
                    "rfid='', rfcn='' WHERE " +
                    "uid='" + row[0].ToString() + "' AND " +
                    "siteid='" + row[1].ToString() + "';");
            }
            // create net
            // 2g
            sql = "DROP TABLE IF EXISTS net_2g_" + puid + ";";
            sql += "CREATE TABLE net_2g_" + puid + " AS ";
            sql += "SELECT * FROM (";
            sql += "SELECT CONCAT(uid,'_',siteid) AS netid, " +
                "rfcn, rfid, wkb_geometry FROM ";
            sql += "pusersnetwork WHERE uid='" + puid + "' AND " +
                "technology='2G' AND " + sitesWhere + " UNION ";
            sql += "SELECT CONCAT(uid,'_',siteid) AS netid, " +
                "rfcn, rfid, wkb_geometry FROM ";
            sql += "pusersnetwork WHERE technology='2G' AND " +
                "rfcn<>'' AND rfid<>'' AND ST_Distance(" +
                "Geography(ST_GeomFromText('POINT(" +
                center.X.ToString() + " " +
                center.Y.ToString() + ")',4326))," +
                "Geography(wkb_geometry),TRUE) <= " +
                within50KmsOfSites.ToString() + " AND " +
                NotSitesWhere + ")tmp;";
            // 4g
            sql += "DROP TABLE IF EXISTS net_4g_" + puid + ";";
            sql += "CREATE TABLE net_4g_" + puid + " AS ";
            sql += "SELECT * FROM (";
            sql += "SELECT CONCAT(uid,'_',siteid) AS netid, " +
                "rfcn, rfid, wkb_geometry FROM ";
            sql += "pusersnetwork WHERE uid='" + puid + "' AND " +
                "technology='4G' AND " + sitesWhere + " UNION ";
            sql += "SELECT CONCAT(uid,'_',siteid) AS netid, " +
                "rfcn, rfid, wkb_geometry FROM ";
            sql += "pusersnetwork WHERE technology='4G' AND " +
                "rfcn<>'' AND rfid<>'' AND ST_Distance(" +
                "Geography(ST_GeomFromText('POINT(" +
                center.X.ToString() + " " +
                center.Y.ToString() + ")',4326))," +
                "Geography(wkb_geometry),TRUE) <= " +
                within50KmsOfSites.ToString() + " AND " +
                NotSitesWhere + ")tmp;";
            ExecuteNonQuery(sql);
        }
        private static void CreateNeighbors(string puid)
        {
            string sql = "";
            // 2g
            sql += "DROP TABLE IF EXISTS deltri2g" + puid + ";";
            sql += "CREATE TABLE deltri2g" + puid + " AS ";
            sql += "SELECT (dt).path[1] AS id, ";
            sql += "ST_ExteriorRing((dt).Geom) AS wkb_geometry ";
            sql += "FROM";
            sql += "(";
            sql += "SELECT ST_Dump(ST_DelaunayTriangles(";
            sql += "ST_Collect(wkb_geometry))) AS dt ";
            sql += "FROM net_2g_" + puid;
            sql += ")tmp;";
            sql += "DROP TABLE IF EXISTS deltri_inter2g" + puid + ";";
            sql += "CREATE TABLE deltri_inter2g" + puid + " AS ";
            sql += "SELECT id, netid ";
            sql += "FROM net_2g_" + puid + ", deltri2g" + puid + " ";
            sql += "WHERE ";
            sql += "ST_Intersects(";
            sql += "deltri2g" + puid + ".wkb_geometry,";
            sql += "net_2g_" + puid + ".wkb_geometry) ";
            sql += "ORDER BY netid;";
            sql += "DROP TABLE IF EXISTS neigh_2g_1_" + puid + ";";
            sql += "CREATE TABLE neigh_2g_1_" + puid + " AS ";
            sql += "SELECT ";
            sql += "deltri_inter2g" + puid + ".netid AS netid, ";
            sql += "deltri_inter_n2g" + puid + ".netid AS neighbor ";
            sql += "FROM deltri_inter2g" + puid + " LEFT JOIN ";
            sql += "deltri_inter2g" + puid + " ";
            sql += "deltri_inter_n2g" + puid + " ON ";
            sql += "deltri_inter_n2g" + puid + ".id=";
            sql += "deltri_inter2g" + puid + ".id ";
            sql += "WHERE ";
            sql += "deltri_inter2g" + puid + ".netid<>";
            sql += "deltri_inter_n2g" + puid + ".netid ";
            sql += "GROUP BY ";
            sql += "deltri_inter2g" + puid + ".netid, ";
            sql += "deltri_inter_n2g" + puid + ".netid ";
            sql += "ORDER BY ";
            sql += "deltri_inter2g" + puid + ".netid;";
            sql += "DROP TABLE IF EXISTS neigh_2g_2_" + puid + ";";
            sql += "CREATE TABLE neigh_2g_2_" + puid + " AS ";
            sql += "SELECT netid, ";
            sql += "STRING_AGG(neighbor, ',') AS neighbors ";
            sql += "FROM neigh_2g_1_" + puid + " ";
            sql += "GROUP BY 1 ";
            sql += "ORDER BY netid;";
            sql += "DROP TABLE IF EXISTS neigh_2g_3_" + puid + ";";
            sql += "CREATE TABLE neigh_2g_3_" + puid + " AS ";
            sql += "SELECT netid, neighbor, " +
                "CONCAT(netid,'_',neighbor) AS netpair ";
            sql += "FROM neigh_2g_1_" + puid + ";";
            sql += "DROP TABLE IF EXISTS neigh_2g_4_" + puid + ";";
            sql += "CREATE TABLE neigh_2g_4_" + puid + " AS ";
            sql += "SELECT DISTINCT ta.netid, " +
                "tb.neighbor AS neighofneigh FROM ";
            sql += "neigh_2g_3_" + puid + " ta, neigh_2g_3_" + puid +
                " tb WHERE ta.neighbor=tb.netid;";
            sql += "DROP TABLE IF EXISTS deltri2g" + puid + ";";
            sql += "DROP TABLE IF EXISTS deltri_inter2g" + puid + ";";
            // 4g
            sql += "DROP TABLE IF EXISTS deltri4g" + puid + ";";
            sql += "CREATE TABLE deltri4g" + puid + " AS ";
            sql += "SELECT (dt).path[1] AS id, ";
            sql += "ST_ExteriorRing((dt).Geom) AS wkb_geometry ";
            sql += "FROM";
            sql += "(";
            sql += "SELECT ST_Dump(ST_DelaunayTriangles(";
            sql += "ST_Collect(wkb_geometry))) AS dt ";
            sql += "FROM net_4g_" + puid;
            sql += ")tmp;";
            sql += "DROP TABLE IF EXISTS deltri_inter4g" + puid + ";";
            sql += "CREATE TABLE deltri_inter4g" + puid + " AS ";
            sql += "SELECT id, netid ";
            sql += "FROM net_4g_" + puid + ", deltri4g" + puid + " ";
            sql += "WHERE ";
            sql += "ST_Intersects(";
            sql += "deltri4g" + puid + ".wkb_geometry,";
            sql += "net_4g_" + puid + ".wkb_geometry) ";
            sql += "ORDER BY netid;";
            sql += "DROP TABLE IF EXISTS neigh_4g_1_" + puid + ";";
            sql += "CREATE TABLE neigh_4g_1_" + puid + " AS ";
            sql += "SELECT ";
            sql += "deltri_inter4g" + puid + ".netid AS netid, ";
            sql += "deltri_inter_n4g" + puid + ".netid AS neighbor ";
            sql += "FROM deltri_inter4g" + puid + " LEFT JOIN ";
            sql += "deltri_inter4g" + puid + " ";
            sql += "deltri_inter_n4g" + puid + " ON ";
            sql += "deltri_inter_n4g" + puid + ".id=";
            sql += "deltri_inter4g" + puid + ".id ";
            sql += "WHERE ";
            sql += "deltri_inter4g" + puid + ".netid<>";
            sql += "deltri_inter_n4g" + puid + ".netid ";
            sql += "GROUP BY ";
            sql += "deltri_inter4g" + puid + ".netid, ";
            sql += "deltri_inter_n4g" + puid + ".netid ";
            sql += "ORDER BY ";
            sql += "deltri_inter4g" + puid + ".netid;";
            sql += "DROP TABLE IF EXISTS neigh_4g_2_" + puid + ";";
            sql += "CREATE TABLE neigh_4g_2_" + puid + " AS ";
            sql += "SELECT netid, ";
            sql += "STRING_AGG(neighbor, ',') AS neighbors ";
            sql += "FROM neigh_4g_1_" + puid + " ";
            sql += "GROUP BY 1 ";
            sql += "ORDER BY netid;";
            sql += "DROP TABLE IF EXISTS neigh_4g_3_" + puid + ";";
            sql += "CREATE TABLE neigh_4g_3_" + puid + " AS ";
            sql += "SELECT netid, neighbor, " +
                "CONCAT(netid,'_',neighbor) AS netpair ";
            sql += "FROM neigh_4g_1_" + puid + ";";
            sql += "DROP TABLE IF EXISTS neigh_4g_4_" + puid + ";";
            sql += "CREATE TABLE neigh_4g_4_" + puid + " AS ";
            sql += "SELECT DISTINCT ta.netid, " +
                "tb.neighbor AS neighofneigh FROM ";
            sql += "neigh_4g_3_" + puid + " ta, neigh_4g_3_" + puid +
                " tb WHERE ta.neighbor=tb.netid;";
            sql += "DROP TABLE IF EXISTS deltri4g" + puid + ";";
            sql += "DROP TABLE IF EXISTS deltri_inter4g" + puid + ";";
            ExecuteNonQuery(sql);
        }
    }
}