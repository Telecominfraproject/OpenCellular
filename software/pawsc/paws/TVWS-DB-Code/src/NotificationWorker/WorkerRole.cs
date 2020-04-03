// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace NotificationWorker
{
    using System;
    using System.Diagnostics;
    using System.Threading;
    using Microsoft.Practices.Unity;
    using Microsoft.Practices.Unity.Configuration;        
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Dalc;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WhiteSpaces.AdminNotificationSystem;
    using Microsoft.WhiteSpaces.BusinessManager;
    using Microsoft.WhiteSpaces.DataAccessManager;
    using Microsoft.WindowsAzure.Diagnostics;
    using Microsoft.WindowsAzure.ServiceRuntime;    

    public class WorkerRole : RoleEntryPoint
    {
        private NotificationSystem auditNotificationSystem;
        private NotificationSystem userRequestsNotificationSystem;
        private IWhitespacesManager whitespacesManager;
        private IAuditor notificationAuditor;
        private ILogger notificationLogger;
        private Stopwatch stopWatch;

        public override void Run()
        {
            // This is a sample worker implementation. Replace with your logic.
            Trace.TraceInformation("NotificationWorker entry point called");

            try
            {
                IUnityContainer container = Utils.Configuration.CurrentContainer;
                container.LoadConfiguration();

                container.RegisterType<IHttpClientManager, HttpClientManager>(
                new PerThreadLifetimeManager(),
                new InjectionConstructor(RoleEnvironment.GetConfigurationSettingValue("AuthorizationSchema")));

                container.RegisterType<IWhitespacesDataClient, WhitespacesDataClient>(
                new InjectionConstructor(new ResolvedParameter<IHttpClientManager>()));

                this.auditNotificationSystem = container.Resolve<NotificationSystem>("auditTracker");
                this.userRequestsNotificationSystem = container.Resolve<NotificationSystem>("userAccessRequestTracker");
                this.whitespacesManager = container.Resolve<WhitespacesManager>();
                this.notificationAuditor = container.Resolve<AzureAuditor>();
                this.notificationLogger = container.Resolve<Logger>();

                while (true)
                {
                    if (!this.auditNotificationSystem.Enabled)
                    {
                        this.auditNotificationSystem.StartTracking();
                    }

                    if (!this.userRequestsNotificationSystem.Enabled)
                    {
                        this.userRequestsNotificationSystem.StartTracking();
                    }

                    this.MonitorGetChannelList();

                    Thread.Sleep(TimeSpan.FromMinutes(10));
                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Trace.TraceError(ex.ToString());

                if (this.auditNotificationSystem.Enabled)
                {
                    this.auditNotificationSystem.StopTracking();
                }

                if (!this.userRequestsNotificationSystem.Enabled)
                {
                    this.userRequestsNotificationSystem.StopTracking();
                }
            }
        }

        public override bool OnStart()
        {
            this.InitDiagnostics();

            return base.OnStart();
        }

        public override void OnStop()
        {
            if (this.auditNotificationSystem.Enabled)
            {
                this.auditNotificationSystem.StopTracking();
            }

            if (!this.userRequestsNotificationSystem.Enabled)
            {
                this.userRequestsNotificationSystem.StopTracking();
            }

            base.OnStop();
        }

        private void InitDiagnostics()
        {
            var config = DiagnosticMonitor.GetDefaultInitialConfiguration();

            int logLevelTransferTime = 0;
            int.TryParse(RoleEnvironment.GetConfigurationSettingValue("LogLevelTransferTime"), out logLevelTransferTime);

            if (logLevelTransferTime == 0)
            {
                logLevelTransferTime = 5;
            }

            config.Logs.ScheduledTransferPeriod = TimeSpan.FromMinutes(logLevelTransferTime);
            config.Logs.ScheduledTransferLogLevelFilter = LogLevel.Undefined;

            // Start the diagnostic monitor with the modified configuration.
            DiagnosticMonitor.Start("AzureStorageAccountConnectionString", config);
        }

        private void MonitorGetChannelList()
        {
            this.GetChannelListForRegion(IncumbentType.Mode_1.ToString(), 47.6694, 122.1239, "United States");
            this.GetChannelListForRegion(IncumbentType.TypeA.ToString(), 51.506420, -0.127210, "United Kingdom");
        }


        private void GetChannelListForRegion(string deviceType, double latitude, double longitude,string region)
        {
            this.notificationLogger.Log(TraceEventType.Information, LoggingMessageId.MonitorGetChannelList, "Get Channel List request started for region " + region);
            this.stopWatch = new Stopwatch();
            this.stopWatch.Start();

            ChannelInfo[] channelList = this.whitespacesManager.GetChannelList(deviceType, latitude, longitude, region);

            this.stopWatch.Stop();
            long elapsedTime = this.stopWatch.ElapsedMilliseconds;

            if (channelList.Length > 0)
            {
                if (elapsedTime > 60000)
                {
                    this.notificationLogger.Log(TraceEventType.Warning, LoggingMessageId.MonitorGetChannelList, "Get Channel List request for "+region+" ended in " + (elapsedTime / 1000) + " secs");
                    this.notificationAuditor.Audit(AuditId.MonitorGetChannelList, AuditStatus.Success, elapsedTime, "Get Channel List request for " + region + "ended in " + (elapsedTime / 1000) + " secs");
                }
                else
                {
                    this.notificationLogger.Log(TraceEventType.Information, LoggingMessageId.MonitorGetChannelList, "Get Channel List request for " + region + " ended in " + (elapsedTime / 1000) + " secs");
                }
            }
            else
            {
                this.notificationLogger.Log(TraceEventType.Warning, LoggingMessageId.MonitorGetChannelList, "No data found");
                this.notificationAuditor.Audit(AuditId.MonitorGetChannelList, AuditStatus.Success, elapsedTime, "No data found");
            }
        }
    }
}
