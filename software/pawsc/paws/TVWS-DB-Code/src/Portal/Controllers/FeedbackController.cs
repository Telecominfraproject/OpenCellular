// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Portal.Controllers
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Web;
    using System.Web.Mvc;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using Microsoft.WhiteSpaces.BusinessManager;
    using Microsoft.WhiteSpaces.Common;    

    [AllowAnonymous]
    public class FeedbackController : Controller
    {
        private readonly IRegionManager regionManager;

        public FeedbackController(IRegionManager regionManager)
        {
            this.regionManager = regionManager;
        }

        [Dependency]
        public IAuditor ProfileAuditor { get; set; }

        [Dependency]
        public ILogger ProfileLogger { get; set; }
                
        public ActionResult Index()
        {
            return this.View();
        }

        public JsonResult Save(FeedbackInfo info)
        {
            if (ModelState.IsValid)
            {
                info.FeedbackTime = DateTime.UtcNow.Ticks;
                bool result = this.regionManager.SaveFeedback(info);

                if (result)
                {
                    this.ProfileAuditor.TransactionId = this.ProfileLogger.TransactionId;
                    this.ProfileAuditor.Audit(AuditId.Feedback, AuditStatus.Success, default(int), info.FirstName + " " + info.LastName + " feedback submission is successful");
                }
                else
                {
                    this.ProfileAuditor.TransactionId = this.ProfileLogger.TransactionId;
                    this.ProfileAuditor.Audit(AuditId.Feedback, AuditStatus.Failure, default(int), info.FirstName + " " + info.LastName + " feedback submission is failed");
                }

                return this.Json(result.ToString());
            }

            return null;
        }
    }
}
