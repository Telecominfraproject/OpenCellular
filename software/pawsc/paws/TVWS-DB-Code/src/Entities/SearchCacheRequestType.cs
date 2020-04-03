// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    /// <summary>
    ///     Represents SearchCacheRequestType
    /// </summary>
    public enum SearchCacheRequestType
    {
        /// <summary>The none</summary>
        None = 0,

        /// <summary>The by call sign</summary>
        ByCallSign = 1,

        /// <summary>The VSD service</summary>
        ByVsdService = 2,

        /// <summary>The easting northing</summary>
        ByEastingNorthing = 3
    }
}
