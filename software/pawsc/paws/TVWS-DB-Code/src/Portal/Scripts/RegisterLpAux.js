// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

$(document).ready(function () {
    //$(".multiselect").multiselect();

    var registerCommon = new RegisterCommon();

    var registerLpaux = new RegisterLpAux(bingMap, registerCommon);

    $("#btnSubmit").click(registerLpaux.beforeSubmit);

    setTimeout(function () {
        bingMap.ZoomtoLocation(39.443256, -98.957336, 4);
    }, 500);

    $("#lnkGetLocation").click(registerLpaux.OnGetLocationClick);
    $("#lnkReoccurence").click(registerCommon.showReoccurence);

    $('#weekly').change(registerCommon.showWeekDays);
    $('#daily').change(registerCommon.showWeekDays);

    $('#stopInstance').change(registerCommon.updteReoccurenceValues);
    $('#StopDate').change(registerCommon.updteReoccurenceValues);

    $('.summary-pane').removeClass('expanded').addClass('left');
    $('#rightPaneContainer').removeClass('bottom-view collapse-view').addClass('left-view');

    $("#StartDate").datepicker({
        dateFormat: "yy-mm-dd",
        minDate: new Date()
    });

    $("#EndDate").datepicker({
        dateFormat: "yy-mm-dd",
        minDate: new Date()
    });

    $("#REndDate").datepicker({
        dateFormat: "yy-mm-dd",
        minDate: new Date()
    });

    var d1 = new Date();
    var d2 = new Date(d1);
    d2.setDate(d1.getDate() + 14);

    d1Month = d1.getMonth() + 1;
    if (d1Month < 10) {
        d1Month = "0" + d1Month;
    }

    d1Date = d1.getDate();
    if (d1Date < 10) {
        d1Date = "0" + d1Date;
    }

    d2Month = d2.getMonth() + 1;
    if (d2Month < 10) {
        d2Month = "0" + d2Month;
    }

    d2Date = d2.getDate();
    if (d2Date < 10) {
        d2Date = "0" + d2Date;
    }

    var startDate = d1.getFullYear() + "-" + d1Month + "-" + d1Date;
    var startTime = d1.getHours() + ":" + d1.getMinutes();

    var endDate = d2.getFullYear() + "-" + d2Month + "-" + d2Date;
    var endTime = d2.getHours() + ":" + d2.getMinutes();

    $("#StartDate").val(startDate);
    $("#StartTime").val(startTime);

    $("#EndDate").val(endDate);
    $("#EndTime").val(endTime);

    $("#lpaux").attr('checked', 'checked');
    $("#REndDate").val("");

    $("#Address").change(registerLpaux.addressChange);
    $("#Lat").change(registerLpaux.coordinateChange);
    $("#Lon").change(registerLpaux.coordinateChange);
    $("#accept").change(registerLpaux.enableSubmit);
})


function RegisterLpAux(bingMapObject, registerCommonObject) {

    var bingMap = bingMapObject;
    var pushPinIcon = "/Content/Images/PushPin.png";
    var self = this;
    var registerCommon = registerCommonObject;
    var requiredControls = [$("#responsibleParty"), $("#contactPhone"), $("#lat"), $("#long"), $("#CallSign"), $("#StartDate"), $("#StartTime"), $("#EndDate"), $("#EndTime")];
    var channelsArray = [];


    self.addressChange = function () {
        $("#lat").val("");
        $("#long").val("");
    };

    self.coordinateChange = function () {
        $("#Address").val("");
    };

    self.OnGetLocationClick = function (e) {
        $("#errAddress").addClass("hide");
        $("#errAddress").text("");

        if ($("#Address").val() == "" && ($("#lat").val() == "" || $("#long").val() == "")) {
            $("#errAddress").removeClass("hide");
            $("#errAddress").text("Please enter valid address or coordinates");
        }
        else {
            var locationFinder = new LocationFinder($("#lat"), $("#long"), $("#Address"), bingMap);
            var latitude = $("#lat").val();
            var longitude = $("#long").val();
            locationFinder.GetLocationDetails(self.FindChannels);
        }
    }

    self.FindChannels = function () {
        var latitude = $("#lat").val();
        var longitude = $("#long").val();

        var url = "/RegisterLicensedLpAux/GetChannelList?latitude=" + latitude + "&longitude=" + longitude;

        var data = { "latitude": latitude, "longitude": longitude };
        $('.preloader').show();
        ServerManager.AjaxRequest(
          {
              url: url,
              data: data,
              type: "POST",
              success: self.OnCallSuccess,
              error: self.OnError
          });
    }

    self.OnError = function (error) {
        var v = error;
        $('#divChannels').removeClass("hide");
        $('.preloader').hide();
    }

    self.OnCallSuccess = function (result, status, xhr) {
        if (result.error) {
            ServerManager.handleError(result, null);
        }

        if ($('#divChannels').hasClass("hide")) {
            $('#divChannels').removeClass("hide");
        }
        else {
            $('#divChannels li').each(function () {
                $(this.children[0]).attr({ 'disabled': 'disabled', 'checked': false });
            })
        }

        $(result).each(function () {
            var id = "#chk" + this.ChannelId;
            $(id).attr({ 'disabled': false });
        });
        $('.preloader').hide();
    };

    self.beforeSubmit = function () {
        var selectedDays = "";

        $(".error").addClass("hide");
        $("#error").removeClass("hide");
        $("#error").text("");
        $("#success").text("");


        $('#lstDays input:checked').each(function () {
            selectedDays = selectedDays + $(this).val() + ",";
        });

        selectedDays = selectedDays.substr(0, selectedDays.length - 1);
        $("#hdnWeekDays").val(selectedDays);

        var formValid = self.IsFormValid();
        if (formValid) {
            var LicensedLpAuxRegistration = {
                "Latitude": $("#lat").val(),
                "Longitude": $("#long").val(),
                "StartTime": $("#StartTime").val(),
                "EndTime": $("#EndTime").val(),
                "StartDate": $("#StartDate").val(),
                "EndDate": $("#EndDate").val(),
                "IsRecurred": $('#hdnIsReOccured').val(),
                "ReoccurrenceEndDate": $('#REndDate').val(),
                "IsReoccurenceDaily": $('#hdnIsReoccurenceDaily').val(),
                "Name": $("#Name").text(),
                "Address1": $("#Address1").text(),
                "City": $("#City").text(),
                "Email": $("#Email").text(),
                "Phone": $("#Phone").text(),
                "CallSign": $("#CallSign").val(),
                "WeekDaysString": $("#hdnWeekDays").val(),
                "ReoccurenceInstance": $("#REndInstance").val(),
                "Channels": channelsArray,
                "FriendlyName": $("#FriendlyName").val(),
                "Description": $("#Description").val(),                
                "ResponsibleParty": $("#responsibleParty").val(),
                "ContactPhone": $("#contactPhone").val()
            };

            var url = "/RegisterLicensedLpAux/Register";
            registerCommon.registerEntity(url, LicensedLpAuxRegistration);
        }
    };

    self.IsFormValid = function () {
        var isAllRequiredFieldsFilled = registerCommon.validateForm(requiredControls);
        var areDatesValid = self.areDatesValid();
        var isChannelsValid = self.validateChannels();
        var istimeValid = registerCommon.validateReoccurence();

        var isValidContactPhone = self.validatePhoneContact();

        if (isAllRequiredFieldsFilled && areDatesValid && isChannelsValid && istimeValid && isValidContactPhone) {
            return true;
        }
        else {
            return false;
        }
    };

    self.validatePhoneContact = function () {
        var contactNumber = $("#contactPhone").val();

        if (contactNumber.length > 0) {
            if (!contactNumber.match(/^\(?([0-9]{3})\)?[- ]?([0-9]{3})[- ]?([0-9]{4})$/)) {
                $("#errcontactPhone").removeClass("hide");
                $('#errcontactPhone').text("Invalid Contact Phone number");

                return false;
            }
        }

        return true;
    }

    self.validateChannels = function () {
        channelsArray = [];
        $('#channelList input:checked').each(function () {
            channelsArray.push($(this).val());
        });

        var error = false;

        if (channelsArray.length <= 0) {
            error = true;
        }
        else if (channelsArray.length > 3) {
            $("#errChannels").text("can't select more than 3 channels");
            error = true;
        }

        if (error) {
            $("#errChannels").removeClass("hide");
            return false;
        }

        return true;
    };

    self.areDatesValid = function () {
        var startDateValid = registerCommon.validateDate($("#StartDate").val());
        if (!startDateValid) {
            $("#errStartDate").removeClass("hide");
            $("#errStartDate").text("Invalid Date");

            return false;
        }

        var endtDateValid = registerCommon.validateDate($("#EndDate").val());
        if (!startDateValid) {
            $("#errEndDate").removeClass("hide");
            $("#errEndDate").text("Invalid Date");

            return false;
        }

        return true;
    };

    self.enableSubmit = function () {
        if (this.checked) {
            $("#btnSubmit")[0].disabled = false;
        }
        else {
            $("#btnSubmit")[0].disabled = true;
        }
    }
}
