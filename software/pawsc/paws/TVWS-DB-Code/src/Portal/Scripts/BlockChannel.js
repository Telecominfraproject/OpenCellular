// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

$(document).ready(function () {
    var blockChannel = new BlockChannel();

    $("#chkSelectAll").change(blockChannel.SelectAll);
    $("#location").change(blockChannel.showCoordinates);
    $("#region").change(blockChannel.showRegion);
    $("#ddlRegion").change(blockChannel.showChannels);
    $("#btnSubmit").click(blockChannel.submit);
    $("#AddRow").click(blockChannel.addRow);

    $("#tblCoordinates a").on('click', function (event) {
        blockChannel.deleteRow(this, event);
    });

    $('#blockedChannels').show();
    $('.blockAChannel').hide();

    $('#channel .sub-tab li input').on('click', function (e) {
        if ($(this).hasClass('blockedChannelBtn')) {
            $('#dvexcludedChannels').show();
            $('.blockAChannel').hide();
        }
        else if ($(this).hasClass('blockAChannelBtn'))
        {
            $('#dvexcludedChannels').hide();
            $('.blockAChannel').show();
        }
    });
})

var BlockChannel = function () {
    var self = this;
    var channels = [];

    self.SelectAll = function () {

        if ($("#ddlRegion").val() == "United States") {

            if (this.checked) {
                for (i = 2; i < 52; i++) {
                    if (!(i == 3 || i == 4 || i == 37)) {
                        id = "#chk" + i;
                        $(id)[0].checked = true;
                    }
                }
            }
            else {
                for (i = 2; i < 52; i++) {
                    id = "#chk" + i;
                    $(id).attr({ 'checked': false })
                }
            }
        }
        else {
            if (this.checked) {
                for (i = 21; i <= 60; i++) {
                    id = "#chkuk" + i;
                    $(id)[0].checked = true;
                }
            }
            else {
                for (i = 21; i <= 60; i++) {
                    id = "#chkuk" + i;
                    $(id).attr({ 'checked': false })
                }
            }
        }
    }

    self.showCoordinates = function () {
        $("#divCoordinates").removeClass("hide");
        $("#divRegion").addClass("hide");
        $("#AddRow").show();
    };

    self.showRegion = function () {
        $("#divCoordinates").addClass("hide");
        $("#divRegion").removeClass("hide");
        $("#AddRow").hide();
    }

    self.showChannels = function () {
        if ($("#ddlRegion").val() == "United States") {
            $("#channelListUK").addClass("hide");
            $("#channelList").removeClass("hide");
            $('#tblCoordinates tr:gt(1)').remove();
            $("#tblCoordinates a").each(function (index) {
                $(this).addClass("hide");
            });
            $('#tblCoordinates tbody>tr:last').find('input:text').val('');
        }
        else {
            $("#channelListUK").removeClass("hide");
            $("#channelList").addClass("hide");
            $('#tblCoordinates tr:gt(1)').remove();
            $("#tblCoordinates a").each(function (index) {
                $(this).addClass("hide");
            });
            $('#tblCoordinates tbody>tr:last').find('input:text').val('');
        }

        $("#chkSelectAll")[0].checked = false;

        for (i = 2; i < 52; i++) {
            id = "#chk" + i;
            $(id).attr({ 'checked': false })
        }

        for (i = 21; i <= 60; i++) {
            id = "#chkuk" + i;
            $(id).attr({ 'checked': false })
        }
    }

    self.validateInputs = function () {
        var isvalid = true;

        if ($("#ddlRegion").val() == "United States") {
            $('#channelList input:checked').each(function () {
                channels.push($(this).val());
            });
        }
        else {
            $('#channelListUK input:checked').each(function () {
                channels.push($(this).val());
            });
        }
        if (channels.length <= 0) {
            $("#errrchannelList").removeClass("hide");
            isvalid = false;
        }

        if ($("#location").is(':checked')) {

            var rowCount = $('#tblCoordinates tr').length;
            var inputs = $('#tblCoordinates tr').find('input:text');
            var filledInputs = 0;
            inputs.each(function (index) {
                if ($(this).val() != null && $(this).val() != "") {
                    filledInputs++;
                }
            });

            if (filledInputs < ((rowCount-1) * 2)) {
                $("#errrLat").removeClass("hide");
                isvalid = false;
            }
        }

        return isvalid;
    }

    self.submit = function () {

        $(".error").addClass('hide');
        $("#error").addClass('hide');
        $("#success").addClass('hide');

        var url = "/RegionManagement/BlockChannel";

        var isformValid = self.validateInputs();

        if (isformValid) {

            var locationArray = [];
            $('#tblCoordinates tr').each(function (index) {
                if (index > 0) {
                    var inputs = $(this).find('input:text').val();
                    var location = { "latitude": $(this).find('input:text')[0].value, "longitude": $(this).find('input:text')[1].value };
                    locationArray.push(location);
                }
            });

            var data =
                {
                    "selectedChannels": channels,
                    "locatioList": JSON.stringify(locationArray),
                    "regionName": $("#ddlRegion").val(),
                    "regionBlocked": $("#region").is(':checked')
                }

            $('.preloader').show();
            ServerManager.AjaxRequest(
            {
                url: url,
                data: JSON.stringify(data),
                type: "POST",
                dataType: "json",
                contentType: "application/json; charset=utf-8",
                success: self.onSuccess,
                error: self.onError
            });
        }
    };

    self.onSuccess = function (result, status, xhr) {
        if (result.error) {
            ServerManager.handleError(result, $("#error"));
        } else {
            $("#success").removeClass("hide");
            $("#success").text(result);

            var requestUrl = '/RegionManagement/GetExcludedChannels';
            $("#dvexcludedChannels").load(requestUrl, function () {
                $("#dvexcludedChannels a").on('click', function (event) {
                    excludeChannelAnchorClick(this, event);
                });
            });
        }

        $('.preloader').hide();
    };

    self.onError = function (error) {
        //alert(error);

        if (error.type) {
            $("#error").text(result.message);
        }

        $('.preloader').hide();
    };

    self.addRow = function () {
        var table = $("#tblCoordinates");
        var lastRow = $('#tblCoordinates tbody>tr:last');
        $('#tblCoordinates tbody>tr:last').clone(true).insertAfter('#tblCoordinates tbody>tr:last');
        $('#tblCoordinates tbody>tr:last').find('input:text').val('');

        var rowCount = $('#tblCoordinates tr').length;
        if (rowCount > 1) {
            $("#tblCoordinates a").each(function (index) {
                $(this).removeClass("hide");
            });
        }

    };

    self.deleteRow = function (deleteAnchar, event) {
        $(deleteAnchar).closest('tr').remove();

        var rowCount = $('#tblCoordinates tr').length;
        if (rowCount == 2) {
            $("#tblCoordinates a").each(function (index) {
                $(this).addClass("hide");
            });
        }

    }
}

