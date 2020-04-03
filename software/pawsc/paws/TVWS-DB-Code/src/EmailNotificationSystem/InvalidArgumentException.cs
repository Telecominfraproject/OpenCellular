// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.EmailNotificationSystem
{
    using System;
    using System.Runtime.Serialization;

    [Serializable]
    internal class InvalidArgumentException : ArgumentException
    {
        public InvalidArgumentException()
        {
        }

        public InvalidArgumentException(string paramName)
            : base(string.Empty, paramName)
        {
        }

        public InvalidArgumentException(string message, Exception innerException)
            : base(message, innerException)
        {
        }

        public InvalidArgumentException(string paramName, string message)
            : base(message, paramName)
        {
        }

        public InvalidArgumentException(string message, string paramName, Exception innerException)
            : base(message, paramName, innerException)
        {
        }

        protected InvalidArgumentException(SerializationInfo info, StreamingContext context)
            : base(info, context)
        {
        }
    }
}
