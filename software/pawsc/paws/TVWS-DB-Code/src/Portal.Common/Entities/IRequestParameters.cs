// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common
{
    using Microsoft.Whitespace.Entities;

    public interface IRequestParameters
    {
        string AccessToken { get; set; }

        Parameters Params { get; set; }

        string RegionName { get; set; }
    }
}
