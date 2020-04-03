// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common
{
    using System;
    using System.Globalization;

    public static class Check
    {
        public static T IsNotNull<T>(T value, string parameterName) where T : class
        {
            if (value == null)
            {
                throw new ArgumentNullException(parameterName);
            }

            return value;
        }

        public static void CheckNotNull<T>(this T objectToCheck) where T : class
        {
            if (objectToCheck == null)
            {
                throw new ArgumentNullException(objectToCheck.GetType().ToString());
            }

            NullChecker<T>.Check(objectToCheck);
        }

        public static string IsNotEmptyOrWhiteSpace(string value, string parameterName)
        {
            if (string.IsNullOrWhiteSpace(value))
            {
                throw new ArgumentException(
                        string.Format(CultureInfo.InvariantCulture, "{0} is Null or Whitespace which are invalid", parameterName));
            }

            return value;
        }
    }
}
