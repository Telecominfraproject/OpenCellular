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
    using System.Web.Security;    
    using DotNetOpenAuth.AspNet;
    using Microsoft.Practices.Unity;
    using Microsoft.Web.WebPages.OAuth;
    using Microsoft.Whitespace.Common;
    using Microsoft.WhiteSpaces.AzureOAuthProvider;
    using Microsoft.WhiteSpaces.AzureTableAccess;
    using Microsoft.WhiteSpaces.Common;    

    /// <summary>
    /// Controller which handles all the operations of authentication
    /// </summary>
    [AllowAnonymous]
    public class AccountController : Controller
    {
        /// <summary>
        /// property to hold instance of <see cref="IAzureTableOperation"/>
        /// </summary>
        private readonly IUserManager userManager;
        private readonly IRegionSource regionSource;

        private string tempUrl = string.Empty;

        /// <summary>
        /// Initializes a new instance of the <see cref="AccountController"/> class
        /// </summary>
        /// <param name="userManager">instance of <see cref="IUserManager"/></param>
        /// <param name="regionSource">instance of IRegionSource.</param>
        public AccountController(IUserManager userManager, IRegionSource regionSource)
        {
            this.userManager = userManager;
            this.regionSource = regionSource;
        }

        [Dependency]
        public IAuditor RegisterAuditor { get; set; }

        [Dependency]
        public ILogger RegisterLogger { get; set; }        

        /// <summary>
        /// Log off authenticated user
        /// </summary>
        /// <returns>redirect to specified action</returns>
        [HttpPost]
        [ValidateAntiForgeryToken]
        public ActionResult LogOff()
        {
            System.Web.Security.FormsAuthentication.SignOut();
            Session.Abandon();
            if (Request.Cookies[FormsAuthentication.FormsCookieName] != null)
            {
                HttpCookie myCookie = new HttpCookie(FormsAuthentication.FormsCookieName);
                myCookie.Expires = DateTime.Now.AddDays(-1d);
                Response.Cookies.Add(myCookie);
            }

            UserPrincipal userPrincipal = (UserPrincipal)User;
            this.RegisterLogger.Log(TraceEventType.Information, LoggingMessageId.PortalUserLoggedOut, userPrincipal.UserName + " has been logged out from portal");

            return this.RedirectToAction("Index", "Home");
        }

        /// <summary>
        /// It will redirect the site for external login like Microsoft for authentication
        /// </summary>
        /// <param name="returnUrlParam">URL to return back after authentication</param>
        /// <returns>redirects to external login result</returns>        
        public ActionResult Login(string returnUrlParam)
        {
            return new ExternalLoginResult(OAuthWebSecurity.RegisteredClientData.FirstOrDefault().AuthenticationClient.ProviderName, Url.Action("ExternalLoginCallback", new { returnUrl = returnUrlParam }));
        }

        /// <summary>
        /// It will redirect the site for external login like Microsoft for authentication
        /// </summary>
        /// <param name="provider">provider name</param>
        /// <param name="returnUrl">URL to return back after authentication</param>
        /// <returns>redirects to external login result</returns>
        [HttpPost]
        [ValidateAntiForgeryToken]
        public ActionResult ExternalLogin(string provider, string returnUrl)
        {
            if (string.IsNullOrEmpty(returnUrl))
            {
                returnUrl = HttpContext.Request.UrlReferrer.ToString();
            }

            return new ExternalLoginResult(provider, Url.Action("ExternalLoginCallback", new { ReturnUrl = returnUrl }));
        }

        /// <summary>
        /// External provider will call this method once authentication is done
        /// </summary>
        /// <param name="returnUrl">URL to return back after authentication</param>
        /// <returns>associated view</returns>
        [AllowAnonymous]
        public ActionResult ExternalLoginCallback(string returnUrl)
        {
            AuthenticationResult result = OAuthWebSecurity.VerifyAuthentication(returnUrl);

            if (!result.IsSuccessful)
            {
                return this.RedirectToAction("ExternalLoginFailure");
            }

            if (OAuthWebSecurity.Login(result.Provider, result.ProviderUserId, createPersistentCookie: false))
            {
                UserProfile userProfile = this.userManager.GetUserProfileFromProvider(result.Provider, result.ProviderUserId);
                IEnumerable<AccessDetails> accessDetails = this.userManager.GetUserAccessDetails(userProfile.RowKey);
                var isRegionAdmin = accessDetails.Any(x => x.AccessLevel == AccessLevels.Admin);

                this.CreateAuthenticationTicket(result.ExtraData["name"].ToString(), result.ExtraData["accesstoken"].ToString(), DateTime.Now, userProfile.IsSuperAdmin, isRegionAdmin);

                this.RegisterLogger.Log(TraceEventType.Information, LoggingMessageId.PortalUserLoggedIn, userProfile.UserName + " has logged in to the portal");

                return this.RedirectToLocal(returnUrl);
                //// return this.Redirect(redirectUrl);
            }

            if (User.Identity.IsAuthenticated)
            {
                // If the current user is logged in add the new account
                OAuthWebSecurity.CreateOrUpdateAccount(result.Provider, result.ProviderUserId, User.Identity.Name);
                UserProfile userProfile = this.userManager.GetUserProfileFromProvider(result.Provider, result.ProviderUserId);
                IEnumerable<AccessDetails> accessDetails = this.userManager.GetUserAccessDetails(userProfile.RowKey);
                var isRegionAdmin = accessDetails.Any(x => x.AccessLevel == AccessLevels.Admin);

                this.CreateAuthenticationTicket(User.Identity.Name, result.ExtraData["accesstoken"].ToString(), DateTime.Now, userProfile.IsSuperAdmin, isRegionAdmin);              

                return this.RedirectToLocal(returnUrl);
            }
            else
            {
                // User is new, ask for them to register
                string accessToken = result.ExtraData["accesstoken"].ToString();
                var userData = this.userManager.GetClientUserDataByAccessToken(accessToken);
                userData.AccessToken = accessToken;

                this.TempData["ClientUserData"] = userData;

                RegisterExternalLoginModel registerModel = new RegisterExternalLoginModel
                {
                    FirstName = userData.FirstName,
                    LastName = userData.LastName,
                    UserName = string.IsNullOrEmpty(userData.UserName) ? userData.Emails.Account : userData.UserName,
                    AccountEmail = userData.Emails.Account,
                    PreferredEmail = userData.Emails.Preferred,
                    City = userData.Business != null ? userData.Business.City : string.Empty,
                    ExternalLoginData = OAuthWebSecurity.SerializeProviderUserId(result.Provider, result.ProviderUserId),
                };

                ViewBag.ProviderDisplayName = OAuthWebSecurity.GetOAuthClientData(result.Provider).DisplayName;
                ViewBag.ReturnUrl = returnUrl;
                ViewBag.Country = new SelectList(Utility.GetCounties());
                ViewBag.TimeZone = new SelectList(Utility.GetTimeZones(), "(UTC-08:00) Pacific Time (US & Canada)");
                ViewBag.PhoneCountryCode = new SelectList(Utility.GetCountryPhoneCodes(), "United States(+1)");
                return this.View("Register", registerModel);
            }
        }

        /// <summary>
        /// New user registration  
        /// </summary>
        /// <param name="model">registration info</param>
        /// <param name="returnUrl">return back URL after completion of registration</param>
        /// <returns>associated view</returns>
        [HttpPost]
        [ValidateAntiForgeryToken]
        public ActionResult ExternalLoginConfirmation(RegisterExternalLoginModel model, string returnUrl)
        {
            string provider = null;
            string providerUserId = null;

            if (User.Identity.IsAuthenticated || !OAuthWebSecurity.TryDeserializeProviderUserId(model.ExternalLoginData, out provider, out providerUserId))
            {
                return this.RedirectToAction("Index", "Home");
            }

            if (ModelState.IsValid)
            {
                UserProfile userProfile = this.userManager.GetUserProfileFromProvider(provider, providerUserId);

                if (userProfile == null)
                {
                    ClientUserData clientUserData = (ClientUserData)TempData["ClientUserData"];

                    Random random = new Random();
                    int userId = random.Next(int.MaxValue);

                    userProfile = new UserProfile
                    {
                        RowKey = userId.ToString(),
                        UserName = string.IsNullOrEmpty(clientUserData.UserName) ? clientUserData.Emails.Account : clientUserData.UserName,
                        AccountEmail = clientUserData.Emails.Account,
                        FirstName = model.FirstName,
                        LastName = model.LastName,
                        City = model.City,
                        Country = model.Country,
                        State = model.State,
                        PreferredEmail = model.PreferredEmail,
                        Gender = clientUserData.Gender,
                        Link = Convert.ToString(clientUserData.Link),
                        IsSuperAdmin = false,
                        TimeZone = model.TimeZone,
                        CreatedTime = DateTime.Now.ToUniversalTime().ToString(),
                        Address1 = model.Address1,
                        Address2 = model.Address2,
                        Phone = model.Phone,
                        PhoneCountryCode = model.PhoneCountryCode,
                        ZipCode = model.ZipCode
                    };

                    var regions = this.regionSource.GetAvailableRegions();
                    List<AccessDetails> defaultAccess = new List<AccessDetails>();

                    foreach (var region in regions)
                    {
                        var accessDetail = new AccessDetails
                        {
                            AccessLevel = AccessLevels.PortalUser,
                            Authority = (Authorities)Convert.ToInt32(region.RegionInformation.Id)
                        };

                        defaultAccess.Add(accessDetail);
                    }

                    this.userManager.SaveUserDetails(
                        new UserDetails
                        {
                            UserInfo = userProfile,
                            AccessInfo = defaultAccess,
                        });

                    this.RegisterAuditor.TransactionId = this.RegisterLogger.TransactionId;
                    this.RegisterAuditor.Audit(AuditId.RegisterUser, AuditStatus.Success, default(int), userProfile.UserName + " Registered Successfully");
                    this.RegisterLogger.Log(TraceEventType.Information, LoggingMessageId.PortalUserRegistration, userProfile.UserName + " registered to the portal");

                    OAuthWebSecurity.CreateOrUpdateAccount(provider, providerUserId, model.UserName);
                    OAuthWebSecurity.Login(provider, providerUserId, createPersistentCookie: false);
                    this.RegisterLogger.Log(TraceEventType.Information, LoggingMessageId.PortalUserLoggedIn, userProfile.UserName + " has logged in to the portal");

                    // By Default IsSuperAdmin is false
                    this.CreateAuthenticationTicket(model.UserName, clientUserData.AccessToken, DateTime.Now, false, false);
                    return this.RedirectToLocal(returnUrl);
                }
                else
                {
                    ModelState.AddModelError("UserName", "User already exists. Please enter a different user name.");
                }
            }

            ViewBag.ProviderDisplayName = OAuthWebSecurity.GetOAuthClientData(provider).DisplayName;
            ViewBag.ReturnUrl = returnUrl;
            ViewBag.Country = new SelectList(Utility.GetCounties());
            ViewBag.TimeZone = new SelectList(Utility.GetTimeZones());
            ViewBag.PhoneCountryCode = new SelectList(Utility.GetCountryPhoneCodes());
            return this.View("Register", model);
        }

        /// <summary>
        /// called by external authentication provider after authentication is failed
        /// </summary>
        /// <returns>external login failure view</returns>
        public ActionResult ExternalLoginFailure()
        {
            return this.View();
        }

        #region Helpers

        /// <summary>
        /// returns code based on membership create status
        /// </summary>
        /// <param name="createStatus">membership create status</param>
        /// <returns>error message</returns>
        private static string ErrorCodeToString(MembershipCreateStatus createStatus)
        {
            // See http://go.microsoft.com/fwlink/?LinkID=177550 for
            // a full list of status codes.
            switch (createStatus)
            {
                case MembershipCreateStatus.DuplicateUserName:
                    return "User name already exists. Please enter a different user name.";

                case MembershipCreateStatus.DuplicateEmail:
                    return "A user name for that e-mail address already exists. Please enter a different e-mail address.";

                case MembershipCreateStatus.InvalidPassword:
                    return "The password provided is invalid. Please enter a valid password value.";

                case MembershipCreateStatus.InvalidEmail:
                    return "The e-mail address provided is invalid. Please check the value and try again.";

                case MembershipCreateStatus.InvalidAnswer:
                    return "The password retrieval answer provided is invalid. Please check the value and try again.";

                case MembershipCreateStatus.InvalidQuestion:
                    return "The password retrieval question provided is invalid. Please check the value and try again.";

                case MembershipCreateStatus.InvalidUserName:
                    return "The user name provided is invalid. Please check the value and try again.";

                case MembershipCreateStatus.ProviderError:
                    return "The authentication provider returned an error. Please verify your entry and try again. If the problem persists, please contact your system administrator.";

                case MembershipCreateStatus.UserRejected:
                    return "The user creation request has been canceled. Please verify your entry and try again. If the problem persists, please contact your system administrator.";

                default:
                    return "An unknown error occurred. Please verify your entry and try again. If the problem persists, please contact your system administrator.";
            }
        }

        /// <summary>
        /// Checks URL and redirect to given URL
        /// </summary>
        /// <param name="returnUrl">URL to which it should be redirected</param>
        /// <returns>redirect to the passing URL or default action</returns>
        private ActionResult RedirectToLocal(string returnUrl)
        {
            if (!string.IsNullOrEmpty(returnUrl))
            {
                return this.Redirect(returnUrl);
            }
            else
            {
                return this.RedirectToAction("Index", "Home");
            }
        }

        /// <summary>
        /// creates authentication ticket
        /// </summary>
        /// <param name="userName">user name</param>
        /// <param name="accessToken">access token for the user</param>
        /// <param name="tokenCreatedTime">time the token was created</param>
        /// <param name="isSuperAdmin">is this is super admin</param>
        /// <param name="isRegionAdmin">is this a region admin</param>
        private void CreateAuthenticationTicket(string userName, string accessToken, DateTime tokenCreatedTime, bool isSuperAdmin, bool isRegionAdmin)
        {
            int superAdmin = isSuperAdmin == true ? 1 : 0;
            int regionAdmin = isRegionAdmin == true ? 1 : 0;
            UserPrincipalSerializationModel serializationModel = new UserPrincipalSerializationModel
            {
                AccessToken = accessToken,
                AccessTokenCreatedTime = tokenCreatedTime,
                UserName = userName,
                IsSuperAdmin = isSuperAdmin
            };

            this.Session["TokenInfo"] = serializationModel;

            var userData = superAdmin.ToString() + ":" + regionAdmin.ToString() + ":" + accessToken;

            FormsAuthenticationTicket authenticationTicket = new FormsAuthenticationTicket(1, userName, DateTime.Now, DateTime.Now.AddHours(12), false, userData);
            string encryptedTicket = FormsAuthentication.Encrypt(authenticationTicket);

            HttpCookie cookie = new HttpCookie(FormsAuthentication.FormsCookieName, encryptedTicket);
            Response.Cookies.Add(cookie);
        }

        #endregion
    }
}
