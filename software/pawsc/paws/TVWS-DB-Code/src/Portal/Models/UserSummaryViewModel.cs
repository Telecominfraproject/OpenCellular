// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Portal
{
    public class UserSummaryViewModel
    {
        public string[] Cities { get; set; }

        public string[] Countries { get; set; }

        public int PortalUserCount { get; set; }

        public int RegionAdminCount { get; set; }

        public int PortalAdminCount { get; set; }

        public int UsersCount { get; set; }

        public int RequestsCount { get; set; }
    }
}
