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
    /// Represents the User Manager interface.
    /// </summary>
    public interface IWhitespaceUserManager
    {
        /// <summary>
        /// Validated that the specified user is valid.
        /// </summary>
        /// <param name="id">The Id of the user.</param>
        /// <param name="password">The password of the user.</param>
        /// <returns>Returns true if the user is valid, false otherwise.</returns>
        bool ValidUser(string id, string password);

        /// <summary>
        /// Adds the specified user to the database.
        /// </summary>
        /// <param name="user">User that is to be added.</param>
        /// <returns> user id </returns>
        string AddUser(User user);

        /// <summary>
        /// Updated the specified user fields in the database.
        /// </summary>
        /// <param name="user">Updated user information.</param>
        /// <returns> user id </returns>
        string UpdateUser(User user);

        /// <summary>
        /// Deletes the specified user from the database.
        /// </summary>
        /// <param name="userId">The user identifier.</param>
        /// <param name="authorizedUser">The authorized user.</param>
        /// <returns><c>true</c> if XXXX, <c>false</c> otherwise.</returns>
        bool DeleteUser(string userId, string authorizedUser);

        /// <summary>
        /// Gets the specified user.
        /// </summary>
        /// <param name="id">The user Id that is to be retrieved.</param>
        /// <returns>Returns the specified user (or null if no user is found with the specified Id).</returns>
        User GetUser(string id);

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

        /// <summary>
        /// Grants the access.
        /// </summary>
        /// <typeparam name="T">type of parameter</typeparam>
        /// <param name="obj">The object.</param>
        /// <param name="errorMessages">The error messages.</param>
        /// <returns>returns String</returns>
        bool IsValid<T>(T obj, out List<string> errorMessages);
    }
}
