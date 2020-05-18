using System.Data;
using System.Web.Mvc;
using static cnip.Models.pgsql;
namespace cnip.Controllers
{
    public class GlavniController : Controller
    {
        public ActionResult Index()
        {
            string puid = (string)Session["uid"];
            string pukey = (string)Session["ukey"];
            // check if ukey is valid
            if (!(puid is null) && !(pukey is null) && puid != "" && pukey != "")
            {
                DataTable dt = GetDataTableFromQuery(
                    "SELECT ukey FROM pusers WHERE uid='" + puid + "';");
                if (dt.Rows.Count > 0)
                {
                    if ((string)dt.Rows[0][0] == pukey)
                    {
                        //user s already logged in 
                        if (puid == "99999999")
                        {
                            ViewBag.Data = "redirecttowebadmin";
                        }
                        else
                        {
                            ViewBag.Data = "redirecttowebui";
                        }
                    }
                }
            }
            return View();
        }
    }
}