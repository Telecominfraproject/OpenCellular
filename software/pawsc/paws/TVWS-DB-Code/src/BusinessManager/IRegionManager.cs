// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.BusinessManager
{
    using System.Collections.Generic;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WhiteSpaces.AzureTableAccess;
    using Microsoft.WhiteSpaces.Common;
    using Microsoft.WhiteSpaces.Common.Enums;

    public interface IRegionManager
    {
        IEnumerable<MVPDRegistration> GetMvpdRegistrations(string userId);

        IEnumerable<LPAuxRegistration> GetLpAuxRegistrations(string userId);

        IEnumerable<TempBASRegistration> GetTempBasRegistrations(string userId);

        void DeleteRegistration(string partitionKey, string rowKey, string etag, RegistrationType registrationType);

        bool SaveFeedback(FeedbackInfo info);

        IEnumerable<ExcludedDevice> GetExcludedIdsByRegionCode(string regionCode);

        void DeleteExcludedId(string regionCode, string partionKey, string rowKey);

        IEnumerable<BlockedChannels> GetExcludedChannelsByRegionCode(string regionCode);

        void DeleteExcludedChannels(string regionCode, string partitionKey, string rowKey);

        string[] ProcessInputFiles(Dictionary<string, System.IO.Stream> fileStreams);

        string[] ProcessStageFiles(string[] stages);

        string ProcessOperationalParameters(Request request);
    }
}
