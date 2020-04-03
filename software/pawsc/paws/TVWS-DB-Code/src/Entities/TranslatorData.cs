// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
 
  /// <summary>
  /// Used to read data from "ws_translator_data.zip" file
  /// </summary>
  public class TranslatorData
    {
      /// <summary>
      /// callsign of a translator
      /// </summary>
      public string TranslatorCallsign { get; set; }

      /// <summary>
      /// whitespace id of a translator
      /// </summary>
      public string Whitespaces_Id { get; set; }

      /// <summary>
      /// Facility id of a translator
      /// </summary>
      public int Facility_Id { get; set; }
    }
}
