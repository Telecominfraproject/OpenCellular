// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.RegionCalculation.Propagation
{
    using System;
    using System.Collections.Generic;
    using System.Security.Permissions;

    /// <summary>
    /// Represents Class OFCOMMatrix.
    /// </summary>
    public static class OFCOMMatrix
    {
        /// <summary>
        /// Initializes static members of the <see cref="OFCOMMatrix"/> class.
        /// </summary>
        static OFCOMMatrix()
        {
            PS0 = new Dictionary<PMSEEquipmentType, int>();
            PS0.Add(PMSEEquipmentType.WirelessMicrophones, -65);
            PS0.Add(PMSEEquipmentType.InEarMonitors, -65);
            PS0.Add(PMSEEquipmentType.Talkback, -65);
            PS0.Add(PMSEEquipmentType.ProgrammeAudioLinks, -73);
            PS0.Add(PMSEEquipmentType.DataLinks, -65);
            PS0.Add(PMSEEquipmentType.ProgrammeVideoLinks, -73);

            PRWirelessMicrophone = new List<int[]>();
            PRInEarMonitor = new List<int[]>();
            PRAudioLinks = new List<int[]>();
            PRVideoLinks = new List<int[]>();

            ////Add PMSE Protection Ratios for Wireless Microphones
            PRWirelessMicrophone.Add(new[]
                                     {
                                         -4, -4, -4, -4, -4
                                     });
            PRWirelessMicrophone.Add(new[]
                                     {
                                         -52, -52, -48, -39, -28
                                     });
            PRWirelessMicrophone.Add(new[]
                                     {
                                         -61, -58, -58, -49, -38
                                     });
            PRWirelessMicrophone.Add(new[]
                                     {
                                         -62, -58, -62, -58, -49
                                     });
            PRWirelessMicrophone.Add(new[]
                                     {
                                         -60, -60, -60, -60, -56
                                     });
            PRWirelessMicrophone.Add(new[]
                                     {
                                         -69, -69, -69, -69, -66
                                     });
            PRWirelessMicrophone.Add(new[]
                                     {
                                         -72, -72, -72, -72, -71
                                     });
            PRWirelessMicrophone.Add(new[]
                                     {
                                         -73, -73, -73, -73, -73
                                     });
            PRWirelessMicrophone.Add(new[]
                                     {
                                         -74, -74, -74, -74, -74
                                     });
            PRWirelessMicrophone.Add(new[]
                                     {
                                         -75, -75, -75, -75, -75
                                     });
            PRWirelessMicrophone.Add(new[]
                                     {
                                         -78, -78, -78, -78, -78
                                     });

            ////Add PMSE Protection Ratios for InEar Monitors
            PRInEarMonitor.Add(new[]
                               {
                                   -2, -2, -2, -2, -2
                               });
            PRInEarMonitor.Add(new[]
                               {
                                   -54, -54, -47, -37, -26
                               });
            PRInEarMonitor.Add(new[]
                               {
                                   -61, -57, -57, -47, -36
                               });
            PRInEarMonitor.Add(new[]
                               {
                                   -64, -57, -64, -57, -47
                               });
            PRInEarMonitor.Add(new[]
                               {
                                   -60, -59, -60, -59, -55
                               });
            PRInEarMonitor.Add(new[]
                               {
                                   -64, -64, -64, -64, -62
                               });
            PRInEarMonitor.Add(new[]
                               {
                                   -64, -64, -64, -64, -64
                               });
            PRInEarMonitor.Add(new[]
                               {
                                   -65, -65, -65, -65, -65
                               });
            PRInEarMonitor.Add(new[]
                               {
                                   -66, -66, -66, -66, -66
                               });
            PRInEarMonitor.Add(new[]
                               {
                                   -67, -67, -67, -67, -67
                               });
            PRInEarMonitor.Add(new[]
                               {
                                   -68, -68, -68, -68, -68
                               });

            ////Add PMSE Protection Ratios for Programme Audio Links
            PRAudioLinks.Add(new[]
                             {
                                 2, 2, 2, 2, 2
                             });
            PRAudioLinks.Add(new[]
                             {
                                 -50, -50, -42, -33, -22
                             });
            PRAudioLinks.Add(new[]
                             {
                                 -57, -53, -53, -43, -32
                             });
            PRAudioLinks.Add(new[]
                             {
                                 -61, -53, -61, -53, -43
                             });
            PRAudioLinks.Add(new[]
                             {
                                 -67, -62, -67, -62, -53
                             });
            PRAudioLinks.Add(new[]
                             {
                                 -69, -67, -69, -67, -62
                             });
            PRAudioLinks.Add(new[]
                             {
                                 -69, -69, -69, -69, -67
                             });
            PRAudioLinks.Add(new[]
                             {
                                 -69, -69, -69, -69, -69
                             });
            PRAudioLinks.Add(new[]
                             {
                                 -69, -69, -69, -69, -69
                             });
            PRAudioLinks.Add(new[]
                             {
                                 -69, -69, -69, -69, -69
                             });
            PRAudioLinks.Add(new[]
                             {
                                 -69, -69, -69, -69, -69
                             });

            ////Add PMSE Protection Ratios for Programme Video Links
            PRVideoLinks.Add(new[]
                             {
                                 17, 17, 17, 17, 17
                             });
            PRVideoLinks.Add(new[]
                             {
                                 -36, -36, -28, -18, -7
                             });
            PRVideoLinks.Add(new[]
                             {
                                 -42, -38, -38, -28, -17
                             });
            PRVideoLinks.Add(new[]
                             {
                                 -45, -38, -45, -38, -28
                             });
            PRVideoLinks.Add(new[]
                             {
                                 -55, -47, -55, -47, -38,
                             });
            PRVideoLinks.Add(new[]
                             {
                                 -56, -49, -56, -49, -41
                             });
            PRVideoLinks.Add(new[]
                             {
                                 -57,
                                 -51,
                                 -57,
                                 -51,
                                 -43
                             });
            PRVideoLinks.Add(new[]
                             {
                                 -57,
                                 -53,
                                 -57,
                                 -53,
                                 -46
                             });
            PRVideoLinks.Add(new[]
                             {
                                 -58,
                                 -55,
                                 -58,
                                 -55,
                                 -48
                             });
            PRVideoLinks.Add(new[]
                             {
                                 -60,
                                 -59,
                                 -60,
                                 -59,
                                 -55
                             });
            PRVideoLinks.Add(new[]
                             {
                                 -62,
                                 -62,
                                 -62,
                                 -62,
                                 -61
                             });
        }

        /// <summary>
        /// Gets the PS0.
        /// </summary>
        /// <value>The PS0.</value>
        public static Dictionary<PMSEEquipmentType, int> PS0 { get; private set; }

        /// <summary>
        /// Gets the Protection Ratio for wireless microphone.
        /// </summary>
        /// <value>The Protection Ratio for wireless microphone.</value>
        public static List<int[]> PRWirelessMicrophone { get; private set; }

        /// <summary>
        /// Gets the Protection Ratio for in ear monitor.
        /// </summary>
        /// <value>The Protection Ratio for in ear monitor.</value>
        public static List<int[]> PRInEarMonitor { get; private set; }

        /// <summary>
        /// Gets the Protection Ratio for audio links.
        /// </summary>
        /// <value>The Protection Ratio for audio links.</value>
        public static List<int[]> PRAudioLinks { get; private set; }

        /// <summary>
        /// Gets the Protection Ratio video links.
        /// </summary>
        /// <value>The video links.</value>
        public static List<int[]> PRVideoLinks { get; private set; }

        /// <summary>
        /// Gets the protection ratio.
        /// </summary>
        /// <param name="pmseUseCase">The PMSE use case.</param>
        /// <returns>returns protection ratios.</returns>
        public static List<int[]> GetProtectionRatio(PMSEEquipmentType pmseUseCase)
        {
            switch (pmseUseCase)
            {
                case PMSEEquipmentType.None:
                    break;

                case PMSEEquipmentType.Talkback:
                case PMSEEquipmentType.WirelessMicrophones:
                case PMSEEquipmentType.DataLinks:

                    return PRWirelessMicrophone;

                case PMSEEquipmentType.ProgrammeAudioLinks:

                    return PRAudioLinks;

                case PMSEEquipmentType.ProgrammeVideoLinks:

                    return PRVideoLinks;

                case PMSEEquipmentType.InEarMonitors:

                    return PRInEarMonitor;

                default:
                    return null;
            }

            return null;
        }
    }
}
