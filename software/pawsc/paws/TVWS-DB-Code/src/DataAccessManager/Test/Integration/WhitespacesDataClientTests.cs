// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace DataAccessManager.Test.Integration
{
    using System;
    using System.Collections.Generic;
    using System.Configuration;
    using System.Linq;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WhiteSpaces.Common;
    using Microsoft.WhiteSpaces.DataAccessManager;

    /// <summary>
    /// Integration Test cases for WhitespacesDataClient class methods
    /// </summary>
    [TestClass]
    public class WhitespacesDataClientTests
    {
        /// <summary>
        /// HttpClientManager reference variable.
        /// </summary>
        private readonly IHttpClientManager httpClientManager;

        /// <summary>
        /// WhitespacesDataClient reference variable.
        /// </summary>
        private readonly WhitespacesDataClient whitespacesDataClient;

        /// <summary>
        /// Initializes a new instance of the <see cref="WhitespacesDataClientTests" /> class.
        /// </summary>
        public WhitespacesDataClientTests()
        {
            this.httpClientManager = new HttpClientManager(ConfigurationManager.AppSettings["AuthorizationSchema"].ToString());
            this.whitespacesDataClient = new WhitespacesDataClient(this.httpClientManager);
        }

        /// <summary>
        /// Testing GetFreeChannels for valid Mode_1 (device type) request returns list of available channels
        /// </summary>
        [TestMethod]
        public void GetFreeChannels_ForValidMode_1DeviceRequest_ReturnsListOfAvailableChannels()
        {
            this.GetChannelListMethod("Mode_1");
        }

        /// <summary>
        /// Testing GetFreeChannels for valid Mode_2 (device type) request returns list of available channels
        /// </summary>
        [TestMethod]
        public void GetFreeChannels_ForValidMode_2DeviceRequest_ReturnsListOfAvailableChannels()
        {
            this.GetChannelListMethod("Mode_2");
        }

        /// <summary>
        /// Testing GetFreeChannels for valid Fixed (device type) request returns list of available channels
        /// </summary>
        [TestMethod]
        public void GetFreeChannels_ForValidFixedDeviceRequest_ReturnsListOfAvailableChannels()
        {
            this.GetChannelListMethod("Fixed");
        }

        /// <summary>
        /// Testing GetFreeChannels for valid LPAux (device type) request returns list of available channels
        /// </summary>
        [TestMethod]
        public void GetFreeChannels_ForValidLPAuxDeviceRequest_ReturnsListOfAvailableChannels()
        {
            this.GetChannelListMethod("LPAux");
        }

        /// <summary>
        /// Testing GetFreeChannels for valid UnlicensedLPAux (device type) request returns list of available channels
        /// </summary>
        [TestMethod]
        public void GetFreeChannels_ForValidUnlicensedLPAuxDeviceRequest_ReturnsListOfAvailableChannels()
        {
            this.GetChannelListMethod("UnlicensedLPAux");
        }

        /// <summary>
        /// Testing GetFreeChannels for valid TBAS (device type) request returns list of available channels
        /// </summary>
        [TestMethod]
        public void GetFreeChannels_ForValidTBASDeviceRequest_ReturnsListOfAvailableChannels()
        {
            this.GetChannelListMethod("TBAS");
        }

        /// <summary>
        /// Testing AddIncumbent for valid MVPD (incumbent type) request registers MVPD incumbent successfully
        /// </summary>
        [TestMethod]
        public void AddIncumbent_ForValidMVPDRequest_RegistersMVPDDeviceSuccessfully()
        {
            IRequestParameters requestParams = this.AddIncumbentRequestParamsForMVPDDeviceType();

            var actualResponse = this.whitespacesDataClient.AddIncumbent(requestParams);

            Assert.AreEqual("Device Registered successfully.", actualResponse.Message);
        }

        /// <summary>
        /// Testing AddIncumbent for valid TBAS (incumbent type) request registers TBAS incumbent successfully
        /// </summary>
        [TestMethod]
        public void AddIncumbent_ForValidTBASRequest_RegistersTBASDeviceSuccessfully()
        {
            IRequestParameters requestParams = this.AddIncumbentRequestParamsForTBASDeviceType();

            var actualResponse = this.whitespacesDataClient.AddIncumbent(requestParams);

            Assert.AreEqual("Device Registered successfully.", actualResponse.Message);
        }

        /// <summary>
        /// Testing AddIncumbent for valid UnlicensedLPAux (incumbent type) request registers UnlicensedLPAux incumbent successfully
        /// </summary>
        [TestMethod]
        public void AddIncumbent_ForValidUnlicensedLPAuxRequest_RegistersUnlicensedLPAuxDeviceSuccessfully()
        {
            IRequestParameters requestParams = this.AddIncumbentRequestParamsForUnlicensedLPAuxDeviceType();

            var actualResponse = this.whitespacesDataClient.AddIncumbent(requestParams);

            Assert.AreEqual("Device Registered successfully.", actualResponse.Message);
        }

        /// <summary>
        /// Testing AddIncumbent for valid LPAux (incumbent type) request registers LPAux incumbent successfully
        /// </summary>
        [TestMethod]
        public void AddIncumbent_ForValidLPAuxRequest_RegistersLPAuxDeviceSuccessfully()
        {
            IRequestParameters requestParams = this.AddIncumbentRequestParamsForLPAuxDeviceType();

            var actualResponse = this.whitespacesDataClient.AddIncumbent(requestParams);

            Assert.AreEqual("Device Registered successfully.", actualResponse.Message);
        }

        /// <summary>
        /// Testing GetIncumbent for valid MVPD (incumbent type) request returns MVPD incumbents successfully
        /// </summary>
        [TestMethod]
        public void GetIncumbent_ValidMVPDIncumbentRequest_ReturnsMVPDIncumbents()
        {
            IRequestParameters requestParams = this.GetIncumbentRequestParams("MVPD");

            var actualResponse = this.whitespacesDataClient.GetIncumbents(requestParams);

            Assert.IsTrue(actualResponse.IncumbentList.Count() > 0);
        }

        /// <summary>
        /// Testing GetIncumbent for valid LPAux (incumbent type) request returns LPAux incumbents successfully
        /// </summary>
        [TestMethod]
        public void GetIncumbent_ValidLPAuxIncumbentRequest_ReturnsLPAuxIncumbents()
        {
            IRequestParameters requestParams = this.GetIncumbentRequestParams("LPAux");

            var actualResponse = this.whitespacesDataClient.GetIncumbents(requestParams);

            Assert.IsTrue(actualResponse.IncumbentList.Count() > 0);
        }

        /// <summary>
        /// Testing GetIncumbent for valid UnlicensedLPAux (incumbent type) request returns UnlicensedLPAux incumbents successfully
        /// </summary>
        [TestMethod]
        public void GetIncumbent_ValidUnlicensedLPAuxIncumbentRequest_ReturnsUnlicensedLPAuxIncumbents()
        {
            IRequestParameters requestParams = this.GetIncumbentRequestParams("UnlicensedLPAux");

            var actualResponse = this.whitespacesDataClient.GetIncumbents(requestParams);

            Assert.IsTrue(actualResponse.IncumbentList.Count() > 0);
        }

        /// <summary>
        /// Testing GetIncumbent for valid TBAS (incumbent type) request returns TBAS incumbents successfully
        /// </summary>
        [TestMethod]
        public void GetIncumbent_ValidTBASIncumbentRequest_ReturnsTBASIncumbents()
        {
            IRequestParameters requestParams = this.GetIncumbentRequestParams("TBAS");

            var actualResponse = this.whitespacesDataClient.GetIncumbents(requestParams);

            Assert.IsTrue(actualResponse.IncumbentList.Count() > 0);
        }

        /// <summary>
        /// Testing DeleteIncumbent for valid MVPD (incumbent type) request deletes MVPD incumbent successfully
        /// </summary>
        [TestMethod]
        public void DeleteIncumbent_ValidMVPDRequest_DeletesMVPDIncumbentSuccesfully()
        {
            IRequestParameters addIncumbentRequestParams = this.AddIncumbentRequestParamsForMVPDDeviceType();
            this.whitespacesDataClient.AddIncumbent(addIncumbentRequestParams);

            IRequestParameters getIncumbentRequestParams = this.GetIncumbentRequestParams("MVPD");

            var getIncumbentsResponse = this.whitespacesDataClient.GetIncumbents(getIncumbentRequestParams);
            var mvpds = getIncumbentsResponse.IncumbentList.Select(obj => JsonSerialization.DeserializeString<MVPDRegistration>(obj.ToString())).ToList();

            ////Get RegistrationId of the added incumbent
            var requiredMvpd = mvpds.Where(x => x.Latitude == addIncumbentRequestParams.Params.MVPDLocation.Latitude && x.Longitude == addIncumbentRequestParams.Params.MVPDLocation.Longitude).FirstOrDefault();

            IRequestParameters deleteIncumbentRequestParams = this.GetRequestParams(
                req =>
                {
                    req.RegionName = "United States";                   
                    req.Params = new Parameters
                    {
                        IncumbentType = "MVPD",
                        RegistrationDisposition = new RegistrationDisposition
                        {
                            RegId = requiredMvpd.Disposition.RegId
                        }
                    };
                });

            var actualResponse = this.whitespacesDataClient.DeleteIncumbent(deleteIncumbentRequestParams);

            Assert.AreEqual("Incumbent deleted successfully.", actualResponse.Message);
        }

        /// <summary>
        /// Testing DeleteIncumbent for valid LPAux (incumbent type) request deletes LPAux incumbent successfully
        /// </summary>
        [TestMethod]
        public void DeleteIncumbent_ValidLPAuxRequest_DeletesLPAuxIncumbentSuccesfully()
        {
            IRequestParameters addIncumbentRequestParams = this.AddIncumbentRequestParamsForLPAuxDeviceType();
            this.whitespacesDataClient.AddIncumbent(addIncumbentRequestParams);

            IRequestParameters getIncumbentRequestParams = this.GetIncumbentRequestParams("LPAux");

            var getIncumbentsResponse = this.whitespacesDataClient.GetIncumbents(getIncumbentRequestParams);

            var lowPowerAuxs = getIncumbentsResponse.IncumbentList.Select(obj => JsonSerialization.DeserializeString<LPAuxRegistration>(obj.ToString())).ToList();

            ////Delete MVPD Incumbents
            ////Get RegistrationId of the added incumbent
            var requiredLpAux = lowPowerAuxs.Where(x => x.PointsArea[0].Latitude.ToString() == addIncumbentRequestParams.Params.PointsArea[0].Latitude && x.PointsArea[0].Longitude.ToString() == addIncumbentRequestParams.Params.PointsArea[0].Longitude).FirstOrDefault();

            IRequestParameters deleteIncumbentRequestParams = this.GetRequestParams(
                req =>
                {
                    req.RegionName = "United States";
                    req.Params = new Parameters
                    {
                        IncumbentType = "LPAux",
                        RegistrationDisposition = new RegistrationDisposition
                        {
                            RegId = requiredLpAux.Disposition.RegId
                        }
                    };
                });

            var actualResponse = this.whitespacesDataClient.DeleteIncumbent(deleteIncumbentRequestParams);

            Assert.AreEqual("Incumbent deleted successfully.", actualResponse.Message);
        }

        /// <summary>
        /// Testing DeleteIncumbent for valid UnlicensedLPAux (incumbent type) request deletes UnlicensedLPAux incumbent successfully
        /// </summary>
        [TestMethod]
        public void DeleteIncumbent_ValidUnlicensedLPAuxRequest_DeletesUnlicensedLPAuxIncumbentSuccesfully()
        {
            IRequestParameters addIncumbentRequestParams = this.AddIncumbentRequestParamsForUnlicensedLPAuxDeviceType();
            this.whitespacesDataClient.AddIncumbent(addIncumbentRequestParams);

            IRequestParameters getIncumbentRequestParams = this.GetIncumbentRequestParams("UnlicensedLPAux");

            var getIncumbentsResponse = this.whitespacesDataClient.GetIncumbents(getIncumbentRequestParams);

            var unlicensedLowPowerAuxs = getIncumbentsResponse.IncumbentList.Select(obj => JsonSerialization.DeserializeString<LPAuxRegistration>(obj.ToString())).ToList();

            ////Delete MVPD Incumbents
            ////Get RegistrationId of the added incumbent
            var requiredUunlicensedLpAux = unlicensedLowPowerAuxs.Where(x => x.PointsArea[0].Latitude.ToString() == addIncumbentRequestParams.Params.PointsArea[0].Latitude && x.PointsArea[0].Longitude.ToString() == addIncumbentRequestParams.Params.PointsArea[0].Longitude).FirstOrDefault();

            IRequestParameters deleteIncumbentRequestParams = this.GetRequestParams(
                req =>
                {
                    req.RegionName = "United States";
                    req.Params = new Parameters
                    {
                        IncumbentType = "UnlicensedLPAux",
                        RegistrationDisposition = new RegistrationDisposition
                        {
                            RegId = requiredUunlicensedLpAux.Disposition.RegId
                        }
                    };
                });

            var actualResponse = this.whitespacesDataClient.DeleteIncumbent(deleteIncumbentRequestParams);

            Assert.AreEqual("Incumbent deleted successfully.", actualResponse.Message);
        }

        /// <summary>
        /// Testing DeleteIncumbent for valid TBAS (incumbent type) request deletes TBAS incumbent successfully
        /// </summary>
        [TestMethod]
        public void DeleteIncumbent_ValidTBASRequest_DeletesTBASIncumbentSuccesfully()
        {
            IRequestParameters addIncumbentRequestParams = this.AddIncumbentRequestParamsForTBASDeviceType();
            this.whitespacesDataClient.AddIncumbent(addIncumbentRequestParams);

            IRequestParameters getIncumbentRequestParams = this.GetIncumbentRequestParams("TBAS");

            var getIncumbentsResponse = this.whitespacesDataClient.GetIncumbents(getIncumbentRequestParams);

            var tempBasCollection = getIncumbentsResponse.IncumbentList.Select(obj => JsonSerialization.DeserializeString<TempBASRegistration>(obj.ToString())).ToList();

            ////Delete MVPD Incumbents
            ////Get RegistrationId of the added incumbent
            var requiredTempBas = tempBasCollection.Where(x => x.TxLatitude == addIncumbentRequestParams.Params.TransmitLocation.Latitude && x.TxLongitude == addIncumbentRequestParams.Params.TransmitLocation.Longitude).FirstOrDefault();

            IRequestParameters deleteIncumbentRequestParams = this.GetRequestParams(
                req =>
                {
                    req.RegionName = "United States";
                    req.Params = new Parameters
                    {
                        IncumbentType = "TBAS",
                        RegistrationDisposition = new RegistrationDisposition
                        {
                            RegId = requiredTempBas.Disposition.RegId
                        }
                    };
                });

            var actualResponse = this.whitespacesDataClient.DeleteIncumbent(deleteIncumbentRequestParams);

            Assert.AreEqual("Incumbent deleted successfully.", actualResponse.Message);
        }

        /// <summary>
        /// Testing GetDeviceList for valid Fixed (device type) request returns Fixed device type list
        /// </summary>
        [TestMethod]
        public void GetDeviceList_FixedType_ReturnsDeviceList()
        {
            this.GetDeviceListMethod("Fixed");
        }

        /// <summary>
        /// Testing GetDeviceList for valid Mode1 (device type) request returns Mode1 device type list
        /// </summary>
        [TestMethod]
        public void GetDeviceList_Mode1Type_ReturnsDeviceList()
        {
            this.GetDeviceListMethod("Mode_1");
        }

        /// <summary>
        /// Testing GetDeviceList for valid Mode2 (device type) request returns Mode2 device type list
        /// </summary>
        [TestMethod]
        public void GetDeviceList_Mode2Type_ReturnsDeviceList()
        {
            this.GetDeviceListMethod("Mode_2");
        }

        /// <summary>
        /// Testing ExcludeChannel for valid request excludes channel successfully
        /// </summary>
        [TestMethod]
        public void ExcludeChannel_ValidRequest_ExcludesChannelSuccessfully()
        {
            IRequestParameters requestParams = this.GetRequestParams(
                req =>
                {
                    req.RegionName = "United States";
                    req.Params = new Parameters
                    {
                        IncumbentType = "MVPD",
                        TvSpectra = new TvSpectrum[]
                        { 
                            new TvSpectrum
                            {
                                Channel = 23,
                                CallSign = "DTV"
                            }
                        },

                        Locations = new GeoLocation[] 
                        {
                            new GeoLocation
                            {
                                Point = new Ellipse
                                {
                                    Center = new Point
                                    {
                                        Latitude = "3",
                                        Longitude = "-1.3"
                                    }
                                }
                            }
                        }
                    };
                });

            var actualResponse = this.whitespacesDataClient.ExcludeChannel(requestParams);

            Assert.AreEqual("Channel Excluded Successfully.", actualResponse.Message);
        }

        /// <summary>
        /// Testing ExcludeIds for valid request excludes device id successfully
        /// </summary>
        [TestMethod]
        public void ExcludeIds_ValidRequest_ExcludeDeviceSuccessfully()
        {
            IRequestParameters requestParams = this.GetRequestParams(
                req =>
                {
                    req.RegionName = "United States";
                    req.Params = new Parameters
                    {
                        DeviceId = Guid.NewGuid().ToString()
                    };
                });

            var actualResponse = this.whitespacesDataClient.ExcludeIds(requestParams);

            Assert.AreEqual("Excluded Id inserted successfully", actualResponse.Message);
        }

        /// <summary>
        /// Testing GetNearByTVStations for valid request returns near by TV stations
        /// </summary>
        [TestMethod]
        public void GetNearByTvStations_ValidRequest_ReturnsNearestTVStations()
        {
            IRequestParameters requestParams = this.GetRequestParams(
                req =>
                {
                    req.RegionName = "United States";
                    req.Params = new Parameters
                    {
                        MVPDLocation = new Location
                        {
                            Latitude = 38.62,
                            Longitude = -77.43
                        }
                    };
                });

            var nearestTVStationsResponse = this.whitespacesDataClient.GetNearByTvStations(requestParams);

            Assert.IsTrue(nearestTVStationsResponse.SearchMVPDCallSigns.Count() > 0);
        }

        /// <summary>
        /// Testing GetAllULSFileNumbers
        /// </summary>
        [TestMethod]
        public void GetAllUlsFileNumbers_ValidRegionName_ReturnsAllUlsFileNumbers()
        {
            var ulsFileNumbers = this.whitespacesDataClient.GetAllUlsFileNumbers("United States");
            Assert.IsTrue(ulsFileNumbers.LpAuxLicenses.Count() > 0);
        }

        /// <summary>
        /// Testing GetAllULSCallSigns
        /// </summary>
        [TestMethod]
        public void GetAllUlsCallSigns_ValidRegionName_ReturnsAllUlsCallSigns()
        {
            var ulsCallSigns = this.whitespacesDataClient.GetAllUlsCallSigns("United States");
            Assert.IsTrue(ulsCallSigns.LpAuxLicenses.Count() > 0);
        }

        /// <summary>
        /// Initialize the request parameters and return it.
        /// </summary>
        /// <param name="parametersInitializer">Parameter which need to be initialize</param>
        /// <returns>Returning RequestParameters</returns>
        private RequestParameters GetRequestParams(Action<RequestParameters> parametersInitializer)
        {
            RequestParameters requestparameters = new RequestParameters();
            parametersInitializer(requestparameters);

            return requestparameters;
        }

        /// <summary>
        /// This method Return the response 
        /// </summary>
        /// <param name="responseInitializer">Response which we are initializing</param>
        /// <returns>Returning RegionManagementResponse</returns>
        private RegionManagementResponse GetResponse(Action<RegionManagementResponse> responseInitializer)
        {
            RegionManagementResponse response = new RegionManagementResponse();
            responseInitializer(response);

            return response;
        }

        /// <summary>
        /// Request Parameters for LPAux incumbent type
        /// </summary>
        /// <returns>request parameters</returns>
        private IRequestParameters AddIncumbentRequestParamsForLPAuxDeviceType()
        {
            return this.GetRequestParams(
              req =>
              {                  
                  req.RegionName = "United States";
                  req.Params = new Parameters
                  {
                      IncumbentType = "LPAux",
                      LPAuxRegistrant = new Microsoft.Whitespace.Entities.Versitcard.VCard
                      {
                          Org = new Microsoft.Whitespace.Entities.Versitcard.Organization
                          {
                              OrganizationName = "Contoso"
                          },
                          Name = new Microsoft.Whitespace.Entities.Versitcard.Name
                          {
                              ContactName = "Test Name"
                          },
                          Telephone = new Microsoft.Whitespace.Entities.Versitcard.Telephone[]
                        {
                             new Microsoft.Whitespace.Entities.Versitcard.Telephone
                             {
                                 TelephoneNumber = "123456"
                             },
                             new Microsoft.Whitespace.Entities.Versitcard.Telephone
                             {
                                 TelephoneNumber = "456435134"
                             },
                        },
                          Address = new Microsoft.Whitespace.Entities.Versitcard.Address
                          {
                              POBox = "1234",
                              Street = "new street",
                              Locality = "Friendship Heights",
                              Region = "1",
                              Code = "wa98008",
                              Country = "India"
                          }
                      },
                      Venue = "Test Venue",
                      PointsArea = new Point[]
                    {
                        new Point
                        {
                            Latitude = "40.7779169",
                            Longitude = "-74.7922202"
                        }
                    },
                      TvSpectrum = new TvSpectrum
                      {
                          Channel = 24,
                          CallSign = "BLN00751"
                      },
                      Event = new Event
                      {
                          Times = new Calendar[]
                        {
                            new Calendar
                            {
                                Start = "2014-01-14T14:00:00.00Z",
                                Stamp = "2014-01-20T14:00:00.00Z",
                                End = "2014-01-20T14:00:00.00Z",
                                UId = "7697c884-573e-4bd3-b676-0392c07c955c"
                            }
                        },
                          Channels = new int[] { 10, 11 }
                      },
                  };
              });
        }

        /// <summary>
        /// Request Parameters for MVPD incumbent type
        /// </summary>
        /// <returns>request parameters</returns>
        private IRequestParameters AddIncumbentRequestParamsForMVPDDeviceType()
        {
            return this.GetRequestParams(
              req =>
              {
                  req.RegionName = "United States";
                  req.Params = new Parameters
                  {
                      IncumbentType = "MVPD",
                      Contact = new Microsoft.Whitespace.Entities.Versitcard.VCard
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
                                    EmailAddress = "test.name@contoso.com"
                                }
                            }
                      },
                      MVPDRegistrant = new Microsoft.Whitespace.Entities.Versitcard.VCard
                      {
                          Org = new Microsoft.Whitespace.Entities.Versitcard.Organization
                          {
                              OrganizationName = "Contoso"
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
                      MVPDLocation = new Location
                      {
                          Latitude = 38.66484069824219,
                          Longitude = -77.43494415283203
                      },
                      TransmitLocation = new Location
                      {
                          Latitude = 39.3362177768783,
                          Longitude = -76.6494136354878
                      },
                      TvSpectrum = new TvSpectrum
                      {
                          Channel = 46,
                          CallSign = "WBFF"
                      }
                  };
              });
        }

        /// <summary>
        /// Request Parameters for TBAS incumbent type
        /// </summary>
        /// <returns>request parameters</returns>
        private IRequestParameters AddIncumbentRequestParamsForTBASDeviceType()
        {
            return this.GetRequestParams(
              req =>
              {                  
                  req.RegionName = "United States";
                  req.Params = new Parameters
                  {
                      IncumbentType = "TBAS",
                      TempBASRegistrant = new Microsoft.Whitespace.Entities.Versitcard.VCard
                      {
                          Org = new Microsoft.Whitespace.Entities.Versitcard.Organization
                          {
                              OrganizationName = "Contoso"
                          },
                          Name = new Microsoft.Whitespace.Entities.Versitcard.Name
                          {
                              ContactName = "Test Name"
                          },
                          Telephone = new Microsoft.Whitespace.Entities.Versitcard.Telephone[]
                            {
                                new Microsoft.Whitespace.Entities.Versitcard.Telephone
                                {
                                    TelephoneNumber = "123456"
                                },
                                new Microsoft.Whitespace.Entities.Versitcard.Telephone
                                {
                                    TelephoneNumber = "456435134"
                                },
                            }
                      },
                      Contact = new Microsoft.Whitespace.Entities.Versitcard.VCard
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
                                    EmailAddress = "test.name@contoso.com"
                                }
                            }
                      },
                      TvSpectrum = new TvSpectrum
                      {
                          Channel = 34,
                          CallSign = "WNVT"
                      },
                      Event = new Event
                      {
                          Times = new Microsoft.Whitespace.Entities.Calendar[] 
                            {
                                new Microsoft.Whitespace.Entities.Calendar
                                {
                                    Stamp = "2013-12-30T04:50:00.500Z",
                                    Start = "2014-12-02T04:50:00.500Z",
                                    End = "2014-12-30T04:50:00.500Z",
                                    UId = "11221111ABCD339"
                                }
                            },
                          Channels = new int[] 
                            {
                                10,
                                11
                            }
                      },
                      TempBasLocation = new Location
                      {
                          Latitude = 83,
                          Longitude = 33,
                          Datum = "NAD-83",
                      },
                      TransmitLocation = new Location
                      {
                          Latitude = 83,
                          Longitude = 34
                      }
                  };
              });
        }

        /// <summary>
        /// Request Parameters for UnlicensedLPAux incumbent type
        /// </summary>
        /// <returns>request parameters</returns>
        private IRequestParameters AddIncumbentRequestParamsForUnlicensedLPAuxDeviceType()
        {
            return this.GetRequestParams(
              req =>
              {                 
                  req.RegionName = "United States";
                  req.Params = new Parameters
                  {
                      IncumbentType = "UnlicensedLPAux",
                      Contact = new Microsoft.Whitespace.Entities.Versitcard.VCard
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
                                    EmailAddress = "test.name@contoso.com"
                                }
                            }
                      },
                      LPAuxRegistrant = new Microsoft.Whitespace.Entities.Versitcard.VCard
                      {
                          Org = new Microsoft.Whitespace.Entities.Versitcard.Organization
                          {
                              OrganizationName = "Contoso"
                          },
                          Name = new Microsoft.Whitespace.Entities.Versitcard.Name
                          {
                              ContactName = "Test Name"
                          },
                          Telephone = new Microsoft.Whitespace.Entities.Versitcard.Telephone[]
                            {
                                new Microsoft.Whitespace.Entities.Versitcard.Telephone
                                {
                                    TelephoneNumber = "123456"
                                },
                                new Microsoft.Whitespace.Entities.Versitcard.Telephone
                                {
                                    TelephoneNumber = "456435134"
                                },
                            }
                      },
                      Venue = "FCC Test 004",
                      ULSFileNumber = "0005122780",
                      TvSpectrum = new TvSpectrum
                      {
                          Channel = 24,
                          CallSign = "WNVT"
                      },
                      Event = new Event
                      {
                          Times = new Microsoft.Whitespace.Entities.Calendar[] 
                            {
                                new Microsoft.Whitespace.Entities.Calendar
                                {
                                    Stamp = "2013-12-30T04:50:00.500Z",
                                    Start = "2014-12-02T04:50:00.500Z",
                                    End = "2014-12-30T04:50:00.500Z",
                                    UId = "11221111ABCD339"
                                }
                            },
                          Channels = new int[] 
                            {
                                45
                            }
                      },
                      PointsArea = new Point[] 
                        {
                            new Point
                            {
                                Latitude = "40.7779169",
                                Longitude = "-74.7922202"
                            }
                        }
                  };
              });
        }

        /// <summary>
        /// This method returns get incumbent request parameters
        /// </summary>
        /// <param name="incumbentType">incumbent type</param>
        /// <returns>get incumbent request parameters</returns>
        private IRequestParameters GetIncumbentRequestParams(string incumbentType)
        {
            return this.GetRequestParams(
              req =>
              {
                  req.RegionName = "United States";
                  req.Params = new Parameters
                  {
                      IncumbentType = incumbentType,
                  };
              });
        }

        /// <summary>
        /// GetChannelList helper method
        /// </summary>
        /// <param name="incumbentType">incumbent type</param>
        private void GetChannelListMethod(string incumbentType)
        {
            IRequestParameters requestParams = null;
            if (string.Compare(incumbentType, "LPAux", true) == 0 || (string.Compare(incumbentType, "UnlicensedLPAux", true) == 0))
            {
                requestParams = this.GetRequestParams(
           req =>
           {
               req.RegionName = "United States";
               req.Params = new Parameters
               {
                   IncumbentType = incumbentType,
                   Location = new GeoLocation
                   {
                       Point = new Ellipse
                       {
                           Center = new Point
                           {
                               Latitude = "29.80",
                               Longitude = "-94.23"
                           }
                       }
                   },
                   PointsArea = new Point[]
                        {
                            new Point
                            {
                                Latitude = "53",
                                Longitude = "-103"
                            },

                            new Point
                            {
                                Latitude = "35",
                                Longitude = "-109"
                            }
                        }
               };
           });
            }
            else
            {
                requestParams = this.GetRequestParams(
            req =>
            {               
                req.RegionName = "United States";
                req.Params = new Parameters
                {
                    IncumbentType = incumbentType,
                    Location = new GeoLocation
                    {
                        Point = new Ellipse
                        {
                            Center = new Point
                            {
                                Latitude = "29.80",
                                Longitude = "-94.23"
                            }
                        }
                    }
                };
            });
            }

            var actualResponse = this.whitespacesDataClient.GetChannelList(requestParams);

            Assert.IsTrue(actualResponse.ChannelInfo.Count() != 0);
        }

        /// <summary>
        /// GetDeviceListMethod helper method
        /// </summary>
        /// <param name="deviceType">device type</param>
        private void GetDeviceListMethod(string deviceType)
        {
            IRequestParameters requestParams = this.GetRequestParams(
                req =>
                {
                    req.RegionName = "United States";
                    req.Params = new Parameters
                    {
                        IncumbentType = deviceType,
                        Location = new GeoLocation
                        {
                            Point = new Ellipse
                            {
                                Center = new Point
                                {
                                    Latitude = "29.80",
                                    Longitude = "-94.23"
                                }
                            }
                        }
                    };
                });

            var actualResponse = this.whitespacesDataClient.GetDevices(requestParams);

            Assert.IsTrue(actualResponse.DeviceList.Count() > 0);
        }
    }
}
