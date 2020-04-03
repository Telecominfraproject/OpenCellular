// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common
{
    using System;
    using System.Runtime.Serialization;
    using System.Security.Permissions;
    
    [Serializable]
    public class DataAccessException : Exception
    {
        public DataAccessException()
        { 
        }

        public DataAccessException(string apiUrl, string method, string message)
            : base(message)
        {
            this.CallingUrl = new Uri(apiUrl);
            this.Method = method;
        }

        protected DataAccessException(SerializationInfo info, StreamingContext context) :
            base(info, context)
        {
            this.CallingUrl = new Uri(info.GetString("CallingUrl"));
            this.Method = info.GetString("Method");
        }

        public Uri CallingUrl { get; set; }

        public string Method { get; set; }

        [SecurityPermissionAttribute(SecurityAction.Demand, SerializationFormatter = true)]
        public override void GetObjectData(SerializationInfo info, StreamingContext context)
        {
            info.AddValue("CallingUrl", this.CallingUrl.ToString());
            info.AddValue("Method", this.Method);
            base.GetObjectData(info, context);
        }
    }
}
