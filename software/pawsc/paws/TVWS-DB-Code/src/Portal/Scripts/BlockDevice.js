// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

$(document).ready(function () {
    var blockDevice = new BlockDevice();
   
    $("#sDevice").change(blockDevice.singleDevice);
    $("#aDevice").change(blockDevice.allDevices);

    $("#btnSubmitDevice").click(blockDevice.submit);

    $('.blockdevice').hide();
    $('#dvexcludedIds').show();

    $('#device .sub-tab li input').on('click', function (e) {
        if ($(this).hasClass('blockDeviceBtn')) {
            $('.blockdevice').show();
            $('#dvexcludedIds').hide();
        }
        else if ($(this).hasClass('excludeDeviceBtn')) {
            $('.blockdevice').hide();
            $('#dvexcludedIds').show();
        }
    });
})

var BlockDevice = function () {
    var self = this;

    self.singleDevice = function () {
        $("#dvSerialNumber").removeClass("hide");
    };

    self.allDevices = function () {
        $("#dvSerialNumber").addClass("hide");
    };

    self.submit = function () {

        $(".error").addClass('hide');
        $("#errorDevice").addClass('hide');
        $("#successDevice").addClass('hide');

        if($("#deviceId").val() == "")
        {
            $("#errrDevice").removeClass("hide");            
        }
        else
        {
            var data = {
                "deviceId": $("#deviceId").val(),
                "serialNumber": $("#serialNumber").val(),
                "regionName": $("#ddlRegionDevice").val(),
            };

            var url = "/RegionManagement/ExcludeDevice";

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
            ServerManager.handleError(result, $("#errorDevice"));
        }
        else {
            $("#successDevice").removeClass("hide");
            $("#successDevice").text(result);

            var requestUrl = '/RegionManagement/GetExcludedIds';
            $("#dvexcludedIds").load(requestUrl, function () {
                $("#dvexcludedIds a").on('click', function (event) {
                    excludeIdAnchorClick(this, event);
                });
            });
        }

        $('.preloader').hide();
    };

    self.onError = function (error) {       
        if (error.type) {
            $("#error").text(result.message);
        }

        $('.preloader').hide();
    };
}
