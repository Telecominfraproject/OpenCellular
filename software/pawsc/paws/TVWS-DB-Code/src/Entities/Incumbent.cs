// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Entities
{
    using System;
    using System.Collections.Generic;
    using WindowsAzure.Storage.Table;

    /// <summary>
    /// Represents Incumbent
    /// </summary>
    public class Incumbent : TableEntity
    {
        /// <summary>
        /// Gets or sets the type of the incumbent.
        /// </summary>
        /// <value>The type of the incumbent.</value>
        public IncumbentType IncumbentType { get; set; }

        /// <summary>
        /// Gets or sets the Location
        /// </summary>
        public Location Location
        {
            get
            {
                return new Location(this.Latitude, this.Longitude, this.SemiMajorAxis, this.SemiMinorAxis);
            }

            set
            {
                this.Latitude = value.Latitude;
                this.Longitude = value.Longitude;
                this.SemiMajorAxis = value.SemiMajorAxis;
                this.SemiMinorAxis = value.SemiMinorAxis;
            }
        }

        /// <summary>
        /// Gets or sets the OSG location.
        /// </summary>
        /// <value>The OSG location.</value>
        public OSGLocation OSGLocation { get; set; }

        /// <summary>
        /// Gets or sets the transmitter location.
        /// </summary>
        /// <value>The transmitter location.</value>
        public Location TxLocation
        {
            get
            {
                return new Location(this.TxLatitude, this.TxLongitude);
            }

            set
            {
                this.TxLatitude = value.Latitude;
                this.TxLongitude = value.Longitude;
            }
        }

        /// <summary>
        /// Gets or sets the WSD usage nature.
        /// </summary>
        /// <value>The WSD usage nature.</value>
        public int WSDUsageNature { get; set; }

        /// <summary>
        /// Gets or sets the antenna identifier.
        /// </summary>
        /// <value>The antenna identifier.</value>
        public int AntennaId { get; set; }

        /// <summary>
        /// Gets or sets the Channel
        /// </summary>
        public int Channel { get; set; }

        /// <summary>
        /// Gets or sets the latitude position.
        /// </summary>
        public double Latitude { get; set; }

        /// <summary>
        /// Gets or sets the semi major axis.
        /// </summary>
        /// <value>The semi major axis.</value>
        public double SemiMajorAxis { get; set; }

        /// <summary>
        /// Gets or sets the semi minor axis.
        /// </summary>
        /// <value>The semi minor axis.</value>
        public double SemiMinorAxis { get; set; }

        /// <summary>
        /// Gets or sets the longitude position.
        /// </summary>
        public double Longitude { get; set; }

        /// <summary>
        /// Gets or sets the transmitter latitude position.
        /// </summary>
        public double TxLatitude { get; set; }

        /// <summary>
        /// Gets or sets the transmitter longitude position.
        /// </summary>
        public double TxLongitude { get; set; }

        /// <summary>
        /// Gets or sets the Height
        /// </summary>
        public double Height { get; set; }

        /// <summary>
        /// Gets or sets the Transmitter Power
        /// </summary>
        public double TxPower { get; set; }

        /// <summary>
        /// Gets or sets the start frequency.
        /// </summary>
        /// <value>The start frequency.</value>
        public double StartHz { get; set; }

        /// <summary>
        /// Gets or sets the stop frequency.
        /// </summary>
        /// <value>The start frequency.</value>
        public double StopHz { get; set; }

        /// <summary>
        /// Gets or sets CallSign
        /// </summary>
        public string CallSign { get; set; }

        /// <summary>
        /// Gets or sets the CountryId
        /// </summary>
        public string CountryId { get; set; }

        /// <summary>
        /// Gets or sets the StartTime
        /// </summary>
        public DateTime StartTime { get; set; }

        /// <summary>
        /// Gets or sets the EndTime
        /// </summary>
        public DateTime EndTime { get; set; }

        /// <summary>
        /// Gets or sets the serialized contour points.
        /// </summary>
        /// <value>The contour points.</value>
        public string Contour { get; set; }

        /// <summary>
        /// Gets or sets the antenna pattern.
        /// </summary>
        /// <value>The antenna pattern.</value>
        public double[] AntennaPattern { get; set; }

        /// <summary>
        /// Gets or sets the antenna pattern data.
        /// </summary>
        /// <value>The antenna pattern data.</value>
        public string AntennaPatternData { get; set; }

        /// <summary>
        /// Gets or sets the PMSE usage nature.
        /// </summary>
        /// <value>The PMSE usage nature.</value>
        public string PMSEUsageNature { get; set; }

        /// <summary>
        /// Gets or sets the PMSE use case.
        /// </summary>
        /// <value>The PMSE use case.</value>
        public string PMSEUseCase { get; set; }

        /// <summary>
        /// Gets or sets the emission class.
        /// </summary>
        /// <value>The emission class.</value>
        public string EmissionClass { get; set; }

        /// <summary>
        /// Gets or sets the serialized event timings.
        /// </summary>
        /// <value>The events.</value>
        public string Events { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether this instance is new.
        /// </summary>
        /// <value><c>true</c> if this instance is new; otherwise, <c>false</c>.</value>
        public bool IsNew { get; set; }

        /// <summary>
        /// Gets or sets the device category.
        /// </summary>
        /// <value>The device category.</value>
        public string DeviceCategory { get; set; }

        /// <summary>
        /// Gets or sets the MVPD channel.
        /// </summary>
        /// <value>The MVPD channel.</value>
        public string MVPDChannel { get; set; }

        /// <summary>
        /// Gets or sets the temporary BAS channel.
        /// </summary>
        /// <value>The temporary BAS channel.</value>
        public string TempBasChannel { get; set; }

        /// <summary>
        /// Gets or sets the VSD service.
        /// </summary>
        /// <value>The VSD service.</value>
        public string VsdService { get; set; }

        /// <summary>
        /// Gets or sets the antenna rotation.
        /// </summary>
        /// <value>The antenna rotation.</value>
        public double AntRotation { get; set; }

        /// <summary>
        /// Gets or sets the type of the antenna.
        /// </summary>
        /// <value>The type of the antenna.</value>
        public string AntennaType { get; set; }

        /// <summary>
        /// Gets or sets the broadcast station contour.
        /// </summary>
        /// <value>The broadcast station contour.</value>
        public string BroadcastStationContour { get; set; }

        /// <summary>
        /// Gets or sets the receiver call sign.
        /// </summary>
        /// <value>The receiver call sign.</value>
        public string TVReceiveSiteReceiveCallSign { get; set; }

        /// <summary>
        /// Gets or sets the channel number.
        /// </summary>
        /// <value>The channel number.</value>
        public int ChannelNumber { get; set; }

        /// <summary>
        /// Gets or sets the type of the height.
        /// </summary>
        /// <value>The type of the height.</value>
        public HeightType HeightType { get; set; }

        /// <summary>
        /// Gets or sets the device identifier.
        /// </summary>
        /// <value>The device identifier.</value>
        public string DeviceId { get; set; }

        /// <summary>
        /// Gets or sets the master device descriptor.
        /// </summary>
        /// <value>The master device descriptor.</value>
        public string MasterDeviceId { get; set; }

        /// <summary>
        /// Gets or sets the type of the request.
        /// </summary>
        /// <value>The type of the request.</value>
        public string RequestType { get; set; }

        /// <summary>
        /// Gets or sets the master device descriptor.
        /// </summary>
        /// <value>The master device descriptor.</value>
        public DeviceDescriptor MasterDeviceDescriptor { get; set; }

        /// <summary>
        /// Gets or sets the master device descriptor.
        /// </summary>
        /// <value>The master device descriptor.</value>
        public DeviceDescriptor DeviceDescriptor { get; set; }

        /// <summary>
        /// Gets or sets the device owner.
        /// </summary>
        /// <value>The device owner.</value>
        public DeviceOwner DeviceOwner { get; set; }

        /// <summary>
        /// Gets or sets the capabilities.
        /// </summary>
        /// <value>The capabilities.</value>
        public DeviceCapabilities Capabilities { get; set; }

        /// <summary>
        /// Gets or sets the PREFSENS.
        /// </summary>
        /// <value>The PREFSENS.</value>
        public double PREFSENS { get; set; }

        /// <summary>
        /// Gets or sets the maximum master EIRP.
        /// </summary>
        /// <value>The maximum master EIRP.</value>
        public double MaxMasterEIRP { get; set; }

        /// <summary>
        /// Gets or sets the unique identifier.
        /// </summary>
        /// <value>The unique identifier.</value>
        public string UniqueId { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether [log contour].
        /// </summary>
        /// <value><c>true</c> if [log contour]; otherwise, <c>false</c>.</value>
        public bool BuildContourItems { get; set; }

        /// <summary>
        /// Gets or sets the parameter PointArea.
        /// </summary>
        public Point[] PointsArea { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether [log details].
        /// </summary>
        /// <value><c>true</c> if [log details]; otherwise, <c>false</c>.</value>
        public bool LogDetails { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether [testing stage].
        /// </summary>
        /// <value><c>true</c> if [testing stage]; otherwise, <c>false</c>.</value>
        public int TestingStage { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether this instance is testing stage.
        /// </summary>
        /// <value><c>true</c> if this instance is testing stage; otherwise, <c>false</c>.</value>
        public bool IsTestingStage { get; set; }

        /// <summary>
        /// Gets or sets the parameter QuadrilateralArea.
        /// </summary>
        public QuadrilateralArea[] QuadrilateralArea { get; set; }

        /// <summary>
        /// Gets or sets the parent latitude.
        /// </summary>
        /// <value>The parent latitude.</value>
        public double ParentLatitude { get; set; }

        /// <summary>
        /// Gets or sets the parent longitude.
        /// </summary>
        /// <value>The parent longitude.</value>
        public double ParentLongitude { get; set; }

        /// <summary>
        /// Gets or sets the PMSE assignment table.
        /// </summary>
        /// <value>The PMSE assignment table.</value>
        public string PMSEAssignmentTable { get; set; }

        /// <summary>
        /// Gets or sets the combined data identifier.
        /// </summary>
        /// <value>The combined data identifier.</value>
        public string MergedDataIdentifier { get; set; }

        /// <summary>
        /// Gets or sets the testing intermediate results1.
        /// </summary>
        /// <value>The testing intermediate results1.</value>
        public IntermediateResults1 TestingIntermediateResults1 { get; set; }

        /// <summary>
        /// Gets or sets the testing intermediate results2.
        /// </summary>
        /// <value>The testing intermediate results2.</value>
        public IntermediateResults2 TestingIntermediateResults2 { get; set; }

        /// <summary>
        /// Gets or sets the testing output.
        /// </summary>
        /// <value>The testing output.</value>
        public IntermediateResults2 TestingOutput { get; set; }

        /// <summary>
        /// Gets or sets the minimum distance.
        /// </summary>
        /// <value>The minimum distance.</value>
        public double MinimumDistance { get; set; }        

        /// <summary>
        /// Gets or sets the type of the WSD clutter.
        /// </summary>
        /// <value>The type of the WSD clutter.</value>
        public int WsdClutterType { get; set; }

        /// <summary>
        /// Determines whether the specified type of incumbent is type.
        /// </summary>
        /// <param name="typeOfIncumbent">The type of incumbent.</param>
        /// <returns><c>true</c> if the specified type of incumbent is type; otherwise, <c>false</c>.</returns>
        public bool IsType(IncumbentType typeOfIncumbent)
        {
            return (int)this.IncumbentType == (int)typeOfIncumbent;
        }
    }
}
