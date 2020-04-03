// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.BusinessManager
{
    using System.Collections.Generic;
    using System.ComponentModel.DataAnnotations;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WhiteSpaces.Common;    
    using INCU = Microsoft.WhiteSpaces.Common.Incumbent;
    using System.Threading.Tasks;

    public interface IWhitespacesManager
    {
        ProtectedDevice[] GetDeviceList(string incumbentType, double latitude, double longitude, string regionName = "United States");

        ChannelInfo[] GetChannelList(string incumbentType, double latitude, double longitude, string regionName = "United States");

        List<INCU> GetIncumbents(string incumbentType, string regionName, IEnumerable<int> channels);

        List<INCU> GetIncumbents(string incumbentType, double latitude, double longitude, string regionName, IEnumerable<int> channels);

        MVPDCallSignsInfo[] GetNearByTvStations(Location location, string accesstoken, string regionName = "United States");

        Task<MVPDCallSignsInfo> GetMvpdCallsignInfoAsync(string callsign, string regionName = "United States");

        LpAuxLicenseInfo GetULSFileInfo(string ulsFileNumber, string regionName = "United States");

        string RegisterLicensedLpAux(LicensedLpAuxRegistration registrationInfo, string accessToken, string regionName = "United States");

        string RegisterMVPD(MvpdRegistrationInfo registrationInfo, string accessToken, string regionName = "United States");

        string RegisterTBasLinks(TBasLinkRegistration registrationInfo, string accessToken, string regionName = "United States");

        string RegisterUnLicensedLpAux(LicensedLpAuxRegistration registrationInfo, string accessToken, string regionName = "United States");

        string ExcludeChannel(int[] channels, Point[] locations, string accesstoken, string regionName);

        string ExcludeDevice(string accesstoken, string regionName, string deviceId, string serialNumber);

        void SubmitPawsInterference(Parameters parameters, string regionName = "paws");
    }
}
