using System;
using System.Collections.Generic;
using static cnip.Models.generic;
using static cnip.Models.strext;

namespace forecast.Models
{
    class Analysis
    {

        public static void PredictCoverage(
            string puid, string tempfol, string resultfol, string resultid, string sites)
        {
            try
            {
                // predict height and power for sites if not assigned
                Sites.PredictHeightAndPower(puid, sites);
                // predict radio plan if not assigned
                RadioPlan.Forecast(puid, tempfol, sites, true);
                // run prediction
                Coverage.Forecast(puid, tempfol, resultfol, resultid, sites,
                    "", "", true, true);
                // clear temp path
                ClearTempPath(tempfol);
            }
            catch (Exception)
            {
                ClearTempPath(tempfol);
            }
        }
        public static void PredictSites(
            string puid, string tempfol, string resultfol, string resultid, string polygonid, string technology)
        {
            try
            {
                // predict sites
                string sites = Sites.PredictSites(puid, polygonid, technology);
                // predict radio plan
                RadioPlan.Forecast(puid, tempfol, sites, true);
                // predict links
                Links.Forecast(puid, tempfol, sites, true);
                // run prediction
                Coverage.InsertResult(puid, resultid, "P" + resultid +
                    ": Predict Site Analysis",
                Coverage.Forecast(puid, tempfol, resultfol, resultid, sites,
                    polygonid, "Received Power (dBm)", true));
                // clear temp path
                ClearTempPath(tempfol);
            }
            catch (Exception)
            {
                ClearTempPath(tempfol);
            }
        }
        public static void BestCandidate(
            string puid, string tempfol, string resultfol, string resultid, string sites, string polygonid)
        {
            try
            {
                // predict height and power for candidates if not assigned
                Sites.PredictHeightAndPower(puid, sites);
                // predict radio plan for sites if not assigned
                RadioPlan.Forecast(puid, tempfol, sites, true);
                // get base result id
                int baseid = resultid.ToInt();
                // run predictions for each site
                List<string> resultStrings = new List<string>();
                foreach (string siteid in sites.Split(','))
                {
                    resultStrings.Add(Coverage.Forecast(puid, tempfol, resultfol,
                        baseid.ToString(), siteid, polygonid, "Received Power (dBm)", true));
                    baseid++;
                }
                // combine results for all sites
                string polygonsString = "";
                string thematicString = ""; string pngString = "";
                foreach (string resultString in resultStrings)
                {
                    string[] result = resultString.Split('>');
                    thematicString = result[0];
                    pngString = result[1];
                    polygonsString += result[2] + "bc" + '@';
                }
                polygonsString = polygonsString.TrimEnd(1);
                // update results
                Coverage.InsertResult(puid, resultid, "P" + resultid +
                    ": Best Candidate Analysis", thematicString + ">" + pngString + ">" + polygonsString);
                // clear temp path
                ClearTempPath(tempfol);
            }
            catch (Exception)
            {
                ClearTempPath(tempfol);
            }
        }
    }
}
