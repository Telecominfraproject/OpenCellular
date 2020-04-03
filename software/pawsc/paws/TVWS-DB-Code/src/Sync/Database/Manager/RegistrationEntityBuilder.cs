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
    using System.Xml.Linq;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Common.Utilities;
    using Microsoft.Whitespace.Entities;
    using Microsoft.Whitespace.Entities.Versitcard;

    /// <summary>
    /// constructs the Registration Entities
    /// </summary>
    public class RegistrationEntityBuilder
    {
        /// <summary>
        ///  Holds logger instance
        /// </summary>
        private ILogger syncLogger;

        /// <summary>
        /// Initializes a new instance of the RegistrationEntityBuilder class.
        /// </summary>
        /// <param name="logger"> logger object </param>
        public RegistrationEntityBuilder(ILogger logger)
        {
            this.syncLogger = logger;
        }

        /// <summary>
        /// constructs the LPAuxRegistration entity
        /// </summary>
        /// <param name="lpauxRegEntity">LPAuxRegistration Entity to be constructed.</param>
        /// <param name="xmlNode">XML Node</param>
        /// <param name="namespaceManager"> Name Space Manager which has all xml name spaces</param>
        public void BuildEntity(LPAuxRegistration lpauxRegEntity, XmlNode xmlNode, XmlNamespaceManager namespaceManager)
        {
            this.syncLogger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "Begin RegistrationEntityBuilder.BuildEntity for LP-Aux Registration");
            XmlNode node = null;

            // Call Sign
            this.BuildLPAuxTvsSpectrum(ref lpauxRegEntity, xmlNode, namespaceManager);

            // Event Deserializing
            lpauxRegEntity.Event = this.BuildRegistrationEvent(xmlNode.SelectSingleNode("ren:LP-Aux_Registration/ren:lpauxEvent", namespaceManager), namespaceManager);

            // LP-Aux Licence value
            if (xmlNode.SelectSingleNode("ren:LP-Aux_Registration/ren:Licensed", namespaceManager) != null)
            {
                lpauxRegEntity.Licensed = Convert.ToBoolean(Convert.ToInt16(xmlNode.SelectSingleNode("ren:LP-Aux_Registration/ren:Licensed", namespaceManager).InnerText));
            }

            // Build Quad array
            if (xmlNode.SelectSingleNode("ren:LP-Aux_Registration/ren:lpauxOperationalArea/ren:lpauxQuadrilateralArea", namespaceManager) != null)
            {
                lpauxRegEntity.QuadrilateralArea = this.BuildLPAuxQuadArea(xmlNode.SelectNodes("ren:LP-Aux_Registration/ren:lpauxOperationalArea/ren:lpauxQuadrilateralArea", namespaceManager), namespaceManager);
            }

            // Build Point Area
            if (xmlNode.SelectSingleNode("ren:LP-Aux_Registration/ren:lpauxOperationalArea/ren:lpauxPointArea", namespaceManager) != null)
            {
                lpauxRegEntity.PointsArea = this.BuildLPAuxPointsArea(xmlNode.SelectSingleNode("ren:LP-Aux_Registration/ren:lpauxOperationalArea/ren:lpauxPointArea", namespaceManager), namespaceManager);
            }

            // build Registration disposition object
            RegistrationDisposition regDisposition = new RegistrationDisposition();
            Utils.DeserializeXMLToObject(ref regDisposition, xmlNode.SelectSingleNode("ren:LP-Aux_Registration/ren:RegistrationDisposition", namespaceManager).OuterXml);
            lpauxRegEntity.Disposition = regDisposition;

            // eamil
            node = xmlNode.SelectSingleNode("ren:LP-Aux_Registration/ren:lpauxContact/vcard:properties/vcard:email", namespaceManager);
            if (node != null)
            {
                lpauxRegEntity.Contact.Email = this.BuildEamils(xmlNode.SelectNodes("ren:LP-Aux_Registration/ren:lpauxContact/vcard:properties/vcard:email", namespaceManager));
            }

            // address
            node = null;
            node = xmlNode.SelectSingleNode("ren:LP-Aux_Registration/ren:lpauxContact/vcard:properties/vcard:adr", namespaceManager);
            if (node != null)
            {
                lpauxRegEntity.Contact.Address = this.BuildAddressEntity(node, namespaceManager);
            }

            // Registrant's org
            node = null;
            node = xmlNode.SelectSingleNode("ren:LP-Aux_Registration/ren:lpauxRegistrant/vcard:properties/vcard:org", namespaceManager);
            if (node != null)
            {
                Entities.Versitcard.Organization org = new Entities.Versitcard.Organization();
                Utils.DeserializeXMLToObject(ref org, node.OuterXml);
                lpauxRegEntity.Registrant.Org = org;
            }

            // Telephones
            node = null;
            node = xmlNode.SelectSingleNode("ren:LP-Aux_Registration/ren:lpauxContact/vcard:properties/vcard:tel", namespaceManager);
            if (node != null)
            {
                lpauxRegEntity.Contact.Telephone = this.BuildTelephones(xmlNode.SelectNodes("ren:LP-Aux_Registration/ren:lpauxContact/vcard:properties/vcard:tel", namespaceManager));
            }

            // Time zone
            node = null;
            node = xmlNode.SelectSingleNode("ren:LP-Aux_Registration/ren:lpauxEvent/ren:presentationTime/ren:tzname", namespaceManager);
            if (node != null)
            {
                lpauxRegEntity.Contact.TimeZone = node.InnerText;
            }

            // Title
            node = null;
            node = xmlNode.SelectSingleNode("ren:LP-Aux_Registration/ren:presentationTime/vcard:title", namespaceManager);
            if (node != null)
            {
                Entities.Versitcard.Title title = new Entities.Versitcard.Title();
                Utils.DeserializeXMLToObject(ref title, node.OuterXml);
                lpauxRegEntity.Contact.Title = title;
            }

            // Venue Name
            if (xmlNode.SelectSingleNode("ren:LP-Aux_Registration/ren:lpauxVenueName", namespaceManager) != null)
            {
                lpauxRegEntity.VenueName = xmlNode.SelectSingleNode("ren:LP-Aux_Registration/ren:lpauxVenueName", namespaceManager).InnerText;
            }

            this.syncLogger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "End RegistrationEntityBuilder.BuildEntity for LP-Aux Registration");
        }

        /// <summary>
        /// constructs the MVPDRegistration entity.
        /// </summary>
        /// <param name="mvpdRegEntity">LPAuxRegistration Entity to be constructed.</param>
        /// <param name="xmlNode">XML Node of the Registration</param>
        /// <param name="namespaceManager">namespace manager for xml name spaces</param>
        public void BuildEntity(MVPDRegistration mvpdRegEntity, XmlNode xmlNode, XmlNamespaceManager namespaceManager)
        {
            this.syncLogger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "Begin RegistrationEntityBuilder.BuildEntity for MVPD Registration");

            // Common Serialization
            XmlNode node = null;

            this.syncLogger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "Begin Build MVPD Registration Entity");

            // build Registration disposition object
            RegistrationDisposition regDisposition = new RegistrationDisposition();
            Utils.DeserializeXMLToObject(ref regDisposition, xmlNode.SelectSingleNode("ren:MVPD_Registration/ren:RegistrationDisposition", namespaceManager).OuterXml);
            mvpdRegEntity.Disposition = regDisposition;

            // eamil
            node = xmlNode.SelectSingleNode("ren:MVPD_Registration/ren:mvpdContact/vcard:properties/vcard:email", namespaceManager);
            if (node != null)
            {
                mvpdRegEntity.Contact.Email = this.BuildEamils(xmlNode.SelectNodes("ren:MVPD_Registration/ren:mvpdContact/vcard:properties/vcard:email", namespaceManager));
            }

            // address
            node = null;
            node = xmlNode.SelectSingleNode("ren:MVPD_Registration/ren:mvpdContact/vcard:properties/vcard:adr", namespaceManager);
            if (node != null)
            {
                mvpdRegEntity.Contact.Address = this.BuildAddressEntity(node, namespaceManager);
            }

            // Registrant's org
            node = null;
            node = xmlNode.SelectSingleNode("ren:MVPD_Registration/ren:mvpdRegistrant/vcard:properties/vcard:org", namespaceManager);
            if (node != null)
            {
                Entities.Versitcard.Organization org = new Entities.Versitcard.Organization();
                Utils.DeserializeXMLToObject(ref org, node.OuterXml);
                mvpdRegEntity.Registrant.Org = org;
            }

            // Telephones
            node = null;
            node = xmlNode.SelectSingleNode("ren:MVPD_Registration/ren:mvpdContact/vcard:properties/vcard:tel", namespaceManager);
            if (node != null)
            {
                mvpdRegEntity.Contact.Telephone = this.BuildTelephones(xmlNode.SelectNodes("ren:MVPD_Registration/ren:mvpdContact/vcard:properties/vcard:tel", namespaceManager));
            }

            // Time zone
            node = null;
            node = xmlNode.SelectSingleNode("ren:MVPD_Registration/ren:mvpdEvent/ren:presentationTime/ren:tzname", namespaceManager);
            if (node != null)
            {
                mvpdRegEntity.Contact.TimeZone = node.InnerText;
            }

            // Title
            node = null;
            node = xmlNode.SelectSingleNode("ren:MVPD_Registration/ren:presentationTime/vcard:title", namespaceManager);
            if (node != null)
            {
                Entities.Versitcard.Title title = new Entities.Versitcard.Title();
                Utils.DeserializeXMLToObject(ref title, node.OuterXml);
                mvpdRegEntity.Contact.Title = title;
            }

            // MVPD Channel serialization
            node = null;
            node = xmlNode.SelectSingleNode("ren:MVPD_Registration/ren:mvpdLocation", namespaceManager);
            if (node != null)
            {
                mvpdRegEntity.Location.Latitude = Convert.ToDouble(node.SelectSingleNode("//ren:locLatitude", namespaceManager).InnerText);
                mvpdRegEntity.Location.Longitude = Convert.ToDouble(node.SelectSingleNode("//ren:locLongitude", namespaceManager).InnerText);
                mvpdRegEntity.Location.Datum = node.SelectSingleNode("//ren:locDatum", namespaceManager).InnerText;
                RadiationCenter radiationCenter = new RadiationCenter();
                Utils.DeserializeXMLToObject(ref radiationCenter, node.SelectSingleNode("//ren:locRadiationCenter", namespaceManager).OuterXml);
                mvpdRegEntity.Location.RadiationCenter = radiationCenter;
            }

            // mvpd Channel serialization
            node = null;
            node = xmlNode.SelectSingleNode("ren:MVPD_Registration/ren:mvpdChannel", namespaceManager);
            if (node != null)
            {
                mvpdRegEntity.Channel.Channel = Convert.ToInt32(node.SelectSingleNode("//ren:ustChannel", namespaceManager).InnerText);
                mvpdRegEntity.Channel.CallSign = node.SelectSingleNode("//ren:ustCallSign", namespaceManager).InnerText;
            }

            // mvpd transmit location serialiation
            node = null;
            node = xmlNode.SelectSingleNode("ren:MVPD_Registration/ren:mvpdXmiterLocation", namespaceManager);
            if (node != null)
            {
                XElement element = XElement.Parse(node.OuterXml);
                var curnamespace = element.GetDefaultNamespace();
                mvpdRegEntity.TransmitLocation.Latitude = element.Descendants(XName.Get("locLatitude", curnamespace.NamespaceName)).FirstOrDefault().Value.ToDouble();
                mvpdRegEntity.TransmitLocation.Longitude = element.Descendants(XName.Get("locLongitude", curnamespace.NamespaceName)).FirstOrDefault().Value.ToDouble();
                mvpdRegEntity.TransmitLocation.Datum = element.Descendants(XName.Get("locDatum", curnamespace.NamespaceName)).FirstOrDefault().Value;
                RadiationCenter radiationCenter = new RadiationCenter();
                Utils.DeserializeXMLToObject(ref radiationCenter, node.SelectSingleNode("//ren:locRadiationCenter", namespaceManager).OuterXml);
                mvpdRegEntity.TransmitLocation.RadiationCenter = radiationCenter;
            }

            this.syncLogger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "End Build MVPD Registration Entity");
        }

        /// <summary>
        /// constructs the TempBASRegistration entity
        /// </summary>
        /// <param name="tempBasRegEntity">TempBASRegistration to be constructed.</param>
        /// <param name="xmlNode">XML Node of the Registration</param>
        /// <param name="namespaceManager">namespace manager for xml name spaces</param>
        public void BuildEntity(TempBASRegistration tempBasRegEntity, XmlNode xmlNode, XmlNamespaceManager namespaceManager)
        {
            this.syncLogger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "Begin RegistrationEntityBuilder.BuildEntity for Temp Bas Registration");

            // Common Serialization
            XmlNode node = null;

            this.syncLogger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "Begin Build Temp Bas Registration Entity");

            // build Registration disposition object
            RegistrationDisposition regDisposition = new RegistrationDisposition();
            Utils.DeserializeXMLToObject(ref regDisposition, xmlNode.SelectSingleNode("ren:Temp_BAS_Registration/ren:RegistrationDisposition", namespaceManager).OuterXml);
            tempBasRegEntity.Disposition = regDisposition;

            // eamil
            node = xmlNode.SelectSingleNode("ren:Temp_BAS_Registration/ren:tbasContact/vcard:properties/vcard:email", namespaceManager);
            if (node != null)
            {
                tempBasRegEntity.Contact.Email = this.BuildEamils(xmlNode.SelectNodes("ren:Temp_BAS_Registration/ren:tbasContact/vcard:properties/vcard:email", namespaceManager));
            }

            // address
            node = null;
            node = xmlNode.SelectSingleNode("ren:Temp_BAS_Registration/ren:tbasContact/vcard:properties/vcard:adr", namespaceManager);
            if (node != null)
            {
                tempBasRegEntity.Contact.Address = this.BuildAddressEntity(node, namespaceManager);
            }

            // Registrant's org
            node = null;
            node = xmlNode.SelectSingleNode("ren:Temp_BAS_Registration/ren:tbasRegistrant/vcard:properties/vcard:org", namespaceManager);
            if (node != null)
            {
                Entities.Versitcard.Organization org = new Entities.Versitcard.Organization();
                Utils.DeserializeXMLToObject(ref org, node.OuterXml);
                tempBasRegEntity.Registrant.Org = org;
            }

            // Telephones
            node = null;
            node = xmlNode.SelectSingleNode("ren:Temp_BAS_Registration/ren:tbasContact/vcard:properties/vcard:tel", namespaceManager);
            if (node != null)
            {
                tempBasRegEntity.Contact.Telephone = this.BuildTelephones(xmlNode.SelectNodes("ren:Temp_BAS_Registration/ren:tbasContact/vcard:properties/vcard:tel", namespaceManager));
            }

            // Time zone
            node = null;
            node = xmlNode.SelectSingleNode("ren:Temp_BAS_Registration/ren:tbasEvent/ren:presentationTime/ren:tzname", namespaceManager);
            if (node != null)
            {
                tempBasRegEntity.Contact.TimeZone = node.InnerText;
            }

            // Title
            node = null;
            node = xmlNode.SelectSingleNode("ren:Temp_BAS_Registration/ren:presentationTime/vcard:title", namespaceManager);
            if (node != null)
            {
                Entities.Versitcard.Title title = new Entities.Versitcard.Title();
                Utils.DeserializeXMLToObject(ref title, node.OuterXml);
                tempBasRegEntity.Contact.Title = title;
            }

            // Temp Bas serialization
            node = null;
            node = xmlNode.SelectSingleNode("ren:Temp_BAS_Registration/ren:tbasRecvLocation", namespaceManager);
            if (node != null)
            {
                tempBasRegEntity.RecvLocation.Latitude = Convert.ToDouble(node.SelectSingleNode("//ren:locLatitude", namespaceManager).InnerText);
                tempBasRegEntity.RecvLocation.Longitude = Convert.ToDouble(node.SelectSingleNode("//ren:locLongitude", namespaceManager).InnerText);
                tempBasRegEntity.RecvLocation.Datum = node.SelectSingleNode("//ren:locDatum", namespaceManager).InnerText;
                RadiationCenter radiationCenter = new RadiationCenter();
                Utils.DeserializeXMLToObject(ref radiationCenter, node.SelectSingleNode("//ren:locRadiationCenter", namespaceManager).OuterXml);
                tempBasRegEntity.RecvLocation.RadiationCenter = radiationCenter;
            }

            // Temp Bas Channel serialization
            node = null;
            node = xmlNode.SelectSingleNode("ren:Temp_BAS_Registration/ren:tbasChannel", namespaceManager);
            if (node != null)
            {
                if (node.SelectSingleNode("//ren:ustChannel", namespaceManager) != null)
                {
                    tempBasRegEntity.Channel.Channel = Convert.ToInt32(node.SelectSingleNode("//ren:ustChannel", namespaceManager).InnerText);
                }

                tempBasRegEntity.Channel.CallSign = node.SelectSingleNode("//ren:ustCallSign", namespaceManager).InnerText;
            }

            // Temp Bas transmit location serialiation
            node = null;
            node = xmlNode.SelectSingleNode("ren:Temp_BAS_Registration/ren:tbasXmitLocation", namespaceManager);
            if (node != null)
            {
                XElement element = XElement.Parse(node.OuterXml);
                var curnamespace = element.GetDefaultNamespace();
                tempBasRegEntity.TransmitLocation.Latitude = element.Descendants(XName.Get("locLatitude", curnamespace.NamespaceName)).FirstOrDefault().Value.ToDouble();
                tempBasRegEntity.TransmitLocation.Longitude = element.Descendants(XName.Get("locLongitude", curnamespace.NamespaceName)).FirstOrDefault().Value.ToDouble();
                tempBasRegEntity.TransmitLocation.Datum = element.Descendants(XName.Get("locDatum", curnamespace.NamespaceName)).FirstOrDefault().Value;
                RadiationCenter radiationCenter = new RadiationCenter();
                Utils.DeserializeXMLToObject(ref radiationCenter, node.SelectSingleNode("//ren:locRadiationCenter", namespaceManager).OuterXml);
                tempBasRegEntity.TransmitLocation.RadiationCenter = radiationCenter;
            }

            // Temp Bas transmit location serialiation
            node = null;
            node = xmlNode.SelectSingleNode("ren:Temp_BAS_Registration/ren:tbasEvent", namespaceManager);
            if (node != null)
            {
                // Event Deserializing
                tempBasRegEntity.Event = this.BuildRegistrationEvent(xmlNode.SelectSingleNode("ren:Temp_BAS_Registration/ren:tbasEvent", namespaceManager), namespaceManager);
            }

            this.syncLogger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "End Build Temp Bas Registration Entity");
        }

        /// <summary>
        /// constructs the Fixed TVBD Registration Entity
        /// </summary>
        /// <param name="fixedTvbdRegEntity">FixedTVBDRegistration to be constructed.</param>
        /// <param name="xmlNode">XML Node of the registration</param>
        /// <param name="namespaceManager">namespace manager for xml name spaces</param>
        public void BuildEntity(FixedTVBDRegistration fixedTvbdRegEntity, XmlNode xmlNode, XmlNamespaceManager namespaceManager)
        {
            this.syncLogger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "Begin RegistrationEntityBuilder.BuildEntity for Fixed TVBD Registration");

            // Common Serialization
            XmlNode node = null;

            this.syncLogger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "Begin Build Fixed TVBD Registration Entity");

            // build Registration disposition object
            RegistrationDisposition regDisposition = new RegistrationDisposition();
            Utils.DeserializeXMLToObject(ref regDisposition, xmlNode.SelectSingleNode("ren:Fixed_TVBD_Registration/ren:RegistrationDisposition", namespaceManager).OuterXml);
            fixedTvbdRegEntity.Disposition = regDisposition;

            // eamil
            node = xmlNode.SelectSingleNode("ren:Fixed_TVBD_Registration/ren:tvbdContact/vcard:properties/vcard:email", namespaceManager);
            if (node != null)
            {
                fixedTvbdRegEntity.Contact.Email = this.BuildEamils(xmlNode.SelectNodes("ren:Fixed_TVBD_Registration/ren:tvbdContact/vcard:properties/vcard:email", namespaceManager));
            }

            // address
            node = null;
            node = xmlNode.SelectSingleNode("ren:Fixed_TVBD_Registration/ren:tvbdContact/vcard:properties/vcard:adr", namespaceManager);
            if (node != null)
            {
                fixedTvbdRegEntity.Contact.Address = this.BuildAddressEntity(node, namespaceManager);
            }

            // Registrant's org
            node = null;
            node = xmlNode.SelectSingleNode("ren:Fixed_TVBD_Registration/ren:tvbdRegistrant/vcard:properties/vcard:org", namespaceManager);
            if (node != null)
            {
                Entities.Versitcard.Organization org = new Entities.Versitcard.Organization();
                Utils.DeserializeXMLToObject(ref org, node.OuterXml);
                fixedTvbdRegEntity.Registrant.Org = org;
            }

            // Telephones
            node = null;
            node = xmlNode.SelectSingleNode("ren:Fixed_TVBD_Registration/ren:tbasContact/vcard:properties/vcard:tel", namespaceManager);
            if (node != null)
            {
                fixedTvbdRegEntity.Contact.Telephone = this.BuildTelephones(xmlNode.SelectNodes("ren:Fixed_TVBD_Registration/ren:tbasContact/vcard:properties/vcard:tel", namespaceManager));
            }

            // Time zone
            node = null;
            node = xmlNode.SelectSingleNode("ren:Fixed_TVBD_Registration/ren:tbasEvent/ren:presentationTime/ren:tzname", namespaceManager);
            if (node != null)
            {
                fixedTvbdRegEntity.Contact.TimeZone = node.InnerText;
            }

            // Title
            node = null;
            node = xmlNode.SelectSingleNode("ren:Fixed_TVBD_Registration/ren:presentationTime/vcard:title", namespaceManager);
            if (node != null)
            {
                Entities.Versitcard.Title title = new Entities.Versitcard.Title();
                Utils.DeserializeXMLToObject(ref title, node.OuterXml);
                fixedTvbdRegEntity.Contact.Title = title;
            }

            // Fixed TVBD location serialization
            node = null;
            node = xmlNode.SelectSingleNode("ren:Fixed_TVBD_Registration/ren:tvbdRegLocation", namespaceManager);
            if (node != null)
            {
                fixedTvbdRegEntity.Loc.Latitude = Convert.ToDouble(node.SelectSingleNode("//ren:locLatitude", namespaceManager).InnerText);
                fixedTvbdRegEntity.Loc.Longitude = Convert.ToDouble(node.SelectSingleNode("//ren:locLongitude", namespaceManager).InnerText);
                fixedTvbdRegEntity.Loc.Datum = node.SelectSingleNode("//ren:locDatum", namespaceManager).InnerText;
                RadiationCenter radiationCenter = new RadiationCenter();
                Utils.DeserializeXMLToObject(ref radiationCenter, node.SelectSingleNode("//ren:locRadiationCenter", namespaceManager).OuterXml);
                fixedTvbdRegEntity.Loc.RadiationCenter = radiationCenter;
            }

            // Fixed TVBD Device ID serialization
            node = null;
            node = xmlNode.SelectSingleNode("ren:Fixed_TVBD_Registration/ren:tvbdRegDeviceId", namespaceManager);
            if (node != null)
            {
                DeviceId device = new DeviceId();
                Utils.DeserializeXMLToObject(ref device, xmlNode.SelectSingleNode("ren:Fixed_TVBD_Registration/ren:tvbdRegDeviceId", namespaceManager).OuterXml);
                fixedTvbdRegEntity.DeviceId = device;
            }

            this.syncLogger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "End RegistrationEntityBuilder.BuildEntity for Fixed TVBD Registration");
        }

        /// <summary>
        /// constructs the TV Receive Site Registration Entity
        /// </summary>
        /// <param name="tvreceiveSiteRegistration"> TV ReceiveSiteRegistration to be constructed.</param>
        /// <param name="xmlNode">XML Registration Node</param>
        /// <param name="namespaceManager">namespace manager for xml name spaces</param>
        public void BuildEntity(TVReceiveSiteRegistration tvreceiveSiteRegistration, XmlNode xmlNode, XmlNamespaceManager namespaceManager)
        {
            this.syncLogger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "Begin RegistrationEntityBuilder.BuildEntity for TV Receive Site Registration");

            // Common Serialization
            XmlNode node = null;

            this.syncLogger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "Begin Build TV Receive Site Registration Entity");

            // build Registration disposition object
            RegistrationDisposition regDisposition = new RegistrationDisposition();
            Utils.DeserializeXMLToObject(ref regDisposition, xmlNode.SelectSingleNode("ren:TV_Receive_Site_Registration/ren:RegistrationDisposition", namespaceManager).OuterXml);
            tvreceiveSiteRegistration.Disposition = regDisposition;

            // eamil
            node = xmlNode.SelectSingleNode("ren:TV_Receive_Site_Registration/ren:tvrcContact/vcard:properties/vcard:email", namespaceManager);
            if (node != null)
            {
                tvreceiveSiteRegistration.Contact.Email = this.BuildEamils(xmlNode.SelectNodes("ren:TV_Receive_Site_Registration/ren:tvrcContact/vcard:properties/vcard:email", namespaceManager));
            }

            // address
            node = null;
            node = xmlNode.SelectSingleNode("ren:TV_Receive_Site_Registration/ren:tvrcContact/vcard:properties/vcard:adr", namespaceManager);
            if (node != null)
            {
                tvreceiveSiteRegistration.Contact.Address = this.BuildAddressEntity(node, namespaceManager);
            }

            // Registrant's org
            node = null;
            node = xmlNode.SelectSingleNode("ren:TV_Receive_Site_Registration/ren:tvrcRegistrant/vcard:properties/vcard:org", namespaceManager);
            if (node != null)
            {
                Entities.Versitcard.Organization org = new Entities.Versitcard.Organization();
                Utils.DeserializeXMLToObject(ref org, node.OuterXml);
                tvreceiveSiteRegistration.Registrant.Org = org;
            }

            // Telephones
            node = null;
            node = xmlNode.SelectSingleNode("ren:TV_Receive_Site_Registration/ren:tvrcContact/vcard:properties/vcard:tel", namespaceManager);
            if (node != null)
            {
                tvreceiveSiteRegistration.Contact.Telephone = this.BuildTelephones(xmlNode.SelectNodes("ren:TV_Receive_Site_Registration/ren:tvrcContact/vcard:properties/vcard:tel", namespaceManager));
            }

            // Title
            node = null;
            node = xmlNode.SelectSingleNode("ren:TV_Receive_Site_Registration/ren:presentationTime/vcard:title", namespaceManager);
            if (node != null)
            {
                Entities.Versitcard.Title title = new Entities.Versitcard.Title();
                Utils.DeserializeXMLToObject(ref title, node.OuterXml);
                tvreceiveSiteRegistration.Contact.Title = title;
            }

            // TV Receive Site serialization
            node = null;
            node = xmlNode.SelectSingleNode("ren:TV_Receive_Site_Registration/ren:tvrcRecvLocation", namespaceManager);
            if (node != null)
            {
                tvreceiveSiteRegistration.ReceiveLocation.Latitude = Convert.ToDouble(node.SelectSingleNode("//ren:locLatitude", namespaceManager).InnerText);
                tvreceiveSiteRegistration.ReceiveLocation.Longitude = Convert.ToDouble(node.SelectSingleNode("//ren:locLongitude", namespaceManager).InnerText);
                tvreceiveSiteRegistration.ReceiveLocation.Datum = node.SelectSingleNode("//ren:locDatum", namespaceManager).InnerText;
                RadiationCenter radiationCenter = new RadiationCenter();
                Utils.DeserializeXMLToObject(ref radiationCenter, node.SelectSingleNode("//ren:locRadiationCenter", namespaceManager).OuterXml);
                tvreceiveSiteRegistration.ReceiveLocation.RadiationCenter = radiationCenter;
            }

            // TV Receive Site Channel serialization
            node = null;
            node = xmlNode.SelectSingleNode("ren:TV_Receive_Site_Registration/ren:tvrcXmitChannel", namespaceManager);
            if (node != null)
            {
                if (node.SelectSingleNode("//ren:ustChannel", namespaceManager) != null)
                {
                    tvreceiveSiteRegistration.TransmitChannel.Channel = Convert.ToInt32(node.SelectSingleNode("//ren:ustChannel", namespaceManager).InnerText);
                }

                tvreceiveSiteRegistration.TransmitChannel.CallSign = node.SelectSingleNode("//ren:ustCallSign", namespaceManager).InnerText;
            }

            // TV Receive Site transmit location serialiation
            node = null;
            node = xmlNode.SelectSingleNode("ren:TV_Receive_Site_Registration/ren:tvrcXmitLocation", namespaceManager);
            if (node != null)
            {
                tvreceiveSiteRegistration.TransmitLocation.Latitude = Convert.ToDouble(node.SelectSingleNode("//ren:locLatitude", namespaceManager).InnerText);
                tvreceiveSiteRegistration.TransmitLocation.Longitude = Convert.ToDouble(node.SelectSingleNode("//ren:locLongitude", namespaceManager).InnerText);
                tvreceiveSiteRegistration.TransmitLocation.Datum = node.SelectSingleNode("//ren:locDatum", namespaceManager).InnerText;
                RadiationCenter radiationCenter = new RadiationCenter();
                Utils.DeserializeXMLToObject(ref radiationCenter, node.SelectSingleNode("//ren:locRadiationCenter", namespaceManager).OuterXml);
                tvreceiveSiteRegistration.TransmitLocation.RadiationCenter = radiationCenter;
            }

            // TV Receive Site receive Call Sign serialiation
            node = null;
            node = xmlNode.SelectSingleNode("ren:TV_Receive_Site_Registration/ren:tvrcRecvCallSign", namespaceManager);
            if (node != null)
            {
                // Event Deserializing
                tvreceiveSiteRegistration.ReceiveCallSign.CallSign = node.SelectSingleNode("//ren:ustCallSign", namespaceManager).InnerText;
            }

            this.syncLogger.Log(TraceEventType.Information, LoggingMessageId.DBSyncPollerGenericMessage, "End Build TV Receive Site Registration Entity");
        }

        /// <summary>
        /// constructs the Spectrum object Entity
        /// </summary>
        /// <param name="lpauxReg">CallSign Object to be deserialized.</param>
        /// <param name="xmlNode">XML Node</param>
        /// <param name="namespaceManager">name space manager</param>
        private void BuildLPAuxTvsSpectrum(ref LPAuxRegistration lpauxReg, XmlNode xmlNode, XmlNamespaceManager namespaceManager)
        {
            if (xmlNode.SelectSingleNode("ren:LP-Aux_Registration/ren:lpauxCallSign", namespaceManager) != null)
            {
                lpauxReg.CallSign.CallSign = xmlNode.SelectSingleNode("ren:LP-Aux_Registration/ren:lpauxCallSign", namespaceManager).InnerText;
            }

            if (xmlNode.SelectSingleNode("ren:LP-Aux_Registration/ren:lpauxEvent/ren:eventChannel", namespaceManager) != null)
            {
                lpauxReg.CallSign.Channel = Convert.ToInt32(xmlNode.SelectNodes("ren:LP-Aux_Registration/ren:lpauxEvent/ren:eventChannel/ren:chanNum", namespaceManager)[0].InnerText);
            }
        }

        /// <summary>
        /// constructs the Spectrum object Entity
        /// </summary>
        /// <param name="xmlNode">XML Node</param>
        /// <param name="namespaceManager">Name Space Manager</param>
        /// <returns> Event Entity </returns>
        private Event BuildRegistrationEvent(XmlNode xmlNode, XmlNamespaceManager namespaceManager)
        {
            Calendar[] cal = null;
            XmlNodeList nodeList = null;
            Event regEvent = new Event();
            int[] channelNumber = null;

            // set channels array
            if (xmlNode.SelectSingleNode("ren:eventChannel", namespaceManager) != null)
            {
                nodeList = xmlNode.SelectNodes("ren:chanNum", namespaceManager);
                channelNumber = new int[nodeList.Count];
                for (int index = 0; index < nodeList.Count; index++)
                {
                    channelNumber[index] = Convert.ToInt32(nodeList[index].InnerText);
                }
            }

            regEvent.Channels = channelNumber;

            // set calender array
            if (xmlNode.SelectNodes("ren:eventTimes", namespaceManager) != null)
            {
                nodeList = xmlNode.SelectNodes("ren:eventTimes", namespaceManager);
                cal = new Calendar[nodeList.Count];
                for (int nodeIndex = 0; nodeIndex < nodeList.Count; nodeIndex++)
                {
                    cal[nodeIndex] = new Calendar();

                    if (nodeList[nodeIndex].SelectSingleNode("ical:properties/ical:dtstamp/ical:utc-date-time", namespaceManager).InnerText != string.Empty)
                    {
                        cal[nodeIndex].Stamp = nodeList[nodeIndex].SelectSingleNode("ical:properties/ical:dtstamp/ical:utc-date-time", namespaceManager).InnerText;
                    }

                    if (nodeList[nodeIndex].SelectSingleNode("ical:properties/ical:dtstart/ical:date-time", namespaceManager).InnerText != string.Empty)
                    {
                        cal[nodeIndex].Start = nodeList[nodeIndex].SelectSingleNode("ical:properties/ical:dtstart/ical:date-time", namespaceManager).InnerText;
                    }

                    if (nodeList[nodeIndex].SelectSingleNode("ical:properties/ical:dtend/ical:date-time", namespaceManager) != null)
                    {
                        cal[nodeIndex].End = nodeList[nodeIndex].SelectSingleNode("ical:properties/ical:dtend/ical:date-time", namespaceManager).InnerText;
                    }

                    if (nodeList[nodeIndex].SelectSingleNode("ical:properties/ical:rrule/ical:recur", namespaceManager) != null)
                    {
                        cal[nodeIndex].Recurrence = this.BuildRecurrenceEvent(nodeList[nodeIndex].SelectSingleNode("ical:properties/ical:rrule/ical:recur", namespaceManager), namespaceManager);
                    }

                    if (cal[nodeIndex].Recurrence != null)
                    {
                        if (!string.IsNullOrEmpty(cal[nodeIndex].Recurrence.Until))
                        {
                            cal[nodeIndex].End = cal[nodeIndex].Recurrence.Until;
                        }
                    }

                    cal[nodeIndex].UId = nodeList[nodeIndex].SelectSingleNode("ical:properties/ical:uid/ical:text", namespaceManager).InnerText;
                }

                regEvent.Times = cal;
            }

            return regEvent;
        }

        /// <summary>
        /// Builds the registration event.
        /// </summary>
        /// <param name="xmlNode">The XML node.</param>
        /// <param name="namespaceManager">The namespace manager.</param>
        /// <returns>returns Event.</returns>
        private EventRecurrence BuildRecurrenceEvent(XmlNode xmlNode, XmlNamespaceManager namespaceManager)
        {
            EventRecurrence regEvent = new EventRecurrence();

            if (xmlNode.SelectSingleNode("ical:freq", namespaceManager) != null)
            {
                regEvent.Frequency = xmlNode.SelectSingleNode("ical:freq", namespaceManager).InnerText;
            }

            if (xmlNode.SelectSingleNode("ical:until/ical:date-time", namespaceManager) != null)
            {
                regEvent.Until = xmlNode.SelectSingleNode("ical:until/ical:date-time", namespaceManager).InnerText;
            }

            if (xmlNode.SelectSingleNode("ical:byday", namespaceManager) != null)
            {
                regEvent.ByDay = xmlNode.SelectSingleNode("ical:byday", namespaceManager).InnerText;
            }

            if (xmlNode.SelectSingleNode("ical:byminute", namespaceManager) != null)
            {
                regEvent.ByMinute = xmlNode.SelectSingleNode("ical:byminute", namespaceManager).InnerText;
            }

            if (xmlNode.SelectSingleNode("ical:byhour", namespaceManager) != null)
            {
                regEvent.ByHour = xmlNode.SelectSingleNode("ical:byhour", namespaceManager).InnerText;
            }

            if (xmlNode.SelectSingleNode("ical:count", namespaceManager) != null)
            {
                regEvent.Count = xmlNode.SelectSingleNode("ical:count", namespaceManager).InnerText.ToInt32();
            }

            if (xmlNode.SelectSingleNode("ical:interval", namespaceManager) != null)
            {
                regEvent.Interval = xmlNode.SelectSingleNode("ical:interval", namespaceManager).InnerText;
            }

            return regEvent;
        }

        /// <summary>
        /// constructs the Quadrilateral Area for LP-AUX Registration
        /// </summary>
        /// <param name="xmlNodeList">XML Node list</param>
        /// <param name="namespaceManager">Name Space Manager which has all xml name spaces</param>
        /// <returns>QuadrilateralArea array</returns>
        private QuadrilateralArea[] BuildLPAuxQuadArea(XmlNodeList xmlNodeList, XmlNamespaceManager namespaceManager)
        {
            QuadrilateralArea[] quadrilArea = null;
            char[] delimiterChars = { ' ' };
            string[] pos = new string[2];
            Position position = null;

            if (xmlNodeList != null)
            {
                quadrilArea = new QuadrilateralArea[xmlNodeList.Count];
                for (int index = 0; index < xmlNodeList.Count; index++)
                {
                    // Ne Point
                    quadrilArea[index] = new QuadrilateralArea();
                    position = new Position();
                    pos = xmlNodeList[index].SelectSingleNode("ren:NE_Point/gml:pos", namespaceManager).InnerText.Split(delimiterChars);
                    position.Latitude = Convert.ToDouble(pos[0]);
                    position.Longitude = Convert.ToDouble(pos[1]);
                    quadrilArea[index].NEPoint = position;

                    // NW point
                    position = new Position();
                    pos = xmlNodeList[index].SelectSingleNode("ren:NW_Point/gml:pos", namespaceManager).InnerText.Split(delimiterChars);
                    position.Latitude = Convert.ToDouble(pos[0]);
                    position.Longitude = Convert.ToDouble(pos[1]);
                    quadrilArea[index].NWPoint = position;

                    // SE point
                    position = new Position();
                    pos = xmlNodeList[index].SelectSingleNode("ren:SE_Point/gml:pos", namespaceManager).InnerText.Split(delimiterChars);
                    position.Latitude = Convert.ToDouble(pos[0]);
                    position.Longitude = Convert.ToDouble(pos[1]);
                    quadrilArea[index].SEPoint = position;

                    // SW point
                    position = new Position();
                    pos = xmlNodeList[index].SelectSingleNode("ren:SW_Point/gml:pos", namespaceManager).InnerText.Split(delimiterChars);
                    position.Latitude = Convert.ToDouble(pos[0]);
                    position.Longitude = Convert.ToDouble(pos[1]);
                    quadrilArea[index].SWPoint = position;
                }
            }

            return quadrilArea;
        }

        /// <summary>
        /// constructs the Quadrilateral Area for LP-AUX Registration
        /// </summary>
        /// <param name="xmlNode"> XML Node</param>
        /// <param name="namespaceManager">xml name space manager</param>
        /// <returns> Position Array</returns>
        private Position[] BuildLPAuxPointsArea(XmlNode xmlNode, XmlNamespaceManager namespaceManager)
        {
            Position[] positions = null;
            XmlNodeList nodeList = null;
            char[] delimiterChars = { ' ' };
            string[] pos = new string[2];
            Position centerPosition = null;

            if (xmlNode != null)
            {
                nodeList = xmlNode.SelectNodes("ren:CenterPoint", namespaceManager);
                positions = new Position[nodeList.Count];

                for (int index = 0; index < nodeList.Count; index++)
                {
                    // SW point
                    positions[index] = new Position();
                    centerPosition = new Position();
                    pos = nodeList[index].SelectSingleNode("gml:pos", namespaceManager).InnerText.Split(delimiterChars);
                    centerPosition.Latitude = Convert.ToDouble(pos[0]);
                    centerPosition.Longitude = Convert.ToDouble(pos[1]);
                    positions[index] = centerPosition;
                }
            }

            return positions;
        }

        /// <summary>
        /// constructs the Quadrilateral Area for LP-AUX Registration
        /// </summary>
        /// <param name="xmlNode">XML Node</param>
        /// <param name="namespaceManager">xml name space manager</param>
        /// <returns> VERSITCARD Address </returns>
        private Entities.Versitcard.Address BuildAddressEntity(XmlNode xmlNode, XmlNamespaceManager namespaceManager)
        {
            Entities.Versitcard.Address add = new Entities.Versitcard.Address();
            if (xmlNode.SelectSingleNode("vcard:street/vcard:text", namespaceManager) != null)
            {
                add.Street = xmlNode.SelectSingleNode("vcard:street/vcard:text", namespaceManager).InnerText;
            }

            if (xmlNode.SelectSingleNode("vcard:locality/vcard:text", namespaceManager) != null)
            {
                add.Locality = xmlNode.SelectSingleNode("vcard:locality/vcard:text", namespaceManager).InnerText;
            }

            if (xmlNode.SelectSingleNode("vcard:region/vcard:text", namespaceManager) != null)
            {
                add.Region = xmlNode.SelectSingleNode("vcard:region/vcard:text", namespaceManager).InnerText;
            }

            if (xmlNode.SelectSingleNode("vcard:code/vcard:text", namespaceManager) != null)
            {
                add.Code = xmlNode.SelectSingleNode("vcard:code/vcard:text", namespaceManager).InnerText;
            }

            if (xmlNode.SelectSingleNode("vcard:country/vcard:text", namespaceManager) != null)
            {
                add.Country = xmlNode.SelectSingleNode("vcard:country/vcard:text", namespaceManager).InnerText;
            }

            return add;
        }

        /// <summary>
        /// constructs the Quadrilateral Area for LP-AUX Registration
        /// </summary>
        /// <param name="xmlNode">XML Node</param>
        /// <param name="namespaceManager">xml name space manager</param>
        /// <returns> VERSITCARD VCard </returns>
        private Entities.Versitcard.VCard BuildVCardEntity(XmlNode xmlNode, XmlNamespaceManager namespaceManager)
        {
            Entities.Versitcard.VCard vcard = new Entities.Versitcard.VCard();

            return vcard;
        }

        /// <summary>
        /// constructs the Quadrilateral Area for LP-AUX Registration
        /// </summary>
        /// <param name="xmlNode">XML Node List</param>
        /// <returns> VERSITCARD Email[] </returns>
        private Entities.Versitcard.Email[] BuildEamils(XmlNodeList xmlNode)
        {
            Entities.Versitcard.Email[] emails = new Entities.Versitcard.Email[xmlNode.Count];
            if (xmlNode != null)
            {
                for (int index = 0; index < xmlNode.Count; index++)
                {
                    Entities.Versitcard.Email email = new Entities.Versitcard.Email();
                    Utils.DeserializeXMLToObject(ref email, xmlNode[index].OuterXml);
                    emails[index] = email;
                }
            }

            return emails;
        }

        /// <summary>
        /// constructs the Quadrilateral Area for LP-AUX Registration
        /// </summary>
        /// <param name="xmlNode">XML Node list</param>
        /// <returns> VERSITCARD Telephone[] </returns>
        private Entities.Versitcard.Telephone[] BuildTelephones(XmlNodeList xmlNode)
        {
            Entities.Versitcard.Telephone[] telno = new Entities.Versitcard.Telephone[xmlNode.Count];
            if (xmlNode != null)
            {
                for (int index = 0; index < xmlNode.Count; index++)
                {
                    Entities.Versitcard.Telephone tel = new Entities.Versitcard.Telephone();
                    Utils.DeserializeXMLToObject(ref tel, xmlNode[index].OuterXml);
                    telno[index] = tel;
                }
            }

            return telno;
        }
    }
}
