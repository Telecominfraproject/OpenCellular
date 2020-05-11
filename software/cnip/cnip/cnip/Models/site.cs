using System;
using System.Data;
using static cnip.Models.gisext;
using static cnip.Models.settings;
using static cnip.Models.pgsql;
using static cnip.Models.generic;

namespace cnip.Models
{
    public static class site
    {
        public struct Site
        {
            public string Puid { get; set; }
            public string Siteid { get; set; }
            public string SiteName { get; set; }
            public string Latitude { get; set; }
            public string Longitude { get; set; }
            public string HBA { get; set; }
            public string Frequency { get; set; }
            public string Polarization { get; set; }
            public string PolarizationNumber { get; set; }
            public string EIRP { get; set; }
            public string AntennaType { get; set; }
            public string AntennaModel { get; set; }
            public double AntennaGain { get; set; }
            public double FeederLoss { get; set; }
            public double RfPower { get; set; }
            public string Azimuth { get; set; }
            public string DownTilt { get; set; }
            public double Bandwidth { get; set; }
            public string SnrFactor { get; set; }
            public string Technology { get; set; }
            public string Height { get; set; }
            public Coords Location;

            public Site(string puid, string siteid)
            {
                Settings settings = new Settings(puid);
                Puid = "";
                Siteid = "";
                SiteName = "";
                Latitude = "";
                Longitude = "";
                HBA = "";
                Frequency = "";
                Polarization = "";
                PolarizationNumber = "";
                EIRP = "";
                AntennaType = "";
                AntennaModel = "";
                AntennaGain = 0;
                FeederLoss = 0;
                RfPower = 0;
                Azimuth = "";
                DownTilt = "";
                Bandwidth = 0;
                SnrFactor = "";
                Technology = "";
                Height = "";
                Location = new Coords(0, 0);
                foreach (DataRow row in GetDataTableFromQuery("SELECT " +
                    "siteid, sitename, latitude, longitude*-1, " +
                    "hba, dlfrequency, polarization, rfpower, " +
                    "antennatype, azimuth, downtilt, antennamodel, " +
                    "antennagain, feederloss, bandwidth, technology, height " +
                    "FROM pusersnetwork WHERE uid = '" + puid + "' AND " +
                    "siteid = '" + siteid + "';").Rows)
                {
                    Puid = puid;
                    Siteid = row[0].ToString();
                    SiteName = row[1].ToString();
                    Latitude = row[2].ToString();
                    Longitude = row[3].ToString();
                    HBA = row[4].ToString() + " m ";
                    Frequency = row[5].ToString();
                    Polarization = row[6].ToString();
                    RfPower = row[7].ToString().ToDouble();
                    AntennaType = row[8].ToString().ToLower();
                    Azimuth = row[9].ToString();
                    DownTilt = row[10].ToString();
                    AntennaModel = row[11].ToString();
                    AntennaGain = row[12].ToString().ToDouble();
                    FeederLoss = row[13].ToString().ToDouble();
                    Bandwidth = row[14].ToString().ToDouble() * 1000000; // unit hertz
                    Technology = row[15].ToString();
                    Height = row[16].ToString();
                    // dependent variables , calculated
                    PolarizationNumber =
                        Polarization.ToLower() == "vertical" ? "1" : "0";
                    // rfpower (W) + antennagain (dBi) - feederloss (dB) + recievergain (dBi)
                    EIRP = DbmToWatt(WattToDbm(RfPower) + AntennaGain - FeederLoss +
                        settings.Pl_ReceiverGain.ToDouble()).ToString();
                    double eirpdbW = WattToDbm(EIRP.ToDouble()) - 30;
                    double noisedbW = -174 + (10 * Math.Log10(Bandwidth)) - 30;
                    SnrFactor = (eirpdbW - noisedbW).ToString();
                    Location = new Coords(Longitude.ToDouble() * -1, Latitude.ToDouble());
                }
            }
            public Site(
                string puid,
                string siteid,
                Coords location)
            {
                Puid = puid;
                Siteid = siteid;
                Location = location;
                SiteName = "";
                Latitude = "";
                Longitude = "";
                HBA = "";
                Frequency = "";
                Polarization = "";
                PolarizationNumber = "";
                EIRP = "";
                AntennaType = "";
                AntennaModel = "";
                AntennaGain = 0;
                FeederLoss = 0;
                RfPower = 0;
                Azimuth = "";
                DownTilt = "";
                Bandwidth = 0;
                SnrFactor = "";
                Technology = "";
                Height = "";
            }
            public Site(
                string puid,
                string siteid,
                double x,
                double y)
            {
                Puid = puid;
                Siteid = siteid;
                Location = new Coords(x, y);
                SiteName = "";
                Latitude = "";
                Longitude = "";
                HBA = "";
                Frequency = "";
                Polarization = "";
                PolarizationNumber = "";
                EIRP = "";
                AntennaType = "";
                AntennaModel = "";
                AntennaGain = 0;
                FeederLoss = 0;
                RfPower = 0;
                Azimuth = "";
                DownTilt = "";
                Bandwidth = 0;
                SnrFactor = "";
                Technology = "";
                Height = "";
            }
        }
    }
}
