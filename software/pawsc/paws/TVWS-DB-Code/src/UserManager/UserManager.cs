// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.UserManager
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Linq;
    using Microsoft.Practices.EnterpriseLibrary.Validation;
    using Microsoft.Practices.EnterpriseLibrary.Validation.Validators;
    using Microsoft.Practices.Unity;
    using Microsoft.Whitespace.Common;
    using Microsoft.Whitespace.Entities;

    /// <summary>Class UserManager</summary>
    public class UserManager : IWhitespaceUserManager
    {
        /// <summary>Gets or sets the DALC user management.</summary>
        /// <value>The DALC user management.</value>
        [Dependency]
        public IDalcUserManagement DalcUserManagement { get; set; }

        /// <summary>Gets or sets ILogger Interface</summary>
        [Dependency]
        public ILogger RegionManagementLogger { get; set; }

        /// <summary>Validated that the specified user is valid.</summary>
        /// <param name="id">The Id of the user.</param>
        /// <param name="password">The password of the user.</param>
        /// <returns>Returns true if the user is valid, false otherwise.</returns>
        public bool ValidUser(string id, string password)
        {
            // ToDo: save value to azure table.
            throw new NotImplementedException();
        }

        /// <summary>Adds the specified user to the database.</summary>
        /// <param name="user">User that is to be added.</param>
        /// <returns>return user id </returns>
        public string AddUser(User user)
        {
            try
            {
                string logMethodName = "UserManager.AddUser(User user)";
                string result = string.Empty;
                //// Begin Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Enter " + logMethodName);
                if (user.Access == null)
                {
                    user.Access = new RegionAccess[1];
                    user.Access[0] = new RegionAccess
                                     {
                                         AccessLevel = AccessLevel.DeviceVendor,
                                         Region = Utils.CurrentRegionId.ToString()
                                     };
                }

                if (this.GetUserByLiveID(user.UserId) == null)
                {
                    this.DalcUserManagement.AddUser(user);
                   result = "User added successfully.";
                }
                else
                {
                    result = "User Already exists.";
                }
                //// End Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Exit " + logMethodName);
                return result;
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.RegionManagementLogger.Log(TraceEventType.Error, LoggingMessageId.DriverGenericMessage, e.ToString());
                throw;
            }
        }

        /// <summary>Updated the specified user fields in the database.</summary>
        /// <param name="user">Updated user information.</param>
        /// <returns> return user id </returns>
        public string UpdateUser(User user)
        {
            try
            {
                string logMethodName = "UserManager.UpdateUser(User user)";

                // Begin Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Enter " + logMethodName);
                string result = this.DalcUserManagement.UpdateUser(user);
               
                // End Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Exit " + logMethodName);
                return result;
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.RegionManagementLogger.Log(TraceEventType.Error, LoggingMessageId.DriverGenericMessage, e.ToString());
                throw;
            }
        }

        /// <summary>Deletes the specified user from the database.</summary>
        /// <param name="userID">The user identifier.</param>
        /// <param name="authorizedUserId">The authorized user identifier.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        public bool DeleteUser(string userID, string authorizedUserId)
        {
            try
            {
                if (userID != null)
                {
                    string logMethodName = "UserManager.DeleteUser(string userID)";

                    // Begin Log transaction
                    this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Enter " + logMethodName);
                    bool result = this.DalcUserManagement.DeleteUser(userID, authorizedUserId);
                    
                    // End Log transaction
                    this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Exit " + logMethodName);
                    return result;
                }

                return false;
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.RegionManagementLogger.Log(TraceEventType.Error, LoggingMessageId.DriverGenericMessage, e.ToString());
                throw;
            }
        }

        /// <summary>Gets the specified user.</summary>
        /// <param name="id">The user Id that is to be retrieved.</param>
        /// <returns>Returns the specified user (or null if no user is found with the specified Id).</returns>
        public User GetUser(string id)
        {
            try
            {
                string logMethodName = "UserManager.GetUser(string id)";

                // Begin Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Enter " + logMethodName);

                User userDetails = this.DalcUserManagement.GetUser(id);

                // End Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Exit " + logMethodName);
                return this.SuppressTableDetails(userDetails);
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.RegionManagementLogger.Log(TraceEventType.Error, LoggingMessageId.DriverGenericMessage, e.ToString());
                throw;
            }
        }
       
        /// <summary>Suppresses the table details.</summary>
        /// <param name="user">The user.</param>
        /// <returns> return User </returns>
        public User SuppressTableDetails(User user)
        {
            if (user != null)
            {
                user.PartitionKey = null;
                user.ETag = null;
                user.RowKey = null;
                foreach (RegionAccess access in user.Access)
                {
                    access.PartitionKey = null;
                    access.ETag = null;
                    access.RowKey = null;
                }

                return user;
            }
            else
            {
                return null;
            }
        }

        /// <summary>Gets all of the registered user.</summary>
        /// <returns>All registered users.</returns>
        public User[] GetUsers()
        {
            // ToDo: save value to azure table.
            throw new NotImplementedException();
        }

        /// <summary>Request for Elevated Access</summary>
        /// <param name="elevatedAccessRequest">The elevated access request.</param>
        /// <returns>returns String</returns>
        public string RequestElevatedAccess(ElevatedAccessRequest elevatedAccessRequest)
        {
            try
            {
                string logMethodName = "UserManager.RequestElevatedAccess(ElevatedAccessRequest elevatedAccessRequest)";

                // Begin Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Enter " + logMethodName);
                string result = this.DalcUserManagement.RequestElevatedAccess(elevatedAccessRequest);
               
                // End Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Exit " + logMethodName);
                return result;
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.RegionManagementLogger.Log(TraceEventType.Error, LoggingMessageId.DriverGenericMessage, e.ToString());
                throw;
            }
        }

        /// <summary>
        /// Grants the access.
        /// </summary>
        /// <param name="elevatedAccessRequest">The elevated access request.</param>
        /// <param name="authorizedUserId">The authorized user identifier.</param>
        /// <returns>returns System.String.</returns>
        public string GrantAccess(ElevatedAccessRequest elevatedAccessRequest, string authorizedUserId)
        {
            try
            {
                string logMethodName = "UserManager.GrantAccess(ElevatedAccessRequest elevatedAccessRequest)";

                // Begin Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Enter " + logMethodName);
                string result = this.DalcUserManagement.GrantAccess(elevatedAccessRequest, authorizedUserId);
                
                // End Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Exit " + logMethodName);

                return result;
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.RegionManagementLogger.Log(TraceEventType.Error, LoggingMessageId.DriverGenericMessage, e.ToString());
                throw;
            }
        }

        /// <summary>Determines whether the specified object is valid.</summary>
        /// <typeparam name="T">Generic Type</typeparam>
        /// <param name="obj">The object.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns><c>true</c> if the specified object is valid; otherwise, <c>false</c>.</returns>
        public bool IsValid<T>(T obj, out List<string> errorMessages)
        {
            errorMessages = new List<string>();
            Validator cusValidator = new ObjectValidator();
            ValidationResults valResults = cusValidator.Validate(obj);
            if (!valResults.IsValid)
            {
                errorMessages.AddRange(valResults.Select(objVal => objVal.Message));
                return false;
            }

            return true;
        }

        /// <summary>
        /// Gets the user by live identifier.
        /// </summary>
        /// <param name="id">The identifier.</param>
        /// <returns>User Object</returns>
        private User GetUserByLiveID(string id)
        {
            try
            {
                string logMethodName = "UserManager.GetUserByLiveID(string id)";
        
                // Begin Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Enter " + logMethodName);

                User userDetails = this.DalcUserManagement.GetUserByLiveId(id);

                // End Log transaction
                this.RegionManagementLogger.Log(TraceEventType.Information, LoggingMessageId.DriverGenericMessage, "Exit " + logMethodName);
                return this.SuppressTableDetails(userDetails);
            }
            catch (Exception e)
            {
                // Log transaction failure
                this.RegionManagementLogger.Log(TraceEventType.Error, LoggingMessageId.DriverGenericMessage, e.ToString());
                throw;
            }
        }
    }
}
