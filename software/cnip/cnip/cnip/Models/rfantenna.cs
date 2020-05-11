namespace cnip.Models
{
    class rfantenna
    {
        public struct RFAntenna
        {
            public string AntennaModel { get; set; }
            public string AntennaType { get; set; }
            public string Polarization { get; set; }
            public string VBeamWidth { get; set; }
            public string HBeamWidth { get; set; }
            public string DownTilt { get; set; }
            public string AntennaGain { get; set; }
        }
        public static RFAntenna GetRFAntennaParameters(string antennaModel)
        {
            RFAntenna rfAntenna = new RFAntenna();
            switch (antennaModel)
            {
                case "LT OD9-5 890-950 MHz":
                    rfAntenna.AntennaModel = "LT OD9-5 890-950 MHz";
                    rfAntenna.AntennaType = "Omni";
                    rfAntenna.Polarization = "Vertical";
                    rfAntenna.VBeamWidth = "36";
                    rfAntenna.HBeamWidth = "360";
                    rfAntenna.DownTilt = "0";
                    rfAntenna.AntennaGain = "5";
                    break;
                case "LT OD9-6 865-945 MHz":
                    rfAntenna.AntennaModel = "LT OD9-6 865-945 MHz";
                    rfAntenna.AntennaType = "Omni";
                    rfAntenna.Polarization = "Vertical";
                    rfAntenna.VBeamWidth = "16";
                    rfAntenna.HBeamWidth = "360";
                    rfAntenna.DownTilt = "0";
                    rfAntenna.AntennaGain = "6";
                    break;
                case "LT OD9-6 860-960 MHz":
                    rfAntenna.AntennaModel = "LT OD9-6 860-960 MHz";
                    rfAntenna.AntennaType = "Omni";
                    rfAntenna.Polarization = "Vertical";
                    rfAntenna.VBeamWidth = "16";
                    rfAntenna.HBeamWidth = "360";
                    rfAntenna.DownTilt = "0";
                    rfAntenna.AntennaGain = "6";
                    break;
                case "LT OD9-8 865-945 MHz":
                    rfAntenna.AntennaModel = "LT OD9-8 865-945 MHz";
                    rfAntenna.AntennaType = "Omni";
                    rfAntenna.Polarization = "Vertical";
                    rfAntenna.VBeamWidth = "10";
                    rfAntenna.HBeamWidth = "360";
                    rfAntenna.DownTilt = "0";
                    rfAntenna.AntennaGain = "8";
                    break;
                case "LT OD9-8 860-960 MHz":
                    rfAntenna.AntennaModel = "LT OD9-8 860-960 MHz";
                    rfAntenna.AntennaType = "Omni";
                    rfAntenna.Polarization = "Vertical";
                    rfAntenna.VBeamWidth = "10";
                    rfAntenna.HBeamWidth = "360";
                    rfAntenna.DownTilt = "0";
                    rfAntenna.AntennaGain = "8";
                    break;
                case "LT OD9-11 860-960 MHz":
                    rfAntenna.AntennaModel = "LT OD9-11 860-960 MHz";
                    rfAntenna.AntennaType = "Omni";
                    rfAntenna.Polarization = "Vertical";
                    rfAntenna.VBeamWidth = "7";
                    rfAntenna.HBeamWidth = "360";
                    rfAntenna.DownTilt = "0";
                    rfAntenna.AntennaGain = "11";
                    break;
                case "LT OD9-11D1 860-960 MHz":
                    rfAntenna.AntennaModel = "LT OD9-11D1 860-960 MHz";
                    rfAntenna.AntennaType = "Omni";
                    rfAntenna.Polarization = "Vertical";
                    rfAntenna.VBeamWidth = "7";
                    rfAntenna.HBeamWidth = "360";
                    rfAntenna.DownTilt = "1";
                    rfAntenna.AntennaGain = "11";
                    break;
                default:
                    rfAntenna.AntennaModel = "LT OD9-5 890-950 MHz";
                    rfAntenna.AntennaType = "Omni";
                    rfAntenna.Polarization = "Vertical";
                    rfAntenna.VBeamWidth = "36";
                    rfAntenna.HBeamWidth = "360";
                    rfAntenna.DownTilt = "0";
                    rfAntenna.AntennaGain = "5";
                    break;
            }
            return rfAntenna;
        }
    }
}
