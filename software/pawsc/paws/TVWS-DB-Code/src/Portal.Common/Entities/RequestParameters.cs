// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common
{
    public class RequestParameters : IRequestParameters
    {
        public string AccessToken { get; set; }

        public Whitespace.Entities.Parameters Params { get; set; }

        public string RegionName { get; set; }
    }
}
