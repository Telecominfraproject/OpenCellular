// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

$(document).ready(function () {

    $('#tempBAS').prop('checked', true);   

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
    d2.setHours(d1.getHours() + 720);

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

    $("#tempBAS").attr("checked", "checked");

    var registerCommon = new RegisterCommon();
    var registerTBas = new RegisterTBas(bingMap, registerCommon);

    setTimeout(function () {
        bingMap.ZoomtoLocation(39.443256, -98.957336, 4);
    }, 500);

    $("#lnkGetRLocation").click(registerTBas.GetRecieverLocation);
    $("#lnkGetTLocation").click(registerTBas.GetTransmitterLocation);
    $("#submit").click(registerTBas.submitRegister);
    $("#accept").change(registerTBas.enableSubmit);
    $("#CallSign").focusout(registerTBas.getContours);
});


function RegisterTBas(bingMapObject, registerCommonObject) {

    var bingMap = bingMapObject;
    var pushPinIcon = "/Content/Images/PushPin.png";
    var registerCommon = registerCommonObject;
    var requiredControls = [$("#FriendlyName"), $("#tLat"), $("#tLong"), $("#rLat"), $("#rLong"), $("#CallSign"), $("#StartDate"), $("#StartTime"), $("#EndDate"), $("#EndTime")];
    var self = this;
    channelsArray = [];

    self.GetTransmitterLocation = function (e) {
        
        $("#errtAddress").removeClass("hide");
        $("#errtAddress").text("");

        if ($("#tAddress").val() == "" && ($("#tLong").val() == "" || $("#tLong").val() == "")) {
            $("#errtAddress").removeClass("hide");
            $("#errtAddress").text("Please enter valid address or coordinates");
        }
        else {
            var locationFinder = new LocationFinder($("#tLat"), $("#tLong"), $("#tAddress"), bingMap);
            locationFinder.GetLocationDetails(self.ShowLocations);
        }
    }

    self.GetRecieverLocation = function (e) {

        $("#errrAddress").removeClass("hide");
        $("#errrAddress").text("");

        if ($("#rAddress").val() == "" && ($("#rLat").val() == "" || $("#rLong").val() == "")) {
            $("#errrAddress").removeClass("hide");
            $("#errrAddress").text("Please enter valid address or coordinates");
        }
        else {
            var locationFinder = new LocationFinder($("#rLat"), $("#rLong"), $("#rAddress"), bingMap);
            locationFinder.GetLocationDetails(self.ShowLocations);
        }
    }

    self.ShowLocations = function () {

        bingMap.ClearMapEntities();

        var tlatitude = $("#tLat").val();
        var tlongitude = $("#tLong").val();

        if (tlatitude != "" && tlongitude != "") {
            registerCommon.locateOnMap(tlatitude, tlongitude, "Transmitter Location", bingMap)
        }

        var rlatitude = $("#rLat").val();
        var rlongitude = $("#rLong").val();

        if (rlatitude != "" && rlongitude != "") {
            registerCommon.locateOnMap(rlatitude, rlongitude, "Reciever Location", bingMap)
        }
    }

    self.submitRegister = function () {

        $(".error").addClass("hide");
        $("#error").removeClass("hide");
        $("#error").text("");
        $("#success").text("");        

        var isFormValid = self.IsFormValid();
        if (isFormValid) {
            var TBasLinkRegistration = {
                "CallSign": $("#CallSign").val(),
                "TransmitterLatitude": $("#tLat").val(),
                "TransmitterLongitude": $("#tLong").val(),
                "RecieverLatitude": $("#rLat").val(),
                "RecieverLongitude": $("#rLong").val(),
                "StartDate": $("#StartDate").val(),
                "EndDate": $("#EndDate").val(),
                "StartTime": $("#StartTime").val(),
                "EndTime": $("#EndTime").val(),
                "Address1": $("#Address1").text(),
                "Address2": $("#Address2").text(),
                // "Phone": $("#Phone").text(),
                "Email": $("#Email").text(),
                "City": $("#City").text(),
                "Country": $("#Country").text(),
                //"State": $("#State").text(),
                "FriendlyName": $("#FriendlyName").val(),
                "Description": $("#Description").val(),
                "Name": $("#Name").text(),
                "Channels": channelsArray,
            };

            url = "/RegisterTBasLink/Register";
            registerCommon.registerEntity(url, TBasLinkRegistration);
        }
    };

    self.IsFormValid = function () {
        var isAllRequiredFieldsFilled = registerCommon.validateForm(requiredControls);
        var areDatesValid = self.areDatesValid();
        var isChannelsValid = self.validateChannels();       

        if (isAllRequiredFieldsFilled && areDatesValid && isChannelsValid ) {
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
            $("#errChannels").text("Please select at least one Channel");
            error =  true;
        }
        else if (channelsArray.length > 5) {           
            $("#errChannels").text("Can't select more than 5 channels");
            error = true;
        }

        if (error)
        {
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

        var startTimeStr = $("#StartTime").val();
        var endTimeStr = $("#EndTime").val();
        var startDate = new Date($("#StartDate").val());
        var endDate = new Date($("#EndDate").val());

        var validationMessage = "";
        var error = false;

        startTimeArray = startTimeStr.split(':');
        endTimeArray = endTimeStr.split(':')

        var startTime = new Date();
        startTime.setHours(startTimeArray[0], startTimeArray[1], 0, 0);

        var endTime = new Date();
        endTime.setHours(endTimeArray[0], endTimeArray[1], 0, 0);

        if (startDate >= endDate && startDate <= endDate) {
            if (endTime.getTime() <= startTime.getTime()) {
                validationMessage = "End Time should be greater than Start Time";
                error = true;
            }
        }
        else if (endDate < startDate) {
            validationMessage = "End Date should be greater than Start Date";
            error = true;
        }

        if (error)
        {
            $("#errDate").text(validationMessage);
            $("#errDate").removeClass("hide");

            return false;
        }

        return true;
    };

    self.enableSubmit = function () {
        if (this.checked) {
            $("#submit")[0].disabled = false;
        }
        else {
            $("#submit")[0].disabled = true;
        }
    }

    self.getContours = function()
    {
        var callsign = this.value;
        channelsArray = [];

        $('#channelList input:checked').each(function () {
            channelsArray.push($(this).val());
        });

        var url = "/RegisterTBasLink/GetContourData";

        var data = { "callsign": callsign, "channels":JSON.stringify(channelsArray) };
        ServerManager.AjaxRequest(
          {
              url: url,
              data: data,
              type: "GET",
              contentType: "application/json; charset=utf-8",
              cache: false,
              success: self.getContourSuceess,
              error: self.onError
          });

    }

    self.getContourSuceess = function (result, status, xhr) {
        if (result.error) {
            ServerManager.handleError(result, null);
        }

        if (result.length > 0) {
            //Following code will be re used once we get contours
            var vertices = [];
            $(result).each(function () {
                vertices.push(new Microsoft.Maps.Location(this.Latitude, this.Longitude));
            });

            var color = Microsoft.Maps.Color.fromHex("#ee8484");

            var polygonOptions = {
                fillColor: new Microsoft.Maps.Color(50, color.r, color.g, color.b),
                strokeColor: color,
                strokeThickness: parseInt(1)
            };

            bingMap.DrawPolygon(vertices, polygonOptions);
        }
    };

    self.getContourError = function (error) {
        var v = error;
    };

}
