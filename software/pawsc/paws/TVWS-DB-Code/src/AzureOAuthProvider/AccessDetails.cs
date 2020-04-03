// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.AzureOAuthProvider
{
    using Microsoft.WhiteSpaces.Common;

    public class AccessDetails
    {
        public Authorities Authority { get; set; }

        public AccessLevels AccessLevel { get; set; }

        public AccessLevels RequestedAccessLevel { get; set; }
    }
}
