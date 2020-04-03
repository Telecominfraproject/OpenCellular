// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common
{
    using System;
    using System.IO;
    using System.Net;

    /// <summary>
    /// Represents Class NetFileStream.
    /// </summary>
    public class NetFileStream : INetFileStream
    {
        /// <summary>
        /// Downloads the file.
        /// </summary>
        /// <param name="uri">The URI.</param>
        /// <param name="contentLength">Length of the content.</param>
        /// <returns>returns Stream.</returns>
        public Stream DownloadFile(string uri, out long contentLength)
        {
            var webRequest = HttpWebRequest.Create(new Uri(uri));
            var response = webRequest.GetResponse();
            contentLength = webRequest.ContentLength;
            return response.GetResponseStream();
        }
    }
}
