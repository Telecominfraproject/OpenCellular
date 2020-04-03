// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace DataAccessManager.Test.Unit
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WhiteSpaces.Common;
    using Microsoft.WhiteSpaces.DataAccessManager;
    using Microsoft.WhiteSpaces.DataAccessManager.Fakes;

    /// <summary>
    /// Unit Test cases for WhitespacesDataClient class methods
    /// </summary>
    [TestClass]
    public class WhitespacesDataClientTests
    {
        /// <summary>
        /// StubIHttpClientManager reference variable.
        /// </summary>
        private readonly StubIHttpClientManager httpClientManager;

        /// <summary>
        /// WhitespacesDataClient reference variable.
        /// </summary>
        private readonly WhitespacesDataClient whitespacesDataClient;

        /// <summary>
        /// Initializes a new instance of the <see cref="WhitespacesDataClientTests" /> class.
        /// </summary>
        public WhitespacesDataClientTests()
        {
            this.httpClientManager = new StubIHttpClientManager();
            this.whitespacesDataClient = new WhitespacesDataClient(this.httpClientManager);
        }

        /// <summary>
        /// Test method to verify all free channel.
        /// </summary>
        [TestMethod]
        public void TestGetChannelList()
        {
            IRequestParameters requestParams = this.GetRequestParams(
            req =>
            {
                req.RegionName = "United States";
                req.Params = new Parameters
                {
                    IncumbentType = "MVPD",
                    Location = new GeoLocation
                    {
                        Point = new Ellipse
                        {
                            Center = new Point
                            {
                                Latitude = "40.5",
                                Longitude = "-74"
                            }
                        }
                    }
                };
            });

            RegionManagementResponse expectedResponse = this.GetResponse(
            response =>
            {
                response.Result = new Result
                {
                    ChannelInfo = new ChannelInfo[]
                        {
                            new ChannelInfo
                            {
                                    ChannelId = 2,
                                    DeviceType = "Fixed"
                            },

                            new ChannelInfo
                            {
                                    ChannelId = 3,
                                    DeviceType = "Fixed"
                            }
                        }
                };
            });

            this.httpClientManager.PostOf1RequestStringString<RegionManagementResponse>((request, regionName, accessToken) => expectedResponse);

            var actualResponse = this.whitespacesDataClient.GetChannelList(requestParams);

            Assert.AreEqual(expectedResponse.Result.ChannelInfo.Count(), actualResponse.ChannelInfo.Count());
        }

        /// <summary>
        /// Testing GetFreeChannels throwing ArgumentNullException if request does not contain RegionName
        /// </summary>
        [TestMethod]
        [ExpectedException(typeof(ArgumentNullException))]
        public void GetFreeChannels_WithoutRegionName_ArgumentNullException()
        {
            IRequestParameters requestParams = this.GetRequestParams(
                req =>
                {
                    req.Params = new Parameters
                    {
                        IncumbentType = "Fixed",
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

            this.whitespacesDataClient.GetChannelList(requestParams);
        }

        /// <summary>
        /// Test method to verify adding incumbent.
        /// </summary>
        [TestMethod]
        public void AddIncumbentTest()
        {
            IRequestParameters requestParams = this.GetRequestParams(
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
                            OrganizationName = "Constso"
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
                        Latitude = 3,
                        Longitude = 3
                    },
                    TransmitLocation = new Location
                    {
                        Latitude = 83,
                        Longitude = 33
                    },
                    TvSpectrum = new TvSpectrum
                    {
                        Channel = 24,
                        CallSign = "WNVT"
                    },
                };
            });

            var expectedResponse = this.GetResponse(
            response =>
            {
                response.Result = new Result
                {
                    Message = "Incumbent added successfully."
                };
            });

            this.httpClientManager.PostOf1RequestStringString<RegionManagementResponse>((request, regionName, accessToken) => expectedResponse);

           var actualResponse = this.whitespacesDataClient.AddIncumbent(requestParams);

            Assert.AreEqual(expectedResponse.Result.Message, actualResponse.Message);
        }

        /// <summary>
        /// Testing AddIncumbent throwing ArgumentNullException if request does not contain RegionName
        /// </summary>
        [TestMethod]
        [ExpectedException(typeof(ArgumentNullException))]
        public void AddIncumbent_WithoutRegionName_ArgumentNullException()
        {
            IRequestParameters requestParams = this.GetRequestParams(
                req =>
                {
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
                                OrganizationName = "Constso"
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
                            Latitude = 47.67858,
                            Longitude = -122.13157
                        },
                        TransmitLocation = new Location
                        {
                            Latitude = 47.61537,
                            Longitude = -122.30929
                        },
                        TvSpectrum = new TvSpectrum
                        {
                            Channel = 24,
                            CallSign = "WNVT"
                        }
                    };
                });

            this.whitespacesDataClient.AddIncumbent(requestParams);
        }

        /// <summary>
        /// Test method to verify getting incumbent based on incumbent type.
        /// </summary>
        [TestMethod]
        public void TestGetIncumbents()
        {
            IRequestParameters requestParams = this.GetRequestParams(
                req =>
                {
                    req.RegionName = "United States";
                    req.AccessToken = "Whitespace001";
                    req.Params = new Parameters
                    {
                        IncumbentType = "MPVD",
                        Registrant = new Vcard
                        {
                            Organization = new Organization
                            {
                                Text = "FCC"
                            }
                        },
                        MVPDLocation = new Location
                        {
                            Latitude = 3,
                            Longitude = 3
                        },
                        TvSpectrum = new TvSpectrum
                        {
                            CallSign = "DTV",
                            Channel = 24
                        }
                    };
                });

            RegionManagementResponse expectedResponse = this.GetResponse(
                response =>
                {
                    response.Result = new Result
                    {
                        IncumbentList = new object[]
                        {
                            new Microsoft.Whitespace.Entities.Incumbent
                            {
                                AntennaId = 1,
                                AntennaType = "SNG-B",
                                IncumbentType = IncumbentType.MVPD
                            },

                            new Microsoft.Whitespace.Entities.Incumbent
                            {
                                AntennaId = 2,
                                AntennaType = "SNG-B",
                                IncumbentType = IncumbentType.MVPD
                            }
                        }
                    };
                });

            this.httpClientManager.PostOf1RequestStringString<RegionManagementResponse>((request, regionName, accessToken) => expectedResponse);

            var actualResponse = this.whitespacesDataClient.GetIncumbents(requestParams);

            Assert.AreEqual(expectedResponse.Result.IncumbentList.Count(), actualResponse.IncumbentList.Count());
        }

        /// <summary>
        /// Testing GetIncumbent throwing ArgumentNullException if request does not contain RegionName
        /// </summary>
        [TestMethod]
        [ExpectedException(typeof(ArgumentNullException))]
        public void GetIncumbent_WithoutRegionName_ArgumentNullException()
        {
            IRequestParameters requestParams = this.GetRequestParams(
                req =>
                {
                    req.Params = new Parameters
                    {
                        IncumbentType = "MVPD"
                    };
                });

            this.whitespacesDataClient.GetIncumbents(requestParams);
        }

        /// <summary>
        /// Test method to verify incumbent deletion.
        /// </summary>
        [TestMethod]
        public void TestDeleteIncumbent()
        {
            IRequestParameters requestParams = this.GetRequestParams(
                req =>
                {
                    req.RegionName = "United States";
                    req.AccessToken = "abcd";
                    req.Params = new Parameters
                    {
                        IncumbentType = "MVPD",
                        Registrant = new Vcard
                        {
                            Organization = new Organization
                            {
                                Text = "abcd"
                            }
                        },
                        MVPDLocation = new Location
                        {
                            Latitude = 3,
                            Longitude = 3
                        },
                        TvSpectrum = new TvSpectrum
                        {
                            CallSign = "DTV",
                            Channel = 24
                        }
                    };
                });

            var expectedResponse = this.GetResponse(
            response =>
            {
                response.Result = new Result
                {
                    Message = "Incumbent Deleted successfully."
                };
            });

            this.httpClientManager.PostOf1RequestStringString<RegionManagementResponse>((request, regionName, accessToken) => expectedResponse);

            var actualResponse = this.whitespacesDataClient.DeleteIncumbent(requestParams);

            Assert.AreEqual(expectedResponse.Result.Message, actualResponse.Message);
        }

        /// <summary>
        /// Testing DeleteIncumbent throwing ArgumentNullException if request does not contain RegionName
        /// </summary>
        [TestMethod]
        [ExpectedException(typeof(ArgumentNullException))]
        public void DeleteIncumbent_WithoutRegionName_ArgumentNullException()
        {
            IRequestParameters requestParams = this.GetRequestParams(
                req =>
                {
                    req.Params = new Parameters
                    {
                        IncumbentType = "MVPD",
                        RegistrationDisposition = new RegistrationDisposition
                        {
                            RegId = "120824SPBR0000001"
                        }
                    };
                });

            this.whitespacesDataClient.DeleteIncumbent(requestParams);
        }

        /// <summary>
        /// Test method to verify getting all devices.
        /// </summary>
        [TestMethod]
        public void TestGetDevices()
        {
          IRequestParameters requestParams = this.GetRequestParams(
           req =>
           {
               req.RegionName = "United States";
               req.AccessToken = "abcd";
               req.Params = new Parameters
               {
                   IncumbentType = "Fixed",
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

            var expectedResponse = this.GetResponse(
            response =>
            {
                response.Result = new Result
                {
                    DeviceList = new List<ProtectedDevice>
                        {
                           new ProtectedDevice
                           {
                               ProtectedDeviceType = ProtectedDeviceType.MVPD,
                                Location = new Location
                                {
                                    Latitude = 3,
                                    Longitude = 3
                                },
                           },

                           new ProtectedDevice
                           {
                               ProtectedDeviceType = ProtectedDeviceType.TBandStation,
                               Location = new Location
                                {
                                    Latitude = 3,
                                    Longitude = 3
                                },
                           }
                        }.ToArray()
                };
            });

            this.httpClientManager.PostOf1RequestStringString<RegionManagementResponse>((request, regionName, accessToken) => expectedResponse);

            var actualResponse = this.whitespacesDataClient.GetDevices(requestParams);

            Assert.AreEqual(expectedResponse.Result.DeviceList.Count(), actualResponse.DeviceList.Count());
        }

        /// <summary>
        /// Testing GetDeviceList throwing ArgumentNullException if request does not contain RegionName
        /// </summary>
        [TestMethod]
        [ExpectedException(typeof(ArgumentNullException))]
        public void GetDeviceList_WithoutRegionName_ArgumentNullException()
        {
            IRequestParameters requestParams = this.GetRequestParams(
                req =>
                {
                    req.Params = new Parameters
                    {
                        IncumbentType = "Fixed",
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

            this.whitespacesDataClient.GetDevices(requestParams);
        }

        /// <summary>
        /// Test method to verify excluding any channel.
        /// </summary>
        [TestMethod]
        public void TestExcludeChannel()
        {
            IRequestParameters requestParams = this.GetRequestParams(
                req =>
                {
                    req.RegionName = "United States";
                    req.AccessToken = "abcd";
                    req.Params = new Parameters
                    {
                        IncumbentType = "Fixed",
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

            var expectedResponse = this.GetResponse(
            response =>
            {
                response.Result = new Result
                {
                    Message = "Incumbent Excluded successfully."
                };
            });

            this.httpClientManager.PostOf1RequestStringString<RegionManagementResponse>((request, regionName, accessToken) => expectedResponse);

            var actualResponse = this.whitespacesDataClient.ExcludeChannel(requestParams);

            Assert.AreEqual(expectedResponse.Result.Message, actualResponse.Message);
        }

        /// <summary>
        /// Testing ExcludeChannel throwing ArgumentNullException if request does not contain RegionName
        /// </summary>
        [TestMethod]
        [ExpectedException(typeof(ArgumentNullException))]
        public void ExcludeChannel_WithoutRegionName_ArgumentNullException()
        {
            IRequestParameters requestParams = this.GetRequestParams(
                req =>
                {
                    req.Params = new Parameters
                    {
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
                                    Center = new Microsoft.Whitespace.Entities.Point
                                    {
                                        Latitude = "3",
                                        Longitude = "-1.3"
                                    }
                                }
                            }
                        }
                    };
                });

            this.whitespacesDataClient.ExcludeIds(requestParams);
        }

        /// <summary>
        /// Test method to verify excluding ids.
        /// </summary>
        [TestMethod]
        public void TestExcludeIds()
        {
            IRequestParameters requestParams = this.GetRequestParams(
                req =>
                {
                    req.RegionName = "United States";
                    req.AccessToken = "abcd";
                    req.Params = new Parameters
                    {
                        DeviceId = "12345"
                    };
                });

            var expectedResponse = this.GetResponse(
                response =>
                {
                    response.Result = new Result
                    {
                        Message = "Excluded successfully."
                    };
                });

            this.httpClientManager.PostOf1RequestStringString<RegionManagementResponse>((request, regionName, accessToken) => expectedResponse);

            var actualResponse = this.whitespacesDataClient.ExcludeIds(requestParams);

            Assert.AreEqual(expectedResponse.Result.Message, actualResponse.Message);
        }

        /// <summary>
        /// Testing ExcludeIds throwing ArgumentNullException if request does not contain RegionName
        /// </summary>
        [TestMethod]
        [ExpectedException(typeof(ArgumentNullException))]
        public void ExcludeIds_WithoutRegionName_ArgumentNullException()
        {
            IRequestParameters requestParams = this.GetRequestParams(
                req =>
                {
                    req.Params = new Parameters
                    {
                        DeviceId = Guid.NewGuid().ToString()
                    };
                });

            this.whitespacesDataClient.ExcludeIds(requestParams);
        }

        /// <summary>
        /// Test method to verify get near by TV stations
        /// </summary>
        [TestMethod]
        public void TestGetNearByTvStations()
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

            var expectedResponse = this.GetResponse(
                response =>
                {
                    response.Result = new Result
                    {
                        SearchMVPDCallSigns = new MVPDCallSignsInfo[] 
                        {
                            new MVPDCallSignsInfo
                            {
                                CallSign = "WWPX-TV",
                                Channel = 12,
                                Latitude = 38.9503885573831,
                                Longitude = -77.0797000775257,
                                ServiceType = "LD"
                            },
                            new MVPDCallSignsInfo
                            {
                                CallSign = "WMDO-CA",
                                Channel = 47,
                                Latitude = 38.9401109541485,
                                Longitude = -77.0813667929585,
                                ServiceType = "CA"
                            }
                        }
                    };
                });

            this.httpClientManager.PostOf1RequestStringString<RegionManagementResponse>((request, regionName, accessToken) => expectedResponse);

            var actualResponse = this.whitespacesDataClient.GetChannelList(requestParams);

            Assert.AreEqual(expectedResponse.Result.SearchMVPDCallSigns.Count(), actualResponse.SearchMVPDCallSigns.Count());
        }

        /// <summary>
        /// Testing GetNearByTVStations throwing ArgumentNullException if request does not contain RegionName
        /// </summary>
        [TestMethod]
        [ExpectedException(typeof(ArgumentNullException))]
        public void GetNearByTvStations_WithoutRegionName_ArgumentNullException()
        {
            IRequestParameters requestParams = this.GetRequestParams(
                req =>
                {
                    req.Params = new Parameters
                    {
                        MVPDLocation = new Location
                        {
                            Latitude = 38.62,
                            Longitude = -77.43
                        }
                    };
                });

            this.whitespacesDataClient.GetNearByTvStations(requestParams);
        }

        /// <summary>
        /// This test method is valid for all methods in which Post request is being used. Method returns No_Data_Found error if there is no data
        /// </summary>
        [TestMethod]
        public void GetNearByTvStations_NoDataFound_ReturnsNoDataFoundError()
        {
            IRequestParameters requestParams = this.GetRequestParams(
                request =>
                {
                    request.RegionName = "United States";
                    request.Params = new Parameters { };
                });

            var expectedResponse = this.GetResponse(
                response =>
                {
                    response.Error = new Result
                    {
                        Data = "NO_DATA_FOUND"
                    };
                });

            this.httpClientManager.PostOf1RequestStringString<RegionManagementResponse>((request, regionName, accessToken) => expectedResponse);

            var actualResponse = this.whitespacesDataClient.GetNearByTvStations(requestParams);

            Assert.AreEqual(expectedResponse.Error.Data, actualResponse.Data);
        }

        /// <summary>
        /// This test method is valid for all methods in which Post request is being used. Method returns ResponseErrorException if error is other than No_Data_Found
        /// </summary>
        [TestMethod]
        [ExpectedException(typeof(ResponseErrorException))]
        public void GetNearByTvStations_ErrorInResponse_ThrowsResponseErrorException()
        {
            IRequestParameters requestParams = this.GetRequestParams(
                request =>
                {
                    request.RegionName = "United States";
                    request.Params = new Parameters { };
                });

            var expectedResponse = this.GetResponse(
                response =>
                {
                    response.Error = new Result
                    {
                        Data = "hlsdhghdfgdlfg"
                    };
                });

            this.httpClientManager.PostOf1RequestStringString<RegionManagementResponse>((request, regionName, accessToken) => expectedResponse);

            this.whitespacesDataClient.GetNearByTvStations(requestParams);
        }

        /// <summary>
        /// Test method to verify get all ULS file numbers
        /// </summary>
        [TestMethod]
        public void GetAllUlsFileNumbers()
        {
            var expectedRespone = this.GetResponse(
                response =>
                    {
                        response.Result = new Result
                        {
                            LpAuxLicenses = new LpAuxLicenseInfo[] 
                            {
                                new LpAuxLicenseInfo
                                {
                                    ULSFileNumber = "0005122780"
                                },
                                new LpAuxLicenseInfo
                                {
                                    ULSFileNumber = "0005189303"
                                }
                            }
                        };
                    });

            this.httpClientManager.GetOf1RequestString<RegionManagementResponse>((request, regionName) => expectedRespone);

            var actualResponse = this.whitespacesDataClient.GetAllUlsFileNumbers("United States");

            Assert.AreEqual(expectedRespone.Result.LpAuxLicenses.Count(), actualResponse.LpAuxLicenses.Count());
        }

        /// <summary>
        /// Method returns ResponseErrorException if error is other than No_Data_Found
        /// </summary>
        [TestMethod]
        [ExpectedException(typeof(ResponseErrorException))]
        public void GetAllUlsFileNumbers_ErrorInResponse_ThrowsResponseErrorException()
        {
            var expectedResponse = this.GetResponse(
                response =>
                {
                    response.Error = new Result
                    {
                        Message = "Server Error"
                    };
                });

            this.httpClientManager.GetOf1RequestString<RegionManagementResponse>((request, regionName) => expectedResponse);

            this.whitespacesDataClient.GetAllUlsFileNumbers("United States");
        }

        /// <summary>
        /// Test method to verify get all ULS call signs
        /// </summary>
        [TestMethod]
        public void GetAllUlsCallSigns()
        {
            var expectedRespone = this.GetResponse(
                response =>
                {
                    response.Result = new Result
                    {
                        LpAuxLicenses = new LpAuxLicenseInfo[] 
                            {
                                new LpAuxLicenseInfo
                                {
                                    CallSign = "WQRY579"
                                },
                                new LpAuxLicenseInfo
                                {
                                    ULSFileNumber = "BLP01125"
                                }
                            }
                    };
                });

            this.httpClientManager.GetOf1RequestString<RegionManagementResponse>((request, regionName) => expectedRespone);

            var actualResponse = this.whitespacesDataClient.GetAllUlsCallSigns("United States");

            Assert.AreEqual(expectedRespone.Result.LpAuxLicenses.Count(), actualResponse.LpAuxLicenses.Count());
        }

        /// <summary>
        /// Method returns ResponseErrorException if error is other than No_Data_Found
        /// </summary>
        [TestMethod]
        [ExpectedException(typeof(ResponseErrorException))]
        public void GetAllUlsCallSigns_ErrorInResponse_ThrowsResponseErrorException()
        {
            var expectedResponse = this.GetResponse(
                response =>
                {
                    response.Error = new Result
                    {
                        Message = "Server Error"
                    };
                });

            this.httpClientManager.GetOf1RequestString<RegionManagementResponse>((request, regionName) => expectedResponse);

            this.whitespacesDataClient.GetAllUlsCallSigns("United States");
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
        /// This method Return the response which we setup in mock.
        /// </summary>
        /// <param name="responseInitializer">Response which we are initializing</param>
        /// <returns>Returning RegionManagementResponse</returns>
        private RegionManagementResponse GetResponse(Action<RegionManagementResponse> responseInitializer)
        {
            RegionManagementResponse response = new RegionManagementResponse();
            responseInitializer(response);

            return response;
        }
    }
}
