// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Portal
{    
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq;
    using System.Web;
    using System.Web.Mvc;
    using System.Web.Routing;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;        
    using Microsoft.WhiteSpaces.AzureOAuthProvider;
    using Microsoft.WhiteSpaces.BusinessManager;
    using Microsoft.WhiteSpaces.Common;
    using Unity.Mvc5;

    public class CustomHandleErrorAttribute : HandleErrorAttribute
    {
        //// private readonly IUserManager userManager;

        public CustomHandleErrorAttribute()
        {
            this.ExceptionAuditor = ConfigHelper.CurrentContainer.Resolve<AzureAuditor>();
            this.ExceptionLogger = ConfigHelper.CurrentContainer.Resolve<Logger>();
        }

        public IAuditor ExceptionAuditor { get; set; }

        public ILogger ExceptionLogger { get; set; }        

        public override void OnException(ExceptionContext filterContext)
        {
            if (new HttpException(null, filterContext.Exception).GetHttpCode() != 500)
            {
                return;
            }

            if (!ExceptionType.IsInstanceOfType(filterContext.Exception))
            {
                return;
            }

            // Audit Exceptions
            if (!(filterContext.Exception.GetType() == typeof(AccessDeniedException) || filterContext.Exception.GetType() == typeof(ResponseErrorException) || filterContext.Exception.GetType() == typeof(ValidationErrorException)))
            {
                if (filterContext.Exception.GetType() == typeof(DataAccessException))
                {
                    if (this.ExceptionAuditor != null)
                    {
                        this.ExceptionAuditor.TransactionId = this.ExceptionLogger.TransactionId;
                        this.ExceptionAuditor.Audit(AuditId.Exception, AuditStatus.Failure, default(int), "Back end services thrown exception");
                    }

                    if (this.ExceptionLogger != null)
                    {
                        this.ExceptionLogger.Log(TraceEventType.Error, LoggingMessageId.PortalException, "Back end services thrown exception");
                    }
                }

                if (filterContext.Exception.GetType() == typeof(AccessDeniedException))
                {
                    if (this.ExceptionAuditor != null)
                    {
                        this.ExceptionAuditor.TransactionId = this.ExceptionLogger.TransactionId;
                        this.ExceptionAuditor.Audit(AuditId.Exception, AuditStatus.Failure, default(int), "Access token is expired");
                    }

                    if (this.ExceptionLogger != null)
                    {
                        this.ExceptionLogger.Log(TraceEventType.Error, LoggingMessageId.PortalException, "Access token is expired");
                    }
                }

                if (filterContext.Exception.GetType() == typeof(ValidationErrorException))
                {
                    if (this.ExceptionAuditor != null)
                    {
                        this.ExceptionAuditor.TransactionId = this.ExceptionLogger.TransactionId;
                        this.ExceptionAuditor.Audit(AuditId.Exception, AuditStatus.Failure, default(int), "Validation exception is thrown");
                    }

                    if (this.ExceptionLogger != null)
                    {
                        this.ExceptionLogger.Log(TraceEventType.Error, LoggingMessageId.PortalException, "Validation exception is thrown");
                    }
                }
                else
                {
                    if (this.ExceptionAuditor != null)
                    {
                        this.ExceptionAuditor.TransactionId = this.ExceptionLogger.TransactionId;
                        this.ExceptionAuditor.Audit(AuditId.Exception, AuditStatus.Failure, default(int), "An unknown exception occured, Message: " + filterContext.Exception.Message + " ," + "and stack trace: " + filterContext.Exception.StackTrace);
                    }

                    if (this.ExceptionLogger != null)
                    {
                        this.ExceptionLogger.Log(TraceEventType.Error, LoggingMessageId.PortalException, "An unknown exception occured, Message: " + filterContext.Exception.Message + " ," + "and stack trace: " + filterContext.Exception.StackTrace);
                    }
                }
            }

            string referringUrl = "http://" + filterContext.HttpContext.Request.Url.Authority + "/" + (string)filterContext.RouteData.Values["controller"];

            // if the request is AJAX return JSON else view.
            if (filterContext.HttpContext.Request.Headers["X-Requested-With"] == "XMLHttpRequest")
            {
                JsonResult errorResult = new JsonResult
                {
                    JsonRequestBehavior = JsonRequestBehavior.AllowGet,
                };

                if (filterContext.Exception.GetType() == typeof(AccessDeniedException))
                {
                    errorResult.Data = new
                    {
                        error = true,
                        message = filterContext.Exception.Message,
                        type = "Access Denied",
                        url = referringUrl
                    };
                }
                else if (filterContext.Exception.GetType() == typeof(ResponseErrorException))
                {
                    ResponseErrorException exception = (ResponseErrorException)filterContext.Exception;
                    errorResult.Data = new
                    {
                        error = true,
                        message = exception.Data == string.Empty ? exception.Message : exception.Data,
                        type = "Response Error"
                    };
                }
                else
                {
                    errorResult.Data = new
                    {
                        error = true,
                        message = filterContext.Exception.Message,
                        type = "Unknown"
                    };
                }

                filterContext.Result = errorResult;
            }
            else
            {
                if (filterContext.Exception.GetType() == typeof(AccessDeniedException))
                {
                    var routeData = new RouteData();
                    routeData.Values["controller"] = "Account";
                    routeData.Values["action"] = "Login";
                    routeData.Values["ReturnUrl"] = referringUrl;

                    IUserManager userManager = ConfigHelper.CurrentContainer.Resolve<IUserManager>();
                    IRegionSource regionSource = ConfigHelper.CurrentContainer.Resolve<IRegionSource>();

                    IController errorsController = new AccountController(userManager, regionSource);
                    var requestContext = new RequestContext(new HttpContextWrapper(HttpContext.Current), routeData);
                    errorsController.Execute(requestContext);
                }
                else
                {                    
                    filterContext.Result = new ViewResult
                    {
                        ViewName = "~/Views/Home/Error.cshtml"
                    };
                }

                base.OnException(filterContext);
            }            

            filterContext.ExceptionHandled = true;
            filterContext.HttpContext.Response.Clear();
            filterContext.HttpContext.Response.TrySkipIisCustomErrors = true;  
        }
    }
}
