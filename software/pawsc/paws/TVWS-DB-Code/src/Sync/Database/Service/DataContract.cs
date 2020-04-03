// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

[assembly: System.Runtime.Serialization.ContractNamespaceAttribute("http://www.whitespace-db-providers.org/2011//InterDB/ws", ClrNamespace = "Microsoft.Whitespace.Sync.Database.Service")]

namespace Microsoft.Whitespace.Sync.Database.Service
{
    using System;
    using System.Runtime.Serialization;
    using System.ServiceModel;
    using System.Xml;
    using System.Xml.Serialization;

     /// <summary>
    /// Real Time Poll Input.
    /// </summary>
    [Serializable]
    [DataContract(Name = "RealTimePollRequest", Namespace = "http://www.whitespace-db-providers.org/2011//InterDB/ws/data")]
    public partial class RealTimePollInput
    {
        /// <summary>
        /// Requested Transaction ID Field
        /// </summary>
        private string requestedTransactionIDField;

        /// <summary>
        /// Command Field
        /// </summary>
        private string commandField;

        /// <summary>
        /// XSD Version Field
        /// </summary>
        private string xsdVersionField;

        /// <summary>
        /// Gets or sets the Requested Transaction ID
        /// </summary>
        [System.Runtime.Serialization.DataMember(Name = "RequestedTransactionID", IsRequired = true, EmitDefaultValue = false)]        
        public string RequestedTransactionID
        {
            get
            {
                return this.requestedTransactionIDField;
            }

            set
            {
                this.requestedTransactionIDField = value;
            }
        }

        /// <summary>
        /// Gets or sets the Command
        /// </summary>
        [XmlElement(DataType = "string", ElementName = "Command", IsNullable = false)]
        [System.Runtime.Serialization.DataMember(Name = "Command", IsRequired = true, EmitDefaultValue = false, Order = 2)]
        public string Command
        {
            get
            {
                return this.commandField;
            }

            set
            {
                this.commandField = value;
            }
        }

        /// <summary>
        /// Gets or sets the XSD Version
        /// </summary>
        [System.Runtime.Serialization.DataMember(Name = "XsdVersion", IsRequired = true, EmitDefaultValue = false, Order = 2)]
        public string XsdVersion
        {
            get
            {
                return this.xsdVersionField;
            }

            set
            {
                this.xsdVersionField = value;
            }
        }
    }
}
