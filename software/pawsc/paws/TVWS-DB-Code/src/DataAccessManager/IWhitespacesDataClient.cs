// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.DataAccessManager
{    
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WhiteSpaces.Common;

    public interface IWhitespacesDataClient
    {
        Result AddIncumbent(IRequestParameters requestParams);

        Result GetChannelList(IRequestParameters requestParams);

        Result GetIncumbents(IRequestParameters requestParams);

        Result DeleteIncumbent(IRequestParameters requestParams);

        Result GetDevices(IRequestParameters requestParams);

        Result ExcludeChannel(IRequestParameters requestParams);

        Result ExcludeIds(IRequestParameters requestParams);

        Result GetNearByTvStations(IRequestParameters requestParams);
        
        Result GetAllUlsFileNumbers(string regionName);

        Result GetAllUlsCallSigns(string regionName);

        void SubmitPawsInterference(IRequestParameters requestParams);

        Task<Result> GetMvpdCallsignInfoAsync(string callsign, string regionName);        
    }
}
