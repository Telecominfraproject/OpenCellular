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
    public class AccessDeniedException : Exception
    {
        public AccessDeniedException()
        { 
        }

        public AccessDeniedException(Error error) : base(error.Message)
        {
            this.AccessError = error;
        }

        protected AccessDeniedException(SerializationInfo info, StreamingContext context) :
            base(info, context)
        {
            this.AccessError = (Error)info.GetValue("AccessError", typeof(Error));
        }

        public Error AccessError { get; set; }

        [SecurityPermissionAttribute(SecurityAction.Demand, SerializationFormatter = true)]
        public override void GetObjectData(SerializationInfo info, StreamingContext context)
        {
            info.AddValue("AccessError", this.AccessError);
            base.GetObjectData(info, context);
        }
    }
}
