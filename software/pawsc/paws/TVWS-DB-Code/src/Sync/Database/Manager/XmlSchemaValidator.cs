// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Sync.Database.Manager
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using System.Xml;
    using System.Xml.Schema;

    /// <summary>
    /// XML Schema Validator
    /// </summary>
    public static class XmlSchemaValidator
    {
        /// <summary>
        /// static xml Schema set holds the XSDs' attached.
        /// </summary>
        private static XmlSchemaSet regisrationsSchema = null;

        /// <summary>
        /// Validates the response xml with the given schema.
        /// </summary>
        /// <param name="xmlDoc"> XML document object</param>
        /// <param name="schemaFilePath"> Schema's file path </param>
        /// <returns> boolean Value</returns>
        public static bool ValidatePollResponseXMLWithSchema(XmlDocument xmlDoc, string schemaFilePath)
        {
            if (regisrationsSchema == null)
            {
                // loading schema
                regisrationsSchema = new XmlSchemaSet();
                regisrationsSchema.Add("http://www.whitespace-db-providers.org/2011//InterDB/xsd", schemaFilePath);
            }

            xmlDoc.Schemas = regisrationsSchema;
            xmlDoc.Validate(new ValidationEventHandler(XmlValidationEventHandler));
            return true;
        }

        /// <summary>
        /// XML Validation event handler.
        /// </summary>
        /// <param name="sender"> sender object </param>
        /// <param name="e"> XML Validation Arguments </param>
        private static void XmlValidationEventHandler(object sender, ValidationEventArgs e)
        {
            // validation error handler
            if (e.Severity == XmlSeverityType.Error)
            {
                throw new Exception("XML is not Valid");
            }
        }
    }
}
