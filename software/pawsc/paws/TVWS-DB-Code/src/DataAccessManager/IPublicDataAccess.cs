// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.DataAccessManager
{
    public interface IPublicDataAccess
    {
        string GetPublicDataWithEvents(string entityType, string regionName);

        string GetPublicData(string entityType, string regionName);

        string GetAuthorizedDeviceModels(string regionName);
    }
}
