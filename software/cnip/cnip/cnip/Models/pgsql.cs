using System;
using System.Data;
using static System.Configuration.ConfigurationManager;
using Npgsql;
using static Npgsql.NpgsqlConnection;
using static cnip.Models.gisext;
using System.Linq;

namespace cnip.Models
{
    public static class pgsql
    {
        public static void DropTable(string tablename)
        {
            ExecuteNonQuery("DROP TABLE IF EXISTS " + tablename + ";");
        }
        public static bool TableExists(string tablename)
        {
            DataTable dt = GetDataTableFromQuery("SELECT * FROM " +
                "information_schema.tables WHERE table_schema = 'public' " +
                "AND table_name = '" + tablename + "';");
            return dt.Rows.Count > 0 ? true : false;
        }
        public static bool TableHasData(string tablename)
        {
            DataTable dt = GetDataTableFromQuery("SELECT EXISTS(SELECT * FROM " +
                tablename + ") AS has_row;");
            return dt.Rows.Count == 0 ? false :
                dt.Rows[0][0].ToString().ToLower() == "true" ? true : false;
        }
        public static DataTable GetDataTableFromQuery(
            string query, string connstr = "")
        {
            if (connstr == "")
            {
                connstr = ConnectionStrings["cnipdb"].ConnectionString;
            }
            GlobalTypeMapper.UseNetTopologySuite();
            using (NpgsqlConnection con = new NpgsqlConnection(connstr))
            using (NpgsqlDataAdapter da = new NpgsqlDataAdapter(query, con))
            {
                DataSet ds = new DataSet();
                da.Fill(ds);
                return ds.Tables[0];
            }
        }
        public static void ExecuteNonQuery(string query, string connstr = "")
        {
            if (connstr == "")
            {
                connstr = ConnectionStrings["cnipdb"].ConnectionString;
            }
            GlobalTypeMapper.UseNetTopologySuite();
            using (NpgsqlConnection con = new NpgsqlConnection(connstr))
            using (NpgsqlCommand cmd = new NpgsqlCommand(query, con))
            {
                con.Open();
                cmd.CommandTimeout = 0;
                cmd.CommandType = CommandType.Text;
                cmd.ExecuteNonQuery();
                con.Close();
            }
        }
        public static string BuildWhere(
            string values,
            string columnname,
            bool equals = true)
        {
            string where = "";
            if (values.Length > 0)
            {
                where = String.Join(" ",
                    values.Split(',').
                    Select(x => columnname +
                    (equals ? "=" : "<>") +
                    "'" + x + "' " +
                    (equals ? "OR" : "AND")).
                    ToArray());
                where =where.TrimEnd(equals ? 3 : 4);
                where = "(" + where + ") ";
            }
            return where;
        }
        public static Bounds MBR(Coords center, double radius)
        {
            Bounds mbr = new Bounds();
            Coords bottomLeft = new Coords();
            Coords topRight = new Coords();
            foreach (DataRow row in GetDataTableFromQuery(
                "SELECT ST_Project(Geography(ST_GeomFromText(" +
                "'POINT(" +
                center.X.ToString() + " " +
                center.Y.ToString() + ")',4326))," +
                Math.Sqrt(2 * Math.Pow(radius, 2)).ToString() +
                ",radians(225));").Rows)
            {
                bottomLeft.X = row[0].ToString().Replace(
                            "POINT(", "").Replace(")", "").Split(' ')[0].ToDouble();
                bottomLeft.Y = row[0].ToString().Replace(
                            "POINT(", "").Replace(")", "").Split(' ')[1].ToDouble();
            }
            foreach (DataRow row in GetDataTableFromQuery(
                "SELECT ST_Project(Geography(ST_GeomFromText(" +
                "'POINT(" +
                center.X.ToString() + " " +
                center.Y.ToString() + ")',4326))," +
                Math.Sqrt(2 * Math.Pow(radius, 2)).ToString() +
                ",radians(45));").Rows)
            {
                topRight.X = row[0].ToString().Replace(
                        "POINT(", "").Replace(")", "").Split(' ')[0].ToDouble();
                topRight.Y = row[0].ToString().Replace(
                            "POINT(", "").Replace(")", "").Split(' ')[1].ToDouble();
            }
            mbr.XMin = bottomLeft.X > topRight.X ? topRight.X : bottomLeft.X;
            mbr.XMax = bottomLeft.X > topRight.X ? bottomLeft.X : topRight.X;
            mbr.YMin = bottomLeft.Y > topRight.Y ? topRight.Y : bottomLeft.Y;
            mbr.YMax = bottomLeft.Y > topRight.Y ? bottomLeft.Y : topRight.Y;
            return mbr;
        }
        public static double Distance(Coords loc1, Coords loc2)
        {
            double d = 0;
            foreach (DataRow row in GetDataTableFromQuery(
                "SELECT ST_Distance(" +
                "Geography(ST_GeomFromText(" +
                "'POINT(" +
                loc1.X.ToString() + " " +
                loc1.Y.ToString() + ")',4326))," +
                "Geography(ST_GeomFromText(" +
                "'POINT(" +
                loc2.X.ToString() + " " +
                loc2.Y.ToString() + ")',4326))," +
                "true);").Rows)
            {
                d = row[0].ToString().ToDouble();
            }
            return d;
        }
    }
}