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
    /// Data contract for Real Time Poll Output.
    /// </summary>
    [Serializable]
    [DataContract(Name = "RealTimePollRespone", Namespace = "http://www.whitespace-db-providers.org/2011//InterDB/ws/")]
    public partial class RealTimePollOutPut
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
        /// Registration Record Ensemble Field
        /// </summary>
        private string registrationRecordEnsembleField;

        /// <summary>
        /// Version Field
        /// </summary>
        private string xsdVersionField;

        /// <summary>
        /// Version Field
        /// </summary>
        private string nextTransactionIDField;

        /// <summary>
        /// Real Time Poll Status Code Field
        /// </summary>
        private int realTimePollStatusCodeField;

        /// <summary>
        /// Gets or sets the Requested Transaction ID
        /// </summary>
        [System.Runtime.Serialization.DataMember(IsRequired = true, EmitDefaultValue = false)]
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
        [System.Runtime.Serialization.DataMember(IsRequired = true, EmitDefaultValue = false, Order = 1)]
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
        /// Gets or sets the Registration Record Ensemble
        /// </summary>
        [System.Runtime.Serialization.DataMember(EmitDefaultValue = false, Order = 2)]
        [XmlElement]
        public string RegistrationRecordEnsemble
        {
            get
            {
                return this.registrationRecordEnsembleField;
            }

            set
            {
                this.registrationRecordEnsembleField = value;
            }
        }

        /// <summary>
        /// Gets or sets the XSD Version
        /// </summary>
        [System.Runtime.Serialization.DataMember(IsRequired = true, EmitDefaultValue = false, Order = 3)]
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

        /// <summary>
        /// Gets or sets the Next Transaction ID
        /// </summary>
        [System.Runtime.Serialization.DataMember(EmitDefaultValue = false, Order = 4)]
        public string NextTransactionID
        {
            get
            {
                return this.nextTransactionIDField;
            }

            set
            {
                this.nextTransactionIDField = value;
            }
        }

        /// <summary>
        /// Gets or sets the RT Poll Status Code
        /// </summary>
        [System.Runtime.Serialization.DataMember(Name = "RT-PollStatusCode", IsRequired = true, Order = 5)]
        public int RTPollStatusCode
        {
            get
            {
                return this.realTimePollStatusCodeField;
            }

            set
            {
                this.realTimePollStatusCodeField = value;
            }
        }
    }
}
