// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.BusinessManager.Test.Unit
{
    using System;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WhiteSpaces.BusinessManager;
    using Microsoft.WhiteSpaces.Common;
    using Microsoft.WhiteSpaces.DataAccessManager.Fakes;

    /// <summary>
    /// Unit Test cases for WhitespacesManager class methods
    /// </summary>
    [TestClass]
    public class WhitespacesManagerTests
    {
        /// <summary>
        /// WhitespacesManager reference variable
        /// </summary>
        private readonly WhitespacesManager whitespacesManager;

        /// <summary>
        /// StubIWhitespacesDataClient reference variable
        /// </summary>
        private readonly StubIWhitespacesDataClient whitespacesDataClient;

        private string accessToken = "abcd";

        /// <summary>
        /// Initializes a new instance of the <see cref="WhitespacesManagerTests"/> class
        /// </summary>
        public WhitespacesManagerTests()
        {
            this.whitespacesDataClient = new StubIWhitespacesDataClient();
            this.whitespacesManager = new WhitespacesManager(this.whitespacesDataClient);
        }

        [TestMethod]
        public void RegisterMVPD_ValidRegistrationInfoWithAllMandatoryParameters_AddsMvpdSuccessfully()
        {
            MvpdRegistrationInfo mvpdRegistrationInfo = new MvpdRegistrationInfo
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
                }
            };

            Result result = this.RegisteredDeviceResult();

            this.whitespacesDataClient.AddIncumbentIRequestParameters = (request) => result;

            string actualMessage = this.whitespacesManager.RegisterMVPD(mvpdRegistrationInfo, accessToken);

            Assert.AreEqual("Device Registered successfully.", actualMessage);
        }

        [TestMethod]
        [ExpectedException(typeof(ValidationErrorException))]
        public void RegisterMVPD_RegistrationInfoWithoutName_ValidationErrorException()
        {
            MvpdRegistrationInfo mvpdRegistrationInfo = new MvpdRegistrationInfo
            {
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
                }
            };

            this.whitespacesManager.RegisterMVPD(mvpdRegistrationInfo, accessToken);
        }

        [TestMethod]
        [ExpectedException(typeof(ValidationErrorException))]
        public void RegisterMVPD_RegistrationInfoWithoutCallSign_ValidationErrorException()
        {
            MvpdRegistrationInfo mvpdRegistrationInfo = new MvpdRegistrationInfo
            {
                Name = "Mvpd Registration",
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
                }
            };

            this.whitespacesManager.RegisterMVPD(mvpdRegistrationInfo, accessToken);
        }

        [TestMethod]
        [ExpectedException(typeof(NullReferenceException))]
        public void RegisterMVPD_RegistrationInfoWithoutChannel_NullReferenceException()
        {
            MvpdRegistrationInfo mvpdRegistrationInfo = new MvpdRegistrationInfo
            {
                Name = "Mvpd Registration",
                CallSign = "WBFF",
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
                }
            };

            this.whitespacesManager.RegisterMVPD(mvpdRegistrationInfo, accessToken);
        }

        [TestMethod]
        [ExpectedException(typeof(ValidationErrorException))]
        public void RegisterMVPD_RegistrationInfoWithoutReceiverLocation_ValidationErrorException()
        {
            MvpdRegistrationInfo mvpdRegistrationInfo = new MvpdRegistrationInfo
            {
                Name = "Mvpd Registration",
                CallSign = "WBFF",
                Channel = 46,
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
                }
            };

            this.whitespacesManager.RegisterMVPD(mvpdRegistrationInfo, accessToken);
        }

        [TestMethod]
        [ExpectedException(typeof(ValidationErrorException))]
        public void RegisterMVPD_RegistrationInfoWithoutTransmitterLocation_ValidationErrorException()
        {
            MvpdRegistrationInfo mvpdRegistrationInfo = new MvpdRegistrationInfo
            {
                Name = "Mvpd Registration",
                CallSign = "WBFF",
                Channel = 46,
                RecieveLocation = new Location
                {
                    Latitude = 38.66484069824219,
                    Longitude = -77.43494415283203
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
                }
            };

            this.whitespacesManager.RegisterMVPD(mvpdRegistrationInfo, accessToken);
        }

        [TestMethod]
        [ExpectedException(typeof(ValidationErrorException))]
        public void RegisterMVPD_RegistrationInfoWithoutContact_ValidationErrorException()
        {
            MvpdRegistrationInfo mvpdRegistrationInfo = new MvpdRegistrationInfo
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
                }
            };

            this.whitespacesManager.RegisterMVPD(mvpdRegistrationInfo, accessToken);
        }

        [TestMethod]
        [ExpectedException(typeof(ValidationErrorException))]
        public void RegisterMVPD_RegistrationInfoWithoutRegistrantInfo_ValidationErrorException()
        {
            MvpdRegistrationInfo mvpdRegistrationInfo = new MvpdRegistrationInfo
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
                }
            };

            this.whitespacesManager.RegisterMVPD(mvpdRegistrationInfo, accessToken);
        }

        [TestMethod]
        [ExpectedException(typeof(ArgumentNullException))]
        public void RegisterMVPD_NullAsRegistrationInfo_ArgumentNullException()
        {
            this.whitespacesManager.RegisterMVPD(null, accessToken);
        }

        [TestMethod]
        [ExpectedException(typeof(ArgumentNullException))]
        public void RegisterMVPD_NullAsAccessToken_ArgumentNullException()
        {
            MvpdRegistrationInfo mvpdRegistrationInfo = new MvpdRegistrationInfo();
            this.whitespacesManager.RegisterMVPD(mvpdRegistrationInfo, null);
        }

        [TestMethod]
        public void RegisterLicensedLpAux_ValidRegistrationInfoWithAllMandatoryParameters_AddsLPAuxSuccessfully()
        {
            LicensedLpAuxRegistration licensedLpAuxRegistration = new LicensedLpAuxRegistration
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

            Result result = this.RegisteredDeviceResult();

            this.whitespacesDataClient.AddIncumbentIRequestParameters = (request) => result;

            string actualMessage = this.whitespacesManager.RegisterLicensedLpAux(licensedLpAuxRegistration, accessToken);

            Assert.AreEqual("Device Registered successfully.", actualMessage);
        }

        [TestMethod]
        public void RegisterLicensedLpAux_ValidRegistrationInfoWithReccurrence_AddsLPAuxSuccessfully()
        {
            LicensedLpAuxRegistration licensedLpAuxRegistration = new LicensedLpAuxRegistration
            {
                CallSign = "BLN00751",
                Channels = new[] { 6, 7 },
                FriendlyName = "Licensed LPAux Registration",                
                IsRecurred = true,
                IsReoccurenceDaily = true,
                StartDate = DateTime.Today.ToString("MM-dd-yyyy"),
                EndDate = DateTime.Today.ToString("MM-dd-yyyy"),
                StartTime = DateTime.Now.ToString("HH:mm:ss"),
                EndTime = DateTime.Now.AddHours(2).ToString("HH:mm:ss"),
                Latitude = "40.7779169",
                Longitude = "-74.7922202",
            };

            Result result = this.RegisteredDeviceResult();

            this.whitespacesDataClient.AddIncumbentIRequestParameters = (request) => result;

            string actualMessage = this.whitespacesManager.RegisterLicensedLpAux(licensedLpAuxRegistration, accessToken);

            Assert.AreEqual("Device Registered successfully.", actualMessage);
        }

        [TestMethod]
        [ExpectedException(typeof(ArgumentNullException))]
        public void RegisterLicensedLpAux_NullAsRegistrationInfo_ArgumentNullException()
        {
            this.whitespacesManager.RegisterLicensedLpAux(null, accessToken);
        }

        [TestMethod]
        [ExpectedException(typeof(ArgumentNullException))]
        public void RegisterLicensedLpAux_NullAsAccessToken_ArgumentNullException()
        {
            LicensedLpAuxRegistration licensedLpAuxRegistration = new LicensedLpAuxRegistration();
            this.whitespacesManager.RegisterLicensedLpAux(licensedLpAuxRegistration, null);
        }

        [TestMethod]
        public void RegisterUnLicensedLpAux_ValidRegistrationInfoWithAllMandatoryParameters_RegistersUnlicensedLPAuxSuccessfully()
        {
            LicensedLpAuxRegistration unLicensedLpAuxRegistration = new LicensedLpAuxRegistration
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

            Result result = this.RegisteredDeviceResult();

            this.whitespacesDataClient.AddIncumbentIRequestParameters = (request) => result;

            string actualMessage = this.whitespacesManager.RegisterUnLicensedLpAux(unLicensedLpAuxRegistration, accessToken);

            Assert.AreEqual("Device Registered successfully.", actualMessage);
        }

        [TestMethod]
        [ExpectedException(typeof(ArgumentNullException))]
        public void RegisterUnLicensedLpAux_NullAsRegistrationInfo_ArgumentNullException()
        {
            this.whitespacesManager.RegisterUnLicensedLpAux(null, accessToken);
        }

        [TestMethod]
        [ExpectedException(typeof(ArgumentNullException))]
        public void RegisterUnLicensedLpAux_NullAsAccessToken_ArgumentNullException()
        {
            LicensedLpAuxRegistration unLicensedLpAuxRegistration = new LicensedLpAuxRegistration();
            this.whitespacesManager.RegisterUnLicensedLpAux(unLicensedLpAuxRegistration, null);
        }

        [TestMethod]
        public void RegisterTBasLinks_ValidRegistrationInfoWithAllMandatoryParameters_RegistersTBasLinksSuccessfully()
        {
            TBasLinkRegistration tBasLinkRegistration = new TBasLinkRegistration
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

            Result result = this.RegisteredDeviceResult();

            this.whitespacesDataClient.AddIncumbentIRequestParameters = (request) => result;

            string actualMessage = this.whitespacesManager.RegisterTBasLinks(tBasLinkRegistration, accessToken);

            Assert.AreEqual("Device Registered successfully.", actualMessage);
        }

        [TestMethod]
        [ExpectedException(typeof(ArgumentNullException))]
        public void RegisterTBasLinks_NullAsRegistrationInfo_ArgumentNullException()
        {
            this.whitespacesManager.RegisterTBasLinks(null, accessToken);
        }

        [TestMethod]
        [ExpectedException(typeof(ArgumentNullException))]
        public void RegisterTBasLinks_NullAsAccessToken_ArgumentNullException()
        {
            TBasLinkRegistration tBasLinkRegistration = new TBasLinkRegistration();
            this.whitespacesManager.RegisterTBasLinks(tBasLinkRegistration, null);
        }

        private Result RegisteredDeviceResult()
        {
            return new Result
            {
                Message = "Device Registered successfully."
            };
        }
    }
}
