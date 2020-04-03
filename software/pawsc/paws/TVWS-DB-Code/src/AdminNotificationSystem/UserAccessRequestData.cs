// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.AdminNotificationSystem
{
    using System;
    using System.Collections;
    using System.Collections.Generic;

    public class UserAccessRequestData : Notification
    {
        private string notificationType;

        public UserAccessRequestData(IEnumerable<UserAccessRequest> userAccessRequestList, string notificationType, DateTime validFrom, DateTime validTo)
            : base(validFrom, validTo)
        {
            this.UserAccessRequests = userAccessRequestList;
            this.notificationType = notificationType;
        }

        public IEnumerable<UserAccessRequest> UserAccessRequests { get; set; }

        public override string NotificationType
        {
            get
            {
                return this.notificationType;
            }
        }
    }
}
