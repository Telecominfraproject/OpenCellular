// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.BusinessManager
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Threading.Tasks;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WhiteSpaces.Common;
    using Microsoft.WhiteSpaces.Common.Enums;
    using Microsoft.WhiteSpaces.DataAccessManager;

    public class UserAdminManager : IUserAdminManager
    {
        private readonly IUsersDataAccess userDataAccess;

        public UserAdminManager(IUsersDataAccess userDataAccess)
        {
            this.userDataAccess = userDataAccess;
        }

        public IEnumerable<UserInfo> GetAllUsersInfo()
        {
            return this.userDataAccess.GetAllUser();
        }

        public IEnumerable<AccessRequest> GetAllAccessRequests()
        {
            return this.userDataAccess.GetAllAccessElevationRequests();
        }

        public bool SaveAccessElevationRequest(AccessRequest accessRequest)
        {
            Microsoft.WhiteSpaces.Common.Check.IsNotNull(accessRequest, "Access Request");
            return this.userDataAccess.SaveAccessElevationRequest(accessRequest);
        }

        public IEnumerable<IntialUserInfo> GetIntialUserList(SearchCriteria criteria)
        {
            IEnumerable<UserInfo> userDetails = this.userDataAccess.GetAllUser();
            List<IntialUserInfo> userList = new List<IntialUserInfo>();

            object lockingObject = new object();
            Parallel.ForEach(
                userDetails, 
                user =>
                {
                    if (user.SuperAdmin)
                    {
                        var intialUseInfo = new IntialUserInfo
                        {
                            Id = user.UserId,
                            UserName = user.UserName,
                            Country = user.Country,
                            PrefferedEmail = user.PreferredEmail,
                            DateRegistered = user.RegisteredDate
                        };

                        intialUseInfo.Role = this.GetRoleFromId(Convert.ToInt32(AccessLevels.SuperAdmin));
                        intialUseInfo.Regulatory = "All";
                        intialUseInfo.RequestStatus = RequestStatus.Approved.ToString();

                        lock (lockingObject)
                        {
                            userList.Add(intialUseInfo);
                        }
                    }
                    else
                    {
                        IEnumerable<AccessRequest> requests = this.userDataAccess.GetAccessElevationRequestsByUserId(user.UserId);

                        if (requests.Count() == 0)
                        {
                            var intialUseInfo = new IntialUserInfo
                            {
                                Id = user.UserId,
                                UserName = user.UserName,
                                Country = user.Country,
                                PrefferedEmail = user.PreferredEmail,
                                DateRegistered = user.RegisteredDate
                            };
                            intialUseInfo.Role = this.GetRoleFromId(Convert.ToInt32(AccessLevels.PortalUser));
                            intialUseInfo.Regulatory = "All";
                            intialUseInfo.RequestStatus = RequestStatus.Approved.ToString();

                            lock (lockingObject)
                            {
                                userList.Add(intialUseInfo);
                            }
                        }
                        else
                        {
                            foreach (var request in requests)
                            {
                                var intialUseInfo = new IntialUserInfo
                                {
                                    Id = user.UserId,
                                    UserName = user.UserName,
                                    Country = user.Country,
                                    PrefferedEmail = user.PreferredEmail,
                                    DateRegistered = user.RegisteredDate
                                };

                                intialUseInfo.Regulatory = request.Regulatory;
                                if (request.RequestStatus == (int)RequestStatus.Approved)
                                {
                                    intialUseInfo.Role = this.GetRoleFromId(request.CurrentAccessLevel);
                                }
                                else
                                {
                                    intialUseInfo.Role = this.GetRoleFromId(request.RequestedAccessLevel);
                                }

                                intialUseInfo.RequestStatus = ((RequestStatus)request.RequestStatus).ToString();

                                lock (lockingObject)
                                {
                                    userList.Add(intialUseInfo);
                                }
                            }
                        }
                    }
                });

            if (criteria != null)
            {
                var filterList = userList;
                if (criteria.RequestStatus == (int)RequestStatus.Approved)
                {
                    filterList = filterList.Where(x => string.Equals(x.RequestStatus, RequestStatus.Approved.ToString(), StringComparison.OrdinalIgnoreCase) == true).ToList();
                }
                else if (criteria.RequestStatus == (int)RequestStatus.Reject)
                {
                    filterList = filterList.Where(x => string.Equals(x.RequestStatus, RequestStatus.Reject.ToString(), StringComparison.OrdinalIgnoreCase) == true).ToList();
                }

                if (!string.IsNullOrEmpty(criteria.SelectedCountry))
                {
                    filterList = filterList.Where(x => string.Equals(x.Country, criteria.SelectedCountry, StringComparison.OrdinalIgnoreCase) == true).ToList();
                }

                if (!string.IsNullOrEmpty(criteria.SelectedRole))
                {
                    filterList = filterList.Where(x => string.Equals(x.Role, criteria.SelectedRole, StringComparison.OrdinalIgnoreCase) == true).ToList();
                }

                if (criteria.NameStartsWith != null && criteria.NameStartsWith.Length > 0)
                {
                    filterList = filterList.Where(x => criteria.NameStartsWith.Contains(x.UserName.Substring(0, 1).ToUpper())).ToList();
                }

                return filterList;
            }

            return userList;
        }

        public IEnumerable<AccessRequest> GetElevationRequestsByUser(string userId)
        {
            return this.userDataAccess.GetAccessElevationRequestsByUserId(userId);
        }

        private string GetRoleFromId(int id)
        {
            return AppConfigRegionSource.GetRoles().Where(x => x.Id == id).First().FriendlyName;
        }
    }
}
