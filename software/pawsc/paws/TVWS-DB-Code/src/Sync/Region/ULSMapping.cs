// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Sync.Region
{
    using System.Collections.Generic;

    /// <summary>Represents Class ULSMapping.</summary>
    public static class ULSMapping
    {
        /// <summary>The mappings</summary>
        private static Dictionary<int, Dictionary<string, int>> mappings = new Dictionary<int, Dictionary<string, int>>();

        /// <summary>Initializes static members of the <see cref="ULSMapping" /> class.</summary>
        static ULSMapping()
        {
            AddHDMapping();

            AddADMapping();

            AddENMapping();

            AddA2Mapping();

            AddREMapping();

            AddMWMapping();

            AddCGMapping();

            AddFAMapping();

            AddSHMapping();

            AddSRMapping();

            AddSEMapping();

            AddSVMapping();

            AddLMMapping();

            AddMIMapping();

            AddBCMapping();

            AddFCMapping();

            AddHSMapping();

            AddCOMapping();

            AddTAMapping();

            AddBDMapping();

            AddASMapping();

            AddCFMapping();

            AddIAMapping();

            AddSCMapping();

            AddSFMapping();

            AddBOMapping();

            AddCPMapping();

            AddSIMapping();

            AddUAMapping();

            AddACMapping();

            AddAMMapping();

            AddVCMapping();

            AddMKMapping();

            AddTLMapping();

            AddMPMapping();

            AddMCMapping();

            AddMFMapping();

            AddLSMapping();

            AddLOMapping();

            AddL2Mapping();

            AddLFMapping();

            AddOPMapping();

            AddBLMapping();

            AddANMapping();

            AddRCMapping();

            AddRZMapping();

            AddFRMapping();

            AddF2Mapping();

            AddFTMapping();

            AddIRMapping();

            AddCSMapping();

            AddFSMapping();

            AddFFMapping();

            AddBFMapping();

            AddRAMapping();

            AddEMMapping();

            AddPCMapping();

            AddPAMapping();

            AddSGMapping();

            AddATMapping();

            AddLHMapping();

            AddBEMapping();

            AddMHMapping();

            AddAHMapping();

            AddMEMapping();

            AddLAMapping();

            AddCDMapping();

            AddRIMapping();

            AddLDMapping();

            AddLLMapping();

            AddLCMapping();

            AddL3Mapping();

            AddL4Mapping();

            AddO2Mapping();

            AddL5Mapping();

            AddL6Mapping();

            AddA3Mapping();

            AddF3Mapping();

            AddF4Mapping();

            AddF5Mapping();

            AddF6Mapping();

            AddP2Mapping();

            AddTPMapping();

            AddNAMapping();

            AddVNMapping();

            AddRDMapping();

            AddL1Mapping();

            AddCHMapping();
        }

        /// <summary>Gets the record mapping.</summary>
        /// <param name="recordType">Type of the record.</param>
        /// <returns>returns mapping.</returns>
        public static Dictionary<string, int> GetRecordMapping(ULSRecordType recordType)
        {
            return mappings[(int)recordType];
        }

        /// <summary>
        /// Adds the HD mapping.
        /// </summary>
        private static void AddHDMapping()
        {
            Dictionary<string, int> mappingHD = new Dictionary<string, int>();
            mappingHD.Add("RecordType", 0);
            mappingHD.Add("UniqueSystemIdentifier", 1);
            mappingHD.Add("ULSFileNumber", 2);
            mappingHD.Add("EBFNumber", 3);
            mappingHD.Add("CallSign", 4);
            mappingHD.Add("LicenseStatus", 5);
            mappingHD.Add("RadioServiceCode", 6);
            mappingHD.Add("GrantDate", 7);
            mappingHD.Add("ExpiredDate", 8);
            mappingHD.Add("CancellationDate", 9);
            mappingHD.Add("EligibilityRuleNum", 10);
            mappingHD.Add("Reserved", 11);
            mappingHD.Add("Alien", 12);
            mappingHD.Add("AlienGovernment", 13);
            mappingHD.Add("AlienCorporation", 14);
            mappingHD.Add("AlienOfficer", 15);
            mappingHD.Add("AlienControl", 16);
            mappingHD.Add("Revoked", 17);
            mappingHD.Add("Convicted", 18);
            mappingHD.Add("Adjudged", 19);
            mappingHD.Add("CommonCarrier", 21);
            mappingHD.Add("NonCommonCarrier", 22);
            mappingHD.Add("PrivateComm", 23);
            mappingHD.Add("Fixed", 24);
            mappingHD.Add("Mobile", 25);
            mappingHD.Add("Radiolocation", 26);
            mappingHD.Add("Satellite", 27);
            mappingHD.Add("DevelopmentalOrSTAOrDemonstration", 28);
            mappingHD.Add("InterconnectedService", 29);
            mappingHD.Add("CertifierFirstName", 30);
            mappingHD.Add("CertifierMI", 31);
            mappingHD.Add("CertifierLastName", 32);
            mappingHD.Add("CertifierSuffix", 33);
            mappingHD.Add("CertifierTitle", 34);
            mappingHD.Add("Female", 35);
            mappingHD.Add("BlackOrAfricanAmerican", 36);
            mappingHD.Add("NativeAmerican", 37);
            mappingHD.Add("Hawaiian", 38);
            mappingHD.Add("Asian", 39);
            mappingHD.Add("White", 40);
            mappingHD.Add("Hispanic", 41);
            mappingHD.Add("EffectiveDate", 42);
            mappingHD.Add("LastActionDate", 43);
            mappingHD.Add("AuctionID", 44);
            mappingHD.Add("BroadcastServicesRegulatoryStatus", 45);
            mappingHD.Add("BandManagerRegulatoryStatus", 46);
            mappingHD.Add("BroadcastServicesTypeOfRadioService", 47);
            mappingHD.Add("AlienRuling", 48);
            mappingHD.Add("LicenseeNameChange", 49);
            mappings.Add((int)ULSRecordType.HD, mappingHD);
        }

        /// <summary>
        /// Adds the AD mapping.
        /// </summary>
        private static void AddADMapping()
        {
            Dictionary<string, int> mappingAD = new Dictionary<string, int>();
            mappingAD.Add("RecordType", 0);
            mappingAD.Add("UniqueSystemIdentifier", 1);
            mappingAD.Add("ULSFileNumber", 2);
            mappingAD.Add("EBFNumber", 3);
            mappingAD.Add("ApplicationPurpose", 4);
            mappingAD.Add("ApplicationStatus", 5);
            mappingAD.Add("ApplicationFeeExempt", 6);
            mappingAD.Add("RegulatoryFeeExempt", 7);
            mappingAD.Add("Source", 8);
            mappingAD.Add("RequestedExpirationDate", 9);
            mappingAD.Add("ReceiptDate", 10);
            mappingAD.Add("NotificationCode", 11);
            mappingAD.Add("NotificationDate", 12);
            mappingAD.Add("ExpandingAreaOrContour", 13);
            mappingAD.Add("MajorMinorIndicator", 14);
            mappingAD.Add("OriginalApplicationPurpose", 15);
            mappingAD.Add("RequestingAWaiver", 16);
            mappingAD.Add("HowManyWaiversRequested", 17);
            mappingAD.Add("AnyAttachments", 18);
            mappingAD.Add("NumberOfRequestedSIDs", 19);
            mappingAD.Add("FeeControlNumber", 20);
            mappingAD.Add("DateEntered", 21);
            mappingAD.Add("(NoLongerUsed)", 22);
            mappingAD.Add("FrequencyCoordinationIndicator", 23);
            mappingAD.Add("EmergencySTA", 24);
            mappingAD.Add("OverallChangeType", 25);
            mappingAD.Add("ExtendedImplementationPlan", 26);
            mappingAD.Add("Grandfathered,ApprovedOrIntegrated", 27);
            mappingAD.Add("Waiver/DeferralOfApplicationFees", 28);
            mappingAD.Add("HasTermPendingInd", 29);
            mappings.Add((int)ULSRecordType.AD, mappingAD);
        }

        /// <summary>
        /// Adds the EN mapping.
        /// </summary>
        private static void AddENMapping()
        {
            Dictionary<string, int> mappingEN = new Dictionary<string, int>();
            mappingEN.Add("RecordType", 0);
            mappingEN.Add("UniqueSystemIdentifier", 1);
            mappingEN.Add("ULSFileNumber", 2);
            mappingEN.Add("EBFNumber", 3);
            mappingEN.Add("CallSign", 4);
            mappingEN.Add("EntityType", 5);
            mappingEN.Add("LicenseeID", 6);
            mappingEN.Add("EntityName", 7);
            mappingEN.Add("FirstName", 8);
            mappingEN.Add("MI", 9);
            mappingEN.Add("LastName", 10);
            mappingEN.Add("Suffix", 11);
            mappingEN.Add("Phone", 12);
            mappingEN.Add("Fax", 13);
            mappingEN.Add("Email", 14);
            mappingEN.Add("StreetAddress", 15);
            mappingEN.Add("City", 16);
            mappingEN.Add("State", 17);
            mappingEN.Add("ZipCode", 18);
            mappingEN.Add("POBox", 19);
            mappingEN.Add("AttentionLine", 20);
            mappingEN.Add("SGIN", 21);
            mappingEN.Add("FCCRegistrationNumber(FRN)", 22);
            mappingEN.Add("ApplicantTypeCode", 23);
            mappingEN.Add("ApplicantTypeCodeOther", 24);
            mappingEN.Add("StatusCode", 25);
            mappingEN.Add("StatusDate", 26);
            mappings.Add((int)ULSRecordType.EN, mappingEN);
        }

        /// <summary>
        /// Adds the A2 mapping.
        /// </summary>
        private static void AddA2Mapping()
        {
            Dictionary<string, int> mappingA2 = new Dictionary<string, int>();
            mappingA2.Add("RecordType", 0);
            mappingA2.Add("UniqueSystemIdentifier", 1);
            mappingA2.Add("ULSFileNumber", 2);
            mappingA2.Add("EBFNumber", 3);
            mappingA2.Add("SpectrumManagerLeasing", 4);
            mappingA2.Add("DefactoTransferLeasing", 5);
            mappingA2.Add("NewSpectrumLeasing", 6);
            mappingA2.Add("SpectrumSubleasing", 7);
            mappingA2.Add("TransferOfControlOfLessee", 8);
            mappingA2.Add("RevisionOfSpectrumLease", 9);
            mappingA2.Add("AssignmentOfSpectrumLease", 10);
            mappingA2.Add("PFRStatus", 11);
            mappings.Add((int)ULSRecordType.A2, mappingA2);
        }

        /// <summary>
        /// Adds the RE mapping.
        /// </summary>
        private static void AddREMapping()
        {
            Dictionary<string, int> mappingRE = new Dictionary<string, int>();
            mappingRE.Add("RecordType", 0);
            mappingRE.Add("UniqueSystemIdentifier", 1);
            mappingRE.Add("ULSFileNumber", 2);
            mappingRE.Add("EBFNumber", 3);
            mappingRE.Add("Reason", 4);
            mappings.Add((int)ULSRecordType.RE, mappingRE);
        }

        /// <summary>
        /// Adds the MW mapping.
        /// </summary>
        private static void AddMWMapping()
        {
            Dictionary<string, int> mappingMW = new Dictionary<string, int>();
            mappingMW.Add("RecordType", 0);
            mappingMW.Add("UniqueSystemIdentifier", 1);
            mappingMW.Add("ULSFileNumber", 2);
            mappingMW.Add("EBFNumber", 3);
            mappingMW.Add("CallSign", 4);
            mappingMW.Add("PackIndicator", 5);
            mappingMW.Add("PackRegistrationNumber", 6);
            mappingMW.Add("PackName", 7);
            mappingMW.Add("TypeOfOperation", 8);
            mappingMW.Add("SMSACode", 9);
            mappingMW.Add("StationClass", 10);
            mappingMW.Add("CummulativeEffectIsMajor", 11);
            mappingMW.Add("StatusCode", 12);
            mappingMW.Add("StatusDate", 13);
            mappings.Add((int)ULSRecordType.MW, mappingMW);
        }

        /// <summary>
        /// Adds the CG mapping.
        /// </summary>
        private static void AddCGMapping()
        {
            Dictionary<string, int> mappingCG = new Dictionary<string, int>();
            mappingCG.Add("RecordType", 0);
            mappingCG.Add("UniqueSystemIdentifier", 1);
            mappingCG.Add("ULSFileNumber", 2);
            mappingCG.Add("EBFNumber", 3);
            mappingCG.Add("CallSign", 4);
            mappingCG.Add("StationAvailable", 5);
            mappingCG.Add("PublicCorrespondence", 6);
            mappingCG.Add("StationIdentifier", 7);
            mappingCG.Add("AeronauticalEnrouteCallSign", 8);
            mappingCG.Add("FAAOfficeNotified", 9);
            mappingCG.Add("DateFAANotified", 10);
            mappingCG.Add("SeekingAuthorization", 11);
            mappingCG.Add("RegularlyEngaged", 12);
            mappingCG.Add("Engaged", 13);
            mappingCG.Add("PublicMooring", 14);
            mappingCG.Add("Servicing", 15);
            mappingCG.Add("FixedStation", 16);
            mappingCG.Add("MaritimeSupport", 17);
            mappingCG.Add("AeronauticalFixed", 18);
            mappingCG.Add("Unicom", 19);
            mappingCG.Add("SearchAndRescue", 20);
            mappingCG.Add("FlightTestUHF", 21);
            mappingCG.Add("FlightTestManufacturer", 22);
            mappingCG.Add("FlightTestParentCorporation", 23);
            mappingCG.Add("FlightTestEducational", 24);
            mappingCG.Add("FlightSchoolCertitication", 25);
            mappingCG.Add("LighterThanAir", 26);
            mappingCG.Add("Ballooning", 27);
            mappingCG.Add("LocatedAtAirport", 28);
            mappingCG.Add("RadiodeterminationNotFAA", 29);
            mappingCG.Add("RadiodeterminationEquipment", 30);
            mappingCG.Add("RadiodeterminationPublic", 31);
            mappingCG.Add("RadiodeterminationELTs", 32);
            mappingCG.Add("CivilAirPatrol", 33);
            mappingCG.Add("AeronauticalEnroute", 34);
            mappingCG.Add("MobileRoutine", 35);
            mappingCG.Add("MobileOwner/Operator", 36);
            mappingCG.Add("MobileAgreement", 37);
            mappingCG.Add("Coast/GroundIdentifier", 38);
            mappingCG.Add("SelectiveCallSignIdentifier", 39);
            mappingCG.Add("ClassStationCode", 40);
            mappingCG.Add("StatusCode", 41);
            mappingCG.Add("StatusDate", 42);
            mappings.Add((int)ULSRecordType.CG, mappingCG);
        }

        /// <summary>
        /// Adds the FA mapping.
        /// </summary>
        private static void AddFAMapping()
        {
            Dictionary<string, int> mappingFA = new Dictionary<string, int>();
            mappingFA.Add("RecordType", 0);
            mappingFA.Add("UniqueSystemIdentifier", 1);
            mappingFA.Add("ULSFileNumber", 2);
            mappingFA.Add("EBFNumber", 3);
            mappingFA.Add("CallSign", 4);
            mappingFA.Add("OperatorClassCode", 5);
            mappingFA.Add("ShipRadarEndorsement", 6);
            mappingFA.Add("SixMonthEndorsement", 7);
            mappingFA.Add("DateOfBirth", 8);
            mappingFA.Add("CertificationNotRestricted", 9);
            mappingFA.Add("CertificationRestrictedPermit", 10);
            mappingFA.Add("CertificationRestrictedPermitLimitedUse", 11);
            mappingFA.Add("ColeManagerCode", 12);
            mappingFA.Add("DMCallSign", 13);
            mappingFA.Add("ValidProofOfPassing", 14);
            mappings.Add((int)ULSRecordType.FA, mappingFA);
        }

        /// <summary>
        /// Adds the SH mapping.
        /// </summary>
        private static void AddSHMapping()
        {
            Dictionary<string, int> mappingSH = new Dictionary<string, int>();
            mappingSH.Add("RecordType", 0);
            mappingSH.Add("UniqueSystemIdentifier", 1);
            mappingSH.Add("ULSFileNumber", 2);
            mappingSH.Add("EBFNumber", 3);
            mappingSH.Add("CallSign", 4);
            mappingSH.Add("TypeOfAuthorization", 5);
            mappingSH.Add("NumberInFleet", 6);
            mappingSH.Add("GeneralClassOfShipCode", 7);
            mappingSH.Add("SpecialClassOfShipCode", 8);
            mappingSH.Add("ShipName", 9);
            mappingSH.Add("OfficialNumberOfShip", 10);
            mappingSH.Add("InternationalVoyagesIndicator", 11);
            mappingSH.Add("ForeignCommunicationsIndicator", 12);
            mappingSH.Add("RadiotelegraphWorkingSeriesRequested", 13);
            mappingSH.Add("RequestForMMSI", 14);
            mappingSH.Add("GrossTonnage", 15);
            mappingSH.Add("ShipLength", 16);
            mappingSH.Add("WorkingFreqS1", 17);
            mappingSH.Add("WorkingFreqS2", 18);
            mappingSH.Add("SelCallNumber", 19);
            mappingSH.Add("SelCallINMARSAT", 20);
            mappingSH.Add("MMSINumber", 21);
            mappingSH.Add("RequiredCatA", 22);
            mappingSH.Add("RequiredCatB", 23);
            mappingSH.Add("RequiredCatC", 24);
            mappingSH.Add("RequiredCatD", 25);
            mappingSH.Add("RequiredCatE", 26);
            mappings.Add((int)ULSRecordType.SH, mappingSH);
        }

        /// <summary>
        /// Adds the SR mapping.
        /// </summary>
        private static void AddSRMapping()
        {
            Dictionary<string, int> mappingSR = new Dictionary<string, int>();
            mappingSR.Add("RecordType", 0);
            mappingSR.Add("UniqueSystemIdentifier", 1);
            mappingSR.Add("ULSFileNumber", 2);
            mappingSR.Add("EBFNumber", 3);
            mappingSR.Add("CallSign", 4);
            mappingSR.Add("EPIRBIdentificationCode", 5);
            mappingSR.Add("INMARSATA", 6);
            mappingSR.Add("INMARSATB", 7);
            mappingSR.Add("INMARSATC", 8);
            mappingSR.Add("INMARSATM", 9);
            mappingSR.Add("INMARSATMini", 10);
            mappingSR.Add("VHF", 11);
            mappingSR.Add("MF", 12);
            mappingSR.Add("HF", 13);
            mappingSR.Add("DSC", 14);
            mappingSR.Add("EPIRB406MHZ", 15);
            mappingSR.Add("EPIRB1215MHZ", 16);
            mappingSR.Add("SART", 17);
            mappingSR.Add("RaftCount", 18);
            mappingSR.Add("LifeboatCount", 19);
            mappingSR.Add("VesselCapacity", 20);
            mappings.Add((int)ULSRecordType.SR, mappingSR);
        }

        /// <summary>
        /// Adds the SE mapping.
        /// </summary>
        private static void AddSEMapping()
        {
            Dictionary<string, int> mappingSE = new Dictionary<string, int>();
            mappingSE.Add("RecordType", 0);
            mappingSE.Add("UniqueSystemIdentifier", 1);
            mappingSE.Add("ULSFileNumber", 2);
            mappingSE.Add("EBFNumber", 3);
            mappingSE.Add("CallSign", 4);
            mappingSE.Add("ShipCallSign", 5);
            mappingSE.Add("PortRegistry", 6);
            mappingSE.Add("Owner", 7);
            mappingSE.Add("Operater", 8);
            mappingSE.Add("Charter", 9);
            mappingSE.Add("Agent", 10);
            mappingSE.Add("RadiotelephoneExemptionRequested", 11);
            mappingSE.Add("GMDSSExemptionRequested", 12);
            mappingSE.Add("RadioDirectionExemptionRequested", 13);
            mappingSE.Add("PreviousExemptionFileNumber", 14);
            mappingSE.Add("ForeignPort", 15);
            mappingSE.Add("VesselSizeException", 16);
            mappingSE.Add("EuipmentExemption", 17);
            mappingSE.Add("LimitedRoutesExemption", 18);
            mappingSE.Add("ConditionOfVoyagesExemption", 19);
            mappingSE.Add("OtherExemption", 20);
            mappingSE.Add("OtherExemptionDescription", 21);
            mappingSE.Add("ShipType", 22);
            mappingSE.Add("NumberOfCrew", 23);
            mappingSE.Add("NumberOfPassengers", 24);
            mappingSE.Add("NumberOfOthers", 25);
            mappingSE.Add("CountOfVHF", 26);
            mappingSE.Add("CountOfVHFDSC", 27);
            mappingSE.Add("CountOfEPIRB", 28);
            mappingSE.Add("CountOfSurvivalCraft", 29);
            mappingSE.Add("CountOfEarthStation", 30);
            mappingSE.Add("CountOfAutoAlarm", 31);
            mappingSE.Add("CountOfSingleSideBand", 32);
            mappingSE.Add("SingleSideBandTypeMF", 33);
            mappingSE.Add("SingleSideBandTypeHF", 34);
            mappingSE.Add("SingleSideBandTypeDSC", 35);
            mappingSE.Add("CountOfNAVTEX", 36);
            mappingSE.Add("CountOf9GHzRadar", 37);
            mappingSE.Add("CountOf500KHzDistress", 38);
            mappingSE.Add("CountOfReservePower", 39);
            mappingSE.Add("CountOfOther", 40);
            mappingSE.Add("DescriptionOfOther", 41);
            mappings.Add((int)ULSRecordType.SE, mappingSE);
        }

        /// <summary>
        /// Adds the SV mapping.
        /// </summary>
        private static void AddSVMapping()
        {
            Dictionary<string, int> mappingSV = new Dictionary<string, int>();
            mappingSV.Add("RecordType", 0);
            mappingSV.Add("UniqueSystemIdentifier", 1);
            mappingSV.Add("ULSFileNumber", 2);
            mappingSV.Add("EBFNumber", 3);
            mappingSV.Add("CallSign", 4);
            mappingSV.Add("VoyageNumber", 5);
            mappingSV.Add("VoyageDescription", 6);
            mappings.Add((int)ULSRecordType.SV, mappingSV);
        }

        /// <summary>
        /// Adds the LM mapping.
        /// </summary>
        private static void AddLMMapping()
        {
            Dictionary<string, int> mappingLM = new Dictionary<string, int>();
            mappingLM.Add("RecordType", 0);
            mappingLM.Add("UniqueSystemIdentifier", 1);
            mappingLM.Add("ULSFileNumber", 2);
            mappingLM.Add("EBFNumber", 3);
            mappingLM.Add("CallSign", 4);
            mappingLM.Add("ExtendedImplementationApproved", 5);
            mappingLM.Add("EligibilityActivity", 6);
            mappingLM.Add("StatusCode", 7);
            mappingLM.Add("StatusDate", 8);
            mappings.Add((int)ULSRecordType.LM, mappingLM);
        }

        /// <summary>
        /// Adds the MI mapping.
        /// </summary>
        private static void AddMIMapping()
        {
            Dictionary<string, int> mappingMI = new Dictionary<string, int>();
            mappingMI.Add("RecordType", 0);
            mappingMI.Add("UniqueSystemIdentifier", 1);
            mappingMI.Add("ULSFileNumber", 2);
            mappingMI.Add("EBFNumber", 3);
            mappingMI.Add("CallSign", 4);
            mappingMI.Add("FacilityTypeCode", 5);
            mappingMI.Add("StatementOfIntention", 6);
            mappingMI.Add("LicenseTypeCode", 7);
            mappings.Add((int)ULSRecordType.MI, mappingMI);
        }

        /// <summary>
        /// Adds the BC mapping.
        /// </summary>
        private static void AddBCMapping()
        {
            Dictionary<string, int> mappingBC = new Dictionary<string, int>();
            mappingBC.Add("RecordType", 0);
            mappingBC.Add("UniqueSystemIdentifier", 1);
            mappingBC.Add("ULSFileNumber", 2);
            mappingBC.Add("EBFNumber", 3);
            mappingBC.Add("CallSign", 4);
            mappingBC.Add("BroadcastCallSign", 5);
            mappingBC.Add("BroadcastCity", 6);
            mappingBC.Add("BroadcastState", 7);
            mappingBC.Add("FacilityIDOfParentStation", 8);
            mappingBC.Add("RadioServiceCodeOfParentStation", 9);
            mappingBC.Add("NonParentTypeCode", 10);
            mappings.Add((int)ULSRecordType.BC, mappingBC);
        }

        /// <summary>
        /// Adds the FC mapping.
        /// </summary>
        private static void AddFCMapping()
        {
            Dictionary<string, int> mappingFC = new Dictionary<string, int>();
            mappingFC.Add("RecordType", 0);
            mappingFC.Add("UniqueSystemIdentifier", 1);
            mappingFC.Add("ULSFileNumber", 2);
            mappingFC.Add("EBFNumber", 3);
            mappingFC.Add("CoordinationNumber", 4);
            mappingFC.Add("CoordinatorName", 5);
            mappingFC.Add("CoordinatorPhone", 6);
            mappingFC.Add("CoordinationDate", 7);
            mappingFC.Add("ActionPerformed", 8);
            mappings.Add((int)ULSRecordType.FC, mappingFC);
        }

        /// <summary>
        /// Adds the HS mapping.
        /// </summary>
        private static void AddHSMapping()
        {
            Dictionary<string, int> mappingHS = new Dictionary<string, int>();
            mappingHS.Add("RecordType", 0);
            mappingHS.Add("UniqueSystemIdentifier", 1);
            mappingHS.Add("ULSFileNumber", 2);
            mappingHS.Add("CallSign", 3);
            mappingHS.Add("LogDate", 4);
            mappingHS.Add("Code", 5);
            mappings.Add((int)ULSRecordType.HS, mappingHS);
        }

        /// <summary>
        /// Adds the CO mapping.
        /// </summary>
        private static void AddCOMapping()
        {
            Dictionary<string, int> mappingCO = new Dictionary<string, int>();
            mappingCO.Add("RecordType", 0);
            mappingCO.Add("UniqueSystemIdentifier", 1);
            mappingCO.Add("ULSFileNumber", 2);
            mappingCO.Add("CallSign", 3);
            mappingCO.Add("CommentDate", 4);
            mappingCO.Add("Description", 5);
            mappingCO.Add("StatusCode", 6);
            mappingCO.Add("StatusDate", 7);
            mappings.Add((int)ULSRecordType.CO, mappingCO);
        }

        /// <summary>
        /// Adds the TA mapping.
        /// </summary>
        private static void AddTAMapping()
        {
            Dictionary<string, int> mappingTA = new Dictionary<string, int>();
            mappingTA.Add("RecordType", 0);
            mappingTA.Add("UniqueSystemIdentifier", 1);
            mappingTA.Add("FileNumber", 2);
            mappingTA.Add("EBFNumber", 3);
            mappingTA.Add("ProForma", 4);
            mappingTA.Add("FullAssignment", 5);
            mappingTA.Add("MethodOfAccomplishment", 6);
            mappingTA.Add("MethodOtherDescription", 7);
            mappingTA.Add("Voluntary/Involuntary", 8);
            mappingTA.Add("AssignorOrTransferorCertifierFirstName", 9);
            mappingTA.Add("AssignorOrTransferorCertifierMI", 10);
            mappingTA.Add("AssignorOrTransferorCertifierLastName", 11);
            mappingTA.Add("AssignorOrTransferorCertifierSuffix", 12);
            mappingTA.Add("AssignorOrTransferorCertifierTitle", 13);
            mappingTA.Add("GrossRevenueYear1", 14);
            mappingTA.Add("GrossRevenueYear2", 15);
            mappingTA.Add("GrossRevenueYear3", 16);
            mappingTA.Add("TotalAssets", 17);
            mappingTA.Add("SameSmallCategory", 18);
            mappingTA.Add("ApplyingForInstallments", 19);
            mappingTA.Add("NotificationOfForebearance?", 20);
            mappingTA.Add("WirelessLicensesNeedApproval", 21);
            mappingTA.Add("NonWirelessLicensesNeedApproval", 22);
            mappingTA.Add("Assignor/TransferorMaleOrFemale", 23);
            mappingTA.Add("Assignor/TransferorAfricanAmerican", 24);
            mappingTA.Add("Assignor/TransferorNativeAmerican", 25);
            mappingTA.Add("Assignor/TransferorHawaiian/OtherPacificIslander", 26);
            mappingTA.Add("Assignor/TransferorAsian", 27);
            mappingTA.Add("Assignor/TransferorWhite", 28);
            mappingTA.Add("Assignor/TransferorEthnicity", 29);
            mappingTA.Add("ConsentDate", 30);
            mappingTA.Add("ConsummationDate", 31);
            mappingTA.Add("ConsummationDeadline", 32);
            mappingTA.Add("EligibilityCategory", 33);
            mappingTA.Add("LeadFileNumber", 34);
            mappingTA.Add("HasAssignmentOrTransferAlreadyOccurred?", 35);
            mappingTA.Add("DateTransactionOccurred", 36);
            mappingTA.Add("DateForbearanceTransactionWasConsummated", 37);
            mappingTA.Add("IsAssignmentPartialAssignmentOfSiteBasedLicenses?", 38);
            mappingTA.Add("WouldApplicationCreateAGeographicOverlap", 39);
            mappingTA.Add("Does…HoldInterestsInAnyEntityHavingAccessTo10MHz", 40);
            mappingTA.Add("WouldApplicationReduceTheNumberOfEntitiesProvidingService?", 41);
            mappingTA.Add("WillFacilitiesBeUsedToProvideMultiChannelVideoProgramming", 42);
            mappingTA.Add("AttributableInterestsInACableTelevisionSystem", 43);
            mappingTA.Add("ComplyWithProgrammingRequirements", 44);
            mappingTA.Add("RequiredToFileForm602", 45);
            mappingTA.Add("Form602FileNumber", 46);
            mappingTA.Add("IsFilingLeadApplication/Notification", 47);
            mappingTA.Add("IsApplicantAPublicSafetyEntity", 48);
            mappingTA.Add("IfFilingIsALongTermDeFactoTransferLease", 49);
            mappingTA.Add("DoesFilingInvolveOneOrMorePointToPointMicrowaveLinks", 50);
            mappings.Add((int)ULSRecordType.TA, mappingTA);
        }

        /// <summary>
        /// Adds the BD mapping.
        /// </summary>
        private static void AddBDMapping()
        {
            Dictionary<string, int> mappingBD = new Dictionary<string, int>();
            mappingBD.Add("RecordType", 0);
            mappingBD.Add("UniqueSystemIdentifier", 1);
            mappingBD.Add("FileNumber", 2);
            mappingBD.Add("EBFNumber", 3);
            mappingBD.Add("InvolvesBiddingCredits", 4);
            mappingBD.Add("InvolvesInstallmentPayments", 5);
            mappingBD.Add("InvolvesClosedBidding", 6);
            mappingBD.Add("HaveFullAmountOfBiddingCreditsBeenPaid", 7);
            mappingBD.Add("BCQualifiesTheSameAsCurrentLicensee", 8);
            mappingBD.Add("BCQualifiesDifferentThanCurrentLicensee", 9);
            mappingBD.Add("BCDoesNotQualify", 10);
            mappingBD.Add("HaveInstallmentPaymentsBeenPaid", 11);
            mappingBD.Add("IPQualifiesTheSameAsCurrentLicensee", 12);
            mappingBD.Add("IPQualifiesDifferentThanCurrentLicensee", 13);
            mappingBD.Add("IPDoesNotQualify", 14);
            mappingBD.Add("HaveAllConstructionNotificationsBeenFiled", 15);
            mappingBD.Add("CBQualifiesForClosedBidding", 16);
            mappingBD.Add("CBDoesNotQualifyForClosedBidding", 17);
            mappingBD.Add("BCDoesTheLesseeHaveAGeneralPartnershipInterest", 18);
            mappingBD.Add("BCIsTheLesseeAControllingInterestOrAffiliate", 19);
            mappingBD.Add("BCDoesTheLicenseeCertifyThatTheLesseeDoesNotAffect", 20);
            mappingBD.Add("BC–ShareOfficeSpaceWithAnyPartyControllingTheEntity", 21);
            mappingBD.Add("IPHaveBothTheLicenseeAndLesseeExecuted", 22);
            mappingBD.Add("IPDoesTheLesseeHaveAGeneralPartnershipInterest", 23);
            mappingBD.Add("IPIsTheLesseeAControllingInterestOrAffiliate", 24);
            mappingBD.Add("IPDoesTheLicenseeCertifyThatTheLesseeDoesNotAffect", 25);
            mappingBD.Add("IPShareOfficeSpaceWithAnyPartyControllingTheEntity", 26);
            mappingBD.Add("IPModificationOfSecurityAgreementDate", 27);
            mappingBD.Add("IPLienAcknowledgementDate", 28);
            mappingBD.Add("CBDoesTheLesseeHaveAGeneralPartnershipInterest", 29);
            mappingBD.Add("CBIsTheLesseeAControllingInterestOrAffiliate", 30);
            mappingBD.Add("CBDoesTheLicenseeCertifyThatTheLesseeDoesNotAffect", 31);
            mappingBD.Add("CBShareOfficeSpaceWithAnyPartyControllingTheEntity", 32);
            mappings.Add((int)ULSRecordType.BD, mappingBD);
        }

        /// <summary>
        /// Adds the AS mapping.
        /// </summary>
        private static void AddASMapping()
        {
            Dictionary<string, int> mappingAS = new Dictionary<string, int>();
            mappingAS.Add("RecordType", 0);
            mappingAS.Add("UniqueSystemIdentifier", 1);
            mappingAS.Add("ULSFileNumber", 2);
            mappingAS.Add("EBFNumber", 3);
            mappingAS.Add("CallSign", 4);
            mappingAS.Add("AssociatedCallSign", 5);
            mappingAS.Add("StatusCode", 6);
            mappingAS.Add("StatusDate", 7);
            mappingAS.Add("ActionPerformed", 8);
            mappings.Add((int)ULSRecordType.AS, mappingAS);
        }

        /// <summary>
        /// Adds the CF mapping.
        /// </summary>
        private static void AddCFMapping()
        {
            Dictionary<string, int> mappingCF = new Dictionary<string, int>();
            mappingCF.Add("RecordType", 0);
            mappingCF.Add("UniqueSystemIdentifier", 1);
            mappingCF.Add("ULSFileNumber", 2);
            mappingCF.Add("EBFNumber", 3);
            mappingCF.Add("ItemTypeIndicator", 4);
            mappingCF.Add("ItemType", 5);
            mappingCF.Add("Constructed?", 6);
            mappingCF.Add("LocationNumber", 7);
            mappingCF.Add("PathNumber", 8);
            mappingCF.Add("FrequencyAssigned", 9);
            mappingCF.Add("FrequencyUpperBand", 10);
            mappingCF.Add("NumberOfMobiles", 11);
            mappingCF.Add("CFActionPerformed", 12);
            mappingCF.Add("ActualDateOfConstruction", 13);
            mappingCF.Add("FrequencyNumber", 14);
            mappingCF.Add("AssignCallsign", 15);
            mappings.Add((int)ULSRecordType.CF, mappingCF);
        }

        /// <summary>
        /// Adds the IA mapping.
        /// </summary>
        private static void AddIAMapping()
        {
            Dictionary<string, int> mappingIA = new Dictionary<string, int>();
            mappingIA.Add("RecordType", 0);
            mappingIA.Add("UniqueSystemIdentifier", 1);
            mappingIA.Add("ULSFileNumber", 2);
            mappingIA.Add("EBFNumber", 3);
            mappingIA.Add("CallSign", 4);
            mappingIA.Add("InternationalAddress1", 5);
            mappingIA.Add("InternationalAddress2", 6);
            mappingIA.Add("InternationalCity", 7);
            mappingIA.Add("Country", 8);
            mappingIA.Add("InternationalZipCode", 9);
            mappingIA.Add("InternationalPhone", 10);
            mappingIA.Add("InternationalFax", 11);
            mappings.Add((int)ULSRecordType.IA, mappingIA);
        }

        /// <summary>
        /// Adds the SC mapping.
        /// </summary>
        private static void AddSCMapping()
        {
            Dictionary<string, int> mappingSC = new Dictionary<string, int>();
            mappingSC.Add("RecordType", 0);
            mappingSC.Add("UniqueSystemIdentifier", 1);
            mappingSC.Add("ULSFileNumber", 2);
            mappingSC.Add("EBFNumber", 3);
            mappingSC.Add("CallSign", 4);
            mappingSC.Add("SpecialConditionType", 5);
            mappingSC.Add("SpecialConditionCode", 6);
            mappingSC.Add("StatusCode", 7);
            mappingSC.Add("StatusDate", 8);
            mappings.Add((int)ULSRecordType.SC, mappingSC);
        }

        /// <summary>
        /// Adds the SF mapping.
        /// </summary>
        private static void AddSFMapping()
        {
            Dictionary<string, int> mappingSF = new Dictionary<string, int>();
            mappingSF.Add("RecordType", 0);
            mappingSF.Add("UniqueSystemIdentifier", 1);
            mappingSF.Add("ULSFileNumber", 2);
            mappingSF.Add("EBFNumber", 3);
            mappingSF.Add("CallSign", 4);
            mappingSF.Add("LicenseFreeFormType", 5);
            mappingSF.Add("UniqueLicenseFreeFormIdentifier", 6);
            mappingSF.Add("SequenceNumber", 7);
            mappingSF.Add("LicenseFreeFormCondition", 8);
            mappingSF.Add("StatusCode", 9);
            mappingSF.Add("StatusDate", 10);
            mappings.Add((int)ULSRecordType.SF, mappingSF);
        }

        /// <summary>
        /// Adds the BO mapping.
        /// </summary>
        private static void AddBOMapping()
        {
            Dictionary<string, int> mappingBO = new Dictionary<string, int>();
            mappingBO.Add("RecordType", 0);
            mappingBO.Add("UniqueSystemIdentifier", 1);
            mappingBO.Add("CallSign", 2);
            mappingBO.Add("BuildoutCode", 3);
            mappingBO.Add("BuildoutDeadline", 4);
            mappingBO.Add("BuildoutDate", 5);
            mappingBO.Add("StatusCode", 6);
            mappingBO.Add("StatusDate", 7);
            mappings.Add((int)ULSRecordType.BO, mappingBO);
        }

        /// <summary>
        /// Adds the CP mapping.
        /// </summary>
        private static void AddCPMapping()
        {
            Dictionary<string, int> mappingCP = new Dictionary<string, int>();
            mappingCP.Add("RecordType", 0);
            mappingCP.Add("UniqueSystemIdentifier", 1);
            mappingCP.Add("ULSFileNumber", 2);
            mappingCP.Add("EBFNumber", 3);
            mappingCP.Add("CallSign", 4);
            mappingCP.Add("ControlPointActionPerformed", 5);
            mappingCP.Add("ControlPointNumber", 6);
            mappingCP.Add("ControlAddress", 7);
            mappingCP.Add("ControlCity", 8);
            mappingCP.Add("StateCode", 9);
            mappingCP.Add("ControlPhone", 10);
            mappingCP.Add("ControlCounty", 11);
            mappingCP.Add("StatusCode", 12);
            mappingCP.Add("StatusDate", 13);
            mappings.Add((int)ULSRecordType.CP, mappingCP);
        }

        /// <summary>
        /// Adds the SI mapping.
        /// </summary>
        private static void AddSIMapping()
        {
            Dictionary<string, int> mappingSI = new Dictionary<string, int>();
            mappingSI.Add("RecordType", 0);
            mappingSI.Add("UniqueSystemIdentifier", 1);
            mappingSI.Add("ULSFileNumber", 2);
            mappingSI.Add("EBFNumber", 3);
            mappingSI.Add("CallSign", 4);
            mappingSI.Add("SID", 5);
            mappingSI.Add("ActionPerformed", 5);
            mappings.Add((int)ULSRecordType.SI, mappingSI);
        }

        /// <summary>
        /// Adds the UA mapping.
        /// </summary>
        private static void AddUAMapping()
        {
            Dictionary<string, int> mappingUA = new Dictionary<string, int>();
            mappingUA.Add("RecordType", 0);
            mappingUA.Add("UniqueSystemIdentifier", 1);
            mappingUA.Add("ULSFileNumber", 2);
            mappingUA.Add("EBFNumber", 3);
            mappingUA.Add("CallSign", 4);
            mappingUA.Add("ActionPerformed", 5);
            mappingUA.Add("CellularPhase", 6);
            mappingUA.Add("MarketCode", 7);
            mappingUA.Add("SubmarketCode", 8);
            mappingUA.Add("ChannelBlock", 9);
            mappings.Add((int)ULSRecordType.UA, mappingUA);
        }

        /// <summary>
        /// Adds the AC mapping.
        /// </summary>
        private static void AddACMapping()
        {
            Dictionary<string, int> mappingAC = new Dictionary<string, int>();
            mappingAC.Add("RecordType", 0);
            mappingAC.Add("UniqueSystemIdentifier", 1);
            mappingAC.Add("ULSFileNumber", 2);
            mappingAC.Add("EBFNumber", 3);
            mappingAC.Add("CallSign", 4);
            mappingAC.Add("AircraftCount", 5);
            mappingAC.Add("TypeOfCarrier", 6);
            mappingAC.Add("PortableIndicator", 7);
            mappingAC.Add("FleetIndicator", 8);
            mappingAC.Add("NNumber", 9);
            mappings.Add((int)ULSRecordType.AC, mappingAC);
        }

        /// <summary>
        /// Adds the AM mapping.
        /// </summary>
        private static void AddAMMapping()
        {
            Dictionary<string, int> mappingAM = new Dictionary<string, int>();
            mappingAM.Add("RecordType", 0);
            mappingAM.Add("UniqueSystemIdentifier", 1);
            mappingAM.Add("ULSFileNumber", 2);
            mappingAM.Add("EBFNumber", 3);
            mappingAM.Add("CallSign", 4);
            mappingAM.Add("OperatorClass", 5);
            mappingAM.Add("GroupCode", 6);
            mappingAM.Add("RegionCode", 7);
            mappingAM.Add("TrusteeCallSign", 8);
            mappingAM.Add("TrusteeIndicator", 9);
            mappingAM.Add("PhysicianCertification", 10);
            mappingAM.Add("VESignature", 11);
            mappingAM.Add("SystematicCallSignChange", 12);
            mappingAM.Add("VanityCallSignChange", 13);
            mappingAM.Add("VanityRelationship", 14);
            mappingAM.Add("PreviousCallSign", 15);
            mappingAM.Add("PreviousOperatorClass", 16);
            mappingAM.Add("TrusteeName", 17);
            mappings.Add((int)ULSRecordType.AM, mappingAM);
        }

        /// <summary>
        /// Adds the VC mapping.
        /// </summary>
        private static void AddVCMapping()
        {
            Dictionary<string, int> mappingVC = new Dictionary<string, int>();
            mappingVC.Add("RecordType", 0);
            mappingVC.Add("UniqueSystemIdentifier", 1);
            mappingVC.Add("ULSFileNumber", 2);
            mappingVC.Add("EBFNumber", 3);
            mappingVC.Add("OrderOfPreference", 4);
            mappingVC.Add("RequestedCallSign", 5);
            mappings.Add((int)ULSRecordType.VC, mappingVC);
        }

        /// <summary>
        /// Adds the MK mapping.
        /// </summary>
        private static void AddMKMapping()
        {
            Dictionary<string, int> mappingMK = new Dictionary<string, int>();
            mappingMK.Add("RecordType", 0);
            mappingMK.Add("UniqueSystemIdentifier", 1);
            mappingMK.Add("ULSFileNumber", 2);
            mappingMK.Add("EBFNumber", 3);
            mappingMK.Add("CallSign", 4);
            mappingMK.Add("MarketCode", 5);
            mappingMK.Add("ChannelBlock", 6);
            mappingMK.Add("SubmarketCode", 7);
            mappingMK.Add("MarketName", 8);
            mappingMK.Add("CoveragePartitioning", 9);
            mappingMK.Add("CoverageDissagregation", 10);
            mappingMK.Add("CellularPhaseID", 11);
            mappingMK.Add("Population", 12);
            mappingMK.Add("TribalCreditIndicator", 13);
            mappingMK.Add("TribalCreditCalculation", 14);
            mappingMK.Add("AdditionalTribalCreditRequested", 15);
            mappingMK.Add("TribalCreditAwarded", 16);
            mappingMK.Add("AdditionalTribalCreditAwarded", 17);
            mappings.Add((int)ULSRecordType.MK, mappingMK);
        }

        /// <summary>
        /// Adds the TL mapping.
        /// </summary>
        private static void AddTLMapping()
        {
            Dictionary<string, int> mappingTL = new Dictionary<string, int>();
            mappingTL.Add("RecordType", 0);
            mappingTL.Add("UniqueSystemIdentifier", 1);
            mappingTL.Add("ULSFileNumber", 2);
            mappingTL.Add("EBFNumber", 3);
            mappingTL.Add("CallSign", 4);
            mappingTL.Add("MarketCode", 5);
            mappingTL.Add("ChannelBlock", 6);
            mappingTL.Add("ActionPerformed", 7);
            mappingTL.Add("TribalLandName", 8);
            mappingTL.Add("TribalCertification", 9);
            mappingTL.Add("TribalLandType", 10);
            mappingTL.Add("SquareKilometers", 11);
            mappings.Add((int)ULSRecordType.TL, mappingTL);
        }

        /// <summary>
        /// Adds the MP mapping.
        /// </summary>
        private static void AddMPMapping()
        {
            Dictionary<string, int> mappingMP = new Dictionary<string, int>();
            mappingMP.Add("RecordType", 0);
            mappingMP.Add("UniqueSystemIdentifier", 1);
            mappingMP.Add("ULSFileNumber", 2);
            mappingMP.Add("EBFNumber", 3);
            mappingMP.Add("CallSign", 4);
            mappingMP.Add("MarketPartitionCode", 5);
            mappingMP.Add("AreaToBePartitioned", 6);
            mappingMP.Add("PopulationOfPartitionedArea", 7);
            mappingMP.Add("IncludeExcludeIndicator", 8);
            mappingMP.Add("PartitionSequenceNumber", 9);
            mappingMP.Add("ActionPerformed", 10);
            mappingMP.Add("CensusFigures", 11);
            mappingMP.Add("DefUndefInd", 12);
            mappings.Add((int)ULSRecordType.MP, mappingMP);
        }

        /// <summary>
        /// Adds the MC mapping.
        /// </summary>
        private static void AddMCMapping()
        {
            Dictionary<string, int> mappingMC = new Dictionary<string, int>();
            mappingMC.Add("RecordType", 0);
            mappingMC.Add("UniqueSystemIdentifier", 1);
            mappingMC.Add("ULSFileNumber", 2);
            mappingMC.Add("EBFNumber", 3);
            mappingMC.Add("CallSign", 4);
            mappingMC.Add("UndefinedPartitionedArea", 5);
            mappingMC.Add("MarketCoodinateSequenceNumber", 6);
            mappingMC.Add("PartitionLatitudeDegrees", 7);
            mappingMC.Add("PartitionLatitudeMinutes", 8);
            mappingMC.Add("PartitionLatitudeSeconds", 9);
            mappingMC.Add("PartitionLatitudeDirection", 10);
            mappingMC.Add("PartitionLongitudeDegrees", 11);
            mappingMC.Add("PartitionLongitudeMinutes", 12);
            mappingMC.Add("PartitionLongitudeSeconds", 13);
            mappingMC.Add("PartitionLongitudeDirection", 14);
            mappings.Add((int)ULSRecordType.MC, mappingMC);
        }

        /// <summary>
        /// Adds the MF mapping.
        /// </summary>
        private static void AddMFMapping()
        {
            Dictionary<string, int> mappingMF = new Dictionary<string, int>();
            mappingMF.Add("RecordType", 0);
            mappingMF.Add("UniqueSystemIdentifier", 1);
            mappingMF.Add("ULSFileNumber", 2);
            mappingMF.Add("EBFNumber", 3);
            mappingMF.Add("CallSign", 4);
            mappingMF.Add("PartitionSequenceNumber", 5);
            mappingMF.Add("LowerFrequency", 6);
            mappingMF.Add("UpperFrequency", 7);
            mappingMF.Add("DefUndefInd", 8);
            mappings.Add((int)ULSRecordType.MF, mappingMF);
        }

        /// <summary>
        /// Adds the LS mapping.
        /// </summary>
        private static void AddLSMapping()
        {
            Dictionary<string, int> mappingLS = new Dictionary<string, int>();
            mappingLS.Add("RecordType", 0);
            mappingLS.Add("UniqueSystemIdentifier", 1);
            mappingLS.Add("CallSign", 2);
            mappingLS.Add("LocationNumber", 3);
            mappingLS.Add("SpecialConditionType", 4);
            mappingLS.Add("SpecialConditionCode", 5);
            mappingLS.Add("StatusCode", 6);
            mappingLS.Add("StatusDate", 7);
            mappings.Add((int)ULSRecordType.LS, mappingLS);
        }

        /// <summary>
        /// Adds the LO mapping.
        /// </summary>
        private static void AddLOMapping()
        {
            Dictionary<string, int> mappingLO = new Dictionary<string, int>();
            mappingLO.Add("RecordType", 0);
            mappingLO.Add("UniqueSystemIdentifier", 1);
            mappingLO.Add("ULSFileNumber", 2);
            mappingLO.Add("EBFNumber", 3);
            mappingLO.Add("CallSign", 4);
            mappingLO.Add("LocationActionPerformed", 5);
            mappingLO.Add("LocationTypeCode", 6);
            mappingLO.Add("LocationClassCode", 7);
            mappingLO.Add("LocationNumber", 8);
            mappingLO.Add("SiteStatus", 9);
            mappingLO.Add("CorrespondingFixedLocation", 10);
            mappingLO.Add("LocationAddress", 11);
            mappingLO.Add("LocationCity", 12);
            mappingLO.Add("LocationCounty/Borough/Parish", 13);
            mappingLO.Add("LocationState", 14);
            mappingLO.Add("RadiusOfOperation", 15);
            mappingLO.Add("AreaOfOperationCode", 16);
            mappingLO.Add("ClearanceIndicator", 17);
            mappingLO.Add("GroundElevation", 18);
            mappingLO.Add("LatitudeDegrees", 19);
            mappingLO.Add("LatitudeMinutes", 20);
            mappingLO.Add("LatitudeSeconds", 21);
            mappingLO.Add("LatitudeDirection", 22);
            mappingLO.Add("LongitudeDegrees", 23);
            mappingLO.Add("LongitudeMinutes", 24);
            mappingLO.Add("LongitudeSeconds", 25);
            mappingLO.Add("LongitudeDirection", 26);
            mappingLO.Add("MaxLatitudeDegrees", 27);
            mappingLO.Add("MaxLatitudeMinutes", 28);
            mappingLO.Add("MaxLatitudeSeconds", 29);
            mappingLO.Add("MaxLatitudeDirection", 30);
            mappingLO.Add("MaxLongitudeDegrees", 31);
            mappingLO.Add("MaxLongitudeMinutes", 32);
            mappingLO.Add("MaxLongitudeSeconds", 33);
            mappingLO.Add("MaxLongitudeDirection", 34);
            mappingLO.Add("Nepa", 35);
            mappingLO.Add("QuietZoneNotificationDate", 36);
            mappingLO.Add("TowerRegistrationNumber", 37);
            mappingLO.Add("HeightOfSupportStructure", 38);
            mappingLO.Add("OverallHeightOfStructure", 39);
            mappingLO.Add("StructureType", 40);
            mappingLO.Add("AirportID", 41);
            mappingLO.Add("LocationName", 42);
            mappingLO.Add("UnitsHandHeld", 43);
            mappingLO.Add("UnitsMobile", 44);
            mappingLO.Add("UnitsTempFixed", 45);
            mappingLO.Add("UnitsAircraft", 46);
            mappingLO.Add("UnitsItinerant", 47);
            mappingLO.Add("StatusCode", 48);
            mappingLO.Add("StatusDate", 49);
            mappingLO.Add("EarthStationAgreement", 50);
            mappings.Add((int)ULSRecordType.LO, mappingLO);
        }

        /// <summary>
        /// Adds the L2 mapping.
        /// </summary>
        private static void AddL2Mapping()
        {
            Dictionary<string, int> mappingL2 = new Dictionary<string, int>();
            mappingL2.Add("RecordType", 0);
            mappingL2.Add("UniqueSystemIdentifier", 1);
            mappingL2.Add("ULSFileNumber", 2);
            mappingL2.Add("EBFNumber", 3);
            mappingL2.Add("CallSign", 4);
            mappingL2.Add("LocationActionPerformed", 5);
            mappingL2.Add("LocationNumber", 6);
            mappingL2.Add("RegistrationRequired", 7);
            mappingL2.Add("ProtectionDate", 8);
            mappingL2.Add("LinkRegistrationNumber", 9);
            mappingL2.Add("LinkRegistrationNumberAction", 10);
            mappingL2.Add("MexicoClearanceIndicator", 11);
            mappingL2.Add("QuietZoneConsent", 12);
            mappingL2.Add("StatusCode", 13);
            mappingL2.Add("StatusDate", 14);
            mappings.Add((int)ULSRecordType.L2, mappingL2);
        }

        /// <summary>
        /// Adds the LF mapping.
        /// </summary>
        private static void AddLFMapping()
        {
            Dictionary<string, int> mappingLF = new Dictionary<string, int>();
            mappingLF.Add("RecordType", 0);
            mappingLF.Add("UniqueSystemIdentifier", 1);
            mappingLF.Add("CallSign", 2);
            mappingLF.Add("LocationNumber", 3);
            mappingLF.Add("LocationFreeFormType", 4);
            mappingLF.Add("UniqueLocationFreeFormIdentifier", 5);
            mappingLF.Add("SequenceNumber", 6);
            mappingLF.Add("LocationFreeFormCondition", 7);
            mappingLF.Add("StatusCode", 8);
            mappingLF.Add("StatusDate", 9);
            mappings.Add((int)ULSRecordType.LF, mappingLF);
        }

        /// <summary>
        /// Adds the OP mapping.
        /// </summary>
        private static void AddOPMapping()
        {
            Dictionary<string, int> mappingOP = new Dictionary<string, int>();
            mappingOP.Add("RecordType", 0);
            mappingOP.Add("UniqueSystemIdentifier", 1);
            mappingOP.Add("ULSFileNumber", 2);
            mappingOP.Add("EBFNumber", 3);
            mappingOP.Add("CallSign", 4);
            mappingOP.Add("LocationNumber", 5);
            mappingOP.Add("AreaTextSequenceNumber", 6);
            mappingOP.Add("AreaOfOperationText", 7);
            mappingOP.Add("StatusCode", 8);
            mappingOP.Add("StatusDate", 9);
            mappings.Add((int)ULSRecordType.OP, mappingOP);
        }

        /// <summary>
        /// Adds the BL mapping.
        /// </summary>
        private static void AddBLMapping()
        {
            Dictionary<string, int> mappingBL = new Dictionary<string, int>();
            mappingBL.Add("RecordType", 0);
            mappingBL.Add("UniqueSystemIdentifier", 1);
            mappingBL.Add("CallSign", 2);
            mappingBL.Add("Location", 3);
            mappingBL.Add("BuildoutCode", 4);
            mappingBL.Add("BuildoutDeadline", 5);
            mappingBL.Add("BuildoutDate", 6);
            mappingBL.Add("StatusCode", 7);
            mappingBL.Add("StatusDate", 8);
            mappings.Add((int)ULSRecordType.BL, mappingBL);
        }

        /// <summary>
        /// Adds the AN mapping.
        /// </summary>
        private static void AddANMapping()
        {
            Dictionary<string, int> mappingAN = new Dictionary<string, int>();
            mappingAN.Add("RecordType", 0);
            mappingAN.Add("UniqueSystemIdentifier", 1);
            mappingAN.Add("ULSFileNumber", 2);
            mappingAN.Add("EBFNumber", 3);
            mappingAN.Add("CallSign", 4);
            mappingAN.Add("AntennaActionPerformed", 5);
            mappingAN.Add("AntennaNumber", 6);
            mappingAN.Add("LocationNumber", 7);
            mappingAN.Add("ReceiveZoneCode", 8);
            mappingAN.Add("AntennaTypeCode", 9);
            mappingAN.Add("HeightToTip", 10);
            mappingAN.Add("HeightToCenterRAAT", 11);
            mappingAN.Add("AntennaMake", 12);
            mappingAN.Add("AntennaModel", 13);
            mappingAN.Add("Tilt", 14);
            mappingAN.Add("PolarizationCode", 15);
            mappingAN.Add("Beamwidth", 16);
            mappingAN.Add("Gain", 17);
            mappingAN.Add("Azimuth", 18);
            mappingAN.Add("HeightAboveAvgTerrain", 19);
            mappingAN.Add("DiversityHeight", 20);
            mappingAN.Add("DiversityGain", 21);
            mappingAN.Add("DiversityBeam", 22);
            mappingAN.Add("ReflectorHeight", 23);
            mappingAN.Add("ReflectorWidth", 24);
            mappingAN.Add("ReflectorSeparation", 25);
            mappingAN.Add("PassiveRepeaterNumber", 26);
            mappingAN.Add("BackToBackTxDishGain", 27);
            mappingAN.Add("BackToBackRxDishGain", 28);
            mappingAN.Add("LocationName", 29);
            mappingAN.Add("PassiveRepeaterSequenceID", 30);
            mappingAN.Add("AlternativeCGSAMethod", 31);
            mappingAN.Add("PathNumber", 32);
            mappingAN.Add("LineLoss", 33);
            mappingAN.Add("StatusCode", 34);
            mappingAN.Add("StatusDate", 35);
            mappings.Add((int)ULSRecordType.AN, mappingAN);
        }

        /// <summary>
        /// Adds the RC mapping.
        /// </summary>
        private static void AddRCMapping()
        {
            Dictionary<string, int> mappingRC = new Dictionary<string, int>();
            mappingRC.Add("RecordType", 0);
            mappingRC.Add("UniqueSystemIdentifier", 1);
            mappingRC.Add("ULSFileNumber", 2);
            mappingRC.Add("EBFNumber", 3);
            mappingRC.Add("CallSign", 4);
            mappingRC.Add("ReceiverActionPerformed", 5);
            mappingRC.Add("LocationNumber", 6);
            mappingRC.Add("AntennaNumber", 7);
            mappingRC.Add("ReceiverMake", 8);
            mappingRC.Add("ReceiverModel", 9);
            mappingRC.Add("ReceiverStability", 10);
            mappingRC.Add("ReceiverNoiseFigure", 11);
            mappingRC.Add("StatusCode", 12);
            mappingRC.Add("StatusDate", 13);
            mappings.Add((int)ULSRecordType.RC, mappingRC);
        }

        /// <summary>
        /// Adds the RZ mapping.
        /// </summary>
        private static void AddRZMapping()
        {
            Dictionary<string, int> mappingRZ = new Dictionary<string, int>();
            mappingRZ.Add("RecordType", 0);
            mappingRZ.Add("UniqueSystemIdentifier", 1);
            mappingRZ.Add("ULSFileNumber", 2);
            mappingRZ.Add("EBFNumber", 3);
            mappingRZ.Add("CallSign", 4);
            mappingRZ.Add("AntennaActionPerformed", 5);
            mappingRZ.Add("LocationNumber", 6);
            mappingRZ.Add("AntennaNumber", 7);
            mappingRZ.Add("ReceiveZoneNumber", 8);
            mappingRZ.Add("ReceiveZone", 9);
            mappings.Add((int)ULSRecordType.RZ, mappingRZ);
        }

        /// <summary>
        /// Adds the FR mapping.
        /// </summary>
        private static void AddFRMapping()
        {
            Dictionary<string, int> mappingFR = new Dictionary<string, int>();
            mappingFR.Add("RecordType", 0);
            mappingFR.Add("UniqueSystemIdentifier", 1);
            mappingFR.Add("ULSFileNumber", 2);
            mappingFR.Add("EBFNumber", 3);
            mappingFR.Add("CallSign", 4);
            mappingFR.Add("FrequencyActionPerformed", 5);
            mappingFR.Add("LocationNumber", 6);
            mappingFR.Add("AntennaNumber", 7);
            mappingFR.Add("ClassStationCode", 8);
            mappingFR.Add("OpAltitudeCode", 9);
            mappingFR.Add("FrequencyAssigned", 10);
            mappingFR.Add("FrequencyUpperBand", 11);
            mappingFR.Add("FrequencyCarrier", 12);
            mappingFR.Add("TimeBeginOperations", 13);
            mappingFR.Add("TimeEndOperations", 14);
            mappingFR.Add("PowerOutput", 15);
            mappingFR.Add("PowerERP", 16);
            mappingFR.Add("Tolerance", 17);
            mappingFR.Add("FrequencyIndicator", 18);
            mappingFR.Add("Status", 19);
            mappingFR.Add("EIRP", 20);
            mappingFR.Add("TransmitterMake", 21);
            mappingFR.Add("TransmitterModel", 22);
            mappingFR.Add("AutoTransmitterPowerControl", 23);
            mappingFR.Add("NumberOfUnits", 24);
            mappingFR.Add("NumberOfPagingReceivers", 25);
            mappingFR.Add("FrequencyNumber", 26);
            mappingFR.Add("StatusCode", 27);
            mappingFR.Add("StatusDate", 28);
            mappings.Add((int)ULSRecordType.FR, mappingFR);
        }

        /// <summary>
        /// Adds the F2 mapping.
        /// </summary>
        private static void AddF2Mapping()
        {
            Dictionary<string, int> mappingF2 = new Dictionary<string, int>();
            mappingF2.Add("RecordType", 0);
            mappingF2.Add("UniqueSystemIdentifier", 1);
            mappingF2.Add("ULSFileNumber", 2);
            mappingF2.Add("EBFNumber", 3);
            mappingF2.Add("CallSign", 4);
            mappingF2.Add("AdditionalFrequencyInfoActionPerformed", 5);
            mappingF2.Add("LocationNumber", 6);
            mappingF2.Add("AntennaNumber", 7);
            mappingF2.Add("FrequencyNumber", 8);
            mappingF2.Add("FrequencyAssigned", 9);
            mappingF2.Add("FrequencyUpperBand", 10);
            mappingF2.Add("Offset", 11);
            mappingF2.Add("FrequencyChannelBlock", 12);
            mappingF2.Add("EquipmentClass", 13);
            mappingF2.Add("MinimumPowerOutput", 14);
            mappingF2.Add("DateFirstUsed", 15);
            mappingF2.Add("StatusCode", 16);
            mappingF2.Add("StatusDate", 17);
            mappingF2.Add("ProtocolRestrictedOrUnrestricted", 18);
            mappings.Add((int)ULSRecordType.F2, mappingF2);
        }

        /// <summary>
        /// Adds the FT mapping.
        /// </summary>
        private static void AddFTMapping()
        {
            Dictionary<string, int> mappingFT = new Dictionary<string, int>();
            mappingFT.Add("RecordType", 0);
            mappingFT.Add("UniqueSystemIdentifier", 1);
            mappingFT.Add("ULSFileNumber", 2);
            mappingFT.Add("EBFNumber", 3);
            mappingFT.Add("CallSign", 4);
            mappingFT.Add("FrequencyTypeActionPerformed", 5);
            mappingFT.Add("LocationNumber", 6);
            mappingFT.Add("AntennaNumber", 7);
            mappingFT.Add("FrequencyAssigned", 8);
            mappingFT.Add("FrequencyTypeNumber", 9);
            mappingFT.Add("FrequencyTypeCode", 10);
            mappings.Add((int)ULSRecordType.FT, mappingFT);
        }

        /// <summary>
        /// Adds the IR mapping.
        /// </summary>
        private static void AddIRMapping()
        {
            Dictionary<string, int> mappingIR = new Dictionary<string, int>();
            mappingIR.Add("RecordType", 0);
            mappingIR.Add("UniqueSystemIdentifier", 1);
            mappingIR.Add("ULSFileNumber", 2);
            mappingIR.Add("CallSign", 3);
            mappingIR.Add("LocationNumber", 4);
            mappingIR.Add("AntennaNumber", 5);
            mappingIR.Add("FrequencyAssigned", 6);
            mappingIR.Add("IRACResults", 7);
            mappingIR.Add("FASDocketNumber", 8);
            mappingIR.Add("FCCMNumber", 9);
            mappingIR.Add("FAANGNumber", 10);
            mappingIR.Add("StatusCode", 11);
            mappingIR.Add("StatusDate", 12);
            mappingIR.Add("CoordinationStatusCode      ", 13);
            mappings.Add((int)ULSRecordType.IR, mappingIR);
        }

        /// <summary>
        /// Adds the CS mapping.
        /// </summary>
        private static void AddCSMapping()
        {
            Dictionary<string, int> mappingCS = new Dictionary<string, int>();
            mappingCS.Add("RecordType", 0);
            mappingCS.Add("UniqueSystemIdentifier", 1);
            mappingCS.Add("ULSFileNumber", 2);
            mappingCS.Add("CallSign", 3);
            mappingCS.Add("LocationNumber", 4);
            mappingCS.Add("AntennaNumber", 5);
            mappingCS.Add("FrequencyAssigned", 6);
            mappingCS.Add("CoserResult", 7);
            mappingCS.Add("CoserNumber", 8);
            mappingCS.Add("CoserRequestType", 9);
            mappingCS.Add("StatusCode", 10);
            mappingCS.Add("StatusDate", 11);
            mappings.Add((int)ULSRecordType.CS, mappingCS);
        }

        /// <summary>
        /// Adds the FS mapping.
        /// </summary>
        private static void AddFSMapping()
        {
            Dictionary<string, int> mappingFS = new Dictionary<string, int>();
            mappingFS.Add("RecordType", 0);
            mappingFS.Add("UniqueSystemIdentifier", 1);
            mappingFS.Add("CallSign", 2);
            mappingFS.Add("LocationNumber", 3);
            mappingFS.Add("AntennaNumber", 4);
            mappingFS.Add("Frequency", 5);
            mappingFS.Add("FrequencyNumber", 6);
            mappingFS.Add("SpecialConditionType", 7);
            mappingFS.Add("SpecialConditionCode", 8);
            mappingFS.Add("StatusCode", 9);
            mappingFS.Add("StatusDate", 10);
            mappings.Add((int)ULSRecordType.FS, mappingFS);
        }

        /// <summary>
        /// Adds the FF mapping.
        /// </summary>
        private static void AddFFMapping()
        {
            Dictionary<string, int> mappingFF = new Dictionary<string, int>();
            mappingFF.Add("RecordType", 0);
            mappingFF.Add("UniqueSystemIdentifier", 1);
            mappingFF.Add("CallSign", 2);
            mappingFF.Add("LocationNumber", 3);
            mappingFF.Add("AntennaNumber", 4);
            mappingFF.Add("Frequency", 5);
            mappingFF.Add("FrequencyNumber", 6);
            mappingFF.Add("FrequencyFreeFormType", 7);
            mappingFF.Add("UniqueFrequencyFreeFormIdentifier", 8);
            mappingFF.Add("SequenceNumber", 9);
            mappingFF.Add("FrequencyFreeFormCondition", 10);
            mappingFF.Add("StatusCode", 11);
            mappingFF.Add("StatusDate", 12);
            mappings.Add((int)ULSRecordType.FF, mappingFF);
        }

        /// <summary>
        /// Adds the BF mapping.
        /// </summary>
        private static void AddBFMapping()
        {
            Dictionary<string, int> mappingBF = new Dictionary<string, int>();
            mappingBF.Add("RecordType", 0);
            mappingBF.Add("UniqueSystemIdentifier", 1);
            mappingBF.Add("CallSign", 2);
            mappingBF.Add("LocationNumber", 3);
            mappingBF.Add("AntennaNumber", 4);
            mappingBF.Add("FrequencyAssigned", 5);
            mappingBF.Add("BuildoutCode", 6);
            mappingBF.Add("BuildoutDeadline", 7);
            mappingBF.Add("BuildoutDate", 8);
            mappingBF.Add("StatusCode", 9);
            mappingBF.Add("StatusDate", 10);
            mappingBF.Add("FrequencyNumber", 11);
            mappings.Add((int)ULSRecordType.BF, mappingBF);
        }

        /// <summary>
        /// Adds the RA mapping.
        /// </summary>
        private static void AddRAMapping()
        {
            Dictionary<string, int> mappingRA = new Dictionary<string, int>();
            mappingRA.Add("RecordType", 0);
            mappingRA.Add("UniqueSystemIdentifier", 1);
            mappingRA.Add("ULSFileNumber", 2);
            mappingRA.Add("EBFNumber", 3);
            mappingRA.Add("CallSign", 4);
            mappingRA.Add("RadialActionPerformed", 5);
            mappingRA.Add("LocationNumber", 6);
            mappingRA.Add("AntennaNumber", 7);
            mappingRA.Add("FrequencyNumber", 8);
            mappingRA.Add("FrequencyAssigned", 9);
            mappingRA.Add("FrequencyUpperBand", 10);
            mappingRA.Add("RadialDirection", 11);
            mappingRA.Add("RadialHAAT", 12);
            mappingRA.Add("RadialERP", 13);
            mappingRA.Add("DistanceToSAB", 14);
            mappingRA.Add("DistanceToCGSA", 15);
            mappingRA.Add("StatusCode", 16);
            mappingRA.Add("StatusDate", 17);
            mappings.Add((int)ULSRecordType.RA, mappingRA);
        }

        /// <summary>
        /// Adds the EM mapping.
        /// </summary>
        private static void AddEMMapping()
        {
            Dictionary<string, int> mappingEM = new Dictionary<string, int>();
            mappingEM.Add("RecordType", 0);
            mappingEM.Add("UniqueSystemIdentifier", 1);
            mappingEM.Add("ULSFileNumber", 2);
            mappingEM.Add("EBFNumber", 3);
            mappingEM.Add("CallSign", 4);
            mappingEM.Add("LocationNumber", 5);
            mappingEM.Add("AntennaNumber", 6);
            mappingEM.Add("FrequencyAssigned/ChannelCenter", 7);
            mappingEM.Add("EmissionActionPerformed", 8);
            mappingEM.Add("EmissionCode", 9);
            mappingEM.Add("DigitalModRate", 10);
            mappingEM.Add("DigitalModType", 11);
            mappingEM.Add("FrequencyNumber", 12);
            mappingEM.Add("StatusCode", 13);
            mappingEM.Add("StatusDate", 14);
            mappingEM.Add("EmissionSequenceId", 15);
            mappings.Add((int)ULSRecordType.EM, mappingEM);
        }

        /// <summary>
        /// Adds the PC mapping.
        /// </summary>
        private static void AddPCMapping()
        {
            Dictionary<string, int> mappingPC = new Dictionary<string, int>();
            mappingPC.Add("RecordType", 0);
            mappingPC.Add("UniqueSystemIdentifier", 1);
            mappingPC.Add("ULSFileNumber", 2);
            mappingPC.Add("EBFNumber", 3);
            mappingPC.Add("CallSign", 4);
            mappingPC.Add("ActionPerformed", 5);
            mappingPC.Add("LocationNumber", 6);
            mappingPC.Add("AntennaNumber", 7);
            mappingPC.Add("Frequency", 8);
            mappingPC.Add("SubscriberCallSign", 9);
            mappingPC.Add("City", 10);
            mappingPC.Add("State", 11);
            mappingPC.Add("LatitudeDegrees", 12);
            mappingPC.Add("LatitudeMinutes", 13);
            mappingPC.Add("LatitudeSeconds", 14);
            mappingPC.Add("LatitudeDirection", 15);
            mappingPC.Add("LongitudeDegrees", 16);
            mappingPC.Add("LongitudeMinutes", 17);
            mappingPC.Add("LongitudeSeconds", 18);
            mappingPC.Add("LongitudeDirection", 19);
            mappingPC.Add("PointOfComFrequency", 20);
            mappingPC.Add("StatusCode", 21);
            mappingPC.Add("StatusDate", 22);
            mappings.Add((int)ULSRecordType.PC, mappingPC);
        }

        /// <summary>
        /// Adds the PA mapping.
        /// </summary>
        private static void AddPAMapping()
        {
            Dictionary<string, int> mappingPA = new Dictionary<string, int>();
            mappingPA.Add("RecordType", 0);
            mappingPA.Add("UniqueSystemIdentifier", 1);
            mappingPA.Add("ULSFileNumber", 2);
            mappingPA.Add("EBFNumber", 3);
            mappingPA.Add("CallSign", 4);
            mappingPA.Add("ActionPerformed", 5);
            mappingPA.Add("PathNumber/LinkNumber", 6);
            mappingPA.Add("TransmitLocationNumber", 7);
            mappingPA.Add("TransmitAntennaNumber", 8);
            mappingPA.Add("ReceiverLocationNumber", 9);
            mappingPA.Add("ReceiverAntennaNumber", 10);
            mappingPA.Add("MAS/DEMSSubTypeOfOperation", 11);
            mappingPA.Add("PathTypeCode", 12);
            mappingPA.Add("PassiveReceiverIndicator", 13);
            mappingPA.Add("CountryCode", 14);
            mappingPA.Add("InterferenceToGSO?", 15);
            mappingPA.Add("ReceiverCallSign", 16);
            mappingPA.Add("AngularSeparation", 17);
            mappingPA.Add("CertNoAlternative", 18);
            mappingPA.Add("CertNoInterference", 19);
            mappingPA.Add("StatusCode", 20);
            mappingPA.Add("StatusDate", 21);
            mappings.Add((int)ULSRecordType.PA, mappingPA);
        }

        /// <summary>
        /// Adds the SG mapping.
        /// </summary>
        private static void AddSGMapping()
        {
            Dictionary<string, int> mappingSG = new Dictionary<string, int>();
            mappingSG.Add("RecordType", 0);
            mappingSG.Add("UniqueSystemIdentifier", 1);
            mappingSG.Add("ULSFileNumber", 2);
            mappingSG.Add("EBFNumber", 3);
            mappingSG.Add("CallSign", 4);
            mappingSG.Add("ActionPerformed", 5);
            mappingSG.Add("PathNumber", 6);
            mappingSG.Add("TransmitLocationNumber", 7);
            mappingSG.Add("TransmitAntennaNumber", 8);
            mappingSG.Add("ReceiverLocationNumber", 9);
            mappingSG.Add("ReceiverAntennaNumber", 10);
            mappingSG.Add("SegmentNumber", 11);
            mappingSG.Add("SegmentLength", 12);
            mappingSG.Add("StatusCode", 13);
            mappingSG.Add("StatusDate", 14);
            mappings.Add((int)ULSRecordType.SG, mappingSG);
        }

        /// <summary>
        /// Adds the AT mapping.
        /// </summary>
        private static void AddATMapping()
        {
            Dictionary<string, int> mappingAT = new Dictionary<string, int>();
            mappingAT.Add("RecordType", 0);
            mappingAT.Add("UniqueSystemIdentifier", 1);
            mappingAT.Add("ULSFileNumber", 2);
            mappingAT.Add("EBFNumber", 3);
            mappingAT.Add("AttachmentCode", 4);
            mappingAT.Add("AttachmentDescription", 5);
            mappingAT.Add("AttachmentDate", 6);
            mappingAT.Add("AttachmentFileName", 7);
            mappingAT.Add("ActionPerformed", 8);
            mappings.Add((int)ULSRecordType.AT, mappingAT);
        }

        /// <summary>
        /// Adds the DA mapping.
        /// </summary>
        private static void AddAHMapping()
        {
            Dictionary<string, int> mappingAH = new Dictionary<string, int>();
            mappingAH.Add("RecordType", 0);
            mappingAH.Add("ULSFileNumber", 1);
            mappingAH.Add("AttachmentDescription", 2);
            mappingAH.Add("AttachmentFileID", 3);
            mappings.Add((int)ULSRecordType.AH, mappingAH);
        }

        /// <summary>
        /// Adds the DA mapping.
        /// </summary>
        private static void AddLHMapping()
        {
            Dictionary<string, int> mappingLH = new Dictionary<string, int>();
            mappingLH.Add("RecordType", 0);
            mappingLH.Add("Callsign", 1);
            mappingLH.Add("AttachmentDescription", 2);
            mappingLH.Add("AttachmentFileID", 3);
            mappings.Add((int)ULSRecordType.LH, mappingLH);
        }

        /// <summary>
        /// Adds the DA mapping.
        /// </summary>
        private static void AddBEMapping()
        {
            Dictionary<string, int> mappingBE = new Dictionary<string, int>();
            mappingBE.Add("RecordType", 0);
            mappingBE.Add("UniqueSystemIdentifier", 1);
            mappingBE.Add("ULSFileNumber", 2);
            mappingBE.Add("EBFNumber", 3);
            mappingBE.Add("CallSign", 4);
            mappingBE.Add("Multichannel", 5);
            mappingBE.Add("CableTv", 6);
            mappingBE.Add("ProgrammingRequirements", 7);
            mappingBE.Add("InterferenceProtection", 8);
            mappings.Add((int)ULSRecordType.BE, mappingBE);
        }

        /// <summary>
        /// Adds the DA mapping.
        /// </summary>
        private static void AddMHMapping()
        {
            Dictionary<string, int> mappingMH = new Dictionary<string, int>();
            mappingMH.Add("RecordType", 0);
            mappingMH.Add("UniqueSystemIdentifier", 1);
            mappingMH.Add("ULSFileNumber", 2);
            mappingMH.Add("EBFNumber", 3);
            mappingMH.Add("CallSign", 4);
            mappingMH.Add("ActionPerformed", 5);
            mappingMH.Add("ChannelPlanNumber", 6);
            mappingMH.Add("ChannelPlan", 7);
            mappings.Add((int)ULSRecordType.MH, mappingMH);
        }

        /// <summary>
        /// Adds the DA mapping.
        /// </summary>
        private static void AddMEMapping()
        {
            Dictionary<string, int> mappingME = new Dictionary<string, int>();
            mappingME.Add("RecordType[ME]", 0);
            mappingME.Add("UniqueSystemIdentifier", 1);
            mappingME.Add("ULSFileNumber", 2);
            mappingME.Add("EBFNumber", 3);
            mappingME.Add("CallSign", 4);
            mappingME.Add("MEANumber", 5);
            mappingME.Add("ActionPerformed", 6);
            mappings.Add((int)ULSRecordType.ME, mappingME);
        }

        /// <summary>
        /// Adds the LA mapping.
        /// </summary>
        private static void AddLAMapping()
        {
            Dictionary<string, int> mappingLA = new Dictionary<string, int>();
            mappingLA.Add("RecordType", 0);
            mappingLA.Add("UniqueSystemIdentifier", 1);
            mappingLA.Add("CallSign", 2);
            mappingLA.Add("AttachmentCode", 3);
            mappingLA.Add("AttachmentDescription", 4);
            mappingLA.Add("AttachmentDate", 5);
            mappingLA.Add("AttachmentFileName", 6);
            mappingLA.Add("ActionPerformed", 7);
            mappings.Add((int)ULSRecordType.LA, mappingLA);
        }

        /// <summary>
        /// Adds the CD mapping.
        /// </summary>
        private static void AddCDMapping()
        {
            Dictionary<string, int> mappingCD = new Dictionary<string, int>();
            mappingCD.Add("RecordType", 0);
            mappingCD.Add("UniqueSystemIdentifier", 1);
            mappingCD.Add("ULSFileNumber", 2);
            mappingCD.Add("EBFNumber", 3);
            mappingCD.Add("YearSequenceID", 4);
            mappingCD.Add("GrossRevenues", 5);
            mappingCD.Add("YearEndDate", 6);
            mappingCD.Add("AggregateGrossRevenueDesignatedEntity", 7);
            mappingCD.Add("AggregateGrossRevenueClosedBidding", 8);
            mappingCD.Add("TotalAssets", 9);
            mappings.Add((int)ULSRecordType.CD, mappingCD);
        }

        /// <summary>
        /// Adds the RI mapping.
        /// </summary>
        private static void AddRIMapping()
        {
            Dictionary<string, int> mappingRI = new Dictionary<string, int>();
            mappingRI.Add("RecordType", 0);
            mappingRI.Add("UniqueSystemIdentifier", 1);
            mappingRI.Add("ULSFileNumber", 2);
            mappingRI.Add("EBFNumber", 3);
            mappingRI.Add("EntityType", 4);
            mappingRI.Add("YearSequenceID", 5);
            mappingRI.Add("GrossRevenues", 6);
            mappingRI.Add("YearEndDate", 7);
            mappingRI.Add("AverageGrossRevenues", 8);
            mappingRI.Add("AssetDisclosure", 9);
            mappingRI.Add("StatementType", 10);
            mappingRI.Add("InExistence", 11);
            mappings.Add((int)ULSRecordType.RI, mappingRI);
        }

        /// <summary>
        /// Adds the LD mapping.
        /// </summary>
        private static void AddLDMapping()
        {
            Dictionary<string, int> mappingLD = new Dictionary<string, int>();
            mappingLD.Add("RecordType", 0);
            mappingLD.Add("UniqueSystemIdentifier", 1);
            mappingLD.Add("ULSFileNumber", 2);
            mappingLD.Add("EBFNumber", 3);
            mappingLD.Add("CallSign(LeaseId)", 4);
            mappingLD.Add("LeaseCommencementDate", 5);
            mappingLD.Add("LeaseRevisedExpirationDate", 6);
            mappingLD.Add("LeaseTerminationDate", 7);
            mappingLD.Add("LeaseNeverCommenced", 8);
            mappings.Add((int)ULSRecordType.LD, mappingLD);
        }

        /// <summary>
        /// Adds the LL mapping.
        /// </summary>
        private static void AddLLMapping()
        {
            Dictionary<string, int> mappingLL = new Dictionary<string, int>();
            mappingLL.Add("RecordType", 0);
            mappingLL.Add("UniqueSystemIdentifier", 1);
            mappingLL.Add("ULSFileNumber", 2);
            mappingLL.Add("EBFNumber", 3);
            mappingLL.Add("CallSign", 4);
            mappingLL.Add("LeaseID", 5);
            mappingLL.Add("LicenseeId(forTheLicense)", 6);
            mappings.Add((int)ULSRecordType.LL, mappingLL);
        }

        /// <summary>
        /// Adds the LC mapping.
        /// </summary>
        private static void AddLCMapping()
        {
            Dictionary<string, int> mappingLC = new Dictionary<string, int>();
            mappingLC.Add("RecordType", 0);
            mappingLC.Add("UniqueSystemIdentifier", 1);
            mappingLC.Add("ULSFileNumber", 2);
            mappingLC.Add("EBFNumber", 3);
            mappingLC.Add("LeaseId", 4);
            mappingLC.Add("ClassificationOfLease", 5);
            mappingLC.Add("LeaseFilingType", 6);
            mappingLC.Add("LeaseTerm", 7);
            mappings.Add((int)ULSRecordType.LC, mappingLC);
        }

        /// <summary>
        /// Adds the L3 mapping.
        /// </summary>
        private static void AddL3Mapping()
        {
            Dictionary<string, int> mappingL3 = new Dictionary<string, int>();
            mappingL3.Add("RecordType", 0);
            mappingL3.Add("UniqueSystemIdentifier", 1);
            mappingL3.Add("ULSFileNumber", 2);
            mappingL3.Add("EBFNumber", 3);
            mappingL3.Add("CallSign", 4);
            mappingL3.Add("LeaseID", 5);
            mappingL3.Add("LeasedSiteLinkIdentifier", 6);
            mappingL3.Add("LocationActionPerformed", 7);
            mappingL3.Add("LocationTypeCode", 8);
            mappingL3.Add("LocationClassCode", 9);
            mappingL3.Add("LocationNumber", 10);
            mappingL3.Add("SiteStatus", 11);
            mappingL3.Add("CorrespondingFixedLocation", 12);
            mappingL3.Add("LocationAddress", 13);
            mappingL3.Add("LocationCity", 14);
            mappingL3.Add("LocationCounty", 15);
            mappingL3.Add("LocationState", 16);
            mappingL3.Add("RadiusOfOperation", 17);
            mappingL3.Add("AreaOfOperationCode", 18);
            mappingL3.Add("ClearanceIndicator", 19);
            mappingL3.Add("GroundElevation", 20);
            mappingL3.Add("LatitudeDegrees", 21);
            mappingL3.Add("LatitudeMinutes", 22);
            mappingL3.Add("LatitudeSeconds", 23);
            mappingL3.Add("LatitudeDirection", 24);
            mappingL3.Add("LongitudeDegrees", 25);
            mappingL3.Add("LongitudeMinutes", 26);
            mappingL3.Add("LongitudeSeconds", 27);
            mappingL3.Add("LongitudeDirection", 28);
            mappingL3.Add("MaxLatitudeDegrees", 29);
            mappingL3.Add("MaxLatitudeMinutes", 30);
            mappingL3.Add("MaxLatitudeSeconds", 31);
            mappingL3.Add("MaxLatitudeDirection", 32);
            mappingL3.Add("MaxLongitudeDegrees", 33);
            mappingL3.Add("MaxLongitudeMinutes", 34);
            mappingL3.Add("MaxLongitudeSeconds", 35);
            mappingL3.Add("MaxLongitudeDirection", 36);
            mappingL3.Add("Nepa", 37);
            mappingL3.Add("QuietZoneNotificationDate", 38);
            mappingL3.Add("TowerRegistrationNumber", 39);
            mappingL3.Add("HeightOfSupportStructure", 40);
            mappingL3.Add("OverallHighOfStructure", 41);
            mappingL3.Add("StructureType", 42);
            mappingL3.Add("AirportID", 43);
            mappingL3.Add("LocationName", 44);
            mappingL3.Add("UnitsHandHeld", 45);
            mappingL3.Add("UnitsMobile", 46);
            mappingL3.Add("UnitsTempFixed", 47);
            mappingL3.Add("UnitsAircraft", 48);
            mappingL3.Add("UnitsItinerant", 49);
            mappingL3.Add("StatusCode", 50);
            mappingL3.Add("StatusDate", 51);
            mappings.Add((int)ULSRecordType.L3, mappingL3);
        }

        /// <summary>
        /// Adds the L4 mapping.
        /// </summary>
        private static void AddL4Mapping()
        {
            Dictionary<string, int> mappingL4 = new Dictionary<string, int>();
            mappingL4.Add("RecordType", 0);
            mappingL4.Add("UniqueSystemIdentifier", 1);
            mappingL4.Add("ULSFileNumber", 2);
            mappingL4.Add("EBFNumber", 3);
            mappingL4.Add("CallSign", 4);
            mappingL4.Add("LeaseID", 5);
            mappingL4.Add("LeasedSiteLinkIdentifier", 6);
            mappingL4.Add("LocationActionPerformed", 7);
            mappingL4.Add("LicensedLocationNumber", 8);
            mappingL4.Add("RegistrationRequired", 9);
            mappingL4.Add("ProtectionDate", 10);
            mappingL4.Add("LinkRegistrationNumber", 11);
            mappingL4.Add("LinkRegistrationNumberAction", 12);
            mappingL4.Add("MexicoClearanceIndicator", 13);
            mappingL4.Add("QuietZoneConsent", 14);
            mappingL4.Add("StatusCode", 15);
            mappingL4.Add("StatusDate", 16);
            mappings.Add((int)ULSRecordType.L4, mappingL4);
        }

        /// <summary>
        /// Adds the O2 mapping.
        /// </summary>
        private static void AddO2Mapping()
        {
            Dictionary<string, int> mappingO2 = new Dictionary<string, int>();
            mappingO2.Add("RecordType", 0);
            mappingO2.Add("UniqueSystemIdentifier", 1);
            mappingO2.Add("ULSFileNumber", 2);
            mappingO2.Add("EBFNumber", 3);
            mappingO2.Add("CallSign", 4);
            mappingO2.Add("LeaseID", 5);
            mappingO2.Add("LeasedSiteLinkIdentifier", 6);
            mappingO2.Add("LeasedLocationNumber", 7);
            mappingO2.Add("AreaTextSequenceNumber", 8);
            mappingO2.Add("AreaOfOperationText", 9);
            mappingO2.Add("StatusCode", 10);
            mappingO2.Add("StatusDate", 11);
            mappings.Add((int)ULSRecordType.O2, mappingO2);
        }

        /// <summary>
        /// Adds the L5 mapping.
        /// </summary>
        private static void AddL5Mapping()
        {
            Dictionary<string, int> mappingL5 = new Dictionary<string, int>();
            mappingL5.Add("RecordType", 0);
            mappingL5.Add("UniqueSystemIdentifier", 1);
            mappingL5.Add("ULSFileNumber", 2);
            mappingL5.Add("EBFNumber", 3);
            mappingL5.Add("CallSign", 4);
            mappingL5.Add("LeaseID", 5);
            mappingL5.Add("LeasedSiteLinkIdentifier", 6);
            mappingL5.Add("LeasedLocationNumber", 7);
            mappingL5.Add("SpecialConditionType", 8);
            mappingL5.Add("SpecialConditionCode", 9);
            mappingL5.Add("StatusCode", 10);
            mappingL5.Add("StatusDate", 11);
            mappings.Add((int)ULSRecordType.L5, mappingL5);
        }

        /// <summary>
        /// Adds the L6 mapping.
        /// </summary>
        private static void AddL6Mapping()
        {
            Dictionary<string, int> mappingL6 = new Dictionary<string, int>();
            mappingL6.Add("RecordType", 0);
            mappingL6.Add("UniqueSystemIdentifier", 1);
            mappingL6.Add("ULSFileNumber", 2);
            mappingL6.Add("EBFNumber", 3);
            mappingL6.Add("CallSign", 4);
            mappingL6.Add("LeaseID", 5);
            mappingL6.Add("LeasedSiteLinkIdentifier", 6);
            mappingL6.Add("LeasedLocationNumber", 7);
            mappingL6.Add("LocationFreeFormType", 8);
            mappingL6.Add("UniqueLocationFreeFormIdentifier", 9);
            mappingL6.Add("SequenceNumber", 10);
            mappingL6.Add("LocationFreeFormCondition", 11);
            mappingL6.Add("StatusCode", 12);
            mappingL6.Add("StatusDate", 13);
            mappings.Add((int)ULSRecordType.L6, mappingL6);
        }

        /// <summary>
        /// Adds the A3 mapping.
        /// </summary>
        private static void AddA3Mapping()
        {
            Dictionary<string, int> mappingA3 = new Dictionary<string, int>();
            mappingA3.Add("RecordType", 0);
            mappingA3.Add("UniqueSystemIdentifier", 1);
            mappingA3.Add("ULSFileNumber", 2);
            mappingA3.Add("EBFNumber", 3);
            mappingA3.Add("CallSign", 4);
            mappingA3.Add("LeaseID", 5);
            mappingA3.Add("LeasedSiteLinkIdentifier", 6);
            mappingA3.Add("AntennaActionPerformed", 7);
            mappingA3.Add("LicensedAntennaNumber", 8);
            mappingA3.Add("LicensedLocationNumber", 9);
            mappingA3.Add("ReceiveZoneCode", 10);
            mappingA3.Add("AntennaTypeCode", 11);
            mappingA3.Add("HeightToTip", 12);
            mappingA3.Add("HeightToCenterRAAT", 13);
            mappingA3.Add("AntennaMake", 14);
            mappingA3.Add("AntennaModel", 15);
            mappingA3.Add("Tilt", 16);
            mappingA3.Add("PolarizationCode", 17);
            mappingA3.Add("Beamwidth", 18);
            mappingA3.Add("Gain", 19);
            mappingA3.Add("Azimuth", 20);
            mappingA3.Add("HeightAboveAvgTerrain", 21);
            mappingA3.Add("DiversityHeight", 22);
            mappingA3.Add("DiversityGain", 23);
            mappingA3.Add("DiversityBeam", 24);
            mappingA3.Add("ReflectorHeight", 25);
            mappingA3.Add("ReflectorWidth", 26);
            mappingA3.Add("ReflectorSeparation", 27);
            mappingA3.Add("PassiveRepeaterNumber", 28);
            mappingA3.Add("BackToBackTxDishGain", 29);
            mappingA3.Add("BackToBackRxDishGain", 30);
            mappingA3.Add("LocationName", 31);
            mappingA3.Add("PassiveRepeaterSequenceID", 32);
            mappingA3.Add("AlternativeCGSAMethod", 33);
            mappingA3.Add("PathNumber", 34);
            mappingA3.Add("LineLoss", 35);
            mappingA3.Add("StatusCode", 36);
            mappingA3.Add("StatusDate", 37);
            mappings.Add((int)ULSRecordType.A3, mappingA3);
        }

        /// <summary>
        /// Adds the F3 mapping.
        /// </summary>
        private static void AddF3Mapping()
        {
            Dictionary<string, int> mappingF3 = new Dictionary<string, int>();
            mappingF3.Add("RecordType", 0);
            mappingF3.Add("UniqueSystemIdentifier", 1);
            mappingF3.Add("ULSFileNumber", 2);
            mappingF3.Add("EBFNumber", 3);
            mappingF3.Add("CallSign", 4);
            mappingF3.Add("LeaseID", 5);
            mappingF3.Add("LeasedSiteLinkIdentifier", 6);
            mappingF3.Add("FrequencyActionPerformed", 7);
            mappingF3.Add("LicensedLocationNumber", 8);
            mappingF3.Add("LicensedAntennaNumber", 9);
            mappingF3.Add("ClassStationCode", 10);
            mappingF3.Add("OpAltitudeCode", 11);
            mappingF3.Add("FrequencyAssigned", 12);
            mappingF3.Add("FrequencyUpperBand", 13);
            mappingF3.Add("FrequencyCarrier", 14);
            mappingF3.Add("TimeBeginOperations", 15);
            mappingF3.Add("TimeEndOperations", 16);
            mappingF3.Add("PowerOutput", 17);
            mappingF3.Add("PowerERP", 18);
            mappingF3.Add("Tolerance", 19);
            mappingF3.Add("FrequencyIndicator", 20);
            mappingF3.Add("Status", 21);
            mappingF3.Add("EIRP", 22);
            mappingF3.Add("TransmitterMake", 23);
            mappingF3.Add("TransmitterModel", 24);
            mappingF3.Add("AutoTransmitterPowerControl", 25);
            mappingF3.Add("NumberOfUnits", 26);
            mappingF3.Add("NumberOfPagingReceivers", 27);
            mappingF3.Add("FrequencyNumber", 28);
            mappingF3.Add("StatusCode", 29);
            mappingF3.Add("StatusDate", 30);
            mappings.Add((int)ULSRecordType.F3, mappingF3);
        }

        /// <summary>
        /// Adds the F4 mapping.
        /// </summary>
        private static void AddF4Mapping()
        {
            Dictionary<string, int> mappingF4 = new Dictionary<string, int>();
            mappingF4.Add("RecordType", 0);
            mappingF4.Add("UniqueSystemIdentifier", 1);
            mappingF4.Add("ULSFileNumber", 2);
            mappingF4.Add("EBFNumber", 3);
            mappingF4.Add("CallSign", 4);
            mappingF4.Add("LeaseID", 5);
            mappingF4.Add("LeasedSiteLinkIdentifier", 6);
            mappingF4.Add("AdditionalFrequencyActionPerformed", 7);
            mappingF4.Add("LicensedLocationNumber", 8);
            mappingF4.Add("LicensedAntennaNumber", 9);
            mappingF4.Add("FrequencyNumber", 10);
            mappingF4.Add("FrequencyAssigned", 11);
            mappingF4.Add("FrequencyUpperBand", 12);
            mappingF4.Add("FrequencyOffset", 13);
            mappingF4.Add("FrequencyChannelBlock", 14);
            mappingF4.Add("EquipmentClass", 15);
            mappingF4.Add("MinimumPowerOutput", 16);
            mappingF4.Add("DateFirstUsed", 17);
            mappingF4.Add("StatusCode", 18);
            mappingF4.Add("StatusDate", 19);
            mappings.Add((int)ULSRecordType.F4, mappingF4);
        }

        /// <summary>
        /// Adds the F5 mapping.
        /// </summary>
        private static void AddF5Mapping()
        {
            Dictionary<string, int> mappingF5 = new Dictionary<string, int>();
            mappingF5.Add("RecordType", 0);
            mappingF5.Add("UniqueSystemIdentifier", 1);
            mappingF5.Add("CallSign", 2);
            mappingF5.Add("LeaseID", 3);
            mappingF5.Add("LeasedSiteLinkIdentifier", 4);
            mappingF5.Add("LeasedLocationNumber", 5);
            mappingF5.Add("LeasedAntennaNumber", 6);
            mappingF5.Add("Frequency", 7);
            mappingF5.Add("FrequencyNumber", 8);
            mappingF5.Add("SpecialConditionType", 9);
            mappingF5.Add("SpecialConditionCode", 10);
            mappingF5.Add("StatusCode", 11);
            mappingF5.Add("StatusDate", 12);
            mappings.Add((int)ULSRecordType.F5, mappingF5);
        }

        /// <summary>
        /// Adds the F6 mapping.
        /// </summary>
        private static void AddF6Mapping()
        {
            Dictionary<string, int> mappingF6 = new Dictionary<string, int>();
            mappingF6.Add("RecordType", 0);
            mappingF6.Add("UniqueSystemIdentifier", 1);
            mappingF6.Add("ULSFileNumber", 2);
            mappingF6.Add("EBFNumber", 3);
            mappingF6.Add("CallSign", 4);
            mappingF6.Add("LeaseID", 5);
            mappingF6.Add("LeasedSiteLinkIdentifier", 6);
            mappingF6.Add("LeasedLocationNumber", 7);
            mappingF6.Add("LeasedAntennaNumber", 8);
            mappingF6.Add("FrequencyNumber", 9);
            mappingF6.Add("Frequency", 10);
            mappingF6.Add("FrequencyFreeFormType", 11);
            mappingF6.Add("UniqueFrequencyFreeFormIdentifier", 12);
            mappingF6.Add("SequenceNumber", 13);
            mappingF6.Add("FrequencyFreeFormCondition", 14);
            mappingF6.Add("StatusCode", 15);
            mappingF6.Add("StatusDate", 16);
            mappings.Add((int)ULSRecordType.F6, mappingF6);
        }

        /// <summary>
        /// Adds the P2 mapping.
        /// </summary>
        private static void AddP2Mapping()
        {
            Dictionary<string, int> mappingP2 = new Dictionary<string, int>();
            mappingP2.Add("RecordType", 0);
            mappingP2.Add("UniqueSystemIdentifier", 1);
            mappingP2.Add("ULSFileNumber", 2);
            mappingP2.Add("EBFNumber", 3);
            mappingP2.Add("CallSign", 4);
            mappingP2.Add("LeaseID", 5);
            mappingP2.Add("LeasedSiteLinkIdentifier", 6);
            mappingP2.Add("ActionPerformed", 7);
            mappingP2.Add("LicensedPathNumber", 8);
            mappingP2.Add("LicensedTransmitLocationNumber", 9);
            mappingP2.Add("LicensedTransmitAntennaNumber", 10);
            mappingP2.Add("LicensedReceiverLocationNumber", 11);
            mappingP2.Add("LicensedReceiverAntennaNumber", 12);
            mappingP2.Add("MAS/DEMSSubTypeOfOperation", 13);
            mappingP2.Add("PathTypeCode", 14);
            mappingP2.Add("PassiveReceiverIndicator", 15);
            mappingP2.Add("CountryCode", 16);
            mappingP2.Add("InterferenceToGSO", 17);
            mappingP2.Add("ReceiverCallSign", 18);
            mappingP2.Add("AngularSeparation", 19);
            mappingP2.Add("CertNoAlternative", 20);
            mappingP2.Add("CertNoInterference", 21);
            mappingP2.Add("StatusCode", 22);
            mappingP2.Add("StatusDate", 23);
            mappings.Add((int)ULSRecordType.P2, mappingP2);
        }

        /// <summary>
        /// Adds the TP mapping.
        /// </summary>
        private static void AddTPMapping()
        {
            Dictionary<string, int> mappingTP = new Dictionary<string, int>();
            mappingTP.Add("RecordType", 0);
            mappingTP.Add("UniqueSystemIdentifier", 1);
            mappingTP.Add("ULSFileNumber", 2);
            mappingTP.Add("EBFNumber", 3);
            mappingTP.Add("CallSign", 4);
            mappingTP.Add("LocationNumber", 5);
            mappingTP.Add("AntennaNumber", 6);
            mappingTP.Add("FrequencyNumber", 7);
            mappingTP.Add("PrototypeSequenceID", 8);
            mappingTP.Add("Protocol", 9);
            mappingTP.Add("ProtocolDescription", 10);
            mappingTP.Add("ActionPerformed", 11);
            mappingTP.Add("StatusCode", 12);
            mappingTP.Add("StatusDate", 13);
            mappings.Add((int)ULSRecordType.TP, mappingTP);
        }

        /// <summary>
        /// Adds the RD mapping.
        /// </summary>
        private static void AddRDMapping()
        {
            Dictionary<string, int> mappingRD = new Dictionary<string, int>();
            mappingRD.Add("RecordType", 0);
            mappingRD.Add("RegistrationUniqueSystemIdentifier", 1);
            mappingRD.Add("ULSFileNumber", 2);
            mappingRD.Add("Source", 3);
            mappingRD.Add("ApplicationPurpose", 4);
            mappingRD.Add("ApplicationStatus", 5);
            mappingRD.Add("RadioServiceCode", 6);
            mappingRD.Add("RequestingAWaiver", 7);
            mappingRD.Add("AtLeastSixDevicesOnEachChannel", 8);

            mappingRD.Add("CertificationforRegistration", 10);
            mappingRD.Add("MinTvChannels", 11);
            mappingRD.Add("MaxTvChannels", 12);
            mappingRD.Add("MaxWirelessMicrophone", 13);

            mappingRD.Add("CertifierFirstName", 14);
            mappingRD.Add("CertifierMI", 15);
            mappingRD.Add("CertifierLastName", 16);
            mappingRD.Add("CertifierSuffix", 17);
            mappingRD.Add("CertifierTitle", 18);

            mappingRD.Add("DateEntered", 19);
            mappingRD.Add("ReceiptDate", 20);
            mappingRD.Add("GrantDate", 21);
            mappingRD.Add("ExpiredDate", 22);
            mappingRD.Add("Last Action Date", 23);
            mappings.Add((int)ULSRecordType.RD, mappingRD);
        }

        /// <summary>
        /// Adds the NA mapping.
        /// </summary>
        private static void AddNAMapping()
        {
            Dictionary<string, int> mappingNA = new Dictionary<string, int>();
            mappingNA.Add("RecordType", 0);
            mappingNA.Add("UniqueSystemIdentifier", 1);
            mappingNA.Add("ULSFileNumber", 2);
            mappingNA.Add("EntityTypeCode", 3);
            mappingNA.Add("FCCRegistrationNumber(FRN)", 4);
            mappingNA.Add("EntityName", 5);
            mappingNA.Add("FirstName", 6);
            mappingNA.Add("MI", 7);
            mappingNA.Add("LastName", 8);
            mappingNA.Add("Suffix", 9);
            mappingNA.Add("AttentionLine", 10);
            mappingNA.Add("POBox", 11);
            mappingNA.Add("StreetAddress", 12);
            mappingNA.Add("City", 13);
            mappingNA.Add("State", 14);
            mappingNA.Add("ZipCode", 15);
            mappingNA.Add("TelePhone", 16);
            mappingNA.Add("Fax", 17);
            mappingNA.Add("Email", 18);
            mappings.Add((int)ULSRecordType.NA, mappingNA);
        }

        /// <summary>
        /// Adds the VN mapping.
        /// </summary>
        private static void AddVNMapping()
        {
            Dictionary<string, int> mappingVN = new Dictionary<string, int>();
            mappingVN.Add("RecordType", 0);
            mappingVN.Add("UniqueSystemIdentifier", 1);
            mappingVN.Add("ULSFileNumber", 2);
            mappingVN.Add("VenueName", 3);
            mappingVN.Add("VenueTypeCode", 4);
            mappingVN.Add("VenueAddress", 6);
            mappingVN.Add("VenueCity", 7);
            mappingVN.Add("VenuState", 9);
            mappingVN.Add("VenueZipCode", 10);
            mappings.Add((int)ULSRecordType.VN, mappingVN);
        }

        /// <summary>
        /// Adds the L1 mapping.
        /// </summary>
        private static void AddL1Mapping()
        {
            Dictionary<string, int> mappingL1 = new Dictionary<string, int>();
            mappingL1.Add("RecordType", 0);
            mappingL1.Add("UniqueSystemIdentifier", 1);
            mappingL1.Add("ULSFileNumber", 2);
            mappingL1.Add("CoordinatesSequenceIdentifier", 3);
            mappingL1.Add("LatitudeDegrees", 4);
            mappingL1.Add("LatitudeMinutes", 5);
            mappingL1.Add("LatitudeSeconds", 6);
            mappingL1.Add("LatitudeDirection", 7);
            mappingL1.Add("LongitudeDegrees", 8);
            mappingL1.Add("LongitudeMinutes", 9);
            mappingL1.Add("LongitudeSeconds", 10);
            mappingL1.Add("LongitudeDirection", 11);
            mappings.Add((int)ULSRecordType.L1, mappingL1);
        }

        /// <summary>
        /// Adds the CH mapping.
        /// </summary>
        private static void AddCHMapping()
        {
            Dictionary<string, int> mappingCH = new Dictionary<string, int>();
            mappingCH.Add("RecordType", 0);
            mappingCH.Add("UniqueSystemIdentifier", 1);
            mappingCH.Add("ULSFileNumber", 2);
            mappingCH.Add("TypeOfChannel", 3);
            mappingCH.Add("TvChannelUniqueIdentifier", 4);
            mappingCH.Add("TvChannelNumber", 5);
            mappingCH.Add("TvChannelSequenceIdentifier", 6);
            mappings.Add((int)ULSRecordType.CH, mappingCH);
        }
    }
}
