// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    /// <summary>Represents Class CDBSAppTracking.</summary>
    public class CDBSAppTracking : CDBSTableEntity
    {
        /// <summary>Gets or sets the APP_STATUS_DATE</summary>
        /// <value>The APP_STATUS_DATE.</value>
        public string AppStatusDate { get; set; }

        /// <summary>Gets or sets the CUTOFF_DATE</summary>
        /// <value>The CUTOFF_DATE.</value>
        public string CutoffDate { get; set; }

        /// <summary>Gets or sets the CUTOFF_TYPE</summary>
        /// <value>The CUTOFF_TYPE.</value>
        public string CutoffType { get; set; }

        /// <summary>Gets or sets the CP_EXP_DATE</summary>
        /// <value>The CP_EXP_DATE.</value>
        public string CpExpDate { get; set; }

        /// <summary>Gets or sets the APP_STATUS</summary>
        /// <value>The APP_STATUS.</value>
        public string AppStatus { get; set; }

        /// <summary>Gets or sets the DTV_CHECKLIST</summary>
        /// <value>The DTV_CHECKLIST.</value>
        public string DtvChecklist { get; set; }

        /////// <summary>
        /////// Gets or sets the AMENDMENT_STAMPED_DATE
        /////// </summary>
        /////// <value>The AMENDMENT_STAMPED_DATE.</value>
        ////public string AmendmentStampedDate { get; set; }

        /// <summary>Gets or sets the ACCEPTED_DATE</summary>
        /// <value>The ACCEPTED_DATE.</value>
        public string AcceptedDate { get; set; }

        /// <summary>Gets or sets the TOLLING_CODE</summary>
        /// <value>The TOLLING_CODE.</value>
        public string TollingCode { get; set; }

        /////// <summary>
        /////// Gets or sets the LAST_CHANGE_DATE
        /////// </summary>
        /////// <value>The LAST_CHANGE_DATE.</value>
        ////public string LastChangeDate { get; set; }
    }
}
