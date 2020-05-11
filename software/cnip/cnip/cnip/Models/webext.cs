using System.Web;
namespace cnip.Models
{
    public class webext
    {
        public static string GetUserIP()
        {
            string ipList = string.Empty;
            HttpContext context = HttpContext.Current;
            if (context.Request.Headers["True-Client-IP"] != null)
            {
                ipList = context.Request.Headers["True-Client-IP"];
            }
            else if (context.Request.Headers["X-ClientSide"] != null)
            {
                ipList = context.Request.Headers["X-ClientSide"];
            }
            if (string.IsNullOrEmpty(ipList))
            {
                ipList = context.Request.ServerVariables["HTTP_X_FORWARDED_FOR"];
            }
            if (string.IsNullOrEmpty(ipList))
            {
                ipList = context.Request.ServerVariables["HTTP_CLIENT_IP"];
            }
            if (string.IsNullOrEmpty(ipList))
            {
                ipList = context.Request.ServerVariables["HTTP_X_FORWARDED"];
            }
            if (string.IsNullOrEmpty(ipList))
            {
                ipList = context.Request.ServerVariables["HTTP_X_CLUSTER_CLIENT_IP"];
            }
            if (string.IsNullOrEmpty(ipList))
            {
                ipList = context.Request.ServerVariables["HTTP_FORWARDED_FOR"];
            }
            if (string.IsNullOrEmpty(ipList))
            {
                ipList = context.Request.ServerVariables["HTTP_FORWARDED"];
            }
            if (string.IsNullOrEmpty(ipList))
            {
                ipList = context.Request.ServerVariables["HTTP_VIA"];
            }
            if (string.IsNullOrEmpty(ipList))
            {
                ipList = context.Request.UserHostAddress;
            }
            if (!string.IsNullOrEmpty(ipList))
            {
                return ipList.Split(',')[0];
            }
            return context.Request.ServerVariables["REMOTE_ADDR"];
        }
    }
}