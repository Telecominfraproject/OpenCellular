// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.AdminNotificationSystem
{
    using System;

    public class UserAccessRequest
    {
        public UserAccessRequest(string name, string location, string currentRole, string requestedRole, string region, string justificationText)
        {
            this.Name = name;
            this.Location = location;
            this.CurrentRole = currentRole;
            this.RequestedRole = requestedRole;
            this.Region = region;
            this.JustificationText = justificationText;
        }

        public string Name
        {
            get;
            private set;
        }

        public string Location
        {
            get;
            private set;
        }

        public string CurrentRole
        {
            get;
            private set;
        }

        public string RequestedRole
        {
            get;
            private set;
        }

        public string Region
        {
            get;
            private set;
        }

        public string JustificationText
        {
            get;
            private set;
        }
    }
}
