using System.Data;
using static cnip.Models.pgsql;

namespace cnip.Models
{
    class settings
    {
        public struct Settings
        {

            public string Pl_MeasurementType { get; set; }
            public string Pl_TerrainResolution { get; set; }
            public string Pl_Thematic { get; set; }
            public string Pl_Radius { get; set; }
            public string Pl_PropagationModel { get; set; }
            public string Pl_ReliabilityS { get; set; }
            public string Pl_ReliabilityT { get; set; }
            public string Pl_TerrainConductivity { get; set; }
            public string Pl_RadioClimate { get; set; }
            public string Pl_ReceiverHeight { get; set; }
            public string Pl_ReceiverGain { get; set; }
            public string Pl_ReceiverSensitivity { get; set; }
            public string Rp_AntennaModel { get; set; }
            public string Rp_CellRadius { get; set; }
            public string Rp_GSMBand { get; set; }
            public string Rp_GSMBandwidth { get; set; }
            public string Rp_LTEBand { get; set; }
            public string Rp_LTEBandwidth { get; set; }
            public string Mw_Frequency { get; set; }
            public string Mw_Channelwidth { get; set; }
            public string Mw_OutputPower { get; set; }
            public string Mw_AntennaGain { get; set; }
            public string Mw_Losses { get; set; }
            public string Mw_FresnelClearance { get; set; }
            public string EarthDielectricConstant { get; set; }
            public string EarthConductivity { get; set; }
            public string RadioClimateNumber { get; set; }
            public string AtmosphericBendingConstant { get; set; }
            public string ThematicBaseType { get; set; }
            public string ThematicExtension { get; set; }
            public string Resolution { get; set; }
            public double XPixelSize { get; set; }
            public double YPixelSize { get; set; }

            public void SetPl_MeasurementType(string pl_measurementType)
            {
                Pl_MeasurementType = pl_measurementType;
                ThematicBaseType =
                    Pl_MeasurementType == "Received Power (dBm)" ? "trp" :
                    Pl_MeasurementType == "Field Strength (dBµV/m)" ? "tfs" : "tpl";
                ThematicExtension =
                    Pl_MeasurementType == "Received Power (dBm)" ? "trp" :
                    Pl_MeasurementType == "Field Strength (dBµV/m)" ? "tfs" :
                    Pl_MeasurementType == "Path Loss (dB)" ? "tfs" :
                    Pl_MeasurementType == "SNR (dB)" ? "tsn" :
                    Pl_MeasurementType == "Voice Quality Score" ? "tvq" :
                    Pl_MeasurementType == "Data Rate Score" ? "tdr" : "tpl";
            }

            public Settings(string puid)
            {
                // default values
                Pl_MeasurementType = "Received Power (dBm)";
                Pl_TerrainResolution = "90m";
                Pl_Thematic = "default";
                Pl_Radius = "1.5";
                Pl_PropagationModel = "ITWOM 3.0 (< 20GHz)";
                Pl_ReliabilityS = "50";
                Pl_ReliabilityT = "90";
                Pl_TerrainConductivity = "Average ground";
                Pl_RadioClimate = "Maritime Temperate (Land)";
                Pl_ReceiverHeight = "1.5";
                Pl_ReceiverGain = "2.14";
                Pl_ReceiverSensitivity = "-90";
                Rp_AntennaModel = "LT OD9-5 890-950 MHz";
                Rp_CellRadius = "1.5";
                Rp_GSMBand = "GSM900 PGSM";
                Rp_GSMBandwidth = "0.2";
                Rp_LTEBand = "B08_FDD_900 E-GSM";
                Rp_LTEBandwidth = "10";
                Mw_Frequency = "5";
                Mw_Channelwidth = "20";
                Mw_OutputPower = "47";
                Mw_AntennaGain = "22.5";
                Mw_Losses = "0";
                Mw_FresnelClearance = "60";
                // dependent variables
                Resolution = Pl_TerrainResolution == "90m" ? "sd" : "hd";
                XPixelSize = Pl_TerrainResolution == "90m" ?
                    0.0008333333333333333868 : 0.0002777777777777777775;
                YPixelSize = Pl_TerrainResolution == "90m" ?
                    -0.0008333333333333333868 : -0.0002777777777777777775;
                ThematicBaseType =
                    Pl_MeasurementType == "Received Power (dBm)" ? "trp" :
                    Pl_MeasurementType == "Field Strength (dBµV/m)" ? "tfs" : "tpl";
                ThematicExtension =
                    Pl_MeasurementType == "Received Power (dBm)" ? "trp" :
                    Pl_MeasurementType == "Field Strength (dBµV/m)" ? "tfs" :
                    Pl_MeasurementType == "Path Loss (dB)" ? "tfs" :
                    Pl_MeasurementType == "SNR (dB)" ? "tsn" :
                    Pl_MeasurementType == "Voice Quality Score" ? "tvq" :
                    Pl_MeasurementType == "Data Rate Score" ? "tdr" : "tpl";
                EarthDielectricConstant = EarthDielectricConstant(Pl_TerrainConductivity);
                EarthConductivity = EarthConductivity(Pl_TerrainConductivity);
                RadioClimateNumber = RadioClimateNumber(Pl_RadioClimate);
                AtmosphericBendingConstant = AtmosphericBendingConstant(Pl_RadioClimate);

                foreach (DataRow row in GetDataTableFromQuery("SELECT " +
                    "pl_measurementtype, pl_terrainresolution, pl_thematic, " +
                    "pl_radius, pl_propagationmodel, pl_reliabilitys, " +
                    "pl_reliabilityt, pl_terrainconductivity, pl_radioclimate, " +
                    "pl_receiverheight, pl_receivergain, pl_receiversensitivity, " +
                    "rp_antennamodel, rp_cellradius, rp_gsmband, " +
                    "rp_gsmbandwidth, rp_lteband, rp_ltebandwidth, " +
                    "mw_frequency, mw_channelwidth, mw_outputpower, " +
                    "mw_antennagain, mw_losses, mw_fresnelclearance " +
                    "FROM puserssettings WHERE uid='" + puid + "';").Rows)
                {
                    Pl_MeasurementType = row[0].ToString();
                    Pl_TerrainResolution = row[1].ToString();
                    Pl_Thematic = row[2].ToString();
                    Pl_Radius = row[3].ToString();
                    Pl_PropagationModel = row[4].ToString();
                    Pl_ReliabilityS = row[5].ToString();
                    Pl_ReliabilityT = row[6].ToString();
                    Pl_TerrainConductivity = row[7].ToString();
                    Pl_RadioClimate = row[8].ToString();
                    Pl_ReceiverHeight = row[9].ToString();
                    Pl_ReceiverGain = row[10].ToString();
                    Pl_ReceiverSensitivity = row[11].ToString();
                    Rp_AntennaModel = row[12].ToString();
                    Rp_CellRadius = row[13].ToString();
                    Rp_GSMBand = row[14].ToString();
                    Rp_GSMBandwidth = row[15].ToString();
                    Rp_LTEBand = row[16].ToString();
                    Rp_LTEBandwidth = row[17].ToString();
                    Mw_Frequency = row[18].ToString();
                    Mw_Channelwidth = row[19].ToString();
                    Mw_OutputPower = row[20].ToString();
                    Mw_AntennaGain = row[21].ToString();
                    Mw_Losses = row[22].ToString();
                    Mw_FresnelClearance = row[23].ToString();
                    // dependent variables
                    Resolution = Pl_TerrainResolution == "90m" ? "sd" : "hd";
                    XPixelSize = Pl_TerrainResolution == "90m" ?
                        0.0008333333333333333868 : 0.0002777777777777777775;
                    YPixelSize = Pl_TerrainResolution == "90m" ?
                        -0.0008333333333333333868 : -0.0002777777777777777775;
                    ThematicBaseType =
                        Pl_MeasurementType == "Received Power (dBm)" ? "trp" :
                        Pl_MeasurementType == "Field Strength (dBµV/m)" ? "tfs" : "tpl";
                    ThematicExtension =
                        Pl_MeasurementType == "Received Power (dBm)" ? "trp" :
                        Pl_MeasurementType == "Field Strength (dBµV/m)" ? "tfs" :
                        Pl_MeasurementType == "Path Loss (dB)" ? "tfs" :
                        Pl_MeasurementType == "SNR (dB)" ? "tsn" :
                        Pl_MeasurementType == "Voice Quality Score" ? "tvq" :
                        Pl_MeasurementType == "Data Rate Score" ? "tdr" : "tpl";
                    EarthDielectricConstant = EarthDielectricConstant(Pl_TerrainConductivity);
                    EarthConductivity = EarthConductivity(Pl_TerrainConductivity);
                    RadioClimateNumber = RadioClimateNumber(Pl_RadioClimate);
                    AtmosphericBendingConstant = AtmosphericBendingConstant(Pl_RadioClimate);
                }
            }
        }
        public static string EarthDielectricConstant(string terrainConductivity)
        {
            string earthDielectricConstant = "";
            switch (terrainConductivity)
            {
                case "Salt water":
                    earthDielectricConstant = "80.000"; break;
                case "Good ground":
                    earthDielectricConstant = "25.000"; break;
                case "Fresh water":
                    earthDielectricConstant = "80.000"; break;
                case "Marshy land":
                    earthDielectricConstant = "12.000"; break;
                case "Farm land":
                    earthDielectricConstant = "15.000"; break;
                case "Forest":
                    earthDielectricConstant = "15.000"; break;
                case "Average ground":
                    earthDielectricConstant = "15.000"; break;
                case "Mountain / Sand":
                    earthDielectricConstant = "13.000"; break;
                case "City":
                    earthDielectricConstant = "05.000"; break;
                case "Poor ground":
                    earthDielectricConstant = "04.000"; break;
                default: earthDielectricConstant = "15.000"; break;
            }
            return earthDielectricConstant;
        }
        public static string EarthConductivity(string terrainConductivity)
        {
            string earthConductivity = "";
            switch (terrainConductivity)
            {
                case "Salt water":
                    earthConductivity = "5.000"; break;
                case "Good ground":
                    earthConductivity = "0.020"; break;
                case "Fresh water":
                    earthConductivity = "0.010"; break;
                case "Marshy land":
                    earthConductivity = "0.007"; break;
                case "Farm land":
                    earthConductivity = "0.005"; break;
                case "Forest":
                    earthConductivity = "0.005"; break;
                case "Average ground":
                    earthConductivity = "0.005"; break;
                case "Mountain / Sand":
                    earthConductivity = "0.002"; break;
                case "City":
                    earthConductivity = "0.001"; break;
                case "Poor ground":
                    earthConductivity = "0.001"; break;
                default: earthConductivity = "0.005"; break;
            }
            return earthConductivity;
        }

        public static string RadioClimateNumber(string radioClimate)
        {
            string radioClimateNumber = "";
            switch (radioClimate)
            {
                case "Equatorial (Congo)":
                    radioClimateNumber = "1"; break;
                case "Continental Subtropical (Sudan)":
                    radioClimateNumber = "2"; break;
                case "Maritime Subtropical (W. Africa)":
                    radioClimateNumber = "3"; break;
                case "Desert (Sahara)":
                    radioClimateNumber = "4"; break;
                case "Continental Temperate":
                    radioClimateNumber = "5"; break;
                case "Maritime Temperate (Land)":
                    radioClimateNumber = "6"; break;
                case "Maritime Temperate (Sea)":
                    radioClimateNumber = "7"; break;
                default: radioClimateNumber = "5"; break;
            }
            return radioClimateNumber;
        }

        public static string AtmosphericBendingConstant(string radioClimate)
        {
            string atmosphericBendingConstant = "";
            switch (radioClimate)
            {
                case "Equatorial (Congo)":
                    atmosphericBendingConstant = "360.000"; break;
                case "Continental Subtropical (Sudan)":
                    atmosphericBendingConstant = "320.000"; break;
                case "Maritime Subtropical (W. Africa)":
                    atmosphericBendingConstant = "370.000"; break;
                case "Desert (Sahara)":
                    atmosphericBendingConstant = "280.000"; break;
                case "Continental Temperate":
                    atmosphericBendingConstant = "301.000"; break;
                case "Maritime Temperate (Land)":
                    atmosphericBendingConstant = "320.000"; break;
                case "Maritime Temperate (Sea)":
                    atmosphericBendingConstant = "350.000"; break;
                default: atmosphericBendingConstant = "301.000"; break;
            }
            return atmosphericBendingConstant;
        }


    }
}
