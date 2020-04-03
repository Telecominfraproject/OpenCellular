// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.EmailNotificationSystem
{
    using System.IO;
    using System.Linq;
    using System.Text;
    using Microsoft.WhiteSpaces.AdminNotificationSystem;
    using UARMBT = Microsoft.WhiteSpaces.EmailNotificationSystem.UserAccessRequestsMailBodyTemplate;

    public class UserAccessRequestsMailBodyTemplate : EmailBodyTemplate
    {
        private const string ResourceManifest = "Microsoft.WhiteSpaces.EmailNotificationSystem";
        private const string FileName = "MailBodyTemplates.UserAccessRequestsEmailBody.html";

        private int requestsCount;

        public UserAccessRequestsMailBodyTemplate(int userRequestsCount = 0)
        {
            this.requestsCount = userRequestsCount;
        }

        public override string GetHtmlBody(Notification notification)
        {
            string emailBody = string.Empty;
            string resourceManifestPath = string.Format("{0}.{1}", UARMBT.ResourceManifest, UARMBT.FileName);

            using (Stream resourceStream = EmbeddedResourceReader.GetResourceStream(resourceManifestPath))
            {
                StreamReader reader = new StreamReader(resourceStream);
                emailBody = reader.ReadToEnd();

                emailBody = this.UpdateEmailBodyPlaceHolder(emailBody, notification);
            }

            return emailBody;
        }

        private string UpdateEmailBodyPlaceHolder(string htmlBody, Notification notification)
        {
            UserAccessRequestData userRequestNotification = (UserAccessRequestData)notification;

            StringBuilder userRequestEntires = new StringBuilder();

            int totalRequests = this.requestsCount != 0 ? this.requestsCount : userRequestNotification.UserAccessRequests.Count();

            for (int i = 0; i < totalRequests; i++)
            {
                userRequestEntires.Append("<tr>");
                userRequestEntires.Append(string.Format("<td>{0}</td>", userRequestNotification.UserAccessRequests.ElementAt(i).Name));
                userRequestEntires.Append(string.Format("<td>{0}</td>", userRequestNotification.UserAccessRequests.ElementAt(i).Location));
                userRequestEntires.Append(string.Format("<td>{0}</td>", userRequestNotification.UserAccessRequests.ElementAt(i).CurrentRole));
                userRequestEntires.Append(string.Format("<td>{0}</td>", userRequestNotification.UserAccessRequests.ElementAt(i).RequestedRole));
                userRequestEntires.Append(string.Format("<td>{0}</td>", userRequestNotification.UserAccessRequests.ElementAt(i).Region));
                userRequestEntires.Append(string.Format("<td>{0}</td>", userRequestNotification.UserAccessRequests.ElementAt(i).JustificationText));
                userRequestEntires.Append("</tr>");
            }

            htmlBody = htmlBody.Replace("{AccessRequests}", userRequestEntires.ToString());

            return htmlBody;
        }
    }
}
