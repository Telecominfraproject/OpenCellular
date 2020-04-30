using System;
using System.Data;
using System.Web;
using static cnip.Models.pgsql;

namespace cnip.pages
{
    public partial class glavni : System.Web.UI.Page
    {
        protected void Page_Load(object sender, EventArgs e)
        {
            string puid = (string)HttpContext.Current.Session["uid"];
            string pukey = (string)HttpContext.Current.Session["ukey"];
            // check if ukey is valid
            if (!(puid is null) && !(pukey is null) && puid != "" && pukey != "")
            {
                DataTable dt = GetDataTableFromQuery("SELECT ukey FROM pusers WHERE uid='" + puid + "';");
                if (dt.Rows.Count > 0)
                {
                    if ((string)dt.Rows[0][0] == pukey)
                    {
                        //user s already logged in 
                        Response.Redirect("~/Home/Index"); return;
                    }
                }
            }
        }
    }
}