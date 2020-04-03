// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.EmailNotificationSystem
{
    using System;
    using System.IO;
    using System.Reflection;

    public class EmbeddedResourceReader
    {
        public static Stream GetResourceStream(string resourceManifestFile)
        {
            Stream embededResourceStream = null;

            try
            {
                Assembly assembly = Assembly.GetExecutingAssembly();
                embededResourceStream = assembly.GetManifestResourceStream(resourceManifestFile);
            }
            catch (Exception ex)
            {
                System.Diagnostics.Trace.TraceError(ex.ToString());
                embededResourceStream = null;
            }

            return embededResourceStream;
        }
    }
}
