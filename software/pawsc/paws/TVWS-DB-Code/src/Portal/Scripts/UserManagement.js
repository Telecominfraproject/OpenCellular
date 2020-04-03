// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

$(document).ready(function () {

    var userManagement = new UserManagement();

    $("#btnBack").click(userManagement.back);

    $("#btnEdit").click(userManagement.editClick);
    $("#btnUpdate").click(userManagement.UpdateClick);
    $("#btnCancel").click(userManagement.cancelClick);

});

function UserManagement() {
    var self = this;
    var optionList = ["Portal User", "Region Administrator", "Portal Administrator"];

    self.getUserDetails = function (id) {

        var url = "/UserManagement/GetUserInfo";
        var data = { "userId": id };

        ServerManager.AjaxRequest(
        {
            url: url,
            data: JSON.stringify(data),
            type: "POST",
            dataType: "json",
            contentType: "application/json; charset=utf-8",
            cache: false,
            success: self.onSuccess,
            error: self.onError
        });
    };

    self.onSuccess = function (result, status, xhr) {
        if (result.error) {
            ServerManager.handleError(result, null);
        }
        if (result != null) {

            $("#FirstName").text(result.FirstName);
            if (result.LastName != null) {
                $("#LastName").text(result.LastName);
            }
            $("#PreferredEmail").text(result.PreferredEmail);
            $("#Phone").text(result.Phone);
            $("#Address1").text(result.Address1);
            if (result.Address2 != null) {
                $("#Address2").text(result.Address2);
            }
            $("#City").text(result.City);
            $("#State").text(result.State);
            $("#Country").text(result.Country);
            $("#ZipCode").text(result.ZipCode);
            $("#AccountEmail").text(result.AccountEmail);
            $("#TimeZone").text(result.TimeZone);
            $("#userId").val(result.UserId);

            $("#accessLevels tbody").empty();
            $("#editAccessLevels tbody").empty();
            $("#elevationRequests tbody").empty();
            $("#dvRequests").addClass("hide");

            self.loadAccessLevels(result.AccessInfo);
            self.loadEditAccessLevels(result.AccessInfo);

            if (result.ElevationRequests.length > 0) {
                self.loadRequests(result.ElevationRequests);
            }

            $("#details").removeClass("hide");
            $("#dvList").addClass("hide");
        }
    };

    self.onError = function (error) {
        var v = error;
    };

    self.back = function () {
        $("#details").addClass("hide");
        $("#dvList").removeClass("hide");
        return false;
    };

    self.loadAccessLevels = function (accesslevels) {
        $(accesslevels).each(function () {

            var authority = "";
            switch (this.Authority.toString()) {
                case "0": authority = "fcc";
                    break;
                case "1": authority = "ofcom";
                    break;
                case "2": authority = "ghana";
                    break;
            }

            var role = "";
            switch (this.AccessLevel.toString()) {
                case "1": role = "Portal User";
                    break;
                case "4": role = "Region Administrator";
                    break;
                case "5": role = "Portal Administrator";
                    break;
            }

            $("#accessLevels tbody").append("<tr>" + "<td>" + authority + "</td>" + "<td>" + role + "</td>" + "</tr>");
        });
    };

    self.loadEditAccessLevels = function (accesslevels) {
        $(accesslevels).each(function () {

            var authority = "";
            switch (this.Authority.toString()) {
                case "0": authority = "fcc";
                    break;
                case "1": authority = "ofcom";
                    break;
                case "2": authority = "ghana";
                    break;
            }

            var role = "";
            switch (this.AccessLevel.toString()) {
                case "1": role = "Portal User";
                    break;
                case "4": role = "Region Administrator";
                    break;
                case "5": role = "Portal Administrator";
                    break;
            }

            $("#editAccessLevels tbody").append("<tr>" + "<td>" + authority + "</td>" + "<td>" + self.getDropdownlist(role) + "</td>" + "</tr>");
        });
    };

    self.getDropdownlist = function (selectedRole) {
        var dropDown = $("<select></select>");

        $(optionList).each(function () {
            if (this == selectedRole) {
                dropDown.append("<option selected='selected'>" + this + "</option>");
            }
            else {
                dropDown.append("<option>" + this + "</option>");
            }
        });

        var val = dropDown[0].outerHTML.toString();
        return val;
    };

    self.loadRequests = function (requests) {
        $("#dvRequests").removeClass("hide");

        $(requests).each(function () {

            var status = "";

            switch (this.RequestStatus.toString()) {
                case "0": status = "Pending";
                    break;
                case "1": status = "Approved";
                    break;
                case "2": status = "Rejected";
                    break;
            }

            var role = "";
            switch (this.RequestedAccessLevel.toString()) {
                case "1": role = "Portal User";
                    break;
                case "4": role = "Region Administrator";
                    break;
                case "5": role = "Portal Administrator";
                    break;
            }

            $("#elevationRequests tbody").append("<tr>" + "<td>" + this.Regulatory + "</td>" + "<td>" + role +
                "</td>" + "<td>" + this.Justification + "</td>" + "<td>" + status + "</td>" + "<td>" + this.ApprovedUser + "</td>" + "<td>" + this.Remarks + "</td>" +
                "<td>" + this.TimeUpdated + "</td>" + "</tr>");
        });
    };

    self.editClick = function () {
        $("#dvRoles").toggleClass("hide");
        $("#dvEditRoles").toggleClass("hide");
    };

    self.UpdateClick = function () {

        var userId = $("#userId").val();
        var accessLevels = [];

        $("#editAccessLevels").find('tr').each(function (i, el) {
            var $tds = $(this).find('td');

            if ($tds.length > 0) {
                var regulatory = "";
                var role = "";

                switch ($tds.eq(0).text()) {
                    case "fcc": regulatory = "0";
                        break;
                    case "ofcom": regulatory = "1";
                        break;
                    case "ghana": regulatory = "2";
                        break;
                }

                switch ($tds.eq(1)[0].firstElementChild.value) {
                    case "Portal User": role = "1";
                        break;
                    case "Region Administrator": role = "4";
                        break;
                    case "Portal Administrator": role = "5";
                        break;
                }

                var access = { "regulatory": regulatory, "role": role };
                accessLevels.push(JSON.stringify(access));
            }
        });

        var url = "/UserManagement/UpdateAccessDetails";
        var data = { "userId": userId, "roles": accessLevels };

        ServerManager.AjaxRequest(
        {
            url: url,
            data: JSON.stringify(data),
            type: "POST",
            dataType: "json",
            contentType: "application/json; charset=utf-8",
            cache: false,
            success: self.onUpdateSuccess,
            error: self.onUpdateError
        });
    };

    self.cancelClick = function () {
        $("#dvRoles").toggleClass("hide");
        $("#dvEditRoles").toggleClass("hide");
    };

    self.onUpdateSuccess = function (result, status, xhr) {
        if (result != null) {
            $("#accessLevels tbody").empty();
            self.loadAccessLevels(result);
        }

        $("#dvRoles").toggleClass("hide");
        $("#dvEditRoles").toggleClass("hide");
    };

    self.onUpdateError = function (error) {
        $("#dvRoles").toggleClass("hide");
        $("#dvEditRoles").toggleClass("hide");
    };
}
