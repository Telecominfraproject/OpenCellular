// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.AdminNotificationSystem
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.WhiteSpaces.AzureTableAccess;
    using Microsoft.WhiteSpaces.Common;
    using Microsoft.WhiteSpaces.Common.Enums;
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Table;

    public class UserAccessRequestTracker : NotificationSystem
    {
        private const string Type = "AccessRequestTrackingSystem";

        private const string RequestStatusKey = "RequestStatus";
        private const string RegulatoryBodyKey = "Regulatory";
        private const string UserIdKey = "RowKey";
        private const string CurrentAccessLevel = "CurrentAccessLevel";
        private const string JustificationText = "Justification";
        private const string RequestedAccessLevel = "RequestedAccessLevel";

        private static object lockObj = new object();

        private readonly IAzureTableOperation azureTableOperation;
        private readonly string[] regulatoryBodyList;
        private readonly TimeSpan scheduleTime;
        private readonly TimeZoneInfo timeZoneInfo;

        private Task pollingThread;
        private CancellationTokenSource cancellationTokenSource;

        public UserAccessRequestTracker(INotificationClient notificationClient, IAzureTableOperation azureTableOperation, string[] regulatoryBodyList, string scheduleTime, string timeZoneId)
            : base(notificationClient, UserAccessRequestTracker.Type)
        {
            if (azureTableOperation == null)
            {
                throw new ArgumentNullException("azureTableOperation");
            }

            if (regulatoryBodyList == null)
            {
                throw new ArgumentNullException("regulatoryBodyList");
            }

            if (string.IsNullOrWhiteSpace(scheduleTime))
            {
                throw new ArgumentException("scheduleTime");
            }

            if (string.IsNullOrWhiteSpace(timeZoneId))
            {
                this.timeZoneInfo = TimeZoneInfo.Utc;
            }

            this.azureTableOperation = azureTableOperation;
            this.regulatoryBodyList = regulatoryBodyList;
            this.scheduleTime = TimeSpan.Parse(scheduleTime);
            this.timeZoneInfo = TimeZoneInfo.FindSystemTimeZoneById(timeZoneId);
        }

        protected string TableName
        {
            get
            {
                return "AccessElevationRequest";
            }
        }

        protected string UserProfileTable
        {
            get
            {
                return "UserProfile";
            }
        }

        public override void StartTracking()
        {
            this.Enabled = true;
            this.TrackingTimeStart = DateTime.UtcNow;

            this.pollingThread = Task.Factory.StartNew(this.PollingThread);
        }

        public override void Notify(DateTime signalTime)
        {
            if (this.regulatoryBodyList.Length == 0)
            {
                System.Diagnostics.Trace.TraceWarning("There are no entries in the regulatoryBodyList");
                return;
            }

            IEnumerable<TableQuery<DynamicTableEntity>> tableQueryList = this.GetTableQueries();
            IEnumerable<UserAccessRequest> accessRequests = this.GetUserAccessRequests(tableQueryList);

            if (accessRequests != null && accessRequests.Any())
            {
                Notification userAccessRequestData = new UserAccessRequestData(accessRequests, UserAccessRequestTracker.Type, this.TrackingTimeStart, signalTime.ToUniversalTime());

                // boolean status would be helpful if any further processing needs to performed based on the status of notification delivery .
                this.NotificationClient.NotifyClient(userAccessRequestData);
            }
        }

        public override void StopTracking()
        {
            this.Enabled = false;

            if (this.cancellationTokenSource != null)
            {
                this.cancellationTokenSource.Cancel();
            }

            try
            {
                this.pollingThread.Wait();
                this.Dispose();
            }
            catch (Exception ex)
            {
                System.Diagnostics.Trace.TraceError(ex.ToString());
            }

            System.Diagnostics.Trace.TraceInformation(string.Format("PeriodicTrackingSystem has stopped tracking {0} | {1}", UserAccessRequestTracker.Type, DateTime.UtcNow.ToString(NotificationSystem.DateTimeFormat)));
        }

        public List<UserAccessRequest> GetUserAccessRequests(IEnumerable<TableQuery<DynamicTableEntity>> tableQueryList)
        {
            List<UserAccessRequest> accessRequests = new List<UserAccessRequest>();

            string userName = "UserName";
            string location = "City";

            try
            {
                Parallel.ForEach(
                    tableQueryList,
                    (tableQuery) =>
                    {
                        IEnumerable<DynamicTableEntity> partialResult = this.azureTableOperation.GetTableEntityProjection(this.TableName, tableQuery);

                        if (partialResult != null)
                        {
                            foreach (var userRequest in partialResult)
                            {
                                string userId = userRequest[UserAccessRequestTracker.UserIdKey].StringValue;
                                TableQuery<DynamicTableEntity> query = new TableQuery<DynamicTableEntity>();

                                query.SelectColumns = new List<string> { userName, location };

                                query = TableAzureQueryHelper.AddCondition(query, QueryComparisons.Equal, TableQuery.GenerateFilterCondition("PartitionKey", QueryComparisons.Equal, "1"));
                                query = TableAzureQueryHelper.AddCondition(query, QueryComparisons.Equal, TableQuery.GenerateFilterCondition("RowKey", QueryComparisons.Equal, userId));

                                DynamicTableEntity user = this.azureTableOperation.GetTableEntityProjection(this.UserProfileTable, query).FirstOrDefault();

                                if (user != null)
                                {
                                    lock (lockObj)
                                    {
                                        string currentAccessLevel = Enum.GetName(typeof(AccessLevels), userRequest[UserAccessRequestTracker.CurrentAccessLevel].Int32Value);
                                        string requestedAccessLevel = Enum.GetName(typeof(AccessLevels), userRequest[UserAccessRequestTracker.RequestedAccessLevel].Int32Value);

                                        accessRequests.Add(
                                            new UserAccessRequest(
                                                user[userName].StringValue,
                                                user[location].StringValue,
                                                currentAccessLevel,
                                                requestedAccessLevel,
                                                userRequest[UserAccessRequestTracker.RegulatoryBodyKey].StringValue,
                                                userRequest[UserAccessRequestTracker.JustificationText].StringValue));
                                    }
                                }
                            }
                        }
                    });
            }
            catch (System.AggregateException ex)
            {
                if (!(ex.InnerException is StorageException))
                {
                    throw;
                }

                System.Diagnostics.Trace.TraceError(ex.ToString());
            }

            return accessRequests;
        }

        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                if (this.cancellationTokenSource != null)
                {
                    this.cancellationTokenSource.Dispose();
                }

                if (this.pollingThread != null)
                {
                    this.pollingThread.Dispose();
                }
            }

            base.Dispose(disposing);
        }

        private void PollingThread()
        {
            System.Diagnostics.Trace.TraceInformation(string.Format("PeriodicTrackingSystem has started tracking {0} | {1}", UserAccessRequestTracker.Type, DateTime.UtcNow.ToString(NotificationSystem.DateTimeFormat)));

            this.cancellationTokenSource = new CancellationTokenSource();
            double lastNotificationSentDay = 0;

            while (!this.cancellationTokenSource.Token.IsCancellationRequested)
            {
                DateTime currentTime = TimeZoneInfo.ConvertTimeFromUtc(DateTime.UtcNow, this.timeZoneInfo);
                TimeSpan timeSpan = new TimeSpan(currentTime.Hour, currentTime.Minute, 00);

                if (timeSpan >= this.scheduleTime && lastNotificationSentDay != currentTime.DayOfYear)
                {
                    this.Notify(currentTime);
                    lastNotificationSentDay = currentTime.DayOfYear;
                }

                Thread.Sleep(TimeSpan.FromHours(1));
            }
        }

        private IEnumerable<TableQuery<DynamicTableEntity>> GetTableQueries()
        {
            List<TableQuery<DynamicTableEntity>> tableQueryList = new List<TableQuery<DynamicTableEntity>>();

            for (int index = 0; index < this.regulatoryBodyList.Length; index++)
            {
                TableQuery<DynamicTableEntity> tableQuery = new TableQuery<DynamicTableEntity>();

                tableQuery.SelectColumns = new List<string> 
                { 
                    UserAccessRequestTracker.UserIdKey,
                    UserAccessRequestTracker.RegulatoryBodyKey, 
                    UserAccessRequestTracker.RequestStatusKey,
                    UserAccessRequestTracker.CurrentAccessLevel,
                    UserAccessRequestTracker.RequestedAccessLevel,
                    UserAccessRequestTracker.JustificationText
                };

                string regulatory = this.regulatoryBodyList[index];

                //tableQuery = TableAzureQueryHelper.AddCondition(tableQuery, TableOperators.And, TableQuery.GenerateFilterCondition("PartitionKey", QueryComparisons.Equal, string.Format("{0}-{1}", regulatory, (int)RequestStatus.Pending)));

                //[Temporary Fix:] The following query is full table scan. We have to re-design the schemal of "AccessEvelationRequest" table, so that it can queried efficiently on number of diff. fields.
                tableQuery = TableAzureQueryHelper.AddCondition(tableQuery, TableOperators.And, TableQuery.GenerateFilterConditionForInt("RequestStatus", QueryComparisons.Equal, (int)RequestStatus.Pending));
                tableQuery = TableAzureQueryHelper.AddCondition(tableQuery, TableOperators.And, TableQuery.GenerateFilterCondition("Regulatory", QueryComparisons.Equal, regulatory.ToLower()));

                tableQueryList.Add(tableQuery);
            }

            return tableQueryList;
        }       
    }
}
