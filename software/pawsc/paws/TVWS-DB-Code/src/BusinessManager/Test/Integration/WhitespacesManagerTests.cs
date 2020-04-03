// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.BusinessManager.Test.Integration
{
    using System;
    using System.Collections.Generic;
    using System.Configuration;
    using MWC = Microsoft.WhiteSpaces.Common;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WhiteSpaces.DataAccessManager;
    using Microsoft.WhiteSpaces.Test.Common;

    [TestClass]
    public class WhitespacesManagerTests
    {
        private WhitespacesManager whitespacesManager;
        private IWhitespacesDataClient whitespacesDataClient;
        private IHttpClientManager httpClientManager;
        private string accessToken = "abcd";

        public WhitespacesManagerTests()
        {
            this.httpClientManager = new HttpClientManager(ConfigurationManager.AppSettings["AuthorizationSchema"].ToString());
            this.whitespacesDataClient = new WhitespacesDataClient(this.httpClientManager);
            this.whitespacesManager = new WhitespacesManager(this.whitespacesDataClient);
        }

        [TestMethod]
        public void RegisterMVPD_ValidMVPDRegistrationInfoWithAllMandatoryParameters_RegistersMVPDSuccessfully()
        {
            MWC.MvpdRegistrationInfo mvpdRegistrationInfo = new MWC.MvpdRegistrationInfo
            {
                Name = "Mvpd Registration",
                CallSign = "WBFF",
                Channel = 46,
                RecieveLocation = new Location
                {
                    Latitude = 38.66484069824219,
                    Longitude = -77.43494415283203
                },
                TransmitLocation = new Location
                {
                    Latitude = 39.3362177768783,
                    Longitude = -76.6494136354878
                },
                Contact = new Whitespace.Entities.Versitcard.VCard
                {
                    Address = new Microsoft.Whitespace.Entities.Versitcard.Address
                          {
                              POBox = "560054",
                              Street = "New Street",
                              Locality = "New Friends Colony",
                              Region = "1",
                              Code = "Wa99808",
                              Country = "India"
                          },
                    Telephone = new Microsoft.Whitespace.Entities.Versitcard.Telephone[] 
                            {
                                new Microsoft.Whitespace.Entities.Versitcard.Telephone
                                {
                                    TelephoneNumber = "123-456-7890"
                                }
                            },
                    Email = new Microsoft.Whitespace.Entities.Versitcard.Email[] 
                            {
                                new Microsoft.Whitespace.Entities.Versitcard.Email
                                {
                                    EmailAddress = "rajendra.gola@hotmail.com"
                                }
                            }
                },
                RegistrantInfo = new Whitespace.Entities.Versitcard.VCard
                {
                    Org = new Microsoft.Whitespace.Entities.Versitcard.Organization
                    {
                        OrganizationName = "Aditi Technologies"
                    },
                    Address = new Microsoft.Whitespace.Entities.Versitcard.Address
                    {
                        POBox = "560054",
                        Street = "New Street",
                        Locality = "New Friends Colony",
                        Region = "1",
                        Code = "Wa99808",
                        Country = "India"
                    }
                },
            };

            string actualResponse = this.whitespacesManager.RegisterMVPD(mvpdRegistrationInfo, accessToken);

            Assert.AreEqual("Device Registered successfully.", actualResponse);
        }

        [TestMethod]
        public void RegisterLicensedLpAux_ValidLicensedLPAuxRegistrationInfoWithAllMandatoryParameters_RegistersLicensedLPAuxSuccessfully()
        {
            MWC.LicensedLpAuxRegistration licensedLpAuxRegistration = new MWC.LicensedLpAuxRegistration
            {
                CallSign = "BLN00751",
                Channels = new[] { 6, 7 },
                FriendlyName = "Licensed LPAux Registration",
                StartDate = DateTime.Today.ToString("MM-dd-yyyy"),
                EndDate = DateTime.Today.AddDays(10).ToString("MM-dd-yyyy"),
                StartTime = DateTime.Now.ToString("HH:mm:ss"),
                EndTime = DateTime.Now.AddHours(2).ToString("HH:mm:ss"),
                Latitude = "40.7779169",
                Longitude = "-74.7922202",
            };

            string actualResponse = this.whitespacesManager.RegisterLicensedLpAux(licensedLpAuxRegistration, accessToken);

            Assert.AreEqual("Device Registered successfully.", actualResponse);
        }

        [TestMethod]
        public void RegisterLicensedLpAux_ValidRegistrationInfoWithReccurrenceDaily_RegistersLicensedLPAuxSuccessfully()
        {
            MWC.LicensedLpAuxRegistration licensedLpAuxRegistration = new MWC.LicensedLpAuxRegistration
            {
                CallSign = "BLN00751",
                Channels = new[] { 6, 7 },
                FriendlyName = "Licensed LPAux Registration",
                IsRecurred = true,
                IsReoccurenceDaily = true,
                ReoccurenceInstance = 3,
                StartDate = DateTime.Today.ToString("MM-dd-yyyy"),
                EndDate = DateTime.Today.ToString("MM-dd-yyyy"),
                StartTime = DateTime.Now.ToString("HH:mm:ss"),
                EndTime = DateTime.Now.AddHours(2).ToString("HH:mm:ss"),
                Latitude = "40.7779169",
                Longitude = "-74.7922202",
            };

            string actualResponse = this.whitespacesManager.RegisterLicensedLpAux(licensedLpAuxRegistration, accessToken);

            Assert.AreEqual("Device Registered successfully.", actualResponse);
        }

        [TestMethod]
        public void RegisterLicensedLpAux_ValidRegistrationInfoWithReccurrenceWeekly_RegistersLicensedLPAuxSuccessfully()
        {
            MWC.LicensedLpAuxRegistration licensedLpAuxRegistration = new MWC.LicensedLpAuxRegistration
            {
                CallSign = "BLN00751",
                Channels = new[] { 6, 7 },
                FriendlyName = "Licensed LPAux Registration",
                IsRecurred = true,
                IsReoccurenceWeekly = true,
                ReoccurrenceEndDate = DateTime.Now.AddDays(28).ToString("MM-dd-yyyy"),
                WeekDaysString = "Monday,Wednesday,Friday",
                StartDate = DateTime.Today.ToString("MM-dd-yyyy"),
                EndDate = DateTime.Today.ToString("MM-dd-yyyy"),
                StartTime = DateTime.Now.ToString("HH:mm:ss"),
                EndTime = DateTime.Now.AddHours(2).ToString("HH:mm:ss"),
                Latitude = "40.7779169",
                Longitude = "-74.7922202",
            };

            string actualResponse = this.whitespacesManager.RegisterLicensedLpAux(licensedLpAuxRegistration, accessToken);

            Assert.AreEqual("Device Registered successfully.", actualResponse);
        }

        [TestMethod]
        public void RegisterUnlicensedLpAux_ValidUnlicensedLPAuxRegistrationInfoWithAllMandatoryParameters_RegistersUnlicensedLPAuxSuccessfully()
        {
            MWC.LicensedLpAuxRegistration unLicensedLpAuxRegistration = new MWC.LicensedLpAuxRegistration
            {
                UlsFileNumber = "0005122780",
                Channels = new[] { 6, 7 },
                StartDate = DateTime.Today.ToString("MM-dd-yyyy"),
                EndDate = DateTime.Today.AddDays(10).ToString("MM-dd-yyyy"),
                StartTime = DateTime.Now.ToString("HH:mm:ss"),
                EndTime = DateTime.Now.AddHours(2).ToString("HH:mm:ss"),
                Latitude = "40.7779169",
                Longitude = "-74.7922202",
                Name = "Rajendra Gola",
            };

            string actualResponse = this.whitespacesManager.RegisterUnLicensedLpAux(unLicensedLpAuxRegistration, accessToken);

            Assert.AreEqual("Device Registered successfully.", actualResponse);
        }

        [TestMethod]
        public void RegisterUnlicensedLpAux_ValidUnlicensedLPAuxRegistrationInfoWithReccurrenceDaily_RegistersUnlicensedLPAuxSuccessfully()
        {
            MWC.LicensedLpAuxRegistration unLicensedLpAuxRegistration = new MWC.LicensedLpAuxRegistration
            {
                UlsFileNumber = "0005122780",
                Channels = new[] { 6, 7 },
                IsRecurred = true,
                IsReoccurenceDaily = true,
                ReoccurenceInstance = 3,
                StartDate = DateTime.Today.ToString("MM-dd-yyyy"),
                EndDate = DateTime.Today.ToString("MM-dd-yyyy"),
                StartTime = DateTime.Now.ToString("HH:mm:ss"),
                EndTime = DateTime.Now.AddHours(2).ToString("HH:mm:ss"),
                Latitude = "40.7779169",
                Longitude = "-74.7922202",
                Name = "Rajendra Gola",
            };

            string actualResponse = this.whitespacesManager.RegisterUnLicensedLpAux(unLicensedLpAuxRegistration, accessToken);

            Assert.AreEqual("Device Registered successfully.", actualResponse);
        }

        [TestMethod]
        public void RegisterUnlicensedLpAux_ValidUnlicensedLPAuxRegistrationInfoWithReccurrenceWeekly_RegistersUnlicensedLPAuxSuccessfully()
        {
            MWC.LicensedLpAuxRegistration unLicensedLpAuxRegistration = new MWC.LicensedLpAuxRegistration
            {
                UlsFileNumber = "0005122780",
                Channels = new[] { 6, 7 },
                IsRecurred = true,
                IsReoccurenceWeekly = true,
                ReoccurrenceEndDate = DateTime.Now.AddDays(28).ToString("MM-dd-yyyy"),
                WeekDaysString = "Monday,Wednesday,Friday",
                StartDate = DateTime.Today.ToString("MM-dd-yyyy"),
                EndDate = DateTime.Today.ToString("MM-dd-yyyy"),
                StartTime = DateTime.Now.ToString("HH:mm:ss"),
                EndTime = DateTime.Now.AddHours(2).ToString("HH:mm:ss"),
                Latitude = "40.7779169",
                Longitude = "-74.7922202",
                Name = "Rajendra Gola",
            };

            string actualResponse = this.whitespacesManager.RegisterUnLicensedLpAux(unLicensedLpAuxRegistration, accessToken);

            Assert.AreEqual("Device Registered successfully.", actualResponse);
        }

        [TestMethod]
        public void RegisterTBasLinks_ValidTBasLinkRegistrationInfoWithAllMandatoryParameters_RegistersTBasLinksSuccessfully()
        {
            MWC.TBasLinkRegistration tBasLinkRegistration = new MWC.TBasLinkRegistration
            {
                CallSign = "WNVT",
                Channels = new[] { 8, 23, 17 },
                StartDate = DateTime.Today.ToString("MM-dd-yyyy"),
                EndDate = DateTime.Today.AddDays(10).ToString("MM-dd-yyyy"),
                StartTime = DateTime.Now.ToString("HH:mm:ss"),
                EndTime = DateTime.Now.AddHours(2).ToString("HH:mm:ss"),
                RecieverLatitude = "47.673988",
                RecieverLongitude = "-122.121512",
                TransmitterLatitude = "47.606209",
                TransmitterLongitude = "-122.332071",
                FriendlyName = "TBAS Registration"
            };

            string actualResponse = this.whitespacesManager.RegisterTBasLinks(tBasLinkRegistration, accessToken);

            Assert.AreEqual("Device Registered successfully.", actualResponse);
        }

        [TestMethod]
        public void ExcludeChannel_ValidInputParametersForUSRegion_ExcludesChannelSuccessfully()
        {
            Point[] locations = new Point[]
            {
                new Point
                {
                    Latitude = "47.606209",
                    Longitude = "-122.332071"
                }
            };

            int[] channels = new[] { 23, 45, 39 };

            string actualResponse = this.whitespacesManager.ExcludeChannel(channels, locations, accessToken, Constants.UnitedStatesRegion);

            Assert.AreEqual("Channel Excluded Successfully.", actualResponse);
        }

        [TestMethod]
        public void ExcludeDevice_ValidInputParametersForUSRegion_ExcludesDeviceSuccessfully()
        {
            string deviceId = Guid.NewGuid().ToString();

            string actualResponse = this.whitespacesManager.ExcludeDevice(accessToken, Constants.UnitedStatesRegion, deviceId);

            Assert.AreEqual("Excluded Id inserted successfully", actualResponse);
        }

        [TestMethod]
        public void GetChannelList_ValidInputParametersForMode_1DeviceType_ReturnsChannelInfoListSuccessfully()
        {
            ChannelInfo[] channelsInfo = this.whitespacesManager.GetChannelList(Constants.Mode1Type, 47.673988, -122.121512);

            Assert.IsTrue(channelsInfo.Length != 0);
        }

        [TestMethod]
        public void GetChannelList_ValidInputParametersForMode_2DeviceType_ReturnsChannelInfoListSuccessfully()
        {
            ChannelInfo[] channelsInfo = this.whitespacesManager.GetChannelList(Constants.Mode2Type, 47.673988, -122.121512);

            Assert.IsTrue(channelsInfo.Length != 0);
        }

        [TestMethod]
        public void GetChannelList_ValidInputParametersForFixedDeviceType_ReturnsChannelInfoListSuccessfully()
        {
            ChannelInfo[] channelsInfo = this.whitespacesManager.GetChannelList(Constants.FixedType, 47.673988, -122.121512);

            Assert.IsTrue(channelsInfo.Length != 0);
        }

        [TestMethod]
        public void GetChannelList_ValidInputParametersForLPAuxDeviceType_ReturnsChannelInfoListSuccessfully()
        {
            ChannelInfo[] channelsInfo = this.whitespacesManager.GetChannelList(Constants.LPAuxType, 47.673988, -122.121512);

            Assert.IsTrue(channelsInfo.Length != 0);
        }

        [TestMethod]
        public void GetChannelList_ValidInputParametersForUnlicensedLPAuxDeviceType_ReturnsChannelInfoListSuccessfully()
        {
            ChannelInfo[] channelsInfo = this.whitespacesManager.GetChannelList(Constants.UnlicensedLPAuxType, 47.673988, -122.121512);

            Assert.IsTrue(channelsInfo.Length != 0);
        }

        [TestMethod]
        public void GetChannelList_ValidInputParametersForTBASDeviceType_ReturnsChannelInfoListSuccessfully()
        {
            ChannelInfo[] channelsInfo = this.whitespacesManager.GetChannelList(Constants.TBASType, 47.673988, -122.121512);

            Assert.IsTrue(channelsInfo.Length != 0);
        }

        [TestMethod]
        public void GetIncumbents_ValidInputParametersForLPAuxIncumbentType_ReturnsIncumbentsOfLPAuxTypeSuccessfully()
        {
            List<MWC.Incumbent> incumbents = this.whitespacesManager.GetIncumbents(Constants.LPAuxType, Constants.UnitedStatesRegion, new int[] { 5, 6 });

            Assert.IsTrue(incumbents.Count != 0);
        }

        [TestMethod]
        public void GetIncumbents_ValidInputParametersForTBASIncumbentType_ReturnsIncumbentsOfTBASTypeSuccessfully()
        {
            List<MWC.Incumbent> incumbents = this.whitespacesManager.GetIncumbents(Constants.TBASType, Constants.UnitedStatesRegion, new int[] { 8 });

            Assert.IsTrue(incumbents.Count != 0);
        }

        [TestMethod]
        public void GetIncumbents_ValidInputParametersForUnlicensedLPAuxIncumbentType_ReturnsIncumbentsOfUnlicensedLPAuxTypeSuccessfully()
        {
            List<MWC.Incumbent> incumbents = this.whitespacesManager.GetIncumbents(Constants.UnlicensedLPAuxType, Constants.UnitedStatesRegion, new int[] { 5 });

            Assert.IsTrue(incumbents.Count != 0);
        }

        [TestMethod]
        public void GetNearByTvStations_ValidUSLocation_ReturnsNearByTVStations()
        {
            Location location = new Location
            {
                Latitude = 38.62,
                Longitude = -77.43
            };

            //MVPDCallSignsInfo[] mVPDCallSignsInfos = this.whitespacesManager.GetNearByTvStations(location);

            //Assert.IsTrue(mVPDCallSignsInfos.Length != 0);
        }

        [TestMethod]
        public void GetULSFileInfo_ValidULSFileNumber_ReturnsULSFileInfo()
        {
            string[] ulsFileInfoParameters = new[] { "GrantDate", "ExpireDate", "Latitude", "Longitude", "ContactEmail", "ContactZipCode", "ContactEntityName" };
            LpAuxLicenseInfo actualULSFileInfo = this.whitespacesManager.GetULSFileInfo("0005122780");

            LpAuxLicenseInfo expectedULSFileInfo = new LpAuxLicenseInfo
            {
                GrantDate = Convert.ToDateTime("4/2/2012 5:30:00 AM +05:30"),
                ExpireDate = Convert.ToDateTime("4/2/2015 5:30:00 AM +05:30"),
                Latitude = 39.99077777777778,
                Longitude = -76.76466666666667,
                ContactEmail = "melissa.hower@fcc.gov",
                ContactZipCode = "17325",
                ContactEntityName = "FCC Test 004"
            };

            CustomAssert.AreEqualByProperties(expectedULSFileInfo, actualULSFileInfo, ulsFileInfoParameters);
        }

        [TestMethod]
        public void GetDeviceList_ValidInputParametersForMode1DeviceTypeRequest_ReturnsMode1DeviceList()
        {
            ProtectedDevice[] deviceList = this.whitespacesManager.GetDeviceList(Constants.Mode1Type, 47.673988, -122.121512);

            Assert.IsTrue(deviceList.Length != 0);
        }

        [TestMethod]
        public void GetDeviceList_ValidInputParametersForMode2DeviceTypeRequest_ReturnsMode2DeviceList()
        {
            ProtectedDevice[] deviceList = this.whitespacesManager.GetDeviceList(Constants.Mode2Type, 47.673988, -122.121512);

            Assert.IsTrue(deviceList.Length != 0);
        }

        [TestMethod]
        public void GetDeviceList_ValidInputParametersForFixedDeviceTypeRequest_ReturnsFixedDeviceList()
        {
            ProtectedDevice[] deviceList = this.whitespacesManager.GetDeviceList(Constants.FixedType, 47.673988, -122.121512);

            Assert.IsTrue(deviceList.Length != 0);
        }
    }
}
