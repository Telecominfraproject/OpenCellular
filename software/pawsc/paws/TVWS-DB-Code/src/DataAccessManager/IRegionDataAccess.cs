// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.DataAccessManager
{
    using System.Collections.Generic;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WhiteSpaces.AzureTableAccess;
    using Microsoft.WhiteSpaces.Common;    

    public interface IRegionDataAccess
    {
        IEnumerable<MVPDRegistration> GetMvpdRegistrations(string userId);

        IEnumerable<LPAuxRegistration> GetLpAuxRegistrations(string userId);

        IEnumerable<TempBASRegistration> GetTempBasRegistrations(string userId);

        void DeleteRegistration(string partitionKey, string rowKey, string etag, Microsoft.WhiteSpaces.Common.Enums.RegistrationType registrationType);

        bool SaveFeedback(FeedbackInfo info);

        IEnumerable<ExcludedDevice> GetExcludedIdsByRegionCode(string regionCode);

        void DeleteExcludedId(string regionCode, string partionKey, string rowKey);

        IEnumerable<ExcludedChannels> GetExcludedChannelsByRegionCode(string regionCode);

        void DeleteExcludedChannels(string regionCode, string partitionKey, string rowKey);

        string[] UploadTestResults(Dictionary<string, System.IO.Stream> blobStreams);

        IEnumerable<PmseAssignment> GetAllPmseAssignementEntities(string tableName);

        IEnumerable<DTTDataAvailability> GetDTTDataAvailability(string tableName);

        void DeletePmseEntities(IEnumerable<PmseAssignment> entities, string tableName);

        void DeleteDTTDataAvailability(IEnumerable<DTTDataAvailability> entities, string tableName);

        void InsertPmseEntities(IEnumerable<PmseAssignment> entities, string tableName);

        void InsertDTTDataAvailability(IEnumerable<DTTDataAvailability> entities, string tableName);
    }
}
