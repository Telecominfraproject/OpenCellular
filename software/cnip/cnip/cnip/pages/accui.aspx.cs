using System;
using Google.Authenticator;
using static cnip.Models.pgsql;
using static cnip.Models.webext;

namespace cnip.pages
{
    public partial class accui : System.Web.UI.Page
    {
        string puaction;
        string pmsg;
        string puid;
        string pukey;
        string pemail;
        string ppassword;
        string pbarcode;
        string psetupcode;
        string p2stepen;
        string ploginattempt;

        protected void Page_Load(object sender, EventArgs e)
        {
            if (!IsPostBack)
            {
                puaction = Request.QueryString["uaction"];
                pmsg = "";
                puid = Request.QueryString["uid"];
                pukey = Request.QueryString["ukey"];
                pemail = Request.QueryString["email"];
                ppassword = Request.QueryString["password"];
                pbarcode = "";
                psetupcode = "";
                p2stepen = "";
                if (puaction == "loginact")
                {
                    try
                    {
                        if (!(GetDataTableFromQuery("SELECT email FROM pusers WHERE email='" + pemail + "';").Rows.Count > 0))
                        {
                            pmsg = puaction + " : email not registered";
                            Response.Redirect("../pages/accsrv.aspx?msg=" + pmsg, false); return;
                        }
                        if (!((bool)GetDataTableFromQuery("SELECT accstatus FROM pusers WHERE email='" + pemail + "';").Rows[0][0]))
                        {
                            pmsg = puaction + " : account not active";
                            Response.Redirect("../pages/accsrv.aspx?msg=" + pmsg, false); return;
                        }
                        if ((bool)GetDataTableFromQuery("SELECT authsetup FROM pusers WHERE email='" + pemail + "';").Rows[0][0])
                        {
                            p2stepen = "true";
                        }

                        puid = (string)GetDataTableFromQuery("SELECT uid FROM pusers WHERE email='" + pemail + "';").Rows[0][0];
                        pukey = Guid.NewGuid().ToString();
                        ExecuteNonQuery("UPDATE pusers SET ukey='" + pukey + "' WHERE uid='" + puid + "';");
                        if (GetDataTableFromQuery("SELECT cast(attempt as text) FROM puserslogin WHERE uid='" + puid + "';").Rows.Count > 0)
                        {
                            ploginattempt = (string)GetDataTableFromQuery("SELECT cast(attempt as text) FROM puserslogin WHERE uid='" + puid + "';").Rows[0][0];
                            ploginattempt = Convert.ToString(Convert.ToInt32(ploginattempt) + 1);
                        }
                        else
                        {
                            ploginattempt = "1";
                        }
                        if (Convert.ToInt32(ploginattempt) > 3)
                        {
                            ExecuteNonQuery("INSERT INTO puserslog (uid, period, activity, location) VALUES ('" + puid + "','" + DateTime.UtcNow.ToString("yyyy-MM-dd hh:mm:ss") + "','" +
                                "account suspended" + "','" + GetUserIP() + "');");
                            pmsg = puaction + " : account suspended";
                            Response.Redirect("../pages/accsrv.aspx?msg=" + pmsg, false); return;
                        }
                        ExecuteNonQuery("INSERT INTO puserslog (uid, period, activity, location) VALUES ('" + puid + "','" + DateTime.UtcNow.ToString("yyyy-MM-dd hh:mm:ss") + "','" +
                            "login request" + "','" + GetUserIP() + "');");
                    }
                    catch (Exception)
                    {
                        pmsg = puaction + " : error accessing server";
                        Response.Redirect("../pages/accsrv.aspx?msg=" + pmsg, false); return;
                    }
                }
                if (puaction == "turnon2stepact")
                {
                    try
                    {
                        if (!(GetDataTableFromQuery("SELECT email FROM pusers WHERE email='" + pemail + "' AND pwd='" + ppassword + "';").Rows.Count > 0))
                        {
                            pmsg = puaction + " : email not registered or invalid password";
                            Response.Redirect("../pages/accsrv.aspx?msg=" + pmsg, false); return;
                        }
                        if ((bool)GetDataTableFromQuery("SELECT authsetup FROM pusers WHERE email='" + pemail + "';").Rows[0][0])
                        {
                            pmsg = puaction + " : 2-Step Verification already setup";
                            Response.Redirect("../pages/accsrv.aspx?msg=" + pmsg, false); return;
                        }
                        puid = (string)GetDataTableFromQuery("SELECT uid FROM pusers WHERE email='" + pemail + "';").Rows[0][0];
                        pukey = Guid.NewGuid().ToString();
                        ExecuteNonQuery("UPDATE pusers SET ukey='" + pukey + "', authkey='" + pukey + "' WHERE uid='" + puid + "';");
                        TwoFactorAuthenticator TwoFacAuth = new TwoFactorAuthenticator();
                        var setupInfo = TwoFacAuth.GenerateSetupCode("cnip", pemail, pukey, 150, 150);
                        pbarcode = setupInfo.QrCodeSetupImageUrl;
                        psetupcode = setupInfo.ManualEntryKey;
                        ExecuteNonQuery("INSERT INTO puserslog (uid, period, activity, location) VALUES ('" + puid + "','" + DateTime.UtcNow.ToString("yyyy-MM-dd hh:mm:ss") + "','" +
                            "turnon2step request" + "','" + GetUserIP() + "');");
                    }
                    catch (Exception)
                    {
                        pmsg = puaction + " : error accessing server";
                        Response.Redirect("../pages/accsrv.aspx?msg=" + pmsg, false); return;
                    }
                }
            }
        }
        public string Getuaction()
        {
            return puaction;
        }
        public string Getuid()
        {
            return puid;
        }
        public string Getukey()
        {
            return pukey;
        }
        public string Getbarcode()
        {
            return pbarcode;
        }
        public string Getsetupcode()
        {
            return psetupcode;
        }
        public string Get2stepen()
        {
            return p2stepen;
        }
        public string Getloginattempt()
        {
            return ploginattempt;
        }
    }
}