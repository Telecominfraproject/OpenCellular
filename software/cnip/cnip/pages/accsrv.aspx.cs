using System;
using System.Web;
using static cnip.Models.pgsql;
using static cnip.Models.webext;
using Google.Authenticator;

namespace cnip.pages
{
    public partial class accsrv : System.Web.UI.Page
    {
        string puaction;
        string pmsg;
        string puid;
        string pukey;
        string ppassword;
        string ppasscode;

        protected void Page_Load(object sender, EventArgs e)
        {
            if (!IsPostBack)
            {
                puaction = Request.QueryString["uaction"];
                pmsg = Request.QueryString["msg"];
                puid = Request.QueryString["uid"];
                pukey = Request.QueryString["ukey"];
                ppassword = Request.QueryString["password"];
                ppasscode = Request.QueryString["passcode"];

                if (!(pmsg is null)) { return; }
                if (puaction is null) { pmsg = "blank form submitted"; return; }

                if (!(puid is null && pukey is null))
                {
                    try
                    {
                        if (GetDataTableFromQuery("SELECT uid FROM pusers WHERE uid='" + puid + "' AND ukey='" + pukey + "';").Rows.Count > 0)
                        {
                            switch (puaction)
                            {
                                case "activate":
                                    if ((bool)GetDataTableFromQuery("SELECT accstatus FROM pusers WHERE uid='" + puid + "' AND ukey='" + pukey + "';").Rows[0][0])
                                    {
                                        ExecuteNonQuery("UPDATE pusers SET ukey='' WHERE uid='" + puid + "';");
                                        pmsg = puaction + " : account already activated";
                                    }
                                    else
                                    {
                                        ExecuteNonQuery("UPDATE pusers SET accstatus=true, ukey='' WHERE uid='" + puid + "';");
                                        pmsg = puaction + " : account activated successfully";
                                    }
                                    ExecuteNonQuery("INSERT INTO puserslog (uid, period, activity, location) VALUES ('" + puid + "','" + DateTime.UtcNow.ToString("yyyy-MM-dd hh:mm:ss") + "','" +
                                        "account activated" + "','" + GetUserIP() + "');");
                                    break;
                                case "resetpassword":
                                    ExecuteNonQuery("UPDATE pusers SET accstatus=true, ukey='', pwd='" + ppassword + "' WHERE uid='" + puid + "';");
                                    pmsg = puaction + " : password changed successfully";
                                    ExecuteNonQuery("INSERT INTO puserslog (uid, period, activity, location) VALUES ('" + puid + "','" + DateTime.UtcNow.ToString("yyyy-MM-dd hh:mm:ss") + "','" +
                                        "password reset" + "','" + GetUserIP() + "');");
                                    ExecuteNonQuery("DELETE FROM puserslogin WHERE uid='" + puid + "';");
                                    break;
                                case "turnoff2step":
                                    ExecuteNonQuery("UPDATE pusers SET authsetup=false, ukey='' WHERE uid='" + puid + "';");
                                    pmsg = puaction + " : 2-Step Verification turned off";
                                    ExecuteNonQuery("INSERT INTO puserslog (uid, period, activity, location) VALUES ('" + puid + "','" + DateTime.UtcNow.ToString("yyyy-MM-dd hh:mm:ss") + "','" +
                                        "2step deactivated" + "','" + GetUserIP() + "');");
                                    break;
                                case "turnon2step":
                                    ExecuteNonQuery("UPDATE pusers SET authsetup=true, ukey='' WHERE uid='" + puid + "';");
                                    pmsg = puaction + " : 2-Step Verification turned on";
                                    ExecuteNonQuery("INSERT INTO puserslog (uid, period, activity, location) VALUES ('" + puid + "','" + DateTime.UtcNow.ToString("yyyy-MM-dd hh:mm:ss") + "','" +
                                        "2step activated" + "','" + GetUserIP() + "');");
                                    break;
                                case "login":
                                    if ((bool)GetDataTableFromQuery("SELECT authsetup FROM pusers WHERE uid='" + puid + "';").Rows[0][0])
                                    {
                                        if ((string)GetDataTableFromQuery("SELECT pwd FROM pusers WHERE uid='" + puid + "';").Rows[0][0] == ppassword)
                                        {
                                            TwoFactorAuthenticator TwoFacAuth = new TwoFactorAuthenticator();
                                            string authkey = (string)GetDataTableFromQuery("SELECT authkey FROM pusers WHERE uid='" + puid + "';").Rows[0][0];
                                            bool isValid = TwoFacAuth.ValidateTwoFactorPIN(authkey, ppasscode);
                                            if (isValid)
                                            {
                                                pmsg = puaction + " : login successful";
                                                ExecuteNonQuery("INSERT INTO puserslog (uid, period, activity, location) VALUES ('" + puid + "','" + DateTime.UtcNow.ToString("yyyy-MM-dd hh:mm:ss") + "','" +
                                                    "logged in" + "','" + GetUserIP() + "');");
                                                ExecuteNonQuery("DELETE FROM puserslogin WHERE uid='" + puid + "';");
                                                // redirect to webui
                                                HttpContext.Current.Session["uid"] = puid;
                                                HttpContext.Current.Session["ukey"] = pukey;
                                                HttpContext.Current.Session["email"] = (string)GetDataTableFromQuery("SELECT email FROM pusers WHERE uid='" + puid + "';").Rows[0][0];
                                                HttpContext.Current.Session["name"] = (string)GetDataTableFromQuery("SELECT name FROM pusers WHERE uid='" + puid + "';").Rows[0][0];
                                                if (puid == "99999999")
                                                {
                                                    Response.Redirect("~/pages/webadmin.aspx"); return;
                                                }
                                                else
                                                {
                                                    Response.Redirect("~/Home/Index"); return;
                                                }
                                            }
                                            else
                                            {
                                                pmsg = puaction + " : invalid passcode";
                                                ExecuteNonQuery("INSERT INTO puserslog (uid, period, activity, location) VALUES ('" + puid + "','" + DateTime.UtcNow.ToString("yyyy-MM-dd hh:mm:ss") + "','" +
                                                    "login attempt invalid passscode" + "','" + GetUserIP() + "');");
                                                if (GetDataTableFromQuery("SELECT cast(attempt as text) FROM puserslogin WHERE uid='" + puid + "';").Rows.Count > 0)
                                                {
                                                    ExecuteNonQuery("UPDATE puserslogin SET attempt=attempt + 1 WHERE uid='" + puid + "';");
                                                }
                                                else
                                                {
                                                    ExecuteNonQuery("INSERT INTO puserslogin (uid, attempt) VALUES ('" + puid + "',1);");
                                                }
                                            }
                                        }
                                        else
                                        {
                                            pmsg = puaction + " : invalid password";
                                            ExecuteNonQuery("INSERT INTO puserslog (uid, period, activity, location) VALUES ('" + puid + "','" + DateTime.UtcNow.ToString("yyyy-MM-dd hh:mm:ss") + "','" +
                                                "login attempt invalid password" + "','" + GetUserIP() + "');");
                                            if (GetDataTableFromQuery("SELECT cast(attempt as text) FROM puserslogin WHERE uid='" + puid + "';").Rows.Count > 0)
                                            {
                                                ExecuteNonQuery("UPDATE puserslogin SET attempt=attempt + 1 WHERE uid='" + puid + "';");
                                            }
                                            else
                                            {
                                                ExecuteNonQuery("INSERT INTO puserslogin (uid, attempt) VALUES ('" + puid + "',1);");
                                            }
                                        }
                                    }
                                    else
                                    {
                                        if ((string)GetDataTableFromQuery("SELECT pwd FROM pusers WHERE uid='" + puid + "';").Rows[0][0] == ppassword)
                                        {
                                            pmsg = puaction + " : login successful";
                                            ExecuteNonQuery("INSERT INTO puserslog (uid, period, activity, location) VALUES ('" + puid + "','" + DateTime.UtcNow.ToString("yyyy-MM-dd hh:mm:ss") + "','" +
                                                "logged in" + "','" + GetUserIP() + "');");
                                            ExecuteNonQuery("DELETE FROM puserslogin WHERE uid='" + puid + "';");
                                            // redirect to webui
                                            HttpContext.Current.Session["uid"] = puid;
                                            HttpContext.Current.Session["ukey"] = pukey;
                                            HttpContext.Current.Session["email"] = (string)GetDataTableFromQuery("SELECT email FROM pusers WHERE uid='" + puid + "';").Rows[0][0];
                                            HttpContext.Current.Session["name"] = (string)GetDataTableFromQuery("SELECT name FROM pusers WHERE uid='" + puid + "';").Rows[0][0];
                                            if (puid == "99999999")
                                            {
                                                Response.Redirect("~/pages/webadmin.aspx"); return;
                                            }
                                            else
                                            {
                                                Response.Redirect("~/Home/Index"); return;
                                            }
                                        }
                                        else
                                        {
                                            pmsg = puaction + " : invalid password";
                                            ExecuteNonQuery("INSERT INTO puserslog (uid, period, activity, location) VALUES ('" + puid + "','" + DateTime.UtcNow.ToString("yyyy-MM-dd hh:mm:ss") + "','" +
                                                "login attempt invalid password" + "','" + GetUserIP() + "');");
                                            if (GetDataTableFromQuery("SELECT cast(attempt as text) FROM puserslogin WHERE uid='" + puid + "';").Rows.Count > 0)
                                            {
                                                ExecuteNonQuery("UPDATE puserslogin SET attempt=attempt + 1 WHERE uid='" + puid + "';");
                                            }
                                            else
                                            {
                                                ExecuteNonQuery("INSERT INTO puserslogin (uid, attempt) VALUES ('" + puid + "',1);");
                                            }
                                        }
                                    }
                                    break;
                                default:
                                    pmsg = "blank form submitted";
                                    break;
                            }
                        }
                        else
                        {
                            pmsg = puaction + " : link expired";
                        }
                    }
                    catch (Exception)
                    {
                        pmsg = puaction + " : error accessing server";
                    }
                }
                else
                {
                    pmsg = "blank form submitted";
                }
            }
        }
        public string Getmsg()
        {
            return pmsg;
        }

    }
}