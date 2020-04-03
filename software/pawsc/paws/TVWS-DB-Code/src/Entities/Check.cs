// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Linq;
    using System.Text;

    /// <summary>
    /// Represents a Check class.
    /// </summary>
   public static class Check
    {
        /// <summary>
        /// Validates if the value is not null. 
        /// </summary>
        /// <typeparam name="T">Class to be validated</typeparam>
        /// <param name="value">Instance to be validated</param>
        /// <param name="parameterName">Instance name</param>
        /// <exception cref="System.ArgumentNullException">Thrown when argument is null</exception>
        /// <returns>Input instance</returns>
        public static T IsNotNull<T>(T value, string parameterName) where T : class
        {
            if (value == null)
            {
                throw new ArgumentNullException(parameterName);
            }

            return value;
        }
       
        /// <summary>
        /// Validates if a string value is not null, empty or white space
        /// </summary>
        /// <param name="value">String value</param>
        /// <param name="parameterName">variable name</param>
        /// <exception cref="System.ArgumentException">Thrown when string is null, empty or whitespace</exception>
        /// <returns>string value</returns>
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
