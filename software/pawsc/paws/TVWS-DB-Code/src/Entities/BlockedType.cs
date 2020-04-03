// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    /// <summary>
    ///     Indicates if a channel is blocked.
    /// </summary>
    public enum BlockedType
    {
        /// <summary>
        ///     Channel is not blocked for either mobile or fixed.
        /// </summary>
        None,

        /// <summary>
        ///     Fixed devices are blocked.
        /// </summary>
        Fixed,

        /// <summary>
        ///     Both Fixed and Mobile devices are blocked.
        /// </summary>
        Both,

        /// <summary>
        ///     Mobile devices are blocked.
        /// </summary>
        Mobile
    }
}
