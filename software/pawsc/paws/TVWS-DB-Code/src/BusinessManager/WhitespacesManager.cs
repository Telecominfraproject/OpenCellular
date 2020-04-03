// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.BusinessManager
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.ComponentModel.DataAnnotations;
    using System.Linq;
    using System.Threading.Tasks;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WhiteSpaces.DataAccessManager;
    using Microsoft.WindowsAzure.Storage.Table;
    using mwc = Microsoft.WhiteSpaces.Common;

    public class WhitespacesManager : IWhitespacesManager
    {
        private static object lockObject = new object();

        private readonly IWhitespacesDataClient whitespacesClient;

        private mwc.IRequestParameters requestParams;

        public WhitespacesManager(IWhitespacesDataClient dataClient)
        {
            Check.IsNotNull(dataClient, "Data Client");

            this.whitespacesClient = dataClient;
        }

        public string RegisterMVPD(mwc.MvpdRegistrationInfo registrationInfo, string accessToken, string regionName = "United States")
        {
            Check.IsNotNull(registrationInfo, "MVPD Registration Info");
            Check.IsNotNull(accessToken, "Access Token");

            var errors = ValidationHelper.Validate<mwc.MvpdRegistrationInfo>(registrationInfo);
            if (errors.Count() > 0)
            {
                throw new mwc.ValidationErrorException(errors);
            }
            else
            {
                this.requestParams = this.GetRequestParams(
                    req =>
                    {
                        req.AccessToken = accessToken;
                        req.Params = new Parameters
                        {
                            IncumbentType = registrationInfo.IncumbentType,
                            MVPDLocation = registrationInfo.RecieveLocation,
                            TransmitLocation = registrationInfo.TransmitLocation,
                            Contact = registrationInfo.Contact,
                            MVPDRegistrant = registrationInfo.RegistrantInfo,
                            TvSpectrum = new TvSpectrum
                            {
                                CallSign = registrationInfo.CallSign,
                                Channel = registrationInfo.Channel,
                            },

                            // Some more name, description and contact Info, need to capture
                        };
                    });

                return this.whitespacesClient.AddIncumbent(this.requestParams).Message;
            }
        }

        public string RegisterLicensedLpAux(mwc.LicensedLpAuxRegistration registrationInfo, string accessToken, string regionName = "United States")
        {
            Check.IsNotNull(registrationInfo, "Licensed LpAux Registration Info");
            Check.IsNotNull(accessToken, "Access Token");

            List<Calendar> dateList = new List<Calendar>();

            if (registrationInfo.IsRecurred)
            {
                dateList = this.GetEventDates(registrationInfo);
            }
            else
            {
                var strartDate = Convert.ToDateTime(registrationInfo.StartDate).Add(TimeSpan.Parse(registrationInfo.StartTime));
                var endDate = Convert.ToDateTime(registrationInfo.EndDate).Add(TimeSpan.Parse(registrationInfo.EndTime));
                dateList.Add(new Calendar { Start = strartDate.ToString(), End = endDate.ToString(), Stamp = DateTime.Now.ToString() });
            }

            Event eventTime = new Event();
            eventTime.Times = dateList.ToArray();
            eventTime.Channels = registrationInfo.Channels;

            this.requestParams = this.GetRequestParams(
                req =>
                {
                    req.AccessToken = accessToken;
                    req.Params = new Parameters
                    {
                        IncumbentType = IncumbentType.LPAux.ToString(),
                        TvSpectrum = new TvSpectrum
                        {
                            CallSign = registrationInfo.CallSign,
                            Channel = registrationInfo.Channels[0]
                        },
                        PointsArea = new Point[] { new Point { Latitude = registrationInfo.Latitude.ToString(), Longitude = registrationInfo.Longitude.ToString() } },
                        Event = eventTime,                        
                        LPAuxRegistrant = new Whitespace.Entities.Versitcard.VCard
                        {
                            Address = new Whitespace.Entities.Versitcard.Address
                           {
                               Locality = registrationInfo.Address1,
                               Street = registrationInfo.Address2,
                               Country = registrationInfo.Country,
                               Region = registrationInfo.City,
                           },
                            Org = new Whitespace.Entities.Versitcard.Organization { OrganizationName = registrationInfo.ResponsibleParty }
                        },
                        Contact = new Whitespace.Entities.Versitcard.VCard
                        {
                            Address = new Whitespace.Entities.Versitcard.Address
                            {
                                Locality = registrationInfo.Address1,
                                Street = registrationInfo.Address2,
                                Country = registrationInfo.Country,
                                Region = registrationInfo.City,
                            },
                            Email = new Whitespace.Entities.Versitcard.Email[] { new Whitespace.Entities.Versitcard.Email { EmailAddress = registrationInfo.Email } },
                            Title = new Whitespace.Entities.Versitcard.Title { Text = registrationInfo.FriendlyName },
                            Telephone = new Whitespace.Entities.Versitcard.Telephone[] { new Whitespace.Entities.Versitcard.Telephone { TelephoneNumber = registrationInfo.ContactPhone, PId = registrationInfo.Phone } },
                            TimeZone = registrationInfo.TimeZone
                        }

                        // Channels need to capture
                    };
                });

            return this.whitespacesClient.AddIncumbent(this.requestParams).Message;
        }

        public string RegisterUnLicensedLpAux(mwc.LicensedLpAuxRegistration registrationInfo, string accessToken, string regionName = "United States")
        {
            Check.IsNotNull(registrationInfo, "UnLicensed LpAux Registration Info");
            Check.IsNotNull(registrationInfo.UlsFileNumber, "ULS FileNumber");
            Check.IsNotNull(accessToken, "Access Token");

            List<Calendar> dateList = new List<Calendar>();

            if (registrationInfo.IsRecurred)
            {
                dateList = this.GetEventDates(registrationInfo);
            }
            else
            {
                var strartDate = Convert.ToDateTime(registrationInfo.StartDate).Add(TimeSpan.Parse(registrationInfo.StartTime));
                var endDate = Convert.ToDateTime(registrationInfo.EndDate).Add(TimeSpan.Parse(registrationInfo.EndTime));
                dateList.Add(new Calendar { Start = strartDate.ToString(), End = endDate.ToString(), Stamp = DateTime.Now.ToString() });
            }

            Event eventTime = new Event();
            eventTime.Times = dateList.ToArray();
            eventTime.Channels = registrationInfo.Channels;

            this.requestParams = this.GetRequestParams(
                req =>
                {
                    req.AccessToken = accessToken;
                    req.Params = new Parameters
                    {
                        IncumbentType = IncumbentType.UnlicensedLPAux.ToString(),
                        ULSFileNumber = registrationInfo.UlsFileNumber,
                        TvSpectrum = new TvSpectrum
                        {
                            CallSign = registrationInfo.CallSign,
                            Channel = registrationInfo.Channels[0]
                        },
                        PointsArea = new Point[] { new Point { Latitude = registrationInfo.Latitude.ToString(), Longitude = registrationInfo.Longitude.ToString() } },
                        Event = eventTime,
                        Venue = registrationInfo.VenueName,
                        LPAuxRegistrant = new Whitespace.Entities.Versitcard.VCard
                        {
                            Address = new Whitespace.Entities.Versitcard.Address
                            {
                                Locality = registrationInfo.Address1,
                                Street = registrationInfo.Address2,
                                Country = registrationInfo.Country,
                                Region = registrationInfo.City,
                            },
                            Email = new Whitespace.Entities.Versitcard.Email[] { new Whitespace.Entities.Versitcard.Email { EmailAddress = registrationInfo.Email } },
                            Org = new Whitespace.Entities.Versitcard.Organization { OrganizationName = registrationInfo.OrgName },
                            Telephone = new Whitespace.Entities.Versitcard.Telephone[] { new Whitespace.Entities.Versitcard.Telephone { TelephoneNumber = registrationInfo.Phone } },
                            TimeZone = registrationInfo.TimeZone,
                            Name = new Whitespace.Entities.Versitcard.Name { ContactName = registrationInfo.Name }
                        },

                        // backend is expecting contact, so same info as registrant is coping..Need Dan Clarification
                        Contact = new Whitespace.Entities.Versitcard.VCard
                        {
                            Address = new Whitespace.Entities.Versitcard.Address
                            {
                                Locality = registrationInfo.Address1,
                                Street = registrationInfo.Address2,
                                Country = registrationInfo.Country,
                                Region = registrationInfo.City,
                            },
                            Email = new Whitespace.Entities.Versitcard.Email[] { new Whitespace.Entities.Versitcard.Email { EmailAddress = registrationInfo.Email } },
                            Org = new Whitespace.Entities.Versitcard.Organization { OrganizationName = registrationInfo.OrgName },
                            Telephone = new Whitespace.Entities.Versitcard.Telephone[] { new Whitespace.Entities.Versitcard.Telephone { TelephoneNumber = registrationInfo.Phone } },
                            TimeZone = registrationInfo.TimeZone
                        },

                        // Channels need to capture
                    };
                });

            return this.whitespacesClient.AddIncumbent(this.requestParams).Message;
        }

        public string RegisterTBasLinks(mwc.TBasLinkRegistration registrationInfo, string accessToken, string regionName = "United States")
        {
            Check.IsNotNull(registrationInfo, "Licensed LpAux Registration Info");
            Check.IsNotNull(accessToken, "Access Token");

            var strartDate = Convert.ToDateTime(registrationInfo.StartDate).Add(TimeSpan.Parse(registrationInfo.StartTime));
            var endDate = Convert.ToDateTime(registrationInfo.EndDate).Add(TimeSpan.Parse(registrationInfo.EndTime));

            this.requestParams = this.GetRequestParams(
                req =>
                {
                    req.AccessToken = accessToken;
                    req.Params = new Parameters
                    {
                        IncumbentType = IncumbentType.TBAS.ToString(),
                        TvSpectrum = new TvSpectrum
                        {
                            CallSign = registrationInfo.CallSign,
                            Channel = registrationInfo.Channels[0]
                        },

                        TransmitLocation = new Location { Latitude = Convert.ToDouble(registrationInfo.TransmitterLatitude), Longitude = Convert.ToDouble(registrationInfo.TransmitterLongitude) },
                        TempBasLocation = new Location { Latitude = Convert.ToDouble(registrationInfo.RecieverLatitude), Longitude = Convert.ToDouble(registrationInfo.RecieverLongitude) },

                        Event = new Event
                        {
                            // Reoccurence logic need to implement here
                            Times = new Calendar[] { new Calendar { Start = strartDate.ToString(), End = endDate.ToString(), Stamp = DateTime.Now.ToString() } },
                            Channels = registrationInfo.Channels
                        },
                        Contact = new Whitespace.Entities.Versitcard.VCard
                        {
                            Address = new Whitespace.Entities.Versitcard.Address
                            {
                                Locality = registrationInfo.Address1,
                                Street = registrationInfo.Address2,
                                Country = registrationInfo.Country,
                                Region = registrationInfo.City,
                            },
                            Email = new Whitespace.Entities.Versitcard.Email[] { new Whitespace.Entities.Versitcard.Email { EmailAddress = registrationInfo.Email } },
                            Telephone = new Whitespace.Entities.Versitcard.Telephone[] { new Whitespace.Entities.Versitcard.Telephone { TelephoneNumber = registrationInfo.Phone } },
                            TimeZone = registrationInfo.TimeZone,
                            Title = new Whitespace.Entities.Versitcard.Title { Text = registrationInfo.FriendlyName }
                        },

                        TempBASRegistrant = new Whitespace.Entities.Versitcard.VCard
                        {
                            Address = new Whitespace.Entities.Versitcard.Address
                            {
                                Locality = registrationInfo.Address1,
                                Street = registrationInfo.Address2,
                                Country = registrationInfo.Country,
                                Region = registrationInfo.City,
                            },
                            Email = new Whitespace.Entities.Versitcard.Email[] { new Whitespace.Entities.Versitcard.Email { EmailAddress = registrationInfo.Email } },
                            Org = new Whitespace.Entities.Versitcard.Organization { OrganizationName = registrationInfo.FriendlyName },
                            Telephone = new Whitespace.Entities.Versitcard.Telephone[] { new Whitespace.Entities.Versitcard.Telephone { TelephoneNumber = registrationInfo.Phone } },
                            TimeZone = registrationInfo.TimeZone,
                            Title = new Whitespace.Entities.Versitcard.Title { Text = registrationInfo.Description }
                        },

                        // Channels need to capture
                    };
                });

            return this.whitespacesClient.AddIncumbent(this.requestParams).Message;
        }

        public string ExcludeChannel(int[] channels, Point[] locations, string accesstoken, string regionName)
        {
            Check.IsNotNull(channels, "channels");
            Check.IsNotEmptyOrWhiteSpace(regionName, "Region Name");

            TvSpectrum[] spectra = new TvSpectrum[channels.Length];
            for (int i = 0; i < channels.Length; i++)
            {
                var spectrum = new TvSpectrum { Channel = channels[i] };
                spectra[i] = spectrum;
            }

            List<GeoLocation> geoLocations = new List<GeoLocation>();
            foreach (Point location in locations)
            {
                geoLocations.Add(new GeoLocation
                    {
                        Point = new Ellipse
                        {
                            Center = location
                        }
                    });
            }

            this.requestParams = this.GetRequestParams(
                req =>
                {
                    req.RegionName = regionName;
                    req.AccessToken = accesstoken;
                    req.Params = new Parameters
                    {
                        Locations = geoLocations.ToArray(),
                        TvSpectra = spectra
                    };
                });

            return this.whitespacesClient.ExcludeChannel(this.requestParams).Message;
        }

        public string ExcludeDevice(string accesstoken, string regionName, string deviceId, string serialNumber = "")
        {
            Check.IsNotEmptyOrWhiteSpace(regionName, "Region Name");
            Check.IsNotEmptyOrWhiteSpace(accesstoken, "Access Token");
            Check.IsNotEmptyOrWhiteSpace(regionName, "Region Name");

            this.requestParams = this.GetRequestParams(
               req =>
               {
                   req.RegionName = regionName;
                   req.AccessToken = accesstoken;
                   req.Params = new Parameters
                   {
                       DeviceId = deviceId,
                       SerialNumber = serialNumber
                   };
               });

            return this.whitespacesClient.ExcludeIds(this.requestParams).Message;
        }

        public ChannelInfo[] GetChannelList(string incumbentType, double latitude, double longitude, string regionName = "United States")
        {
            Check.IsNotNull(incumbentType, "IncumbentType");

            mwc.IRequestParameters requestParams = this.GetRequestParams(
                 req =>
                 {
                     req.RegionName = regionName;
                     req.Params = new Parameters
                     {
                         IncumbentType = incumbentType,
                         Location = new GeoLocation
                         {
                             Point = new Ellipse
                             {
                                 Center = new Point
                                 {
                                     Latitude = latitude.ToString(),
                                     Longitude = longitude.ToString()
                                 }
                             }
                         },

                         // Required for LpAux or UnLpAux
                         PointsArea = new Point[] { new Point { Latitude = latitude.ToString(), Longitude = longitude.ToString() } },
                     };

                     // TODO: Should following fix be only for United Kingdom?
                     if (string.Compare(regionName, "United Kingdom", StringComparison.OrdinalIgnoreCase) == 0)
                     {
                         req.Params.RequestType = "Specific";
                     }
                 });

            return this.whitespacesClient.GetChannelList(requestParams).ChannelInfo;
        }

        public List<mwc.Incumbent> GetIncumbents(string incumbentType, string regionName, IEnumerable<int> channels)
        {
            Check.IsNotEmptyOrWhiteSpace(incumbentType, "incumbentType");
            Check.IsNotEmptyOrWhiteSpace(regionName, "regionName");

            IncumbentType enumType = (IncumbentType)Enum.Parse(typeof(IncumbentType), incumbentType, true);

            // Todo: This is temporary code until backend supports returning TV_US via the GetIncumbents call.
            if (enumType == IncumbentType.TV_US || enumType == IncumbentType.MVPD || enumType == IncumbentType.TV_TRANSLATOR)
            {
                return this.GetIncumbentsFromTable(channels, enumType);
            }

            mwc.IRequestParameters requestParams = this.GetRequestParams(
                 req =>
                 {
                     req.RegionName = regionName;
                     req.Params = new Parameters
                     {
                         IncumbentType = incumbentType
                     };
                 });

            object[] rawIncumbents = this.whitespacesClient.GetIncumbents(requestParams).IncumbentList;

            return WhitespacesManager.ParseRawIncumbents(enumType, rawIncumbents, channels);
        }

        public List<mwc.Incumbent> GetIncumbents(string incumbentType, double latitude, double longitude, string regionName, IEnumerable<int> channels)
        {
            Check.IsNotEmptyOrWhiteSpace(incumbentType, "incumbentType");
            Check.IsNotEmptyOrWhiteSpace(regionName, "regionName");

            IncumbentType enumType = (IncumbentType)Enum.Parse(typeof(IncumbentType), incumbentType, true);

            // Todo: This is temporary code until backend supports returning TV_US via the GetIncumbents call.
            if (enumType == IncumbentType.TV_US || enumType == IncumbentType.TV_TRANSLATOR || enumType == IncumbentType.MVPD)
            {
                return this.GetIncumbentsFromTable(channels, enumType, latitude, longitude);
            }

            // [Note:] As of now, it doesn't make any sense passing latitude and longitude values to filter the incumbents
            // based on the search region, but if it is enabled in back-end in future we can use following request.
            mwc.IRequestParameters requestParams = this.GetRequestParams(
                 req =>
                 {
                     req.RegionName = regionName;
                     req.Params = new Parameters
                     {
                         IncumbentType = incumbentType,
                         Location = new GeoLocation
                         {
                             Point = new Ellipse
                             {
                                 Center = new Point
                                 {
                                     Latitude = latitude.ToString(),
                                     Longitude = longitude.ToString()
                                 }
                             }
                         }
                     };
                 });

            object[] rawIncumbents = this.whitespacesClient.GetIncumbents(requestParams).IncumbentList;

            return WhitespacesManager.ParseRawIncumbents(enumType, rawIncumbents, channels);
        }

        public MVPDCallSignsInfo[] GetNearByTvStations(Location location, string accessToken, string regionName = "United States")
        {
            Check.IsNotNull(location, "Location");

            mwc.IRequestParameters requestParams = this.GetRequestParams(
                req =>
                {
                    req.RegionName = regionName;
                    req.Params = new Parameters
                    {
                        MVPDLocation = location,

                    };
                    req.AccessToken = accessToken;
                });

            return this.whitespacesClient.GetNearByTvStations(requestParams).SearchMVPDCallSigns;
        }

        public async Task<MVPDCallSignsInfo> GetMvpdCallsignInfoAsync(string callsign, string regionName = "United States")
        {
            Check.IsNotNull(callsign, "callsign");

            mwc.IRequestParameters requestParams = this.GetRequestParams(
                req =>
                {
                    req.RegionName = regionName;
                });

            Result result = await this.whitespacesClient.GetMvpdCallsignInfoAsync(callsign, regionName);

            return result.CallsignInfo;
        }

        public LpAuxLicenseInfo GetULSFileInfo(string ulsFileNumber, string regionName = "United States")
        {
            Check.IsNotNull(ulsFileNumber, "Uls File Number");
            mwc.IRequestParameters requestParams = this.GetRequestParams(
            req =>
            {
                req.RegionName = regionName;
                req.Params = new Parameters();
            });

            LpAuxLicenseInfo[] lpAuxLicenses = this.whitespacesClient.GetAllUlsFileNumbers(regionName).LpAuxLicenses;

            return lpAuxLicenses.Where(x => x.ULSFileNumber == ulsFileNumber).FirstOrDefault();
        }

        public ProtectedDevice[] GetDeviceList(string incumbentType, double latitude, double longitude, string regionName = "United States")
        {
            mwc.IRequestParameters requestParams = this.GetRequestParams(
                req =>
                {
                    req.RegionName = regionName;
                    req.Params = new Parameters
                    {
                        IncumbentType = incumbentType,
                        Location = new GeoLocation
                        {
                            Point = new Ellipse
                            {
                                Center = new Point
                                {
                                    Latitude = latitude.ToString(),
                                    Longitude = longitude.ToString()
                                }
                            }
                        }
                    };
                });

            return this.whitespacesClient.GetDevices(requestParams).DeviceList;
        }

        public void SubmitPawsInterference(Parameters parameters, string regionName = Microsoft.WhiteSpaces.Common.Constants.Paws)
        {
            mwc.IRequestParameters requestParams = this.GetRequestParams(
               req =>
               {
                   req.RegionName = regionName;
                   req.Params = parameters;
               });

            this.whitespacesClient.SubmitPawsInterference(requestParams);
        }

        private static List<T> GetDeserializedIncumbents<T>(object[] rawIncumbents)
        {
            return rawIncumbents.Select(obj => JsonSerialization.DeserializeString<T>(obj.ToString())).ToList();
        }

        private static List<Position> GetContourPointsSpaceSeperator(string rawContour)
        {
            List<Position> contourPoints = new List<Position>();

            if (!string.IsNullOrWhiteSpace(rawContour))
            {
                string[] points = rawContour.Split(' ');
                double latitude = 0;
                double longitude = 0;
                int count = 0;
                foreach (string point in points)
                {
                    if (count % 2 == 0)
                    {
                        latitude = double.Parse(point);
                    }
                    else
                    {
                        longitude = double.Parse(point);
                        contourPoints.Add(new Position
                        {
                            Latitude = latitude,
                            Longitude = longitude
                        });
                    }

                    count++;
                }
            }

            return contourPoints;
        }

        private static List<Position> GetContourPoints(string rawContour)
        {
            List<Position> contourPoints = new List<Position>();

            if (!string.IsNullOrWhiteSpace(rawContour))
            {
                Contour contour = JsonSerialization.DeserializeString<Contour>(rawContour);

                if (contour.ContourPoints != null)
                {
                    foreach (Location location in contour.ContourPoints)
                    {
                        contourPoints.Add(new Position
                        {
                            Latitude = location.Latitude,
                            Longitude = location.Longitude
                        });
                    }
                }
            }

            return contourPoints;
        }

        private static mwc.Incumbent GetIncumbent(object incumbent, IncumbentType requestedIncumbentType)
        {
            mwc.Incumbent incumbentModel = null;

            if (incumbent is MVPDRegistration)
            {
                MVPDRegistration mvpdRegistration = (MVPDRegistration)incumbent;

                // TODO: BroadcastStationContour was removed
                //                List<Position> contourPoints = WhitespacesManager.GetContourPoints(mvpdRegistration.BroadcastStationContour);
                List<Position> contourPoints = new List<Position>();

                incumbentModel = new mwc.Incumbent(
                    mvpdRegistration.Channel.CallSign,
                    mvpdRegistration.ChannelNumber,
                    contourPoints,
                    null,
                    requestedIncumbentType,
                    mvpdRegistration.Location,
                    mvpdRegistration.TransmitLocation);
            }
            else if (incumbent is TempBASRegistration)
            {
                TempBASRegistration tempBasRegistration = (TempBASRegistration)incumbent;

                // TODO: BroadcastStationContour was removed
                //                List<Position> contourPoints = WhitespacesManager.GetContourPoints(tempBasRegistration.BroadcastStationContour);
                List<Position> contourPoints = new List<Position>();

                incumbentModel = new mwc.Incumbent(
                    tempBasRegistration.Channel.CallSign,
                    tempBasRegistration.ChannelNumber,
                    contourPoints,
                    tempBasRegistration.Event,
                    requestedIncumbentType,
                    tempBasRegistration.RecvLocation,
                    tempBasRegistration.TransmitLocation);
            }
            else if (incumbent is LPAuxRegistration)
            {
                LPAuxRegistration lpAuxIncumbent = (LPAuxRegistration)incumbent;
                List<Position> contourPoints = new List<Position>();

                if (lpAuxIncumbent.PointsArea != null)
                {
                    contourPoints = lpAuxIncumbent.PointsArea.ToList();
                }
                else if (lpAuxIncumbent.QuadrilateralArea != null)
                {
                    foreach (QuadrilateralArea area in lpAuxIncumbent.QuadrilateralArea)
                    {
                        contourPoints.Add(area.NEPoint);
                        contourPoints.Add(area.NWPoint);
                        contourPoints.Add(area.SEPoint);
                        contourPoints.Add(area.SWPoint);
                    }
                }

                // TODO: How to get ReceiveLocation for LPAuxRegistration entity.
                incumbentModel = new mwc.Incumbent(
                     lpAuxIncumbent.CallSign.CallSign,
                     lpAuxIncumbent.CallSign.Channel.Value,
                     contourPoints,
                     lpAuxIncumbent.Event,
                     requestedIncumbentType,
                     null,
                     new Location(lpAuxIncumbent.Latitude, lpAuxIncumbent.Longitude));
            }

            //// TODO: Logic to create Incumbent object for TV_US incumbent type, as now not sure what Object type to be compare with.

            return incumbentModel;
        }

        private static IEnumerable<mwc.Incumbent> FilterIncumbentsByChannel(IEnumerable<mwc.Incumbent> incumbents, IEnumerable<int> channels)
        {
            if (incumbents != null)
            {
                incumbents = incumbents.Where(incumbent => channels.Contains(incumbent.Channel));
            }

            return incumbents;
        }

        private static List<mwc.Incumbent> ParseRawIncumbents(IncumbentType incumbentType, object[] rawIncumbents, IEnumerable<int> channels)
        {
            dynamic incumbentList = null;
            List<mwc.Incumbent> incumbents = new List<mwc.Incumbent>();

            if (rawIncumbents != null)
            {
                switch (incumbentType)
                {
                    case IncumbentType.MVPD:
                        incumbentList = WhitespacesManager.GetDeserializedIncumbents<MVPDRegistration>(rawIncumbents);
                        break;
                    case IncumbentType.LPAux:
                    case IncumbentType.UnlicensedLPAux:
                        incumbentList = WhitespacesManager.GetDeserializedIncumbents<LPAuxRegistration>(rawIncumbents);
                        break;
                    case IncumbentType.TBAS:
                        incumbentList = WhitespacesManager.GetDeserializedIncumbents<TempBASRegistration>(rawIncumbents);
                        break;
                    case IncumbentType.TV_US:
                        // TODO: Logic to de-serialize TV_US incumbents type. [Need to wait until back-end supports this]
                        break;
                }

                if (incumbentList != null)
                {
                    foreach (object incumbent in incumbentList)
                    {
                        mwc.Incumbent incumbentModel = WhitespacesManager.GetIncumbent(incumbent, incumbentType);
                        incumbents.Add(incumbentModel);
                    }
                }

                if (channels != null)
                {
                    IEnumerable<mwc.Incumbent> filteredIncumbents = WhitespacesManager.FilterIncumbentsByChannel(incumbents, channels);
                    incumbents = filteredIncumbents != null ? filteredIncumbents.ToList() : incumbents;
                }
            }

            return incumbents;
        }

        private IEnumerable<string> ValidateLPAuxRegisterRequest(Parameters parameters)
        {
            var pointArea = parameters.PointsArea;
            var quadralateralArea = parameters.QuadrilateralArea;
            var events = parameters.Event;

            if (pointArea == null && quadralateralArea == null)
            {
                yield return "Atleast one of PointArea or QuadralateralArea is Required";
            }
            else if (pointArea != null && quadralateralArea == null)
            {
                if (pointArea.Length < 1 || pointArea.Length > 25)
                {
                    yield return "PointArea should have point 1 to 25 points";
                }
            }
            else if (pointArea == null && quadralateralArea != null)
            {
                if (quadralateralArea.Length < 1 || quadralateralArea.Length > 4)
                {
                    yield return "QuadrilateralArea should have point 1 to 4 quads";
                }
            }

            if (events.Channels.Length < 1)
            {
                yield return "Atleast one channel is required";
            }
        }

        private mwc.RequestParameters GetRequestParams(Action<mwc.RequestParameters> parametersInitializer)
        {
            mwc.RequestParameters requestparameters = new mwc.RequestParameters();
            requestparameters.RegionName = "United States"; // hard coded as registration is only for fcc
            parametersInitializer(requestparameters);

            return requestparameters;
        }

        /// <summary>
        /// Gets all the incumbents based on either channel numbers or position (if channels value is not null, then it will
        /// retrieve based on Channel number; otherwise it will retrieve incumbents based on location).
        /// </summary>
        /// <param name="channels">Retrieves incumbents that match the specified channels (if null, then it will use the latitude and longitude field)</param>
        /// <param name="incumbentType">Retrieves incumbents that match the specified type</param>
        /// <param name="latitude">Retrieves incumbents that are near the specified latitude (only used if channels parameter is null).</param>
        /// <param name="longitude">Retrieves incumbents that are near the specified longitude (only used if channels parameter is null).</param>
        /// <returns>List of incumbents that match the passed in parameters</returns>
        private List<mwc.Incumbent> GetIncumbentsFromTable(IEnumerable<int> channels, IncumbentType incumbentType = IncumbentType.None, double latitude = 0, double longitude = 0)
        {
            // Todo: this is a temporary method provided until the back-end supports retrieving incumbents of type "tv_us"
            Microsoft.WhiteSpaces.AzureTableAccess.AzureTableOperation azureTableOperations = new Microsoft.WhiteSpaces.AzureTableAccess.AzureTableOperation();

            List<mwc.Incumbent> incumbents = new List<mwc.Incumbent>();

            if (channels == null)
            {
                // Just get the TV Towers that are located around the specified latitude or longitude.
                string partitionKey = string.Format("RGN1-Lat{0}Long{1}", (int)latitude, (int)longitude);
                IEnumerable<mwc.Incumbent> tvEngDataList = this.GetIncumbentsFromTable("PortalSummary", partitionKey, incumbentType);
                incumbents.AddRange(tvEngDataList);
            }
            else
            {
                // Get all incumbents associated with the specified channels.

                // NB: Creating multiple queries and executing them in parallel will get the results faster, rather scanning through entire table.
                // For more reference refer http://msdn.microsoft.com/en-us/magazine/ff796231.aspx
                try
                {
                    Parallel.ForEach<int>(
                        channels,
                        channel =>
                        {
                            System.Diagnostics.Stopwatch watch = new System.Diagnostics.Stopwatch();
                            watch.Start();
                            System.Diagnostics.Trace.TraceError(string.Format("***PERF: Retrieving {0} channel elements", channel));

                            int elementsCount = 0;

                            IEnumerable<mwc.Incumbent> tvEngDataList = GetIncumbentsFromTable("PortalContours", "RGN1-" + channel, incumbentType);

                            if (tvEngDataList != null)
                            {
                                lock (lockObject)
                                {
                                    elementsCount = incumbents.Count;

                                    incumbents.AddRange(tvEngDataList);

                                    elementsCount = incumbents.Count - elementsCount;
                                }
                            }

                            watch.Stop();
                            System.Diagnostics.Trace.TraceError(string.Format("***PERF: Retrieved {0} elements in {1} seconds ({2})", tvEngDataList != null ? elementsCount : 0, watch.Elapsed.ToString(), channel));
                        });
                }
                catch (AggregateException)
                {
                    throw;
                }
            }

            return incumbents;
        }

        private IEnumerable<mwc.Incumbent> GetIncumbentsFromTable(string tableName, string partitionKey, IncumbentType incumbentType)
        {
            // Todo: this is a temporary method provided until the backend supports retriving incumbents of type "tv_us"
            Microsoft.WhiteSpaces.AzureTableAccess.AzureTableOperation azureTableOperations = new Microsoft.WhiteSpaces.AzureTableAccess.AzureTableOperation();

            string tableQuery = TableQuery.GenerateFilterCondition("PartitionKey", QueryComparisons.Equal, partitionKey);

            if (incumbentType != IncumbentType.None)
            {
                tableQuery = TableQuery.CombineFilters(tableQuery, TableOperators.And, TableQuery.GenerateFilterConditionForInt("Type", QueryComparisons.Equal, (int)incumbentType));
            }

            IEnumerable<mwc.Incumbent> tvEngDataList = azureTableOperations.GetAllEntities<CDBSTvEngData>(tableName, tableQuery)
                .Select(cdbsTvEndData =>
                {
                    List<Position> contourPoints = WhitespacesManager.GetContourPointsSpaceSeperator(cdbsTvEndData.Contour);
                    mwc.Incumbent incumbent = new mwc.Incumbent(
                    cdbsTvEndData.CallSign,
                    cdbsTvEndData.Channel,
                    contourPoints,
                    null,
                    incumbentType,
                    null,
                    new Location(cdbsTvEndData.Latitude, cdbsTvEndData.Longitude));

                    return incumbent;
                });

            return tvEngDataList;
        }

        private List<Calendar> GetEventDates(mwc.LicensedLpAuxRegistration registrationInfo)
        {
            List<Calendar> dateList = new List<Calendar>();
            var strartDate = Convert.ToDateTime(registrationInfo.StartDate);
            var endDate = Convert.ToDateTime(registrationInfo.EndDate);

            string startTime = registrationInfo.StartTime;
            string endTime = registrationInfo.EndTime;

            if (registrationInfo.IsReoccurenceDaily)
            {
                if (registrationInfo.ReoccurenceInstance > 0)
                {
                    for (int i = 0; i < registrationInfo.ReoccurenceInstance; i++)
                    {
                        var newStart = strartDate.AddDays(i).Add(TimeSpan.Parse(startTime));
                        var newEnd = strartDate.AddDays(i).Add(TimeSpan.Parse(endTime));
                        dateList.Add(new Calendar { Start = newStart.ToString(), End = newEnd.ToString(), Stamp = DateTime.Now.ToString() });
                    }
                }
                else
                {
                    var reoccurenceEndDate = Convert.ToDateTime(registrationInfo.ReoccurrenceEndDate);
                    List<DateTime> allDates = new List<DateTime>();
                    for (DateTime date = strartDate; date <= reoccurenceEndDate; date = date.AddDays(1))
                    {
                        allDates.Add(date);
                    }

                    foreach (DateTime date in allDates)
                    {
                        var newStart = date.Add(TimeSpan.Parse(startTime));
                        var newEnd = date.Add(TimeSpan.Parse(endTime));
                        dateList.Add(new Calendar { Start = newStart.ToString(), End = newEnd.ToString(), Stamp = DateTime.Now.ToString() });
                    }
                }
            }
            else
            {
                int i = 0;
                registrationInfo.WeekDays = registrationInfo.WeekDaysString.Split(',');
                var reoccurenceEndDate = Convert.ToDateTime(registrationInfo.ReoccurrenceEndDate);

                while (true)
                {
                    var date = strartDate.AddDays(i);
                    if (registrationInfo.WeekDays.Contains(date.DayOfWeek.ToString()))
                    {
                        var newStart = date.Add(TimeSpan.Parse(startTime));
                        var newEnd = date.Add(TimeSpan.Parse(endTime));
                        dateList.Add(new Calendar { Start = newStart.ToString(), End = newEnd.ToString(), Stamp = DateTime.Now.ToString() });
                    }

                    if (registrationInfo.ReoccurenceInstance > 0)
                    {
                        if (dateList.Count == registrationInfo.ReoccurenceInstance)
                        {
                            break;
                        }
                    }
                    else if (date.Date == reoccurenceEndDate.Date)
                    {
                        break;
                    }

                    i++;
                }
            }

            return dateList;
        }
    }
}
