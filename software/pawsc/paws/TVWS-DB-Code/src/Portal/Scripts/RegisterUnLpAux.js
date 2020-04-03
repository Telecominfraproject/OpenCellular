// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

$(document).ready(function () {
    //$(".multiselect").multiselect();

    $('#wlessMic').prop('checked', true);

    var registerCommon = new RegisterCommon();
    var registerUnLpaux = new RegisterUnLpAux(bingMap, registerCommon);

    setTimeout(function () {
        bingMap.ZoomtoLocation(39.443256, -98.957336, 4);
    }, 500);

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

    $("#wlessMic").attr('checked', 'checked');

    $("#btnLookUp").click(registerUnLpaux.getUlsFileDetails);

    $('#weekly').change(registerCommon.showWeekDays);
    $('#daily').change(registerCommon.showWeekDays);
    $("#REndDate").datepicker({
        dateFormat: "yy-mm-dd",
        minDate: new Date()
    });
    $("#REndDate").val("");
    $("#lnkReoccurence").click(registerCommon.showReoccurence);
    $('#stopInstance').change(registerCommon.updteReoccurenceValues);
    $('#StopDate').change(registerCommon.updteReoccurenceValues);
    $("#btnSubmit").click(registerUnLpaux.onSubmit);
    $("#accept").change(registerUnLpaux.enableSubmit);
    $("#ulsFileNumber").keypress(registerUnLpaux.keypress);
})

function RegisterUnLpAux(bingMapObject, registerCommonObject) {
    var bingMap = bingMapObject;
    var registerCommon = registerCommonObject;
    var pushPinIcon = "/Content/Images/PushPin.png";
    var self = this;
    var requiredControls = [$("#ulsFileNumber"), $("#OrgName"), $("#Phone"), $("#StartDate"), $("#StartTime"), $("#EndDate"), $("#EndTime"), $("#Venue")];
    var channelsArray = [];

    self.ulsData = {
        "CallSign": "",
        "ContactEntityName": "",
        "ContactPhone": "",
        "ContactEmail": "",
        "ContactStreetAddress": "",
        "ContactCity": "",
        "ContactState": "",
        "ULSFileNumber": "",
        "GrantDate": "",
        "ExpireDate": "",
        "VenueName": "",
        "Latitude": "",
        "Longitude": "",
        "Channels": ""
    };

    self.keypress = function (e) {
        if (e.which === 13) {
            e.preventDefault();
            self.getUlsFileDetails();
        }
    };

    self.OnGetLocationClick = function (e) {

        var latitude = $("#lat").text();
        var longitude = $("#long").text();

        var location = new Microsoft.Maps.Location(latitude, longitude);
        var pushpinOptions = {
            zIndex: 100,
            icon: pushPinIcon,
            height: 36,
            width: 26
        };

        bingMap.ClearMapEntities();

        var pushpin = bingMap.addPushPin(location, pushpinOptions);
        var infoboxOptions = {
            title: "MicroPhone Location",
            offset: new Microsoft.Maps.Point(10, 0),
            pushpin: pushpin,

        };

        bingMap.addInfobox(location, infoboxOptions);       

        //draw a circle around location
        var color = Microsoft.Maps.Color.fromHex("#ee8484");

        var polygonOptions = {
            fillColor: new Microsoft.Maps.Color(50, color.r, color.g, color.b),
            strokeColor: color,
            strokeThickness: parseInt(1)
        };

        var center = new Microsoft.Maps.Location(latitude, longitude);
        bingMap.DrawCircle(center, 1000, 1000, polygonOptions);
        bingMap.ZoomtoLocation(latitude, longitude, 14);

        self.FindChannels(latitude, longitude);
    };

    self.getUlsFileDetails = function () {

        if (!$('#errUlsnumber').hasClass('hide')) {
            $('#errUlsnumber').addClass('hide');
        }

        var ulsnumber = $("#ulsFileNumber").val();
        if (ulsnumber != "") {
            var url = "/RegisterUnLicensedLpAux/GetUlsFileDetails";

            var data = { "ulsFileNumber": ulsnumber };
            $('.preloader').show();
            ServerManager.AjaxRequest(
              {
                  url: url,
                  data: data,
                  type: "POST",
                  success: self.populateUlsFileData,
                  error: self.OnError
              });
        }
        else {
            //alert("Please enter UlsFileNumber");
        }
    };

    self.populateUlsFileData = function (result, status, xhr) {
        if (result.error) {
            ServerManager.handleError(result, null);
        }

        if (!$('#divChannels').hasClass("hide")) {
            $(self.ulsData.Channels).each(function () {
                var id = "#chk" + this;
                $(id).attr({ 'checked': false });
            });
        }


        if (result == null || result == "") {
            alert("No Data Found");
        }
        else {
            $("#ulsDetails").removeClass("hide");
            self.ulsData = result;

            var name = result.ContactEntityName == "" ? result.ContactFirstName+" " + result.ContactLastName : result.ContactEntityName;
            $("#Name").text(name);
            $("#Address1").text(result.ContactStreetAddress);
            $("#City").text(result.ContactCity);            
            $("#Email").text(result.ContactEmail);
            $("#Phone").text(result.ContactPhone);
            $("#GrantDate").text(result.GrantDate);
            $("#ExpireDate").text(result.ExpireDate);
            $("#lat").text(result.Latitude);
            $("#long").text(result.Longitude);

            var address = result.ContactStreetAddress + ", " + result.ContactCity + ", ";
            address = address + result.ContactState + " " + result.ContactZipCode;
            $("#Address").text(address);

            utcgrantDate = result.GrantDate.replace("/Date(", '').replace(')/', '');
            utcexpirationDate = result.ExpireDate.replace("/Date(", '').replace(')/', '');

            var grantDate = new Date(parseInt(utcgrantDate));
            var expirationDate = new Date(parseInt(utcexpirationDate));
            $("#GrantDate").text(grantDate.getFullYear() + "-" + grantDate.getMonth() + "-" + grantDate.getDate());
            $("#ExpireDate").text(expirationDate.getFullYear() + "-" + expirationDate.getMonth() + "-" + expirationDate.getDate());

            self.OnGetLocationClick();
        }
    };

    self.FindChannels = function (latitude, longitude) {

        var url = "/RegisterUnLicensedLpAux/GetChannelList";

        var data = { "latitude": latitude, "longitude": longitude };
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
        $('#errUlsnumber').removeClass('hide');

        if (!$('#divChannels').hasClass("hide")) {
            $('#divChannels').addClass("hide");
            $(self.ulsData.Channels).each(function () {
                var id = "#chk" + this;
                $(id).attr({ 'checked': false });
            });
        }
        if (!$('#ulsDetails').hasClass("hide")) {
            $('#ulsDetails').addClass("hide");
        }
        
        $('.preloader').hide();
    }

    self.OnCallSuccess = function (result, status, xhr) {
        $('#divChannels').removeClass("hide");

        $('#divChannels li').each(function () {
            $(this.children[0]).attr({ 'disabled': 'disabled' });
        });

        $(result).each(function () {
            var id = "#chk" + this.ChannelId;
            $(id).attr({ 'disabled': false });
        });


        $(self.ulsData.Channels).each(function () {
            var id = "#chk" + this;
            $(id).attr({ 'checked': 'checked', 'disabled': 'disabled' });
        });

        $('.preloader').hide();
    };

    self.showReoccurence = function () {
        $('#divReoccurence').toggleClass("hide");

        if ($('#divReoccurence').hasClass('hide')) {
            $('#hdnIsReOccured').val("false");
        }
        else {
            $('#hdnIsReOccured').val("true");
        }
    };

    self.showWeekDays = function () {
        $("#divWeekDays").toggleClass("hide");
        if ($('#daily').is(':checked')) {
            $('#hdnIsReoccurenceDaily').val("true");
        }
        else {
            $('#hdnIsReoccurenceDaily').val("false");
        }
    };

    self.updteReoccurenceValues = function () {
        if ($('#stopInstance').is(':checked')) {
            $("#REndDate").val("");
        }
        if ($('#StopDate').is(':checked')) {
            $("#REndInstance").val("0");
        }
    }

    self.onSubmit = function () {
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
        var isFormValid = self.IsFormValid();

        if (isFormValid) {
                var LicensedLpAuxRegistration = {
                    "Latitude": $("#lat").text(),
                    "Longitude": $("#long").text(),
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
                    "Phone": $("#Phone").val(),
                    "UlsFileNumber": $("#ulsFileNumber").val(),
                    "ChannelListString": $("#hdnChannelList").val(),
                    "WeekDaysString": $("#hdnWeekDays").val(),
                    "ReoccurenceInstance": $("#REndInstance").val(),
                    "VenueName": self.ulsData.VenueName,
                    "Channels": channelsArray,
                    "OrgName": $("#OrgName").val(),
                };

                var url = "/RegisterUnLicensedLpAux/Register";
                registerCommon.registerEntity(url, LicensedLpAuxRegistration);
            }       
    };

    self.IsFormValid = function () {
        var isAllRequiredFieldsFilled = registerCommon.validateForm(requiredControls);
        var areDatesValid = self.areDatesValid();
        var isChannelsValid = self.validateChannels();
        var istimeValid = registerCommon.validateReoccurence();

        if (isAllRequiredFieldsFilled && areDatesValid && isChannelsValid && istimeValid) {
            return true;
        }
        else {
            return false;
        }
    };

    self.validateChannels = function () {
        channelsArray = [];
        $('#channelList input:checked').each(function () {
            channelsArray.push($(this).val());
        });

        var error = false;

        if (channelsArray.length <= 0) {
            error = true;
        }
        else if (channelsArray.length > self.ulsData.Channels.length + 3) {
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
