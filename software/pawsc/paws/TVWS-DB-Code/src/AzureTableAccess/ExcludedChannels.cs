// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.AzureTableAccess
{
    using Microsoft.WindowsAzure.Storage.Table;

    public class ExcludedChannels : TableEntity
    {
        public string ChannelList { get; set; }

        public string Location { get; set; }
    }
}
