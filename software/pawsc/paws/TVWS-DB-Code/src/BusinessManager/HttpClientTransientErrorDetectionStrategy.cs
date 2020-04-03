// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.BusinessManager
{
    using System;
    using System.Threading.Tasks;
    using Microsoft.Practices.EnterpriseLibrary.TransientFaultHandling;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;

    public class HttpClientTransientErrorDetectionStrategy : ITransientErrorDetectionStrategy
    {
        [Dependency]
        public IAuditor HttpClientAuditor { get; set; }

        [Dependency]
        public ILogger HttpClientLogger { get; set; }

        public bool IsTransient(Exception ex)
        {
            // TODO: Other cases to consider as TransientErrors.
            TaskCanceledException taskCanceledException = ex as TaskCanceledException;

            if (taskCanceledException != null)
            {
                this.HttpClientAuditor.TransactionId = this.HttpClientLogger.TransactionId;
                this.HttpClientAuditor.Audit(AuditId.RequestAccessElevation, AuditStatus.Failure, default(int), "Portal unable to connect backend. Backend thrown connection timeout exception at" + System.DateTime.UtcNow);

                // Request Time out exception.
                return !taskCanceledException.CancellationToken.IsCancellationRequested;
            }

            return false;
        }
    }
}
