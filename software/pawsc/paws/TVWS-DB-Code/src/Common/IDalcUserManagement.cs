// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Whitespace.Common
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using Microsoft.Whitespace.Entities;

    /// <summary>
    /// Represents the User Management interface into the data access layer component.
    /// </summary>
    public interface IDalcUserManagement
    {
        /// <summary>
        /// Adds the specified user into the database.
        /// </summary>
        /// <param name="user">User information that is to be added.</param>
        /// <returns> user id </returns>
        string AddUser(User user);

        /// <summary>
        /// Deletes the specified user from the database.
        /// </summary>
        /// <param name="userId">The user identifier.</param>
        /// <param name="authorizedUser">The authorized user.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        bool DeleteUser(string userId, string authorizedUser);

        /// <summary>
        /// Updated the specified user fields in the database.
        /// </summary>
        /// <param name="user">Updated user information.</param>
        /// <returns> user id </returns>
        string UpdateUser(User user);

        /// <summary>
        /// Gets the specified user.
        /// </summary>
        /// <param name="userId">The user Id that is to be retrieved.</param>
        /// <returns>Returns the specified user (or null if no user is found with the specified Id).</returns>
        User GetUser(string userId);

        /// <summary>
        /// Gets the user by live identifier.
        /// </summary>
        /// <param name="userId">The user identifier.</param>
        /// <returns>User Object</returns>
        User GetUserByLiveId(string userId);

        /// <summary>
        /// Request for Elevated Access
        /// </summary>
        /// <param name="elevatedAccessRequest">The elevated access request.</param>
        /// <returns>returns String </returns>
        string RequestElevatedAccess(ElevatedAccessRequest elevatedAccessRequest);

        /// <summary>
        /// Gets all of the registered user.
        /// </summary>
        /// <returns>All registered users.</returns>
        User[] GetUsers();

        /// <summary>
        /// Grants the access.
        /// </summary>
        /// <param name="elevatedAccessRequest">The elevated access request.</param>
        /// <param name="authorizedUser">The authorized user.</param>
        /// <returns>returns String</returns>
        string GrantAccess(ElevatedAccessRequest elevatedAccessRequest, string authorizedUser);
    }
}
