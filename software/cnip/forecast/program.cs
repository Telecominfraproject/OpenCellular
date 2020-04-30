using forecast.Models;

namespace forecast
{
    class Program
    {
        static void Main(string[] args)
        {
            if (args.Length > 0)
            {
                GdalConfiguration.ConfigureGdal();
                GdalConfiguration.ConfigureOgr();
                switch (args[0])
                {
                    case "PredictCoverage":
                        Analysis.PredictCoverage(args[1], args[2], args[3], args[4], args[5]);
                        break;
                    case "Links":
                        Links.Forecast(args[1], args[2], args[3]);
                        break;
                    case "RadioPlan":
                        RadioPlan.Forecast(args[1], args[2], args[3]);
                        break;
                    case "PredictSites":
                        Analysis.PredictSites(args[1], args[2], args[3], args[4], args[5], args[6]);
                        break;
                    case "BestCandidate":
                        Analysis.BestCandidate(args[1], args[2], args[3], args[4], args[5], args[6]);
                        break;
                    default: break;
                }
            }
        }
    }
}
