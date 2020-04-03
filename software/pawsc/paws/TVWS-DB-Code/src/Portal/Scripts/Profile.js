// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

$(document).ready(function () {
    var profile = new Profile();
    $("#Regulatory").change(profile.getAccessLevels);
});


function Profile() {

    this.getAccessLevels = function (e) {
        var regulatory = this.value;
        $('#AccessLevel').children('option:not(:first)').remove();

        if (regulatory != "") {
            var userId = $("#userId").val();

            var url = "/Profile/GetAccessLevels?userId=" + userId + "&regulatory=" + regulatory;
            ServerManager.GetJsonData(
                {
                    url: url,
                    success: OnCallSuccess,
                });
        }
    }

    var OnCallSuccess = function (result, status, xhr) {
        var ddl = $('#AccessLevel');        

        $(result).each(function () {
            $(document.createElement('option'))
               .attr('value', this.Id)
               .text(this.FriendlyName)
               .appendTo(ddl);
        });
    };
};
