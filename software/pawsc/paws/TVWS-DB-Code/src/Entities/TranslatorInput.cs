// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    /// <summary>
    /// used to read data from "ws_translator_input_channels.zip" file
    /// </summary>
   public class TranslatorInput
    {       
       /// <summary>
        /// Whitespace id of a updated translator
       /// </summary>
       public string Whitespace_Id { get; set; }

       /// <summary>
       /// Primary / Parent callsign of a updated translator
       /// </summary>
       public string PrimaryCallSign { get; set; }

       /// <summary>
       /// Primary channel of a updated translator
       /// </summary>
       public int PrimaryChannel { get; set; }

       /// <summary>
       /// Primaru facility id of a updated translator
       /// </summary>
       public int PrimaryFacilityId { get; set; }

       /// <summary>
       /// primary common city of a updated translator
       /// </summary>
       public string PrimaryCommonCity { get; set; }

       /// <summary>
       /// Delivery method of a updated translator
       /// </summary>
       public string DelivaryMethod { get; set; }

       /// <summary>
       /// Program Original callsign of a updated translator
       /// </summary>
       public string ProgramOriginalCallsign { get; set; }

       /// <summary>
       /// Program Original channel of a updated translator
       /// </summary>
       public int ProgramOriginalChannel { get; set; }

       /// <summary>
       /// Program Original facility id of a updated translator
       /// </summary>
       public int ProgramOriginalFacilityId { get; set; }     
    }
}
