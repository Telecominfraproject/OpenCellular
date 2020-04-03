// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.WhiteSpaces.DataAccessManager
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Threading.Tasks;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Entities;
    using Microsoft.WhiteSpaces.AzureTableAccess;
    using Microsoft.WhiteSpaces.Common;
    using Microsoft.WhiteSpaces.Common.Enums;
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Table;        

    public class UsersDataAccess : IUsersDataAccess
    {
        private readonly IAzureTableOperation azureTableOperations;

        [Microsoft.Practices.Unity.InjectionConstructor]
        public UsersDataAccess()
            : this(new AzureTableOperation())
        { 
        }

        public UsersDataAccess(IAzureTableOperation azureTableOperations)
        {
            this.azureTableOperations = azureTableOperations;
        }

        public IEnumerable<UserInfo> GetAllUser()
        {
            List<UserInfo> usersData = new List<UserInfo>();
            object lockingObject = new object();
            var userProfiles = this.azureTableOperations.GetAllEntities<UserProfile>();

            Parallel.ForEach(
                userProfiles, 
                userProfile =>
                {
                    var userInfo = new UserInfo()
                    {
                        UserId = userProfile.RowKey,
                        UserName = userProfile.UserName,
                        FirstName = userProfile.FirstName,
                        LastName = userProfile.LastName,
                        AccountEmail = userProfile.AccountEmail,
                        PreferredEmail = userProfile.PreferredEmail,
                        Address1 = userProfile.Address1,
                        Address2 = userProfile.Address2,
                        City = userProfile.City,
                        Country = userProfile.Country,
                        Phone = userProfile.Phone,
                        PhoneCountryCode = userProfile.PhoneCountryCode,
                        ZipCode = userProfile.ZipCode,
                        SuperAdmin = userProfile.IsSuperAdmin,
                        RegisteredDate = userProfile.CreatedTime
                    };

                    var accesslevels = GetAuthorityAccessByUser(userProfile.RowKey);
                    Dictionary<string, string> roles = new Dictionary<string, string>();

                    foreach (var accesslevel in accesslevels)
                    {
                        roles.Add(System.Enum.Parse(typeof(Authorities), accesslevel.RowKey).ToString(), Convert.ToInt16(accesslevel.AccessLevel).ToString());
                    }

                    userInfo.RolesInfo = roles;

                    lock (lockingObject)
                    {
                        usersData.Add(userInfo);
                    }
                });

            return usersData;
        }

        public IEnumerable<AccessRequest> GetAllAccessElevationRequests()
        {
            List<AccessRequest> userRegulatoryAccesslist = new List<AccessRequest>();
            object lockingObject = new object();

            var requests = this.azureTableOperations.GetAllEntities<AccessElevationRequest>();
            var pendingRequests = requests.Where(x => Convert.ToInt32(x.RequestStatus) == (int)RequestStatus.Pending);

            try
            {
                Parallel.ForEach(
                    pendingRequests, 
                    authorityAccess =>
                    {
                        var userRegulatoryAccess = new AccessRequest()
                        {
                            UserId = authorityAccess.PartitionKey,
                            Regulatory = authorityAccess.Regulatory,
                            CurrentAccessLevel = authorityAccess.CurrentAccessLevel,
                            RequestedAccessLevel = authorityAccess.RequestedAccessLevel,
                            Justification = authorityAccess.Justification,
                            TimeUpdated = authorityAccess.Timestamp.ToString(),
                        };

                        lock (lockingObject)
                        {
                            userRegulatoryAccesslist.Add(userRegulatoryAccess);
                        }
                    });

                return userRegulatoryAccesslist;
            }
            catch (System.AggregateException ex)
            {
                if (ex.GetType() == typeof(StorageException))
                {
                    StorageException exception = (StorageException)ex.InnerException;

                    // If azure table is not found
                    if (exception.Message.Contains("(404) Not Found"))
                    {
                        return userRegulatoryAccesslist;
                    }
                }

                throw;
            }
        }

        public IEnumerable<AccessRequest> GetAccessElevationRequestsByUserId(string userId)
        {
            List<AccessRequest> userRegulatoryAccesslist = new List<AccessRequest>();
            object lockingObject = new object();

            var requests = this.azureTableOperations.GetEntityByPartitionKey<AccessElevationRequest>(userId);

            Parallel.ForEach(
                requests, 
                authorityAccess =>
                {
                    var userRegulatoryAccess = new AccessRequest()
                    {
                        UserId = authorityAccess.PartitionKey,
                        Regulatory = authorityAccess.Regulatory,
                        CurrentAccessLevel = authorityAccess.CurrentAccessLevel,
                        RequestedAccessLevel = authorityAccess.RequestedAccessLevel,
                        Justification = authorityAccess.Justification,
                        RequestStatus = authorityAccess.RequestStatus,
                        ApprovedUser = authorityAccess.ApprovedUser,
                        Remarks = authorityAccess.Remarks,
                        TimeUpdated = authorityAccess.Timestamp.ToString()
                    };

                    lock (lockingObject)
                    {
                        userRegulatoryAccesslist.Add(userRegulatoryAccess);
                    }
                });

            return userRegulatoryAccesslist;
        }

        public bool SaveAccessElevationRequest(AccessRequest accessRequest)
        {
            IEnumerable<AccessElevationRequest> userRequests = this.azureTableOperations.GetEntityByPartitionKey<AccessElevationRequest>(accessRequest.UserId).Where(x => Convert.ToInt32(x.RequestStatus) == (int)RequestStatus.Pending);
            var currentRequest = userRequests.Where(x => x.Regulatory == accessRequest.Regulatory).FirstOrDefault();

            if (currentRequest != null)
            {
                if (accessRequest.RequestStatus == (int)RequestStatus.Approved)
                {
                    currentRequest.CurrentAccessLevel = currentRequest.RequestedAccessLevel;
                    currentRequest.RequestedAccessLevel = 0;
                }

                // update Request
                currentRequest.RequestStatus = accessRequest.RequestStatus;
                currentRequest.Remarks = accessRequest.Remarks;
                currentRequest.ApprovedUser = accessRequest.ApprovedUser;
                this.azureTableOperations.InsertEntity(currentRequest);

                // update user profile, if requested for 
                if (currentRequest.CurrentAccessLevel == (int)AccessLevels.SuperAdmin)
                {
                    var userProfile = this.azureTableOperations.FetchEntity<UserProfile>("1", currentRequest.PartitionKey);
                    userProfile.IsSuperAdmin = true;
                    this.azureTableOperations.InsertEntity(userProfile);
                }

                // update authority access table
                int authority = (int)((Authorities)System.Enum.Parse(typeof(Authorities), accessRequest.Regulatory));
                AuthorityAccess currentaccessLevels = this.azureTableOperations.FetchEntity<AuthorityAccess>(accessRequest.UserId, authority.ToString());
                currentaccessLevels.AccessLevel = currentRequest.CurrentAccessLevel;
                this.azureTableOperations.InsertEntity(currentaccessLevels);

                return true;
            }

            return false;
        }

        public UserInfo GetUserInfoById(string userId)
        {
            var userProfile = this.azureTableOperations.FetchEntity<UserProfile>("1", userId);

            return new UserInfo
            {
                UserId = userProfile.RowKey,
                UserName = userProfile.UserName,
                FirstName = userProfile.FirstName,
                LastName = userProfile.LastName,
                AccountEmail = userProfile.AccountEmail,
                PreferredEmail = userProfile.PreferredEmail,
                Address1 = userProfile.Address1,
                Address2 = userProfile.Address2,
                City = userProfile.City,
                Country = userProfile.Country,
                Phone = userProfile.Phone,
                PhoneCountryCode = userProfile.PhoneCountryCode,
                ZipCode = userProfile.ZipCode
            };
        }

        private IEnumerable<AuthorityAccess> GetAuthorityAccessByUser(string userId)
        {
            return this.azureTableOperations.GetEntityByPartitionKey<AuthorityAccess>(userId);
        }
    }
}
