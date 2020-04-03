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
    public class ValidationErrorException : Exception
    {
        public ValidationErrorException(IEnumerable<string> errors)
        {
            this.ValidationMessages = errors;
        }

        protected ValidationErrorException(SerializationInfo info, StreamingContext context) :
            base(info, context)
        {
            this.ValidationMessages = (IEnumerable<string>)info.GetValue("ValidationMessages", typeof(IEnumerable<string>));
        }

        public IEnumerable<string> ValidationMessages { get; set; }

        [SecurityPermissionAttribute(SecurityAction.Demand, SerializationFormatter = true)]
        public override void GetObjectData(SerializationInfo info, StreamingContext context)
        {
            info.AddValue("ValidationMessages", this.ValidationMessages);
            base.GetObjectData(info, context);
        }
    }
}
