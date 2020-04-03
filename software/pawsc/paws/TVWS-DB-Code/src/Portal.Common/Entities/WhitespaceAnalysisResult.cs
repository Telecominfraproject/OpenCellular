// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;

    public class WhitespaceAnalysisResult
    {
        private List<WhitespaceSummary> whitespaceSummaryCollection;

        public WhitespaceAnalysisResult(List<WhitespaceSummary> whitespaceSummaryDataCollection)
        {
            this.whitespaceSummaryCollection = whitespaceSummaryDataCollection;
        }

        public WhitespaceSummary this[string deviceType]
        {
            get
            {
                return this.whitespaceSummaryCollection.Where(summary => string.Compare(summary.DeviceType, deviceType, StringComparison.OrdinalIgnoreCase) == 0).FirstOrDefault();
            }
        }
    }
}
