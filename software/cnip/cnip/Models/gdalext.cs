using System;
using System.Runtime.InteropServices;
using OSGeo.GDAL;

namespace cnip.Models
{
    public static class gdalext
    {
        public struct GdalInfo
        {
            // GDAL affine transform coefficients 
            //    GT[0], X Min
            //    GT[1], X Pixel Size
            //    GT[2], X Coefficients 
            //    GT[3], Y Max
            //    GT[4], Y Coefficients 
            //    GT[5], Y Pixel size
            public double XMin { get; set; }
            public double XPixelSize { get; set; }
            public double XCoefficient { get; set; }
            public double YMax { get; set; }
            public double YCoefficient { get; set; }
            public double YPixelSize { get; set; }
            public double XMax { get; set; } // extended properties
            public double YMin { get; set; } // extended properties
            public double XRasterSize { get; set; } // extended properties
            public double YRasterSize { get; set; } // extended properties
            public GdalInfo(
                double xmin,
                double xpixelsize,
                double ymax,
                double ypixelsize)
            {
                XMin = xmin;
                XPixelSize = xpixelsize;
                YMax = ymax;
                YPixelSize = ypixelsize;
                XRasterSize = 0;
                YRasterSize = 0;
                XCoefficient = 0;
                YCoefficient = 0;
                XMax = 0;
                YMin = 0;
            }
        }
        public static bool Pgsql2Json(
            string pgSqlConn, string jsonFile, string[] options)
        {
            using (Dataset inputds = Gdal.OpenEx(
                pgSqlConn, (uint)GdalConst.OF_VECTOR, null, null, null))
            {
                if (inputds == null) { return false; }
                try
                {
                    Dataset test = Gdal.wrapper_GDALVectorTranslateDestName(
                        jsonFile, inputds,
                        new GDALVectorTranslateOptions(options), null, null);
                    if (test == null)
                    {
                        inputds.Dispose();
                        return false;
                    }
                    test.Dispose();
                }
                catch (Exception) { return false; }
                finally
                {
                    inputds.Dispose();
                }
            }
            return true;
        }
        public static bool Json2Pgsql(
            string jsonFile, string pgSqlConn, string[] options)
        {
            using (Dataset inputds = Gdal.OpenEx(
                jsonFile, (uint)GdalConst.OF_READONLY, null, null, null))
            {
                if (inputds == null) { return false; }
                Dataset outputds = Gdal.OpenEx(
                    pgSqlConn, (uint)(GdalConst.OF_VECTOR), null, null, null);
                if (outputds == null) { inputds.Dispose(); return false; }
                try
                {
                    int test = Gdal.wrapper_GDALVectorTranslateDestDS(
                        outputds, inputds,
                        new GDALVectorTranslateOptions(options), null, null);
                }
                catch (Exception) { return false; }
                finally
                {
                    inputds.Dispose();
                    outputds.Dispose();
                }
            }
            return true;
        }
        public static GdalInfo GetGdalInfo(string inputFile)
        {
            GdalInfo info = new GdalInfo();
            Dataset ds = Gdal.Open(inputFile, Access.GA_ReadOnly);
            double[] GT = new double[6];
            ds.GetGeoTransform(GT);
            info.XMin = GT[0];
            info.XPixelSize = GT[1];
            info.XCoefficient = GT[2];
            info.YMax = GT[3];
            info.YCoefficient = GT[4];
            info.YPixelSize = GT[5];
            info.XMax = GT[0] + GT[1] * ds.RasterXSize + GT[2] * ds.RasterYSize;
            info.YMin = GT[3] + GT[4] * ds.RasterXSize + GT[5] * ds.RasterYSize;
            info.XRasterSize = ds.RasterXSize;
            info.YRasterSize = ds.RasterYSize;
            ds.Dispose();
            return info;
        }
        public static void SetGdalInfo(string inputFile, GdalInfo info)
        {
            Dataset ds = Gdal.Open(inputFile, Access.GA_Update);
            double[] GT = new double[6];
            ds.GetGeoTransform(GT);
            GT[1] = info.XPixelSize; GT[5] = info.YPixelSize;
            GT[0] = info.XMin; GT[3] = info.YMax;
            ds.SetGeoTransform(GT);
            ds.Dispose();
        }
        public static bool Translate(
            string inputFile, string outputFile, string[] options)
        {
            using (Dataset inputds = Gdal.OpenEx(
                inputFile, (uint)GdalConst.OF_RASTER, null, null, null))
            {
                if (inputds == null) { return false; }
                try
                {
                    Dataset test = Gdal.wrapper_GDALTranslate(
                        outputFile, inputds,
                        new GDALTranslateOptions(options), null, null);
                    if (test == null)
                    {
                        inputds.Dispose();
                        return false;
                    }
                    test.Dispose();
                }
                catch (Exception) { return false; }
                finally
                {
                    inputds.Dispose();
                }
            }
            return true;
        }
        public static bool Warp(
            string inputFile, string outputFile, string[] options)
        {
            using (Dataset inputds = Gdal.Open(
                inputFile, Access.GA_ReadOnly))
            {
                if (inputds == null)
                {
                    return false;
                }
                IntPtr[] ptr = { Dataset.getCPtr(inputds).Handle };
                GCHandle gcHandle = GCHandle.Alloc(ptr, GCHandleType.Pinned);
                Dataset result = null;
                try
                {
                    SWIGTYPE_p_p_GDALDatasetShadow dss =
                        new SWIGTYPE_p_p_GDALDatasetShadow(
                            gcHandle.AddrOfPinnedObject(), false, null);
                    result = Gdal.wrapper_GDALWarpDestName(
                        outputFile, 1, dss,
                        new GDALWarpAppOptions(options), null, null);
                    if (result == null)
                    {
                        throw new Exception("GdalWarp failed: " +
                        Gdal.GetLastErrorMsg());
                    }
                }
                catch (Exception) { return false; }
                finally
                {
                    gcHandle.Free();
                    result.Dispose();
                }
            }
            return true;
        }
    }
}