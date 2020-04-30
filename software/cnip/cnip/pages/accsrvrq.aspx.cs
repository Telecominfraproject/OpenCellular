using System;
using System.Data;
using System.Linq;
using System.Net;
using System.Net.Mail;
using static cnip.Models.pgsql;
using static cnip.Models.webext;

namespace cnip.pages
{
    public partial class accsrvrq : System.Web.UI.Page
    {
        string puaction;
        string pmsg;
        string pname;
        string pcompany;
        string pemail;
        string ppassword;
        string puid;
        string pukey;

        protected void Page_Load(object sender, EventArgs e)
        {
            if (!IsPostBack)
            {
                puaction = Request.QueryString["uaction"];
                pmsg = Request.QueryString["msg"];
                pname = Request.QueryString["name"];
                pcompany = Request.QueryString["company"];
                pemail = Request.QueryString["email"];
                ppassword = Request.QueryString["password"];

                if (!(pmsg is null)) { return; }
                if (puaction is null) { pmsg = "blank form submitted"; return; }

                switch (puaction)
                {
                    case "turnoff2step":
                        if (!(pemail is null && ppassword is null))
                        {
                            try
                            {
                                if (!(GetDataTableFromQuery("SELECT email FROM pusers WHERE email='" + pemail + "' AND pwd='" + ppassword + "';").Rows.Count > 0))
                                {
                                    pmsg = puaction + " : email not registered or invalid password";
                                }
                                else
                                {

                                    if (!((bool)GetDataTableFromQuery("SELECT accstatus FROM pusers WHERE email='" + pemail + "';").Rows[0][0] == true))
                                    {
                                        pmsg = puaction + " : account not active"; return;
                                    }
                                    if (!((bool)GetDataTableFromQuery("SELECT authsetup FROM pusers WHERE email='" + pemail + "';").Rows[0][0] == true))
                                    {
                                        pmsg = puaction + " : 2-Step Verification not setup"; return;
                                    }
                                    DataTable dt = GetDataTableFromQuery("SELECT uid, name FROM pusers WHERE email='" + pemail + "';");
                                    puid = (string)dt.Rows[0][0];
                                    pname = (string)dt.Rows[0][1];
                                    pukey = Guid.NewGuid().ToString();
                                    ExecuteNonQuery("UPDATE pusers SET ukey='" + pukey + "' WHERE uid='" + puid + "';");

                                    ExecuteNonQuery("INSERT INTO puserslog (uid, period, activity, location) VALUES ('" + puid + "','" + DateTime.UtcNow.ToString("yyyy-MM-dd hh:mm:ss") + "','" +
                                        "turnoff2step request" + "','" + GetUserIP() + "');");

                                    var body =
                                    "	<div style=\"background-color: white; font-size: 16px; color: #333333; font-family:'Numans', sans-serif; width: 100%; height: 100%; margin: 0px; padding: 0px\">" +
                                    "		<center style=\"width: 100%; table-layout: fixed; background-color: white\">" +
                                    "			<div style=\"max-width:600px;Margin:0 auto\">" +
                                    "				<table align=\"center\" width=\"100%\" bgcolor=\"white\" cellpadding=\"0\" cellspacing=\"0\" border=\"0\" style=\"background-color:white;width:100%;max-width:600px;margin:0px auto;padding:0px;border-collapse:collapse;border-spacing:0;border:0 none\">" +
                                    "					<tbody>" +
                                    "						<tr>" +
                                    "							<td align=\"left\" width=\"100%\" bgcolor=\"white\" cellpadding=\"0\" cellspacing=\"0\" border=\"0\" style=\"background-color:white;width:100%;Margin:0px;padding:0px 30px;border-collapse:collapse;border-spacing:0;border:0 none;font-family:'Numans', sans-serif;font-size:16px;line-height:24px;color:#333333\">" +
                                    "								<p style=\"height:30px;Margin:0px;padding:0px;font-size:1px;line-height:1px\">&nbsp;</p>" +
                                    "								<a href=\"http://18.220.148.72/cnip/\" target=\"_blank\">" +
                                    "									<img src=\"https://drive.google.com/uc?id=1Dt2FVWAuihcTwZ3tyhjOzbJ89SsdCGbo\" alt=\"CNIP\" width=\"440\" height=\"27\" border=\"0\" style=\"display:block;width:440px;height:27px;margin:0;padding:0;border:0 none\" class=\"CToWUd\">" +
                                    "								</a>" +
                                    "								<p style=\"height:30px;Margin:0px;padding:0px;font-size:1px;line-height:1px\">&nbsp;</p>" +
                                    "								<table width=\"100%\" cellpadding=\"0\" cellspacing=\"0\" border=\"0\">" +
                                    "									<tbody>" +
                                    "										<tr>" +
                                    "											<td height=\"1\" style=\"background:none;border:solid 1px #d6e3e3;border-width:1px 0 0 0;height:1px;width:100%;margin:0px 0px 0px 0px;font-size:0px;line-height:0px\">" +
                                    "												&nbsp;" +
                                    "											</td>" +
                                    "										</tr>" +
                                    "									</tbody>" +
                                    "								</table>" +
                                    "								<p style=\"height:40px;Margin:0px;padding:0px;font-size:1px;line-height:1px\">&nbsp;</p>" +
                                    "								<h3 style=\"font-family:'Numans', sans-serif;font-size:16px;line-height:24px;color:#000000;padding:0px;margin:0px 0px 10px 0px\">" +
                                    "									<strong>Hello " + pname + ",</strong>" +
                                    "								</h3>" +
                                    "								<p style=\"font-family:'Numans', sans-serif;font-size:16px;line-height:24px;color:#000000;padding:0px;margin:0px 0px 10px 0px\">" +
                                    "									You recently requested to deactivate 2-Step Verification for your account.<br/>" +
                                    "									To deactivate 2-Step Verification, click the button below." +
                                    "								</p>" +
                                    "								<p style=\"height:20px;Margin:0px;padding:0px;font-size:1px;line-height:1px\">&nbsp;</p>" +
                                    "								<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\">" +
                                    "									<tbody>" +
                                    "										<tr>" +
                                    "											<td align=\"center\" bgcolor=\"#0094ff\" style=\"border-radius:3px\">" +
                                    "												<a href=\"http://18.220.148.72/cnip/pages/accsrv.aspx?uid=" + puid + "&ukey=" + pukey + "&uaction=turnoff2step\" style =\"font-size:18px;font-family:'Numans', sans-serif;color:white;text-decoration:none;text-decoration:none;border-radius:3px;padding:12px 18px;border:1px solid #0094ff;display:inline-block;font-weight:bold\" target=\"_blank\">" +
                                    "													Turn Off 2 Step Verification" +
                                    "												</a>" +
                                    "											</td>" +
                                    "										</tr>" +
                                    "									</tbody>" +
                                    "								</table>" +
                                    "								<p style=\"height:20px;Margin:0px;padding:0px;font-size:1px;line-height:1px\">&nbsp;</p>" +
                                    "								<p style=\"font-family:'Numans', sans-serif;font-size:14px;line-height:22px;color:#5c6a70;padding:0px;Margin:0px\">" +
                                    "									If you didn\'t request to deactivate 2-Step Verification, you can safely ignore this email." +
                                    "								</p>" +
                                    "								<p style=\"height:40px;Margin:0px;padding:0px;font-size:1px;line-height:1px\">&nbsp;</p>" +
                                    "								<table width=\"100%\" cellpadding=\"0\" cellspacing=\"0\" border=\"0\">" +
                                    "									<tbody>" +
                                    "										<tr>" +
                                    "											<td height=\"1\" style=\"background:none;border:solid 1px #d6e3e3;border-width:1px 0 0 0;height:1px;width:100%;margin:0px 0px 0px 0px;font-size:0px;line-height:0px\">" +
                                    "												&nbsp;" +
                                    "											</td>" +
                                    "										</tr>" +
                                    "									</tbody>" +
                                    "								</table>" +
                                    "								<p style=\"height:30px;Margin:0px;padding:0px;font-size:1px;line-height:1px\">&nbsp;</p>" +
                                    "								<p style=\"font-family:'Numans', sans-serif;font-size:16px;line-height:24px;color:#000000;padding:0px;margin:0px 0px 10px 0px\">" +
                                    "									 Best regards," +
                                    "								</p>" +
                                    "								<p style=\"font-family:'Numans', sans-serif;font-size:16px;line-height:24px;color:#000000;padding:0px;margin:0px 0px 10px 0px\">" +
                                    "									<a href=\"mailto:opencellularcnip@gmail.com\" style=\"color:#000000;text-decoration:none\" target=\"_blank\">" +
                                    "										<strong>CNIP Team</strong> <br/> opencellularcnip@gmail.com" +
                                    "									</a>" +
                                    "								</p>" +
                                    "								<p style=\"height:20px;Margin:0px;padding:0px;font-size:1px;line-height:1px\">&nbsp;</p>" +
                                    "								<p style=\"font-family:'Numans', sans-serif;font-size:14px;line-height:22px;color:#5c6a70;padding:0px;margin:0px 0px 30px 0px\">" +
                                    "									This message is sent to " +
                                    "									<a href=\"mailto:" + pemail + "\" target=\"_blank\">" +
                                    "										" + pemail +
                                    "									</a>" +
                                    "									 because this email address was registered in CNIP service" +
                                    "								</p>" +
                                    "							</td>" +
                                    "						</tr>" +
                                    "					</tbody>" +
                                    "				</table>" +
                                    "			</div>" +
                                    "		</center>" +
                                    "	</div>";
                                    try
                                    {
                                        var senderEmail = new MailAddress("opencellularcnip@gmail.com", "CNIP");
                                        var receiverEmail = new MailAddress(pemail, pname);
                                        var password = "opencellularAdmin123";
                                        var sub = "Deactivate 2-Step Verification";
                                        var smtp = new SmtpClient
                                        {
                                            Host = "smtp.gmail.com",
                                            Port = 587,
                                            EnableSsl = true,
                                            DeliveryMethod = SmtpDeliveryMethod.Network,
                                            UseDefaultCredentials = false,
                                            Credentials = new NetworkCredential(senderEmail.Address, password)
                                        };
                                        using (var mess = new MailMessage(senderEmail, receiverEmail)
                                        {
                                            Subject = sub,
                                            Body = body,
                                            IsBodyHtml = true
                                        })
                                        {
                                            smtp.Send(mess);
                                        }

                                        pmsg = puaction + " : turn off 2-Step Verification link sent";
                                    }
                                    catch (Exception)
                                    {
                                        pmsg = puaction + " : failed to send email";
                                    }
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
                        break;
                    case "resetpassword":
                        if (!(pemail is null))
                        {
                            try
                            {
                                if (!(GetDataTableFromQuery("SELECT email FROM pusers WHERE email='" + pemail + "';").Rows.Count > 0))
                                {
                                    pmsg = puaction + " : email not registered";
                                }
                                else
                                {
                                    DataTable dt = GetDataTableFromQuery("SELECT uid, name FROM pusers WHERE email='" + pemail + "';");
                                    puid = (string)dt.Rows[0][0];
                                    pname = (string)dt.Rows[0][1];
                                    pukey = Guid.NewGuid().ToString();
                                    ExecuteNonQuery("UPDATE pusers SET ukey='" + pukey + "' WHERE uid='" + puid + "';");

                                    ExecuteNonQuery("INSERT INTO puserslog (uid, period, activity, location) VALUES ('" + puid + "','" + DateTime.UtcNow.ToString("yyyy-MM-dd hh:mm:ss") + "','" +
                                        "resetpassword request" + "','" + GetUserIP() + "');");

                                    var body =
                                    "	<div style=\"background-color: white; font-size: 16px; color: #333333; font-family:'Numans', sans-serif; width: 100%; height: 100%; margin: 0px; padding: 0px\">" +
                                    "		<center style=\"width: 100%; table-layout: fixed; background-color: white\">" +
                                    "			<div style=\"max-width:600px;Margin:0 auto\">" +
                                    "				<table align=\"center\" width=\"100%\" bgcolor=\"white\" cellpadding=\"0\" cellspacing=\"0\" border=\"0\" style=\"background-color:white;width:100%;max-width:600px;margin:0px auto;padding:0px;border-collapse:collapse;border-spacing:0;border:0 none\">" +
                                    "					<tbody>" +
                                    "						<tr>" +
                                    "							<td align=\"left\" width=\"100%\" bgcolor=\"white\" cellpadding=\"0\" cellspacing=\"0\" border=\"0\" style=\"background-color:white;width:100%;Margin:0px;padding:0px 30px;border-collapse:collapse;border-spacing:0;border:0 none;font-family:'Numans', sans-serif;font-size:16px;line-height:24px;color:#333333\">" +
                                    "								<p style=\"height:30px;Margin:0px;padding:0px;font-size:1px;line-height:1px\">&nbsp;</p>" +
                                    "								<a href=\"http://18.220.148.72/cnip/\" target=\"_blank\">" +
                                    "									<img src=\"https://drive.google.com/uc?id=1Dt2FVWAuihcTwZ3tyhjOzbJ89SsdCGbo\" alt=\"CNIP\" width=\"440\" height=\"27\" border=\"0\" style=\"display:block;width:440px;height:27px;margin:0;padding:0;border:0 none\" class=\"CToWUd\">" +
                                    "								</a>" +
                                    "								<p style=\"height:30px;Margin:0px;padding:0px;font-size:1px;line-height:1px\">&nbsp;</p>" +
                                    "								<table width=\"100%\" cellpadding=\"0\" cellspacing=\"0\" border=\"0\">" +
                                    "									<tbody>" +
                                    "										<tr>" +
                                    "											<td height=\"1\" style=\"background:none;border:solid 1px #d6e3e3;border-width:1px 0 0 0;height:1px;width:100%;margin:0px 0px 0px 0px;font-size:0px;line-height:0px\">" +
                                    "												&nbsp;" +
                                    "											</td>" +
                                    "										</tr>" +
                                    "									</tbody>" +
                                    "								</table>" +
                                    "								<p style=\"height:40px;Margin:0px;padding:0px;font-size:1px;line-height:1px\">&nbsp;</p>" +
                                    "								<h3 style=\"font-family:'Numans', sans-serif;font-size:16px;line-height:24px;color:#000000;padding:0px;margin:0px 0px 10px 0px\">" +
                                    "									<strong>Hello " + pname + ",</strong>" +
                                    "								</h3>" +
                                    "								<p style=\"font-family:'Numans', sans-serif;font-size:16px;line-height:24px;color:#000000;padding:0px;margin:0px 0px 10px 0px\">" +
                                    "									You recently requested to reset the password for your CNIP account.<br/>" +
                                    "									To reset password, click the button below." +
                                    "								</p>" +
                                    "								<p style=\"height:20px;Margin:0px;padding:0px;font-size:1px;line-height:1px\">&nbsp;</p>" +
                                    "								<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\">" +
                                    "									<tbody>" +
                                    "										<tr>" +
                                    "											<td align=\"center\" bgcolor=\"#0094ff\" style=\"border-radius:3px\">" +
                                    "												<a href=\"http://18.220.148.72/cnip/pages/accui.aspx?uaction=resetpasswordact&uid=" + puid + "&ukey=" + pukey + "\" style =\"font-size:18px;font-family:'Numans', sans-serif;color:white;text-decoration:none;text-decoration:none;border-radius:3px;padding:12px 18px;border:1px solid #0094ff;display:inline-block;font-weight:bold\" target=\"_blank\">" +
                                    "													Reset password" +
                                    "												</a>" +
                                    "											</td>" +
                                    "										</tr>" +
                                    "									</tbody>" +
                                    "								</table>" +
                                    "								<p style=\"height:20px;Margin:0px;padding:0px;font-size:1px;line-height:1px\">&nbsp;</p>" +
                                    "								<p style=\"font-family:'Numans', sans-serif;font-size:14px;line-height:22px;color:#5c6a70;padding:0px;Margin:0px\">" +
                                    "									If you didn\'t request a reset password, you can safely ignore this email." +
                                    "								</p>" +
                                    "								<p style=\"height:40px;Margin:0px;padding:0px;font-size:1px;line-height:1px\">&nbsp;</p>" +
                                    "								<table width=\"100%\" cellpadding=\"0\" cellspacing=\"0\" border=\"0\">" +
                                    "									<tbody>" +
                                    "										<tr>" +
                                    "											<td height=\"1\" style=\"background:none;border:solid 1px #d6e3e3;border-width:1px 0 0 0;height:1px;width:100%;margin:0px 0px 0px 0px;font-size:0px;line-height:0px\">" +
                                    "												&nbsp;" +
                                    "											</td>" +
                                    "										</tr>" +
                                    "									</tbody>" +
                                    "								</table>" +
                                    "								<p style=\"height:30px;Margin:0px;padding:0px;font-size:1px;line-height:1px\">&nbsp;</p>" +
                                    "								<p style=\"font-family:'Numans', sans-serif;font-size:16px;line-height:24px;color:#000000;padding:0px;margin:0px 0px 10px 0px\">" +
                                    "									 Best regards," +
                                    "								</p>" +
                                    "								<p style=\"font-family:'Numans', sans-serif;font-size:16px;line-height:24px;color:#000000;padding:0px;margin:0px 0px 10px 0px\">" +
                                    "									<a href=\"mailto:opencellularcnip@gmail.com\" style=\"color:#000000;text-decoration:none\" target=\"_blank\">" +
                                    "										<strong>CNIP Team</strong> <br/> opencellularcnip@gmail.com" +
                                    "									</a>" +
                                    "								</p>" +
                                    "								<p style=\"height:20px;Margin:0px;padding:0px;font-size:1px;line-height:1px\">&nbsp;</p>" +
                                    "								<p style=\"font-family:'Numans', sans-serif;font-size:14px;line-height:22px;color:#5c6a70;padding:0px;margin:0px 0px 30px 0px\">" +
                                    "									This message is sent to " +
                                    "									<a href=\"mailto:" + pemail + "\" target=\"_blank\">" +
                                    "										" + pemail +
                                    "									</a>" +
                                    "									 because this email address was registered in CNIP service" +
                                    "								</p>" +
                                    "							</td>" +
                                    "						</tr>" +
                                    "					</tbody>" +
                                    "				</table>" +
                                    "			</div>" +
                                    "		</center>" +
                                    "	</div>";
                                    try
                                    {
                                        var senderEmail = new MailAddress("opencellularcnip@gmail.com", "CNIP");
                                        var receiverEmail = new MailAddress(pemail, pname);
                                        var password = "opencellularAdmin123";
                                        var sub = "Reset password";
                                        var smtp = new SmtpClient
                                        {
                                            Host = "smtp.gmail.com",
                                            Port = 587,
                                            EnableSsl = true,
                                            DeliveryMethod = SmtpDeliveryMethod.Network,
                                            UseDefaultCredentials = false,
                                            Credentials = new NetworkCredential(senderEmail.Address, password)
                                        };
                                        using (var mess = new MailMessage(senderEmail, receiverEmail)
                                        {
                                            Subject = sub,
                                            Body = body,
                                            IsBodyHtml = true
                                        })
                                        {
                                            smtp.Send(mess);
                                        }
                                        pmsg = puaction + " : user created successfully";

                                        pmsg = puaction + " : link sent";
                                    }
                                    catch (Exception)
                                    {
                                        pmsg = puaction + " : failed to send email";
                                    }
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
                        break;
                    case "signup":
                        if (!(pname is null && pcompany is null && pemail is null && ppassword is null))
                        {
                            try
                            {
                                if (GetDataTableFromQuery("SELECT email FROM pusers WHERE email='" + pemail + "';").Rows.Count > 0)
                                {
                                    pmsg = puaction + " : email address already registered";
                                }
                                else
                                {

                                    pukey = Guid.NewGuid().ToString();
                                    puid = new Random().Next(1, 65535).ToString("D6");
                                    while (GetDataTableFromQuery("SELECT uid FROM pusers WHERE uid='" + puid + "';").Rows.Count > 0)
                                    {
                                        puid = new Random().Next(1, 65535).ToString("D6");
                                    }
                                    ExecuteNonQuery("INSERT INTO pusers(uid, ukey, name, company, email, pwd, accstatus, authsetup, authkey) VALUES ('" + puid + "','" + pukey + "','" + pname + "','" + pcompany + "','" + pemail + "','" + ppassword + "',false,false,'');");

                                    ExecuteNonQuery("INSERT INTO puserslog (uid, period, activity, location) VALUES ('" + puid + "','" + DateTime.UtcNow.ToString("yyyy-MM-dd hh:mm:ss") + "','" +
                                        "signup request" + "','" + GetUserIP() + "');");

                                    var body =
                                    "	<div style=\"background-color: #ffffff; font-size: 16px; color: #333333; font-family:'Numans', sans-serif; width: 100%; height: 100%; margin: 0px; padding: 0px\">" +
                                    "		<center style=\"width: 100%; table-layout: fixed; background-color: #ffffff\">" +
                                    "			<div style=\"max-width:600px;Margin:0 auto\">" +
                                    "				<table align=\"center\" width=\"100%\" bgcolor=\"white\" cellpadding=\"0\" cellspacing=\"0\" border=\"0\" style=\"background-color:white;width:100%;max-width:600px;margin:0px auto;padding:0px;border-collapse:collapse;border-spacing:0;border:0 none\">" +
                                    "					<tbody>" +
                                    "						<tr>" +
                                    "							<td align=\"left\" width=\"100%\" bgcolor=\"white\" cellpadding=\"0\" cellspacing=\"0\" border=\"0\" style=\"background-color:white;width:100%;Margin:0px;padding:0px 30px;border-collapse:collapse;border-spacing:0;border:0 none;font-family:'Numans', sans-serif;font-size:16px;line-height:24px;color:#333333\">" +
                                    "								<p style=\"height:30px;Margin:0px;padding:0px;font-size:1px;line-height:1px\">&nbsp;</p>" +
                                    "								<a href=\"http://18.220.148.72/cnip/\" target=\"_blank\">" +
                                    "									<img src=\"https://drive.google.com/uc?id=1Dt2FVWAuihcTwZ3tyhjOzbJ89SsdCGbo\" alt=\"CNIP\" width=\"440\" height=\"27\" border=\"0\" style=\"display:block;width:440px;height:27px;margin:0;padding:0;border:0 none\" class=\"CToWUd\">" +
                                    "								</a>" +
                                    "								<p style=\"height:30px;Margin:0px;padding:0px;font-size:1px;line-height:1px\">&nbsp;</p>" +
                                    "								<table width=\"100%\" cellpadding=\"0\" cellspacing=\"0\" border=\"0\">" +
                                    "									<tbody>" +
                                    "										<tr>" +
                                    "											<td height=\"1\" style=\"background:none;border:solid 1px #d6e3e3;border-width:1px 0 0 0;height:1px;width:100%;margin:0px 0px 0px 0px;font-size:0px;line-height:0px\">" +
                                    "												&nbsp;" +
                                    "											</td>" +
                                    "										</tr>" +
                                    "									</tbody>" +
                                    "								</table>" +
                                    "								<p style=\"height:40px;Margin:0px;padding:0px;font-size:1px;line-height:1px\">&nbsp;</p>" +
                                    "								<h3 style=\"font-family:'Numans', sans-serif;font-size:16px;line-height:24px;color:#000000;padding:0px;margin:0px 0px 10px 0px\">" +
                                    "									<strong>Hello " + pname + ",</strong>" +
                                    "								</h3>" +
                                    "								<p style=\"font-family:'Numans', sans-serif;font-size:16px;line-height:24px;color:#000000;padding:0px;margin:0px 0px 10px 0px\">" +
                                    "									Thank you for registering with CNIP.<br/>" +
                                    "									To confirm your email, click the button below." +
                                    "								</p>" +
                                    "								<p style=\"height:20px;Margin:0px;padding:0px;font-size:1px;line-height:1px\">&nbsp;</p>" +
                                    "								<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\">" +
                                    "									<tbody>" +
                                    "										<tr>" +
                                    "											<td align=\"center\" bgcolor=\"#0094ff\" style=\"border-radius:3px\">" +
                                    "												<a href=\"http://18.220.148.72/cnip/pages/accsrv.aspx?uid=" + puid + "&ukey=" + pukey + "&uaction=activate\" style =\"font-size:18px;font-family:'Numans', sans-serif;color:white;text-decoration:none;text-decoration:none;border-radius:3px;padding:12px 18px;border:1px solid #0094ff;display:inline-block;font-weight:bold\" target=\"_blank\">" +
                                    "													Confirm email" +
                                    "												</a>" +
                                    "											</td>" +
                                    "										</tr>" +
                                    "									</tbody>" +
                                    "								</table>" +
                                    "								<p style=\"height:20px;Margin:0px;padding:0px;font-size:1px;line-height:1px\">&nbsp;</p>" +
                                    "								<p style=\"font-family:'Numans', sans-serif;font-size:14px;line-height:22px;color:#5c6a70;padding:0px;Margin:0px\">" +
                                    "									After confirming your email address, you will receive only important messages from CNIP." +
                                    "									Thank you for choosing CNIP!" +
                                    "								</p>" +
                                    "								<p style=\"height:40px;Margin:0px;padding:0px;font-size:1px;line-height:1px\">&nbsp;</p>" +
                                    "								<table width=\"100%\" cellpadding=\"0\" cellspacing=\"0\" border=\"0\">" +
                                    "									<tbody>" +
                                    "										<tr>" +
                                    "											<td height=\"1\" style=\"background:none;border:solid 1px #d6e3e3;border-width:1px 0 0 0;height:1px;width:100%;margin:0px 0px 0px 0px;font-size:0px;line-height:0px\">" +
                                    "												&nbsp;" +
                                    "											</td>" +
                                    "										</tr>" +
                                    "									</tbody>" +
                                    "								</table>" +
                                    "								<p style=\"height:30px;Margin:0px;padding:0px;font-size:1px;line-height:1px\">&nbsp;</p>" +
                                    "								<p style=\"font-family:'Numans', sans-serif;font-size:16px;line-height:24px;color:#000000;padding:0px;margin:0px 0px 10px 0px\">" +
                                    "									 Best regards," +
                                    "								</p>" +
                                    "								<p style=\"font-family:'Numans', sans-serif;font-size:16px;line-height:24px;color:#000000;padding:0px;margin:0px 0px 10px 0px\">" +
                                    "									<a href=\"mailto:opencellularcnip@gmail.com\" style=\"color:#000000;text-decoration:none\" target=\"_blank\">" +
                                    "										<strong>CNIP Team</strong> <br/> opencellularcnip@gmail.com" +
                                    "									</a>" +
                                    "								</p>" +
                                    "								<p style=\"height:20px;Margin:0px;padding:0px;font-size:1px;line-height:1px\">&nbsp;</p>" +
                                    "								<p style=\"font-family:'Numans', sans-serif;font-size:14px;line-height:22px;color:#5c6a70;padding:0px;margin:0px 0px 30px 0px\">" +
                                    "									This message is sent to " +
                                    "									<a href=\"mailto:" + pemail + "\" target=\"_blank\">" +
                                    "										" + pemail +
                                    "									</a>" +
                                    "									 because this email address was registered in CNIP service" +
                                    "								</p>" +
                                    "							</td>" +
                                    "						</tr>" +
                                    "					</tbody>" +
                                    "				</table>" +
                                    "			</div>" +
                                    "		</center>" +
                                    "	</div>";
                                    try
                                    {
                                        var senderEmail = new MailAddress("opencellularcnip@gmail.com", "CNIP");
                                        var receiverEmail = new MailAddress(pemail, pname);
                                        var password = "opencellularAdmin123";
                                        var sub = "Confirm your CNIP account, " + pname;
                                        var smtp = new SmtpClient
                                        {
                                            Host = "smtp.gmail.com",
                                            Port = 587,
                                            EnableSsl = true,
                                            DeliveryMethod = SmtpDeliveryMethod.Network,
                                            UseDefaultCredentials = false,
                                            Credentials = new NetworkCredential(senderEmail.Address, password)
                                        };
                                        using (var mess = new MailMessage(senderEmail, receiverEmail)
                                        {
                                            Subject = sub,
                                            Body = body,
                                            IsBodyHtml = true
                                        })
                                        {
                                            smtp.Send(mess);
                                        }
                                        pmsg = puaction + " : user created successfully";
                                    }
                                    catch (Exception)
                                    {
                                        ExecuteNonQuery("Delete FROM pusers WHERE uid='" + puid + "';");
                                        pmsg = puaction + " : failed to send email";
                                    }
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
                        break;
                    default:
                        pmsg = "blank form submitted";
                        break;
                }
            }
        }
        public string Getmsg()
        {
            return pmsg;
        }
    }
}