// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

function LocationFinder(latitudeControl, longitudeControl, addressControl, bingMapObject) {
    var LatitudeControl = latitudeControl;
    var LongitudeControl = longitudeControl;
    var AddressControl = addressControl;
    var CallBack = null;
    var bingMap = bingMapObject;

    this.latitude = LatitudeControl.val();
    this.longitude = LongitudeControl.val();
    this.address = AddressControl.val();
    var pushPinIcon = "/Content/Images/PushPin.png";
    function IsInputValid() {
        if ((this.latitude == "" && this.longitude == "") && (this.address == "")) {
            return false;
        }
        else if (this.address != "") {
            return true;
        }
        else {
            if ((this.latitude != "") && (this.longitude != "")) {
                return true;
            }
            else {
                return false;
            }
        }
    }

    this.GetLocationDetails = function (callbackFunction) {
        var isValid = IsInputValid();
        if (isValid == true) {
            var locationInput = this.address != "" ? this.address : this.latitude + "," + this.longitude;

            var searchRequest = {
                where: locationInput,
                callback: onLocationFound,
                errorCallback: onLocationSearchFailed,
                userData: { callback: onSearchSuccess }
            }

            bingMap.locationFinder(searchRequest, bingMap.SearchRequestType.searchRequest);
            CallBack = callbackFunction;
        }
        else {
            //alert("Please enter coordinates or address");
        }
    };

    var onLocationFound = function (searchResult, userData) {
        if (searchResult) {
            var searchRegion = searchResult.searchRegion;

            if (searchRegion) {

                if (searchRegion.matchCode == Microsoft.Maps.Search.MatchCode.none) {
                    var errorControlId = "#" + "err" + AddressControl[0].id;
                    $(errorControlId).removeClass("hide");
                    $(errorControlId).text("Invalid Search");

                    return;
                }

                if (searchRegion.address.countryRegion) {
                    userData.callback(
                        searchRegion.explicitLocation.location.latitude,
                        searchRegion.explicitLocation.location.longitude,
                        searchRegion.address);
                }
                else {
                    var reverseGeocoderequest = {
                        location: searchRegion.explicitLocation.location,
                        count: 10,
                        callback: geocodeCallback,
                        errorCallback: onSerachFailed,
                    }

                    bingMap.locationFinder(reverseGeocoderequest, bingMap.SearchRequestType.reverseGeocodeRequest);
                }
            }
            else {
                var errorControlId = "#" + "err" + AddressControl[0].id;
                $(errorControlId).removeClass("hide");
                $(errorControlId).text("Invalid Search");
            }
        }
    };

    var geocodeCallback = function (result, userData) {
        if (result.name) {
            var searchRequest = {
                where: result.name,
                callback: onLocationFound,
                errorCallback: onLocationSearchFailed,
                userData: { callback: onSearchSuccess }
            }

            bingMap.locationFinder(searchRequest, bingMap.SearchRequestType.searchRequest)
        }
        else {
            // Unknown location.
        }
    };

    var onSearchSuccess = function (latitude, longitude, address) {
        LatitudeControl.val(latitude);
        LongitudeControl.val(longitude);
        if (AddressControl.val() == "") {
            AddressControl.val(address.formattedAddress);
        }

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
            title: "Receive Location",
            offset: new Microsoft.Maps.Point(10, 0),
            pushpin: pushpin,

        };

        bingMap.addInfobox(location, infoboxOptions);

        bingMap.ZoomtoLocation(latitude, longitude, 10);

        if (CallBack != null) {
            CallBack(latitude, longitude);
        };
    };

    var onLocationSearchFailed = function (error) {
        //unimplemented
    };

    function onSerachFailed(result, userData) {
        //TODO: Display error messages here.
    }
}


$(function () {
    $('#registerDiv').perfectScrollbar({
        wheelSpeed: 20,
        wheelPropagation: true,
        minScrollbarLength: 20
    });
});


function RegisterCommon() {
    var self = this;
    var pushPinIcon = "/Content/Images/PushPin.png";

    self.validateForm = function (requiredControls) {
        isValid = true;
        $(requiredControls).each(function () {
            var errId = "#err" + this.attr('id');
            if (this.val() == "" || this.val() == null) {
                isValid = false;
                $(errId).removeClass("hide");
                $(errId).text("Required");
            }
            else {
                if (!$(errId).hasClass("hide")) {
                    $(errId).addClass("hide");
                }
            }
        });

        return isValid;
    };

    self.locateOnMap = function (latitude, longitude, title, bingMap) {
        var location = new Microsoft.Maps.Location(latitude, longitude);
        var pushpinOptions = {
            zIndex: 100,
            icon: pushPinIcon,
            height: 36,
            width: 26
        };

        var pushpin = bingMap.addPushPin(location, pushpinOptions);
        var infoboxOptions = {
            title: title,
            offset: new Microsoft.Maps.Point(10, 0),
            pushpin: pushpin,
            showPointer: true
        };

        bingMap.addInfobox(location, infoboxOptions);

        bingMap.ZoomtoLocation(latitude, longitude, 10);
    };

    self.registerEntity = function (url, data) {
        $('.preloader').show();
        ServerManager.AjaxRequest(
               {
                   url: url,
                   data: JSON.stringify(data),
                   type: "POST",
                   dataType: "json",
                   contentType: "application/json; charset=utf-8",
                   success: self.registerSuccess,
                   error: self.registerError
               });
    };

    self.registerSuccess = function (result, status, xhr) {
        if (result.error) {
            ServerManager.handleError(result, $("#error"));
        } else {
            $("#success").text(result.Message);
        }

        $('.preloader').hide();
    };

    self.registerError = function (error) {
        //alert(error);

        if (error.type) {
            $("#error").text(result.message);
        }


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
    };

    self.validateReoccurence = function () {
        var isReoocurence = ($('#hdnIsReOccured').val()).toLowerCase();
        var isReoccurenceDaily = ($('#hdnIsReoccurenceDaily').val()).toLowerCase();

        var startTimeStr = $("#StartTime").val();
        var endTimeStr = $("#EndTime").val();
        var startDate = new Date($("#StartDate").val());
        var endDate = new Date($("#EndDate").val());
        var stopDate = new Date($("#REndDate").val());

        var validationMessage = "";

        startTimeArray = startTimeStr.split(':');
        endTimeArray = endTimeStr.split(':')

        var startTime = new Date();
        startTime.setHours(startTimeArray[0], startTimeArray[1], 0, 0);

        var endTime = new Date();
        endTime.setHours(endTimeArray[0], endTimeArray[1], 0, 0);

        if (isReoocurence == "true") {

            if (startDate.getTime() != endDate.getTime()) {
                return self.showReoccurenceError("Both dates should be same");
            }

            if (endTime.getTime() <= startTime.getTime()) {
                return self.showReoccurenceError("End Time should be greater than Start Time");
            }

            if (isReoccurenceDaily == "true") {
                if ($('#stopInstance').is(':checked')) {
                    if (parseInt($("#REndInstance").val()) <= 0) {
                        return self.showReoccurenceError("Recurrence instance should be greater than 0");
                    }
                }
                if ($('#StopDate').is(':checked')) {
                    if ($("#REndDate").val() == "") {
                        return self.showReoccurenceError("Enter recurrence stop Date");
                    }

                    if (startDate > stopDate) {
                        return self.showReoccurenceError("Recurrence stop date should be greater than start date");
                    }
                }
            }
            else {
                if ($('#lstDays input:checked').length <= 0) {
                    return self.showReoccurenceError("Please select week days");
                }
                if ($('#stopInstance').is(':checked')) {
                    if (parseInt($("#REndInstance").val()) <= 0) {
                        return self.showReoccurenceError("Recurrence instance should be greater than 0");
                    }
                }
                if ($('#StopDate').is(':checked')) {
                    if ($("#REndDate").val() == "") {
                        return self.showReoccurenceError("Enter Recurrence stop Date");
                    }

                    if (startDate > stopDate) {
                        return self.showReoccurenceError("Recurrence stop date should be greater than start date");
                    }
                }
            }
        }

        else {
            if (startDate >= endDate && startDate <= endDate) {
                if (endTime.getTime() <= startTime.getTime()) {
                    return self.showReoccurenceError("End Time should be greater than Start Time");
                }
            }
            else if (endDate < startDate) {
                return self.showReoccurenceError("End Date should be greater than Start Date");
            }
        }

        return true;
    };

    self.showReoccurenceError = function (message) {
        $("#errReoccurence").removeClass("hide");
        $("#errReoccurence").text(message);
        return false;
    }

    self.validateDate = function (date) {
        var currVal = date;
        if (currVal == '')
            return false;
        //Declare Regex 
        var rxDatePattern = /^(\d{4})(\/|-)(\d{1,2})(\/|-)(\d{1,2})$/;
        var dtArray = currVal.match(rxDatePattern); // is format OK?
        if (dtArray == null)
            return false;
        //Checks for yyyy-mm-dd format.
        dtMonth = dtArray[3];
        dtDay = dtArray[5];
        dtYear = dtArray[1];
        if (dtMonth < 1 || dtMonth > 12)
            return false;
        else if (dtDay < 1 || dtDay > 31)
            return false;
        else if ((dtMonth == 4 || dtMonth == 6 || dtMonth == 9 || dtMonth == 11) && dtDay == 31)
            return false;
        else if (dtMonth == 2) {
            var isleap = (dtYear % 4 == 0 && (dtYear % 100 != 0 || dtYear % 400 == 0));
            if (dtDay > 29 || (dtDay == 29 && !isleap))
                return false;
        }

        return true;
    }

}
