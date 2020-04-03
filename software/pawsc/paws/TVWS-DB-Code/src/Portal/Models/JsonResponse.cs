// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.Portal
{
    public class JsonResponse
    {
        public bool IsError { get; set; }

        public string Message { get; set; }

        public bool IsValidationError { get; set; }

        public string[] ValidationErrors { get; set; }

        public bool IsSuccess { get; set; }
    }
}
