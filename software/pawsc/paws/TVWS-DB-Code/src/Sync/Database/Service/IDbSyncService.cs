// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Sync.Database.Service
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Runtime.Serialization;
    using System.ServiceModel;
    using System.ServiceModel.Web;
    using System.Text;    
    using System.Xml;
    using System.Xml.Serialization;

    /// <summary>
    /// Default generated WCF Service contract.
    /// </summary>
    /// <remarks>
    /// NOTE: You can use the "Rename" command on the "Refactor" menu to change the interface name "IDBSyncService" in both code and config file together.
    /// </remarks>    
    [ServiceContract(Namespace = "http://www.whitespace-db-providers.org/2011//InterDB/ws")]
    public interface IDbSyncService
    {
        /// <summary>
        /// Default WCF generated code creates a sample public method.
        /// </summary>
        /// <param name="realtimePollRequest">Value passed in from web service.</param>
        /// <returns>Returns a string format of the passed in value.</returns>        
        [WebInvoke(BodyStyle = WebMessageBodyStyle.Wrapped, Method = "POST", RequestFormat = WebMessageFormat.Xml, ResponseFormat = WebMessageFormat.Xml)]
        [OperationContract(Action = "http://www.whitespace-db-providers.org/2011//InterDB/ws/RealTimePoll", Name = "RealTimePoll", ReplyAction = "http://www.whitespace-db-providers.org/2011//InterDB/ws/RealTimePollResponse")]
        [return: MessageParameter(Name = "RealTimePollResponse")]
        RealTimePollOutPut RealTimePoll(RealTimePollInput realtimePollRequest);
    }
}
