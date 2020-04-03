// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Sync.Database.Manager
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using System.Xml;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Entities;
    using Microsoft.Whitespace.Entities.Versitcard;

    /// <summary>
    /// constructs the Xml Node
    /// </summary>
    public static class RegistrationXmlNodeBuilder
    {
        /// <summary>
        /// Build Fixed TVBD Registration Xml Node
        /// </summary>
        /// <param name="node"> Xml Node </param>
        /// <param name="fixedTVBDRegistration"> Fixed TVBD Registration </param>
        /// <param name="namespaceManager"> Namespace Manager </param>
        public static void BuildFixedTVBDRegistrationXmlNode(XmlNode node, FixedTVBDRegistration fixedTVBDRegistration, XmlNamespaceManager namespaceManager)
        {
            fixedTVBDRegistration.DeSerializeObjectsFromJson();

            // disposition
            if (fixedTVBDRegistration.Disposition != null)
            {
                node.SelectSingleNode("//ren:Fixed_TVBD_Registration/ren:RegistrationDisposition/ren:RegistrationDate", namespaceManager).InnerText = fixedTVBDRegistration.Disposition.RegDate.ToString();
                node.SelectSingleNode("//ren:Fixed_TVBD_Registration/ren:RegistrationDisposition/ren:RegID", namespaceManager).InnerText = fixedTVBDRegistration.Disposition.RegId;
                node.SelectSingleNode("//ren:Fixed_TVBD_Registration/ren:RegistrationDisposition/ren:Action", namespaceManager).InnerText = fixedTVBDRegistration.Disposition.Action.ToString();
            }

            // Organization
            if (fixedTVBDRegistration.Registrant.Org != null)
            {
                node.SelectSingleNode("//ren:Fixed_TVBD_Registration/ren:tvbdRegistrant/vcard:properties/vcard:org/vcard:text", namespaceManager).InnerText = fixedTVBDRegistration.Registrant.Org.OrganizationName;
            }

            // Formatted Name
            if (fixedTVBDRegistration.Contact != null && fixedTVBDRegistration.Contact.FN != null)
            {
                node.SelectSingleNode("//ren:Fixed_TVBD_Registration/ren:tvbdContact/vcard:properties/vcard:fn/vcard:text", namespaceManager).InnerText = fixedTVBDRegistration.Contact.FN.PrefText;
            }

            // Address
            if (fixedTVBDRegistration.Contact != null && fixedTVBDRegistration.Contact.Address != null)
            {
                node.SelectSingleNode("//ren:Fixed_TVBD_Registration/ren:tvbdContact/vcard:properties/vcard:adr/vcard:street/vcard:text", namespaceManager).InnerText = fixedTVBDRegistration.Contact.Address.Street;
                node.SelectSingleNode("//ren:Fixed_TVBD_Registration/ren:tvbdContact/vcard:properties/vcard:adr/vcard:locality/vcard:text", namespaceManager).InnerText = fixedTVBDRegistration.Contact.Address.Locality;
                node.SelectSingleNode("//ren:Fixed_TVBD_Registration/ren:tvbdContact/vcard:properties/vcard:adr/vcard:region/vcard:text", namespaceManager).InnerText = fixedTVBDRegistration.Contact.Address.Region;
                node.SelectSingleNode("//ren:Fixed_TVBD_Registration/ren:tvbdContact/vcard:properties/vcard:adr/vcard:code/vcard:text", namespaceManager).InnerText = fixedTVBDRegistration.Contact.Address.Code;
                node.SelectSingleNode("//ren:Fixed_TVBD_Registration/ren:tvbdContact/vcard:properties/vcard:adr/vcard:country/vcard:text", namespaceManager).InnerText = fixedTVBDRegistration.Contact.Address.Country;
            }

            // Email
            if (fixedTVBDRegistration.Contact != null && fixedTVBDRegistration.Contact.Email != null)
            {
                node.SelectSingleNode("//ren:Fixed_TVBD_Registration/ren:tvbdContact/vcard:properties/vcard:email/vcard:text", namespaceManager).InnerText = fixedTVBDRegistration.Contact.Email[0].EmailAddress;
            }

            // Telephone
            if (fixedTVBDRegistration.Contact != null && fixedTVBDRegistration.Contact.Telephone != null)
            {
                node.SelectSingleNode("//ren:Fixed_TVBD_Registration/ren:tvbdContact/vcard:properties/vcard:tel/vcard:text", namespaceManager).InnerText = fixedTVBDRegistration.Contact.Telephone[0].TelephoneNumber;
            }

            // tvbdRegLocation
            if (fixedTVBDRegistration.Loc != null)
            {
                node.SelectSingleNode("//ren:Fixed_TVBD_Registration/ren:tvbdRegLocation/ren:locLatitude", namespaceManager).InnerText = fixedTVBDRegistration.Loc.Latitude.ToString();
                node.SelectSingleNode("//ren:Fixed_TVBD_Registration/ren:tvbdRegLocation/ren:locLongitude", namespaceManager).InnerText = fixedTVBDRegistration.Loc.Longitude.ToString();
            }

            // Location's Datum
            if (fixedTVBDRegistration.Loc != null && fixedTVBDRegistration.Loc.Datum != null)
            {
                node.SelectSingleNode("//ren:Fixed_TVBD_Registration/ren:tvbdRegLocation/ren:locDatum", namespaceManager).InnerText = fixedTVBDRegistration.Loc.Datum.ToString();
            }

            // Location's Radiation Center
            if (fixedTVBDRegistration.Loc != null && fixedTVBDRegistration.Loc.RadiationCenter != null)
            {
                XmlElement rcamsl = node.OwnerDocument.CreateElement("n1:rcamsl", node.OwnerDocument.DocumentElement.NamespaceURI);
                rcamsl.InnerText = fixedTVBDRegistration.Loc.RadiationCenter.AMSL.ToString();
                node.SelectSingleNode("//ren:Fixed_TVBD_Registration/ren:tvbdRegLocation/ren:locRadiationCenter", namespaceManager).AppendChild(rcamsl);

                XmlElement rchaat = node.OwnerDocument.CreateElement("n1:rchaat", node.OwnerDocument.DocumentElement.NamespaceURI);
                rchaat.InnerText = fixedTVBDRegistration.Loc.RadiationCenter.HAAT.ToString();
                node.SelectSingleNode("//ren:Fixed_TVBD_Registration/ren:tvbdRegLocation/ren:locRadiationCenter", namespaceManager).AppendChild(rchaat);

                XmlElement rchag = node.OwnerDocument.CreateElement("n1:rcHAAG", node.OwnerDocument.DocumentElement.NamespaceURI);
                rchag.InnerText = fixedTVBDRegistration.Loc.RadiationCenter.HAG.ToString();
                node.SelectSingleNode("//ren:Fixed_TVBD_Registration/ren:tvbdRegLocation/ren:locRadiationCenter", namespaceManager).AppendChild(rchag);
            }

            // tvbd Device Details
            if (Utils.Configuration.CurrentRegionId == 1)
            {
                node.SelectSingleNode("//ren:Fixed_TVBD_Registration/ren:tvbdRegDeviceId/ren:didSeriesName", namespaceManager).InnerText = "FCC-ID";
            }
            else if (Utils.Configuration.CurrentRegionId == 5)
            {
                node.SelectSingleNode("//ren:Fixed_TVBD_Registration/ren:tvbdRegDeviceId/ren:didSeriesName", namespaceManager).InnerText = "OFCOM";
            }

            if (fixedTVBDRegistration.DeviceId.SeriesValue != null)
            {
                node.SelectSingleNode("//ren:Fixed_TVBD_Registration/ren:tvbdRegDeviceId/ren:didSeriesValue", namespaceManager).InnerText = fixedTVBDRegistration.DeviceId.SeriesValue.ToString();
            }

            if (fixedTVBDRegistration.DeviceId.SerialNumber != null)
            {
                node.SelectSingleNode("//ren:Fixed_TVBD_Registration/ren:tvbdRegDeviceId/ren:didSerialNumber", namespaceManager).InnerText = fixedTVBDRegistration.DeviceId.SerialNumber.ToString();
            }
        }

        /// <summary>
        /// Build LPAUX Registration Xml Node
        /// </summary>
        /// <param name="node"> Xml Node </param>
        /// <param name="lpauxRegistration"> LPAux Registration </param>
        /// <param name="namespaceManager"> Namespace Manager </param>
        public static void BuildLPAUXRegistrationXmlNode(XmlNode node, LPAuxRegistration lpauxRegistration, XmlNamespaceManager namespaceManager)
        {
            lpauxRegistration.DeSerializeObjectsFromJson();

            // disposition
            if (lpauxRegistration.Disposition != null)
            {
                node.SelectSingleNode("//ren:LP-Aux_Registration/ren:RegistrationDisposition/ren:RegistrationDate", namespaceManager).InnerText = lpauxRegistration.Disposition.RegDate.ToString();
                node.SelectSingleNode("//ren:LP-Aux_Registration/ren:RegistrationDisposition/ren:RegID", namespaceManager).InnerText = lpauxRegistration.Disposition.RegId;
                node.SelectSingleNode("//ren:LP-Aux_Registration/ren:RegistrationDisposition/ren:Action", namespaceManager).InnerText = lpauxRegistration.Disposition.Action.ToString();
            }

            // Organization
            if (lpauxRegistration.Registrant.Org != null)
            {
                node.SelectSingleNode("//ren:LP-Aux_Registration/ren:lpauxRegistrant/vcard:properties/vcard:org/vcard:text", namespaceManager).InnerText = lpauxRegistration.Registrant.Org.OrganizationName;
            }

            // Formatted Name
            if (lpauxRegistration.Contact != null && lpauxRegistration.Contact.FN != null)
            {
                node.SelectSingleNode("//ren:Fixed_TVBD_Registration/ren:lpauxContact/vcard:properties/vcard:fn/vcard:text", namespaceManager).InnerText = lpauxRegistration.Contact.FN.PrefText;
            }

            // Address
            if (lpauxRegistration.Contact != null && lpauxRegistration.Contact.Address != null)
            {
                node.SelectSingleNode("//ren:LP-Aux_Registration/ren:lpauxContact/vcard:properties/vcard:adr/vcard:street/vcard:text", namespaceManager).InnerText = lpauxRegistration.Contact.Address.Street;
                node.SelectSingleNode("//ren:LP-Aux_Registration/ren:lpauxContact/vcard:properties/vcard:adr/vcard:locality/vcard:text", namespaceManager).InnerText = lpauxRegistration.Contact.Address.Locality;
                node.SelectSingleNode("//ren:LP-Aux_Registration/ren:lpauxContact/vcard:properties/vcard:adr/vcard:region/vcard:text", namespaceManager).InnerText = lpauxRegistration.Contact.Address.Region;
                node.SelectSingleNode("//ren:LP-Aux_Registration/ren:lpauxContact/vcard:properties/vcard:adr/vcard:code/vcard:text", namespaceManager).InnerText = lpauxRegistration.Contact.Address.Code;
                node.SelectSingleNode("//ren:LP-Aux_Registration/ren:lpauxContact/vcard:properties/vcard:adr/vcard:country/vcard:text", namespaceManager).InnerText = lpauxRegistration.Contact.Address.Country;
            }

            // Email
            if (lpauxRegistration.Contact != null && lpauxRegistration.Contact.Email != null)
            {
                node.SelectSingleNode("//ren:LP-Aux_Registration/ren:lpauxContact/vcard:properties/vcard:email/vcard:text", namespaceManager).InnerText = lpauxRegistration.Contact.Email[0].EmailAddress;
            }

            // Telephone
            if (lpauxRegistration.Contact != null && lpauxRegistration.Contact.Telephone != null)
            {
                node.SelectSingleNode("//ren:LP-Aux_Registration/ren:lpauxContact/vcard:properties/vcard:tel/vcard:text", namespaceManager).InnerText = lpauxRegistration.Contact.Telephone[0].TelephoneNumber;
            }

            // LP-Aux venue name
            if (lpauxRegistration.VenueName != null)
            {
                node.SelectSingleNode("//ren:LP-Aux_Registration/ren:lpauxVenueName", namespaceManager).InnerText = lpauxRegistration.VenueName;
            }

            // LP-Aux operational area
            node = BuildLpAuxOperationalArea(node, lpauxRegistration, namespaceManager);

            // Callsign
            if (lpauxRegistration.CallSign != null)
            {
                node.SelectSingleNode("//ren:LP-Aux_Registration/ren:lpauxCallSign", namespaceManager).InnerText = lpauxRegistration.CallSign.CallSign;
            }

            // Event
            if (lpauxRegistration.Event != null)
            {
                node = BuildEvent(node, lpauxRegistration.Event, namespaceManager, "//ren:LP-Aux_Registration/ren:lpauxEvent/");
            }

            // Licensed
            node.SelectSingleNode("//ren:LP-Aux_Registration/ren:Licensed", namespaceManager).InnerText = lpauxRegistration.Licensed.ToString();
        }

        /// <summary>
        /// Build MVPD Registration Xml Node
        /// </summary>
        /// <param name="node"> Xml Node </param>
        /// <param name="mvpdRegistration"> MVPD Registration </param>
        /// <param name="namespaceManager"> Namespace Manager </param>
        public static void BuildMVPDRegistrationXmlNode(XmlNode node, MVPDRegistration mvpdRegistration, XmlNamespaceManager namespaceManager)
        {
            mvpdRegistration.DeSerializeObjectsFromJson();

            // disposition
            if (mvpdRegistration.Disposition != null)
            {
                node.SelectSingleNode("//ren:MVPD_Registration/ren:RegistrationDisposition/ren:RegistrationDate", namespaceManager).InnerText = mvpdRegistration.Disposition.RegDate.ToString();
                node.SelectSingleNode("//ren:MVPD_Registration/ren:RegistrationDisposition/ren:RegID", namespaceManager).InnerText = mvpdRegistration.Disposition.RegId;
                node.SelectSingleNode("//ren:MVPD_Registration/ren:RegistrationDisposition/ren:Action", namespaceManager).InnerText = mvpdRegistration.Disposition.Action.ToString();
            }

            // organization
            if (mvpdRegistration.Registrant.Org != null)
            {
                node.SelectSingleNode("//ren:MVPD_Registration/ren:mvpdRegistrant/vcard:properties/vcard:org/vcard:text", namespaceManager).InnerText = mvpdRegistration.Registrant.Org.OrganizationName;
            }

            // Formatted Name
            if (mvpdRegistration.Contact != null && mvpdRegistration.Contact.FN != null)
            {
                node.SelectSingleNode("//ren:MVPD_Registration/ren:mvpdContact/vcard:properties/vcard:fn/vcard:text", namespaceManager).InnerText = mvpdRegistration.Contact.FN.PrefText;
            }

            // Address
            if (mvpdRegistration.Contact != null && mvpdRegistration.Contact.Address != null)
            {
                node.SelectSingleNode("//ren:MVPD_Registration/ren:mvpdContact/vcard:properties/vcard:adr/vcard:street/vcard:text", namespaceManager).InnerText = mvpdRegistration.Contact.Address.Street;
                node.SelectSingleNode("//ren:MVPD_Registration/ren:mvpdContact/vcard:properties/vcard:adr/vcard:locality/vcard:text", namespaceManager).InnerText = mvpdRegistration.Contact.Address.Locality;
                node.SelectSingleNode("//ren:MVPD_Registration/ren:mvpdContact/vcard:properties/vcard:adr/vcard:region/vcard:text", namespaceManager).InnerText = mvpdRegistration.Contact.Address.Region;
                node.SelectSingleNode("//ren:MVPD_Registration/ren:mvpdContact/vcard:properties/vcard:adr/vcard:code/vcard:text", namespaceManager).InnerText = mvpdRegistration.Contact.Address.Code;
                node.SelectSingleNode("//ren:MVPD_Registration/ren:mvpdContact/vcard:properties/vcard:adr/vcard:country/vcard:text", namespaceManager).InnerText = mvpdRegistration.Contact.Address.Country;
            }

            // Email
            if (mvpdRegistration.Contact != null && mvpdRegistration.Contact.Email != null)
            {
                node.SelectSingleNode("//ren:MVPD_Registration/ren:mvpdContact/vcard:properties/vcard:email/vcard:text", namespaceManager).InnerText = mvpdRegistration.Contact.Email[0].EmailAddress;
            }

            // Telephone
            if (mvpdRegistration.Contact != null && mvpdRegistration.Contact.Telephone != null)
            {
                node.SelectSingleNode("//ren:MVPD_Registration/ren:mvpdContact/vcard:properties/vcard:tel/vcard:text", namespaceManager).InnerText = mvpdRegistration.Contact.Telephone[0].TelephoneNumber;
            }

            // MVPD Location
            if (mvpdRegistration.Location != null)
            {
                node = BuildLocation(node, mvpdRegistration.TransmitLocation, namespaceManager, "//ren:MVPD_Registration/ren:mvpdLocation/");
            }

            // Channel
            if (mvpdRegistration.Channel != null)
            {
                if (mvpdRegistration.Channel.Channel != null)
                {
                    node.SelectSingleNode("//ren:MVPD_Registration/ren:mvpdChannel/ren:ustChannel", namespaceManager).InnerText = mvpdRegistration.Channel.Channel.Value.ToString();
                }
                else
                {
                    XmlNode nodeChannel = node.SelectSingleNode("//ren:Temp_BAS_Registration/ren:tbasChannel/ren:ustChannel", namespaceManager);
                    node.SelectSingleNode("//ren:Temp_BAS_Registration/ren:tbasChannel", namespaceManager).RemoveChild(nodeChannel);
                }

                if (mvpdRegistration.Channel.CallSign != null)
                {
                    node.SelectSingleNode("//ren:MVPD_Registration/ren:mvpdChannel/ren:ustCallSign", namespaceManager).InnerText = mvpdRegistration.Channel.CallSign.ToString();
                }
            }

            // Xmitter
            if (mvpdRegistration.TransmitLocation != null)
            {
                node = BuildLocation(node, mvpdRegistration.TransmitLocation, namespaceManager, "//ren:MVPD_Registration/ren:mvpdXmiterLocation/");
            }
        }

        /// <summary>
        /// Build TBas Registration Xml Node
        /// </summary>
        /// <param name="node"> Xml Node </param>
        /// <param name="tempBasRegistration"> Temp BAS Registration </param>
        /// <param name="namespaceManager"> Namespace Manager </param>
        public static void BuildTBasRegistrationXmlNode(XmlNode node, TempBASRegistration tempBasRegistration, XmlNamespaceManager namespaceManager)
        {
            tempBasRegistration.DeSerializeObjectsFromJson();

            // disposition
            if (tempBasRegistration.Disposition != null)
            {
                node.SelectSingleNode("//ren:Temp_BAS_Registration/ren:RegistrationDisposition/ren:RegistrationDate", namespaceManager).InnerText = tempBasRegistration.Disposition.RegDate.ToString();
                node.SelectSingleNode("//ren:Temp_BAS_Registration/ren:RegistrationDisposition/ren:RegID", namespaceManager).InnerText = tempBasRegistration.Disposition.RegId;
                node.SelectSingleNode("//ren:Temp_BAS_Registration/ren:RegistrationDisposition/ren:Action", namespaceManager).InnerText = tempBasRegistration.Disposition.Action.ToString();
            }

            // organization
            if (tempBasRegistration.Registrant.Org != null)
            {
                node.SelectSingleNode("//ren:Temp_BAS_Registration/ren:tbasRegistrant/vcard:properties/vcard:org/vcard:text", namespaceManager).InnerText = tempBasRegistration.Registrant.Org.OrganizationName;
            }

            // Formatted Name
            if (tempBasRegistration.Contact != null && tempBasRegistration.Contact.FN != null)
            {
                node.SelectSingleNode("//ren:Temp_BAS_Registration/ren:tbasContact/vcard:properties/vcard:fn/vcard:text", namespaceManager).InnerText = tempBasRegistration.Contact.FN.PrefText;
            }

            // Address
            if (tempBasRegistration.Contact != null && tempBasRegistration.Contact.Address != null)
            {
                node.SelectSingleNode("//ren:Temp_BAS_Registration/ren:tbasContact/vcard:properties/vcard:adr/vcard:street/vcard:text", namespaceManager).InnerText = tempBasRegistration.Contact.Address.Street;
                node.SelectSingleNode("//ren:Temp_BAS_Registration/ren:tbasContact/vcard:properties/vcard:adr/vcard:locality/vcard:text", namespaceManager).InnerText = tempBasRegistration.Contact.Address.Locality;
                node.SelectSingleNode("//ren:Temp_BAS_Registration/ren:tbasContact/vcard:properties/vcard:adr/vcard:region/vcard:text", namespaceManager).InnerText = tempBasRegistration.Contact.Address.Region;
                node.SelectSingleNode("//ren:Temp_BAS_Registration/ren:tbasContact/vcard:properties/vcard:adr/vcard:code/vcard:text", namespaceManager).InnerText = tempBasRegistration.Contact.Address.Code;
                node.SelectSingleNode("//ren:Temp_BAS_Registration/ren:tbasContact/vcard:properties/vcard:adr/vcard:country/vcard:text", namespaceManager).InnerText = tempBasRegistration.Contact.Address.Country;
            }

            // Email
            if (tempBasRegistration.Contact != null && tempBasRegistration.Contact.Email != null)
            {
                node.SelectSingleNode("//ren:Temp_BAS_Registration/ren:tbasContact/vcard:properties/vcard:email/vcard:text", namespaceManager).InnerText = tempBasRegistration.Contact.Email[0].EmailAddress;
            }

            // Telephone
            if (tempBasRegistration.Contact != null && tempBasRegistration.Contact.Telephone != null)
            {
                node.SelectSingleNode("//ren:Temp_BAS_Registration/ren:tbasContact/vcard:properties/vcard:tel/vcard:text", namespaceManager).InnerText = tempBasRegistration.Contact.Telephone[0].TelephoneNumber;
            }

            // temp bas
            if (tempBasRegistration.RecvLocation != null)
            {
                node = BuildLocation(node, tempBasRegistration.RecvLocation, namespaceManager, "//ren:Temp_BAS_Registration/ren:tbasRecvLocation/");
            }

            // Channel
            if (tempBasRegistration.Channel != null)
            {
                if (tempBasRegistration.Channel.Channel != null)
                {
                    node.SelectSingleNode("//ren:Temp_BAS_Registration/ren:tbasChannel/ren:ustChannel", namespaceManager).InnerText = tempBasRegistration.Channel.Channel.Value.ToString();
                }
                else
                {
                    XmlNode nodeChannel = node.SelectSingleNode("//ren:Temp_BAS_Registration/ren:tbasChannel/ren:ustChannel", namespaceManager);
                    node.SelectSingleNode("//ren:Temp_BAS_Registration/ren:tbasChannel", namespaceManager).RemoveChild(nodeChannel);
                }

                if (tempBasRegistration.Channel.CallSign != null)
                {
                    node.SelectSingleNode("//ren:Temp_BAS_Registration/ren:tbasChannel/ren:ustCallSign", namespaceManager).InnerText = tempBasRegistration.Channel.CallSign.ToString();
                }
            }

            // Xmitter
            if (tempBasRegistration.TransmitLocation != null)
            {
                node = BuildLocation(node, tempBasRegistration.TransmitLocation, namespaceManager, "//ren:Temp_BAS_Registration/ren:tbasXmitLocation/");
            }

            // event
            if (tempBasRegistration.Event != null)
            {
                node = BuildEvent(node, tempBasRegistration.Event, namespaceManager, "//ren:Temp_BAS_Registration/ren:tbasEvent");
            }
        }

        /// <summary>
        /// Build TV Receive Site Registration Xml Node
        /// </summary>
        /// <param name="node"> Xml Node </param>
        /// <param name="tvreceiveSiteRegistration"> TV Receive Site Registration </param>
        /// <param name="namespaceManager"> Namespace Manager </param>
        public static void BuildTVReceiveSiteRegistrationXmlNode(XmlNode node, TVReceiveSiteRegistration tvreceiveSiteRegistration, XmlNamespaceManager namespaceManager)
        {
            tvreceiveSiteRegistration.DeSerializeObjectsFromJson();

            // disposition
            if (tvreceiveSiteRegistration.Disposition != null)
            {
                node.SelectSingleNode("//ren:TV_Receive_Site_Registration/ren:RegistrationDisposition/ren:RegistrationDate", namespaceManager).InnerText = tvreceiveSiteRegistration.Disposition.RegDate.ToString();
                node.SelectSingleNode("//ren:TV_Receive_Site_Registration/ren:RegistrationDisposition/ren:RegID", namespaceManager).InnerText = tvreceiveSiteRegistration.Disposition.RegId;
                node.SelectSingleNode("//ren:TV_Receive_Site_Registration/ren:RegistrationDisposition/ren:Action", namespaceManager).InnerText = tvreceiveSiteRegistration.Disposition.Action.ToString();
            }

            // Organiation
            if (tvreceiveSiteRegistration.Registrant.Org != null)
            {
                node.SelectSingleNode("//ren:TV_Receive_Site_Registration/ren:tvrcRegistrant/vcard:properties/vcard:org/vcard:text", namespaceManager).InnerText = tvreceiveSiteRegistration.Registrant.Org.OrganizationName;
            }

            // Formatted Name
            if (tvreceiveSiteRegistration.Contact != null && tvreceiveSiteRegistration.Contact.FN != null)
            {
                node.SelectSingleNode("//ren:TV_Receive_Site_Registration/ren:tvrcContact/vcard:properties/vcard:fn/vcard:text", namespaceManager).InnerText = tvreceiveSiteRegistration.Contact.FN.PrefText;
            }

            // Address
            if (tvreceiveSiteRegistration.Contact != null && tvreceiveSiteRegistration.Contact.Address != null)
            {
                node.SelectSingleNode("//ren:TV_Receive_Site_Registration/ren:tvrcContact/vcard:properties/vcard:adr/vcard:street/vcard:text", namespaceManager).InnerText = tvreceiveSiteRegistration.Contact.Address.Street;
                node.SelectSingleNode("//ren:TV_Receive_Site_Registration/ren:tvrcContact/vcard:properties/vcard:adr/vcard:locality/vcard:text", namespaceManager).InnerText = tvreceiveSiteRegistration.Contact.Address.Locality;
                node.SelectSingleNode("//ren:TV_Receive_Site_Registration/ren:tvrcContact/vcard:properties/vcard:adr/vcard:region/vcard:text", namespaceManager).InnerText = tvreceiveSiteRegistration.Contact.Address.Region;
                node.SelectSingleNode("//ren:TV_Receive_Site_Registration/ren:tvrcContact/vcard:properties/vcard:adr/vcard:code/vcard:text", namespaceManager).InnerText = tvreceiveSiteRegistration.Contact.Address.Code;
                node.SelectSingleNode("//ren:TV_Receive_Site_Registration/ren:tvrcContact/vcard:properties/vcard:adr/vcard:country/vcard:text", namespaceManager).InnerText = tvreceiveSiteRegistration.Contact.Address.Country;
            }

            // Email
            if (tvreceiveSiteRegistration.Contact != null && tvreceiveSiteRegistration.Contact.Email != null)
            {
                node.SelectSingleNode("//ren:TV_Receive_Site_Registration/ren:tvrcContact/vcard:properties/vcard:email/vcard:text", namespaceManager).InnerText = tvreceiveSiteRegistration.Contact.Email[0].EmailAddress;
            }

            // Telephone
            if (tvreceiveSiteRegistration.Contact != null && tvreceiveSiteRegistration.Contact.Telephone != null)
            {
                node.SelectSingleNode("//ren:TV_Receive_Site_Registration/ren:tvrcContact/vcard:properties/vcard:tel/vcard:text", namespaceManager).InnerText = tvreceiveSiteRegistration.Contact.Telephone[0].TelephoneNumber;
            }

            // temp bas
            if (tvreceiveSiteRegistration.ReceiveLocation != null)
            {
                node = BuildLocation(node, tvreceiveSiteRegistration.ReceiveLocation, namespaceManager, "//ren:TV_Receive_Site_Registration/ren:tvrcRecvLocation/");
            }

            // Transmit channel Call Sign
            if (tvreceiveSiteRegistration.TransmitChannel != null)
            {
                if (tvreceiveSiteRegistration.TransmitChannel.Channel != null)
                {
                    node.SelectSingleNode("//ren:TV_Receive_Site_Registration/ren:tvrcXmitChannel/ren:ustChannel", namespaceManager).InnerText = tvreceiveSiteRegistration.TransmitChannel.Channel.Value.ToString();
                }
                else
                {
                    XmlNode nodeChannel = node.SelectSingleNode("//ren:TV_Receive_Site_Registration/ren:tvrcXmitChannel/ren:ustChannel", namespaceManager);
                    node.SelectSingleNode("//ren:TV_Receive_Site_Registration/ren:tvrcXmitChannel", namespaceManager).RemoveChild(nodeChannel);
                }

                if (tvreceiveSiteRegistration.TransmitChannel.CallSign != null)
                {
                    node.SelectSingleNode("//ren:TV_Receive_Site_Registration/ren:tvrcXmitChannel/ren:ustCallSign", namespaceManager).InnerText = tvreceiveSiteRegistration.TransmitChannel.CallSign.ToString();
                }
            }

            // Receive Call Sign
            if (tvreceiveSiteRegistration.ReceiveCallSign != null)
            {
                if (tvreceiveSiteRegistration.ReceiveCallSign.Channel != null)
                {
                    node.SelectSingleNode("//ren:TV_Receive_Site_Registration/ren:tvrcRecvCallSign/ren:ustChannel", namespaceManager).InnerText = tvreceiveSiteRegistration.ReceiveCallSign.Channel.Value.ToString();
                }
                else
                {
                    XmlNode nodeChannel = node.SelectSingleNode("//ren:TV_Receive_Site_Registration/ren:tvrcRecvCallSign/ren:ustChannel", namespaceManager);
                    node.SelectSingleNode("//ren:TV_Receive_Site_Registration/ren:tvrcRecvCallSign", namespaceManager).RemoveChild(nodeChannel);
                }

                if (tvreceiveSiteRegistration.ReceiveCallSign.CallSign != null)
                {
                    node.SelectSingleNode("//ren:TV_Receive_Site_Registration/ren:tvrcRecvCallSign/ren:ustCallSign", namespaceManager).InnerText = tvreceiveSiteRegistration.ReceiveCallSign.CallSign.ToString();
                }
            }

            // Xmitter
            if (tvreceiveSiteRegistration.TransmitLocation != null)
            {
                node = BuildLocation(node, tvreceiveSiteRegistration.TransmitLocation, namespaceManager, "//ren:TV_Receive_Site_Registration/ren:tvrcXmitLocation/");
            }
        }

        /// <summary>
        /// Build operational area xml node.
        /// </summary>
        /// <param name="xmlNode"> Registration Xml </param>
        /// <param name="lpauxRegn"> LP-Aux Registration </param>
        /// <param name="namespaceManager"> Namespace Manager </param>
        /// <returns> Xml Node </returns>
        private static XmlNode BuildLpAuxOperationalArea(XmlNode xmlNode, LPAuxRegistration lpauxRegn, XmlNamespaceManager namespaceManager)
        {
            XmlNode nodeOperationalArea = xmlNode.SelectSingleNode("//ren:LP-Aux_Registration/ren:lpauxOperationalArea", namespaceManager);

            // Quad Points Check
            if (lpauxRegn.QuadrilateralArea != null && lpauxRegn.QuadrilateralArea.Length > 0)
            {
                for (int index = 0; index < lpauxRegn.QuadrilateralArea.Length; index++)
                {
                    XmlNode nodeQuad = xmlNode.OwnerDocument.CreateNode(XmlNodeType.Element, "n1:lpauxQuadrilateralArea", xmlNode.OwnerDocument.NamespaceURI);
                    XmlNode nodeNEPoint = xmlNode.OwnerDocument.CreateNode(XmlNodeType.Element, "n1:NE_Point", xmlNode.OwnerDocument.NamespaceURI);
                    XmlNode nodeSEPoint = xmlNode.OwnerDocument.CreateNode(XmlNodeType.Element, "n1:SE_Point", xmlNode.OwnerDocument.NamespaceURI);
                    XmlNode nodeSWPoint = xmlNode.OwnerDocument.CreateNode(XmlNodeType.Element, "n1:SW_Point", xmlNode.OwnerDocument.NamespaceURI);
                    XmlNode nodeNWPoint = xmlNode.OwnerDocument.CreateNode(XmlNodeType.Element, "n1:NW_Point", xmlNode.OwnerDocument.NamespaceURI);
                    XmlElement neposition = xmlNode.OwnerDocument.CreateElement("gml:pos", "http://www.opengis.net/gml");
                    XmlElement seposition = xmlNode.OwnerDocument.CreateElement("gml:pos", "http://www.opengis.net/gml");
                    XmlElement swposition = xmlNode.OwnerDocument.CreateElement("gml:pos", "http://www.opengis.net/gml");
                    XmlElement nwposition = xmlNode.OwnerDocument.CreateElement("gml:pos", "http://www.opengis.net/gml");
                    neposition.InnerText = lpauxRegn.QuadrilateralArea[index].NEPoint.Latitude.ToString() + " " + lpauxRegn.QuadrilateralArea[index].NEPoint.Longitude.ToString();
                    seposition.InnerText = lpauxRegn.QuadrilateralArea[index].SEPoint.Latitude.ToString() + " " + lpauxRegn.QuadrilateralArea[index].SEPoint.Longitude.ToString();
                    swposition.InnerText = lpauxRegn.QuadrilateralArea[index].SWPoint.Latitude.ToString() + " " + lpauxRegn.QuadrilateralArea[index].SWPoint.Longitude.ToString();
                    nwposition.InnerText = lpauxRegn.QuadrilateralArea[index].NWPoint.Latitude.ToString() + " " + lpauxRegn.QuadrilateralArea[index].NWPoint.Longitude.ToString();
                    nodeNEPoint.AppendChild(neposition);
                    nodeSEPoint.AppendChild(seposition);
                    nodeSWPoint.AppendChild(swposition);
                    nodeNWPoint.AppendChild(nwposition);
                    nodeQuad.AppendChild(nodeNEPoint);
                    nodeQuad.AppendChild(nodeSEPoint);
                    nodeQuad.AppendChild(nodeSWPoint);
                    nodeQuad.AppendChild(nodeNWPoint);
                    nodeOperationalArea.AppendChild(nodeQuad);
                }
            }
            else if (lpauxRegn.PointsArea != null && lpauxRegn.PointsArea.Length > 0)
            {
                // Points Area Adding
                for (int index = 0; index < lpauxRegn.PointsArea.Length; index++)
                {
                    XmlNode nodePoint = xmlNode.OwnerDocument.CreateNode(XmlNodeType.Element, "n1:lpauxPointArea", xmlNode.NamespaceURI);
                    XmlNode nodeCentrePoint = xmlNode.OwnerDocument.CreateNode(XmlNodeType.Element, "n1:CenterPoint", xmlNode.NamespaceURI);
                    XmlElement centrePosition = xmlNode.OwnerDocument.CreateElement("gml:pos", "http://www.opengis.net/gml");
                    centrePosition.InnerText = lpauxRegn.PointsArea[index].Latitude.ToString() + " " + lpauxRegn.PointsArea[index].Longitude.ToString();
                    nodeCentrePoint.AppendChild(centrePosition);
                    nodePoint.AppendChild(nodeCentrePoint);
                    nodeOperationalArea.AppendChild(nodePoint);
                }
            }

            return xmlNode;
        }

        /// <summary>
        /// Build Event xml node
        /// </summary>
        /// <param name="xmlNode"> Registration Xml</param>
        /// <param name="eventRgn">Registration Event </param>
        /// <param name="namespaceManager"> Namespace Manager </param>
        /// <param name="xpath"> x path </param>
        /// <returns>Xml Node</returns>
        private static XmlNode BuildEvent(XmlNode xmlNode, Event eventRgn, XmlNamespaceManager namespaceManager, string xpath)
        {
            // Adding multiple events
            for (int index = 0; index < eventRgn.Times.Length; index++)
            {
                if (index == 0)
                {
                    xmlNode.SelectSingleNode(xpath + "/ren:eventTimes/ical:properties/ical:uid/ical:text", namespaceManager).InnerText = eventRgn.Times[index].UId;
                    xmlNode.SelectSingleNode(xpath + "/ren:eventTimes/ical:properties/ical:dtstamp/ical:utc-date-time", namespaceManager).InnerText = Utils.ConvertDateZ(Convert.ToDateTime(eventRgn.Times[index].Stamp));
                    xmlNode.SelectSingleNode(xpath + "/ren:eventTimes/ical:properties/ical:dtstart/ical:date-time", namespaceManager).InnerText = Utils.ConvertDateZ(Convert.ToDateTime(eventRgn.Times[index].Start));
                    xmlNode.SelectSingleNode(xpath + "/ren:eventTimes/ical:properties/ical:dtend/ical:date-time", namespaceManager).InnerText = Utils.ConvertDateZ(Convert.ToDateTime(eventRgn.Times[index].End));
                    if (eventRgn.Times[index].Recurrence != null)
                    {
                        BuildEventRecurrence(xmlNode, eventRgn.Times[index].Recurrence, namespaceManager, xpath + "/ren:eventTimes/ical:properties");
                    }
                }
                else
                {
                    XmlNode nodeEvent = xmlNode.SelectSingleNode(xpath + "ren:eventTimes", namespaceManager).CloneNode(true);
                    nodeEvent.SelectSingleNode("ren:eventTimes/ical:properties/ical:uid/ical:text", namespaceManager).InnerText = eventRgn.Times[index].UId;
                    nodeEvent.SelectSingleNode("ren:eventTimes/ical:properties/ical:dtstamp/ical:utc-date-time", namespaceManager).InnerText = Utils.ConvertDateZ(Convert.ToDateTime(eventRgn.Times[index].Stamp));
                    nodeEvent.SelectSingleNode("ren:eventTimes/ical:properties/ical:dtstart/ical:date-time", namespaceManager).InnerText = Utils.ConvertDateZ(Convert.ToDateTime(eventRgn.Times[index].Start));
                    nodeEvent.SelectSingleNode("ren:eventTimes/ical:properties/ical:dtend/ical:date-time", namespaceManager).InnerText = Utils.ConvertDateZ(Convert.ToDateTime(eventRgn.Times[index].End));
                    xmlNode.SelectSingleNode(xpath, namespaceManager).InsertAfter(nodeEvent, xmlNode.SelectNodes(xpath + "/ren:eventTimes", namespaceManager)[0]);
                    if (eventRgn.Times[index].Recurrence != null)
                    {
                        BuildEventRecurrence(xmlNode, eventRgn.Times[index].Recurrence, namespaceManager, xpath + "/ren:eventTimes/ical:properties");
                    }
                }
            }

            // adding events channels
            if (eventRgn.Channels != null)
            {
                for (int index = 0; index < eventRgn.Channels.Length; index++)
                {
                    if (index == 0)
                    {
                        xmlNode.SelectSingleNode(xpath + "/ren:eventChannel/ren:chanNum", namespaceManager).InnerText = eventRgn.Channels[index].ToString();
                    }
                    else
                    {
                        XmlNode nodeEvent = xmlNode.SelectSingleNode(xpath + "/ren:eventChannel", namespaceManager).CloneNode(true);
                        nodeEvent.SelectSingleNode("//ren:chanNum", namespaceManager).InnerText = eventRgn.Channels[index].ToString();
                        xmlNode.SelectSingleNode(xpath, namespaceManager).InsertAfter(nodeEvent, xmlNode.SelectNodes(xpath + "/ren:eventChannel", namespaceManager)[0]);
                    }
                }
            }

            xmlNode.SelectSingleNode(xpath + "/ren:presentationTime/ren:tzName", namespaceManager).InnerText = string.Empty;
            xmlNode.SelectSingleNode(xpath + "/ren:presentationTime/ren:startTime", namespaceManager).InnerText = Utils.ConvertDateZ(Convert.ToDateTime(eventRgn.Times[0].Start));
            xmlNode.SelectSingleNode(xpath + "/ren:presentationTime/ren:endTime", namespaceManager).InnerText = Utils.ConvertDateZ(Convert.ToDateTime(eventRgn.Times[0].End));

            return xmlNode;
        }

        /// <summary>
        /// Builds the event recurrence.
        /// </summary>
        /// <param name="xmlNode">The XML node.</param>
        /// <param name="eventRgn">The event RGN.</param>
        /// <param name="namespaceManager">The namespace manager.</param>
        /// <param name="xpath">The xpath.</param>
        /// <returns>returns XmlNode.</returns>
        private static XmlNode BuildEventRecurrence(XmlNode xmlNode, EventRecurrence eventRgn, XmlNamespaceManager namespaceManager, string xpath)
        {
            // Adding recurrence information
            if (!string.IsNullOrEmpty(eventRgn.Frequency))
            {
                xmlNode.SelectSingleNode(xpath + "/ical:rrule/ical:recur/ical:freq", namespaceManager).InnerText = eventRgn.Frequency;
            }

            if (!string.IsNullOrEmpty(eventRgn.Until))
            {
                xmlNode.SelectSingleNode(xpath + "/ical:rrule/ical:recur/ical:until/ical:date-time", namespaceManager).InnerText = eventRgn.Until;
            }

            if (!string.IsNullOrEmpty(eventRgn.ByDay))
            {
                xmlNode.SelectSingleNode(xpath + "/ical:rrule/ical:recur/ical:byday", namespaceManager).InnerText = eventRgn.ByDay;
            }

            if (!string.IsNullOrEmpty(eventRgn.ByHour))
            {
                xmlNode.SelectSingleNode(xpath + "/ical:rrule/ical:recur/ical:byhour", namespaceManager).InnerText = eventRgn.ByHour;
            }

            if (!string.IsNullOrEmpty(eventRgn.ByMinute))
            {
                xmlNode.SelectSingleNode(xpath + "/ical:rrule/ical:recur/ical:byminute", namespaceManager).InnerText = eventRgn.ByMinute;
            }

            if (!string.IsNullOrEmpty(eventRgn.Interval))
            {
                xmlNode.SelectSingleNode(xpath + "/ical:rrule/ical:recur/ical:interval", namespaceManager).InnerText = eventRgn.Interval;
            }

            if (eventRgn.Count > 0)
            {
                xmlNode.SelectSingleNode(xpath + "/ical:rrule/ical:recur/ical:count", namespaceManager).InnerText = eventRgn.Count.ToString();
            }

            return xmlNode;
        }

        /// <summary>
        /// Build Location xml node.
        /// </summary>
        /// <param name="xmlNode"> Registration Xml </param>
        /// <param name="loc"> Location of the registration </param>
        /// <param name="namespaceManager"> Namespace Manager </param>
        /// <param name="xpath"> x path string</param>
        /// <returns>Xml Node</returns>
        private static XmlNode BuildLocation(XmlNode xmlNode, Location loc, XmlNamespaceManager namespaceManager, string xpath)
        {
            // longitude and lattitude nodes
            if (loc != null)
            {
                xmlNode.SelectSingleNode(xpath + "ren:locLatitude", namespaceManager).InnerText = loc.Latitude.ToString();
                xmlNode.SelectSingleNode(xpath + "ren:locLongitude", namespaceManager).InnerText = loc.Longitude.ToString();
            }

            // Datum
            if (loc != null && loc.Datum != null)
            {
                xmlNode.SelectSingleNode(xpath + "ren:locDatum", namespaceManager).InnerText = loc.Datum.ToString();
            }

            // Radiation Center
            if (loc != null && loc.RadiationCenter != null)
            {
                XmlElement rcamsl = xmlNode.OwnerDocument.CreateElement("n1:rcAMSL", xmlNode.OwnerDocument.DocumentElement.NamespaceURI);
                rcamsl.InnerText = loc.RadiationCenter.AMSL.ToString();
                xmlNode.SelectSingleNode(xpath + "ren:locRadiationCenter", namespaceManager).AppendChild(rcamsl);

                XmlElement rchaat = xmlNode.OwnerDocument.CreateElement("n1:rcHAAT", xmlNode.OwnerDocument.DocumentElement.NamespaceURI);
                rchaat.InnerText = loc.RadiationCenter.HAAT.ToString();
                xmlNode.SelectSingleNode(xpath + "ren:locRadiationCenter", namespaceManager).AppendChild(rchaat);

                XmlElement rchag = xmlNode.OwnerDocument.CreateElement("n1:rcHAG", xmlNode.OwnerDocument.DocumentElement.NamespaceURI);
                rchag.InnerText = loc.RadiationCenter.HAG.ToString();
                xmlNode.SelectSingleNode(xpath + "ren:locRadiationCenter", namespaceManager).AppendChild(rchag);
            }

            return xmlNode;
        }
    }
}
