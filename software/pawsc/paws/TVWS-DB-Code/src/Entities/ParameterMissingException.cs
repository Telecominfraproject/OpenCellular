// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Runtime.Serialization;
    using System.Text;

    /// <summary>
    /// Represents Parameter Missing Exception class.
    /// </summary>
    [Serializable]
    public class ParameterMissingException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the ParameterMissingException class.
        /// </summary>
        public ParameterMissingException()
            : base()
        {
        }

        /// <summary>
        /// Initializes a new instance of the ParameterMissingException class.
        /// </summary>
        /// <param name="message">Post Register Request</param>
        /// <returns>response of registration</returns>
        public ParameterMissingException(string message)
            : base(message)
        {
        }

        /// <summary>
        /// Initializes a new instance of the ParameterMissingException class.
        /// </summary>
        /// <param name="message">Error Message</param>
        /// <param name="errors">Errors List</param>
        /// <returns>response of registration</returns>
        public ParameterMissingException(string message, IEnumerable<string> errors)
            : base(message)
        {
            this.Errors = errors;
        }

        /// <summary>
        /// Gets or sets the list errors
        /// </summary>
        public IEnumerable<string> Errors
        {
            get;
            set;
        }

        /// <summary>
        /// When overridden in a derived class, sets the <see cref="T:System.Runtime.Serialization.SerializationInfo"/> with information about the exception.
        /// </summary>
        /// <param name="info">The <see cref="T:System.Runtime.Serialization.SerializationInfo"/> that holds the serialized object data about the exception being thrown. </param><param name="context">The <see cref="T:System.Runtime.Serialization.StreamingContext"/> that contains contextual information about the source or destination. </param><exception cref="T:System.ArgumentNullException">The <paramref name="info"/> parameter is a null reference (Nothing in Visual Basic). </exception><PermissionSet><IPermission class="System.Security.Permissions.FileIOPermission, mscorlib, Version=2.0.3600.0, Culture=neutral, PublicKeyToken=b77a5c561934e089" version="1" Read="*AllFiles*" PathDiscovery="*AllFiles*"/><IPermission class="System.Security.Permissions.SecurityPermission, mscorlib, Version=2.0.3600.0, Culture=neutral, PublicKeyToken=b77a5c561934e089" version="1" Flags="SerializationFormatter"/></PermissionSet>
        public override void GetObjectData(SerializationInfo info, StreamingContext context)
        {
            base.GetObjectData(info, context);
        }
    }
}
