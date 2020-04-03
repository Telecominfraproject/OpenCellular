// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.BusinessManager
{
    using System.Collections.Generic;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WhiteSpaces.Common;
    using Microsoft.WhiteSpaces.Common.Enums;

    public interface IUserAdminManager
    {
        IEnumerable<UserInfo> GetAllUsersInfo();

        IEnumerable<AccessRequest> GetAllAccessRequests();

        bool SaveAccessElevationRequest(AccessRequest accessRequest);

        IEnumerable<IntialUserInfo> GetIntialUserList(SearchCriteria criteria);

        IEnumerable<AccessRequest> GetElevationRequestsByUser(string userId);        
    }
}
