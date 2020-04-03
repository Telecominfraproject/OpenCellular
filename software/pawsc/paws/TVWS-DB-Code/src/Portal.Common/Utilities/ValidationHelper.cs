// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Common.Utilities
{
    using System.Collections.Generic;
    using Microsoft.Practices.EnterpriseLibrary.Validation;
    using Microsoft.Practices.EnterpriseLibrary.Validation.Validators;

    /// <summary>
    /// helper class for validating object
    /// </summary>
    public static class ValidationHelper
    {
        /// <summary>
        /// checks for validations defined on each property in a object
        /// </summary>
        /// <typeparam name="T">type of a object</typeparam>
        /// <param name="obj">input object</param>
        /// <returns>validation errors</returns>
        public static IEnumerable<string> Validate<T>(T obj)
        {
            Validator cusValidator = new ObjectValidator();
            ValidationResults valResults = cusValidator.Validate(obj);
            if (!valResults.IsValid)
            {
                foreach (ValidationResult item in valResults)
                {
                    yield return item.Message;
                }
            }
        }
    }
}
