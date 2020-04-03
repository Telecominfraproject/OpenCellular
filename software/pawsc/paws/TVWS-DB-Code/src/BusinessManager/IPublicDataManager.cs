// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.BusinessManager
{
    public interface IPublicDataManager
    {
        string GetPublicDataWithEvents(string entityType, string regionName = "United States");

        string GetPublicData(string entityType, string regionName = "United States");

        string GetAuthorizedDeviceModels(string regionName = "United States");

        string GetUlsCallSigns(string regionName = "United States");

        string GetUlsFileNumbers(string regionName = "United States");
    }
}
