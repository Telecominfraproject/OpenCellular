// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.AzureTableAccess
{
    using Microsoft.WindowsAzure.Storage.Table;

    public class Feedbacks : TableEntity
    {
        public Feedbacks()
        {
            this.PartitionKey = "1";
        }

        public string FirstName { get; set; }

        public string LastName { get; set; }

        public string Email { get; set; }

        public string Phone { get; set; }

        public string Subject { get; set; }

        public string Message { get; set; }
    }
}
