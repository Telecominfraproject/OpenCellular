// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common
{
    using System.Linq;

    public class AccessRequest
    {
        public string UserId { get; set; }

        public string UserName { get; set; }

        public string Regulatory { get; set; }

        public int CurrentAccessLevel { get; set; }

        public int RequestedAccessLevel { get; set; }

        public string Justification { get; set; }

        public int RequestStatus { get; set; }

        public string ApprovedUser { get; set; }

        public string Remarks { get; set; }

        public string TimeUpdated { get; set; }

        public string CurrentRole
        {
            get
            {
                return AppConfigRegionSource.GetRoles().Where(x => x.Id == this.CurrentAccessLevel).First().FriendlyName;
            }
        }

        public string RequestedRole
        {
            get
            {
                return AppConfigRegionSource.GetRoles().Where(x => x.Id == this.RequestedAccessLevel).First().FriendlyName;
            }
        }
    }
}
