using System;

namespace cnip.Models
{
    public static class strext
    {
        public static int ToInt(this string value)
        {
            int parsedInt;
            if (int.TryParse(value, out parsedInt))
            {
                return parsedInt;
            }
            return 0;
        }
        public static double ToDouble(this string value)
        {
            double parsedDouble;
            if (double.TryParse(value, out parsedDouble))
            {
                return parsedDouble;
            }
            return 0.0;
        }
        public static String TrimEnd(this string value, int NumChar)
        {
            if (value.Length > NumChar)
            {
                return value.Substring(0, value.Length - NumChar);
            }
            return "";
        }
        public static String Left(this string input, int length)
        {
            var result = "";
            if ((input.Length <= 0)) return result;
            if ((length > input.Length))
            {
                length = input.Length;
            }
            result = input.Substring(0, length);
            return result;
        }
        public static String Mid(this string input, int start, int length)
        {
            var result = "";
            if (((input.Length <= 0) ||
                (start >= input.Length))) return result;
            if ((start + length > input.Length))
            {
                length = (input.Length - start);
            }
            result = input.Substring(start, length);
            return result;
        }
        public static String Right(this string input, int length)
        {
            var result = "";
            if ((input.Length <= 0)) return result;
            if ((length > input.Length))
            {
                length = input.Length;
            }
            result = input.Substring((input.Length - length), length);
            return result;
        }
    }
}