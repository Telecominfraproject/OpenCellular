// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Runtime.Serialization;
    using System.Security.Permissions;
    using System.Text;
    using System.Threading.Tasks;

    [Serializable]
    public class ResponseErrorException : Exception
    {
        public ResponseErrorException()
        { 
        }

        public ResponseErrorException(string message)
            : base(message)
        { 
        }

        public ResponseErrorException(string type, string data, string code, string message)
        {
            this.Type = type;
            this.Data = data;
            this.Code = code;
            this.Message = message;
        }

        protected ResponseErrorException(SerializationInfo info, StreamingContext context) :
            base(info, context)
        {
            this.Type = info.GetString("Type");
            this.Data = info.GetString("Data");
            this.Code = info.GetString("Code");
            this.Message = info.GetString("Message");
        }

        public string Type { get; set; }

        public new string Data { get; set; }

        public string Code { get; set; }

        public new string Message { get; set; }

        [SecurityPermissionAttribute(SecurityAction.Demand, SerializationFormatter = true)]
        public override void GetObjectData(SerializationInfo info, StreamingContext context)
        {
            info.AddValue("Type", this.Type);
            info.AddValue("Data", this.Data);
            info.AddValue("Code", this.Code);
            info.AddValue("Message", this.Message);
            base.GetObjectData(info, context);
        }
    }
}
