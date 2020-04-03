// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

$(document).ready(function () {

    $('#mvpd').prop('checked', true);

    //bingMap instance comes from MapPartial
    var registerMvpd = new RegisterMvpd(bingMap);
        
    setTimeout(function () {
        bingMap.ZoomtoLocation(39.443256, -98.957336, 4);
    }, 500);

    $("#lnkGetLocation").click(registerMvpd.OnGetLocationClick);
    $("#ddlCallSign").change(registerMvpd.GetChannel);

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
    var endDate = d2.getFullYear() + "-" + d2Month + "-" + d2Date;

    $("#StartDate").val(startDate);

    $("#EndDate").val(endDate);

    $("#mvpd").prop("checked", "checked");

    $("#submit").click(registerMvpd.onSubmit);

    $("#MAddress").change(registerMvpd.addressChange);
    $("#mLat").change(registerMvpd.coordinateChange);
    $("#mLon").change(registerMvpd.coordinateChange);
    $("#accept").change(registerMvpd.enableSubmit);

    $("#txtCallsign").change(registerMvpd.callsignChange);
});

function RegisterMvpd(bingMapObject) {
    var bingMap = bingMapObject;
    var callsignArray = null;
    var pushPinIcon = "/Content/Images/PushPin.png";
    var self = this;
    var isFormValid = true;
    var requiredControls = [$("#txtCallsign"), $("#mLat"), $("#mLon"), $("#tLat"), $("#tLong"), $("#mvpdCompanyName")];
    var registerCommon = new RegisterCommon();

    self.addressChange = function () {
        $("#mLat").val("");
        $("#mLon").val("");
    };
    self.coordinateChange = function () {
        $("#MAddress").val("");
    };    

    self.OnGetLocationClick = function (e) {
        $('#ddlCallSign').empty();
        $("#errLoadData").addClass("hide");
        $("#errMAddress").addClass("hide");
        $("#errMAddress").text("");

        if ($("#MAddress").val() == "" && ($("#mLat").val() == "" || $("#mLon").val() == "")) {
            $("#errMAddress").removeClass("hide");
            $("#errMAddress").text("Please enter valid address or coordinates");
        }
        else {
            var locationFinder = new LocationFinder($("#mLat"), $("#mLon"), $("#MAddress"), bingMap);
            locationFinder.GetLocationDetails();

            $("#tLat").val("");
            $("#tLong").val("");
            $("#txtChannel").val("");
            $("#txtCallsign").val("");
        }
    };

    self.LoadCallSigns = function () {
        $('.preloader').show();
        var latitude = $("#mLat").val();
        var longitude = $("#mLon").val();

        var url = "/RegisterMVPD/GetCallSignInfo";

        var data = { "latitude": latitude, "longitude": longitude };
        ServerManager.AjaxRequest(
          {
              url: url,
              data: data,
              type: "POST",
              success: self.OnCallSuccess,
              error: self.OnError
          });
    };

    self.OnCallSuccess = function (result, status, xhr) {

        if (result.error) {
            ServerManager.handleError(result, null);
        }

        callsignArray = result;
        var ddl = $('#ddlCallSign');

        $(document.createElement('option'))
                 .attr('value', "Select CallSign")
                 .text("Select CallSign")
                 .appendTo(ddl);

        $(result).each(function () {
            $(document.createElement('option'))
               .attr('value', this.CallSign)
               .text(this.CallSign)
               .appendTo(ddl);
        });

        $('.preloader').hide();
    };

    self.OnError = function (error) {
        $("#errLoadData").removeClass("hide");
        $('.preloader').hide();
    }

    self.callsignChange = function () {
        var requestedCallsign = $("#txtCallsign").val();
        if (requestedCallsign != "") {
            $("#txtCallsign").val(requestedCallsign.toUpperCase());
            $("#tLat").val("");
            $("#tLong").val("");
            $("#txtChannel").val("");
            $("#errLoadData").addClass("hide");
            $('.preloader').show();

            var url = "/RegisterMVPD/GetMVPDCallsignInfo";

            var data = { "callsign": requestedCallsign.toUpperCase() };

            ServerManager.AjaxRequest(
              {
                  url: url,
                  data: data,
                  type: "POST",
                  success: self.OnInfoAvailable,
                  error: self.OnError
              });
        }
    };

    self.OnInfoAvailable = function (result, status, xhr) {
        if (result.error) {
            //ServerManager.handleError(result, null);
            $("#errLoadData").removeClass("hide");
        } else {

            if (result != null && result != "") {
                $('#txtChannel').val(result.Channel);
                self.ShowCoverageArea(result.Latitude, result.Longitude);
                //self.drawcontours($("#txtCallsign").val(), result.Channel);
                var contours = jQuery.parseJSON(result.Contour);
                self.getContourSuceess(contours.contourPoints, null, null);

                bingMap.ZoomtoLocation(result.Latitude, result.Longitude, 7);
            }
            else {
                $("#errLoadData").removeClass("hide");
            }
        }
        $('.preloader').hide();
    };

    self.GetChannel = function (e) {
        var selectedValue = this.value;

        if (selectedValue != "Select CallSign") {
            $(callsignArray).each(function () {
                if (this.CallSign == selectedValue) {
                    $('#txtChannel').val(this.Channel);
                    self.ShowCoverageArea(this.Latitude, this.Longitude);
                    self.drawcontours(selectedValue, this.Channel);
                    return;
                }
            });
        }
    };

    self.ShowCoverageArea = function (latitude, longitude) {

        //Following code will be re used once we get contours
        //var vertices = [];
        //$(contour.ContourPoints).each(function () {
        //    vertices.push(new Microsoft.Maps.Location(this.Latitude, this.Longitude));
        //});

        //bingMap.DrawPolygon(vertices);
        bingMap.ClearMapEntities();

        var location = new Microsoft.Maps.Location(latitude, longitude);
        $('#tLat').val(latitude);
        $('#tLong').val(longitude);
        registerCommon.locateOnMap(latitude, longitude, "Transmitter Location", bingMap)


        var rlatitude = $("#mLat").val();
        var rlongitude = $("#mLon").val();

        if (rlatitude != "" && rlongitude != "") {
            registerCommon.locateOnMap(rlatitude, rlongitude, "Receiver Location", bingMap)
        }
    };

    self.drawcontours = function (callsign, channel) {
        var url = "/RegisterMVPD/GetContourData";

        var data = { "callsign": callsign, "channel": channel };
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
    };

    self.getContourSuceess = function (result, status, xhr) {        
        if (result.length > 0) {
            //Following code will be re used once we get contours
            var vertices = [];
            $(result).each(function () {
                vertices.push(new Microsoft.Maps.Location(this.latitude, this.longitude));
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

    self.onSubmit = function () {
        $(".error").addClass("hide");
        $("#error").removeClass("hide");
        $("#error").text("");
        $("#success").text("");

        isValid = self.ValidateInputs();

        if (isValid) {
            var MVPDRegisterViewModel = {
                "FriendlyName": $("#FriendlyName").val(),
                "Description": $("#Description").val(),
                "MVPDLocationLatittude": $("#mLat").val(),
                "MVPDLocationLongitude": $("#mLon").val(),
                "TransmitterLatittude": $("#tLat").val(),
                "TransmitterLongitude": $("#tLong").val(),
                "Channel": $("#txtChannel").val(),
                "CableCompanyName": $("#mvpdCompanyName").val(),
                "CallSign": $("#txtCallsign").val(),
                //"StartDate": $("#StartDate").val(),
                //"EndDate": $("#EndDate").val(),
                "Name": $("#Name").text(),
                "Address1": $("#Address1").text(),
                "Address2": $("#Address2").text(),
                "City": $("#City").text(),
                "Country": $("#Country").text(),
                "Email": $("#Email").text(),
                "Phone": $("#Phone").text()
            };

            var url = "/RegisterMVPD/Register";
            registerCommon.registerEntity(url, MVPDRegisterViewModel);
        }
    };

    self.ValidateInputs = function () {
        if (!registerCommon.validateForm(requiredControls)) {
            return false;
        }
        else {
            return true;
        }

        //var isValid = true;
        //var startDate = new Date($("#StartDate").val());
        //var endDate = new Date($("#EndDate").val());

        //if (!registerCommon.validateDate($("#StartDate").val())) {
        //    $("#errStartDate").removeClass("hide");
        //    $("#errStartDate").text("Invalid Date");
        //    isValid = false;
        //}

        //if (!registerCommon.validateDate($("#EndDate").val())) {
        //    $("#errEndDate").removeClass("hide");
        //    $("#errEndDate").text("Invalid Date");
        //    isValid = false;
        //}

        //if (isValid == false) {
        //    return false;
        //}

        //if (startDate > endDate) {
        //    $("#errDate").removeClass("hide");
        //    $("#errDate").text("Start Date should be less than End Date")
        //    return false;
        //}

        //return isValid;
    }

    self.enableSubmit = function () {
        if (this.checked) {
            $("#submit")[0].disabled = false;
        }
        else {
            $("#submit")[0].disabled = true;
        }
    }
}
