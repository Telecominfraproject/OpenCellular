// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System.ComponentModel.DataAnnotations;
    using Newtonsoft.Json;

    /// <summary>
    /// Represents an email component of a paws request.
    /// </summary>
    public class Email
    {
        /// <summary>
        /// Gets or sets the email text.
        /// </summary>
        [JsonProperty(Constants.PropertyNameText)]
        [RegularExpression(Constants.RegExEmail, ErrorMessage = Constants.ErrorMessageInvalidEmail)]
        public string Text { get; set; }
    }
}
