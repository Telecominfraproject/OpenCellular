// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common
{
    using System.IO;
    using System.Net;

    /// <summary>
    /// Interface INetFileStream
    /// </summary>
    public interface INetFileStream
    {
        /// <summary>
        /// Downloads the file.
        /// </summary>
        /// <param name="uri">The URI.</param>
        /// <param name="contentLength">Length of the content.</param>
        /// <returns>returns Stream.</returns>
        Stream DownloadFile(string uri, out long contentLength);
    }
}
