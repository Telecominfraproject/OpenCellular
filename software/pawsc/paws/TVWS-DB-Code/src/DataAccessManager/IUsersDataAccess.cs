// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.DataAccessManager
{
    using System.Collections.Generic;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WhiteSpaces.Common;
    using Microsoft.WhiteSpaces.Common.Enums;

    public interface IUsersDataAccess
    {
        IEnumerable<UserInfo> GetAllUser();

        IEnumerable<AccessRequest> GetAllAccessElevationRequests();

        UserInfo GetUserInfoById(string userId);

        bool SaveAccessElevationRequest(AccessRequest accessRequest);

        IEnumerable<AccessRequest> GetAccessElevationRequestsByUserId(string userId);       
    }
}
