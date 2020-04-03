// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.AzureTableAccess
{ 
    using System;
    using Microsoft.WindowsAzure.Storage.Table;

    /// <summary>
    /// table entity to store Access Elevation Requests. Here Partition Key is UserId
    /// </summary>
    public class AccessElevationRequest : TableEntity
    {
        public AccessElevationRequest()
        {
            this.RowKey = Guid.NewGuid().ToString();
        }

        public string Regulatory { get; set; }

        public int CurrentAccessLevel { get; set; }

        public int RequestedAccessLevel { get; set; }

        public string Justification { get; set; }

        public int RequestStatus { get; set; }

        public string ApprovedUser { get; set; }

        public string Remarks { get; set; }
    }
}
