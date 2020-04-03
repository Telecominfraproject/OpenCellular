// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

var ServerManager = {
    AjaxRequest: function (options) {
        $.ajax({
            url: options.url,
            dataType: options.dataType,
            contentType: options.contentType,
            type: options.type,
            traditional: options.traditional,
            data: options.data,
            cache: options.cache,
            success: options.success,
            error: options.error
        });
    },

    GetJsonData: function (options) {
        $.getJSON(options.url, options.success);
    },

    GetJsonPData: function (options) {
        $.ajax({
            url: options.url,
            dataType: "jsonp",
            jsonp: "jsonp",
            success: options.success,
            error: options.error
        });
    },

    handleError: function (result, erroControl) {
        if (result.type == "Access Denied") {
            window.location = "/Account/Login?ReturnUrl=" + result.url;
        }
        else if (result.type == "Unknown") {
            window.location = "/Home/Error";
        }
        else {
            if (erroControl != null) {
                $("#error").removeClass("hide");
                $("#error").text(result.message);
            }
        }
    }
}

var HelpControl = function () {
    var self = this;

    // An option decide where should notification balloon should be placed w.r.t current position of the Help Control.
    // TODO: This member could probably be exposed to outside members.
    var alignmentEnum = {
        left: "left",
        right: "right",
        top: "top",
        bottom: "bottom"
    };

    // Default setting values for the HelpControl
    var defaults = {
        // An option to decide notification balloon be automatically hidden after a set interval of time.
        autoHide: true,

        // Interval of time in milliseconds after which notification balloon will be automatically hidden/disappear if "autoHide" is set.
        autoHideTimeout: 500000,

        // An option to position the notification balloon w.r.t the current Help Control position.
        align: alignmentEnum.right,

        // An offset value to be considered while positioning the notification balloon for the given "align" option.
        offset: 10 + "px"
    };

    var autoHide = function (elementId) {
        $('#' + elementId).remove();
    }

    var alignHorizontally = function (alignment, offset, domElementId, helpControlDomElement) {
        var leftOffset = 0;

        /// Formula is: 
        /// Distance from the top of browser Window to top edge of the Help icon + Help Icon middle offset of its height - Notification Balloon  middle offset of its height.
        /// [Above formula will vertically center align the Notification to the center of the Help Control.]
        var topOffset = (helpControlDomElement.offset().top + helpControlDomElement.outerHeight() / 2) - $('#' + domElementId + ' ul').outerHeight() / 2

        if (alignment === alignmentEnum.left) {
            leftOffset = helpControlDomElement.offset().left - ($('#' + domElementId + ' ul').outerWidth() + offset);
        }
        else if (alignment === alignmentEnum.right) {
            leftOffset = helpControlDomElement.offset().left + helpControlDomElement.outerWidth() + offset;
        }

        $('#' + domElementId + ' ul').css('left', leftOffset);
        $('#' + domElementId + ' ul').css('top', topOffset)

        // TODO: Need to handle corner cases where the Notification Balloon appears partially,
        // when the Help Icon positioned at the edge of the browser window.
    }

    var alignVertically = function (alignment, offset, domElementId, helpControlDomElement) {
        var topOffset = 0;

        /// Formula is: 
        /// Distance from the left of browser Window to left edge of the Help icon + Help Icon middle offset of its width - Notification Balloon  middle offset of its width.
        /// [Above formula will horizontally center align the Notification to the center of the Help Control.]
        var leftOffset = helpControlDomElement.offset().left + helpControlDomElement.outerWidth() / 2 - $('#' + domElementId + ' ul').outerWidth() / 2;

        if (alignment === alignmentEnum.top) {
            topOffset = helpControlDomElement.offset().top - ($('#' + domElementId + ' ul').outerHeight() + offset);
        }
        else if (alignment === alignmentEnum.bottom) {
            topOffset = helpControlDomElement.offset().top + helpControlDomElement.outerHeight() + offset;
        }

        $('#' + domElementId + ' ul').css('top', topOffset);
        $('#' + domElementId + ' ul').css('left', leftOffset);

        // TODO: Need to handle corner cases where the Notification Balloon appears partially,
        // when the Help Icon positioned at the edge of the browser window.
    }

    var alignNotificationBalloon = function (alignment, offset, helpControlDomElement) {
        var notificationBollon = helpControlDomElement.next().html();
        var elementId = helpControlDomElement.attr('name');

        $('body').prepend($(notificationBollon).attr('id', elementId));

        if (alignment == alignmentEnum.left || alignment === alignmentEnum.right) {
            alignHorizontally(alignment, offset, elementId, helpControlDomElement);
        }
        else if (alignment == alignmentEnum.top || alignment === alignmentEnum.bottom) {
            alignVertically(alignment, offset, elementId, helpControlDomElement);
        }
    }

    var helpButtonClickHandler = function helpButtonClicked(e) {
        var self = $(this);

        self.addClass('active');
        var elementId = HelpControl.openElementId = self.attr('name');

        if ($('#' + elementId).length > 0) {
            self.removeClass('active');
            $('#' + elementId).remove();
            HelpControl.isVisible = false;
            return;
        }

        alignNotificationBalloon(defaults.align, parseInt(defaults.offset), self);

        $('#' + elementId).find('.close').click(function () {
            self.removeClass('active');
            $('#' + elementId).remove();
            HelpControl.isVisible = false;
        })

        if (defaults.autoHide) {
            setTimeout(autoHide, defaults.autoHideTimeout, elementId);
        }

        HelpControl.isVisible = true;
        e.stopPropagation();
    }

    var reset = function () {
        $('button.icon').unbind('click');

        $('button.icon').click(HelpControl.handler);
    }

    $('button.icon').click(helpButtonClickHandler);

    $('html').click(function (e) {
        if (!$(e.target).closest('.minimal-info').length
                && !$(e.target).closest('button.icon').length
                && HelpControl.isVisible) {
            hideNotification();
        }
    });

    var hideNotification = function () {
        $('button.icon').removeClass('active');
        $('#' + HelpControl.openElementId).remove();
        HelpControl.isVisible = false;
    }

    return {
        handler: helpButtonClickHandler,
        reset: reset,
        align: alignNotificationBalloon
    }
}();
