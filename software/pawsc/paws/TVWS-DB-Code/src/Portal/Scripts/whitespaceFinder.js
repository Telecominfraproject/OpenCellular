// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

//bingMap instance is created in MapPrtial.cshtml
var whitespaceFinder;

$(document).ready(function () {

    whitespaceFinder = new WhitespaceFinder(bingMap);

    $('.regions ul li').click(whitespaceFinder.onRegionTileClick);

    $('.summary-pane').one('click', whitespaceFinder.onSummaryPaneClick)

    $('.summary-pane').find('ul li button').click(whitespaceFinder.onSplitButtonClick)

    $('.summary-pane').find('ul li label,ul li a').click(whitespaceFinder.onSummaryLabelClick)

    $('#adressFinder').click(whitespaceFinder.onAddressFinderClick);

    $('#requireAntennaHeight').change(whitespaceFinder.onAntennaHeightCheckboxChecked);

    $('#location,#antennaHeight').change(whitespaceFinder.onLocationTextBox);

    $('#findIncumbents').click(whitespaceFinder.onFindIncumbentsClick);

    $('input[name = "deviceType"]').click(whitespaceFinder.onDeviceTypeSelectionChange);

    $("#leftPaneContentPlaceHolder .search").keypress(whitespaceFinder.onEnterKeyPressed);

    $('input[name="protectedNav"]').click(whitespaceFinder.onProtectedEntitySelectionChanged);

    $('.protected-area .channels .channel-list ul li').click(whitespaceFinder.onProtectedEntityChannelSelected);

    $('.summary-pane .container #summarySection #detailsTable table tr').click(whitespaceFinder.onWhitespaceDetailRowSelected);
});

function WhitespaceFinder(bingMapObject) {
    var self = this;

    var colorCodingScheme = {
        MVPD: "#ee8484",
        TV_TRANSLATOR: "#00E8C1",
        TV_US: "#7d85f4",
        TBAS: "#0c9000",
        LPAux: "#41bf4a", // TODO: Update this color
        Microphone: "#a864a8"
    }

    var channelRequestType = {
        SearchLocationChannelRequest: 1,
        ProtectedEntityChannelRequest: 2
    }

    var incumbentCollection = null;
    var channelToEntityMapper = [];
    var bingMap = bingMapObject;
    var enableAjaxCaching = true;
    var invalidSearchMsg = "Invalid Search!";
    var locationRequiredMsg = "Location field is required";

    this.onRegionTileClick = function () {
        var selectedRegion = $(this).children('.regionName').text();

        $('.regions ul .content').not($(this).next('.content')).hide();
        $(this).next('.content').toggle();

        $('.regions ul li').not($(this)).removeClass('active');
        $(this).toggleClass('active');

        $('.preloader').show();

        if ($(this).hasClass('active')) {
            self.loadFeatures(selectedRegion);

            var geoCodeRequest = {
                where: selectedRegion,
                callback: bingMap.onGeocodeSuccess,
                errorCallback: onSerachFailed,
                userData: {
                    callback: function (searchResult) {
                        UpdateInfoboxHtml(searchResult, selectedRegion)
                    }
                }
            }

            bingMap.findAddress(geoCodeRequest, true, bingMap.SearchRequestType.geocodeRequest);
        }
        else {
            ajaxRequest(
                "POST",
                "/Home/GetAvailableRegions"
                , null,
            function (regions) {
                $('.preloader').hide();
                $("#liDownload").addClass("hide");
                $("#liRegister").addClass("hide");
                bingMap.LoadPushPinsForRegions(regions, "/Content/Images/PushPin.png");
            },
            onError);
        }
    };

    self.loadFeatures = function (selectedRegion) {

        $("#liDownload").addClass("hide");
        $("#liRegister").addClass("hide");
        $("#liTest").addClass("hide");
        $("#liSpectrumUse").addClass("hide");

        ajaxRequest("POST", "/Home/GetAvailableFeaturesforRegion", { regionName: selectedRegion },
             function (result) {
                 if (result != null && result != "") {
                     if (result.indexOf("download") >= 0) {
                         $("#liDownload").removeClass("hide");
                     }

                     if (result.indexOf("register incumbents") >= 0) {
                         $("#liRegister").removeClass("hide");
                     }
                     if (result.indexOf("test evaluation") >= 0) {
                         $("#liTest").removeClass("hide");
                     }
                     if (result.indexOf("spectrum use") >= 0) {
                         $("#liSpectrumUse").removeClass("hide");
                     }
                 }
             }, onError);
    }

    this.onEnterKeyPressed = function (e) {

        if (e.which === 13) {
            e.preventDefault();
            self.onFindIncumbentsClick();
        }
    }

    function UpdateInfoboxHtml(searchResult, selectedRegion) {

        var hoverOverStateHtml;
        var selectedStateHtml;

        var pushpin = bingMap.addPushPin(searchResult.bestView.center, { state: Microsoft.Maps.EntityState.selected });
        bingMap.RegionInfobox.initializeInfobox(selectedRegion, pushpin, function () {
            $('.preloader').hide();
        })
    }

    function DisplaySmallInfobox(e) {

        //TODO: Handle info box stalk visibility

        bingMap.addInfobox();
        //addInfobox
    }

    this.onSummaryPaneClick = function () {

        if ($(this).hasClass('collapsed')) {
            $(this).removeClass('left collapsed').addClass('bottom');
            AlignBingMapTop();

            $('#summarySection .details').hide();
            $(this).find('#summaryPaneTitle li:first input').prop('checked', true);
            $('#summarySection #summaryTable').show();

            summaryPaneResize();
        }
    }

    this.onSplitButtonClick = function (event) {

        var orientationState = $(this).parent('li').prop('class');

        var latitude = $("#latitude").val();
        var longitude = $("#longitude").val();

        //ResetMapView(new Microsoft.Maps.Location(latitude, longitude), 200000, 75000);

        if (orientationState == "split" || orientationState == "left") {
            AlignBingMapRight();
        }
        else if (orientationState == "expand" || orientationState == "bottom") {
            AlignBingMapTop();
        }
        else if (orientationState == "full") {
            CollaspseBingMap();
        }
        else {
            BingMapFullView();
        }

        switch (orientationState) {
            case "split":
                $('.summary-pane').removeClass('expanded').addClass('left');
                break;
            case "left":
                $('.summary-pane').removeClass('bottom').addClass('left');
                break;
            case "bottom":
                $('.summary-pane').removeClass('left collapsed').addClass('bottom');
                break;
            case "full":
                $('.summary-pane').removeClass('bottom left').addClass('expanded');
                break;
            case "collapse":
                $('.summary-pane').removeClass('left bottom expanded').addClass('collapsed');
                $('.summary-pane').one('click', self.onSummaryPaneClick)
                break;
        }

        summaryPaneResize();

        event.stopPropagation();
    };

    this.onSummaryLabelClick = function (e) {

        if (!$(this).hasClass('hyperLink')) {
            if ($('.summary-pane').hasClass('collapsed')) {
                $('.summary-pane').removeClass('left collapsed').addClass('bottom');
                AlignBingMapTop();
            }

            var labelFor = $(this).attr('for');
            $('#summarySection .details').hide();

            switch (labelFor) {
                case 'channelsSummary':
                    $('#summarySection #summaryTable').show();
                    break;
                case 'channelsDetail':
                    $('#detailsTable').show();

                    summaryPaneResize();
                    break
            }
        }

        e.stopPropagation();
    }

    this.initializeIncumbentCollection = function (collection) {
        incumbentCollection = null;
        incumbentCollection = collection;

        if (incumbentCollection && incumbentCollection.length > 0) {
            incumbentCollection = RemoveDuplicateCallSigns(incumbentCollection);

            incumbentCollection.forEach(function (item, index) {
                if (item.ContourPoints.length > 0) {
                    $('.ws-finder .wrap .channels .channel-list ul .none #' + item.Channel).parent('li').addClass('contours-available');
                }

                var title = $('.ws-finder .wrap .channels .channel-list ul .none #' + item.Channel).attr('title');

                if (title === "") {
                    title = item.CallSign;
                }
                else {
                    title = title + "," + item.CallSign;
                }

                $('.ws-finder .wrap .channels .channel-list ul .none #' + item.Channel).attr('title', title);
            });
        }
    }

    this.onAddressFinderClick = function (e) {
        if ($('#location').val() == "") {
            $('#locationValMsg').text(locationRequiredMsg);
            return false;
        }

        var searchRequest = {
            where: $('#location').val(),
            callback: onLocationFound,
            errorCallback: onLocationSearchFailed,
            userData: {
                callback: onSearchSuccess
            }
        }

        bingMap.locationFinder(searchRequest, bingMap.SearchRequestType.searchRequest);
    };

    this.onAntennaHeightCheckboxChecked = function () {
        var antennaHeightRequired = $('#requireAntennaHeight').prop("checked");
        $('#antennaHeight').prop('disabled', !antennaHeightRequired);
        $('.wrap section #locationSearchForm').toggleClass("enableAntennaHeight", antennaHeightRequired);
    };

    this.onLocationTextBox = function () {
        $('.field-validation-error').empty();
    };

    this.onFindIncumbentsClick = function () {
        $('.field-validation-error').empty();

        if ($('#location').val() == "") {
            $('#locationValMsg').text(locationRequiredMsg);
            $('#locationValMsg').removeClass('field-validation-valid').addClass('field-validation-error');
            return false;
        }

        BingMapFullView();
        $('.summary-pane').removeClass('left bottom expanded').addClass('collapsed');

        $('#preloader').show();

        var searchRequest = {
            where: $('#location').val(),
            callback: onLocationFound,
            errorCallback: onLocationSearchFailed,
            userData: {
                callback: onRegionFound
            }
        }

        bingMap.locationFinder(searchRequest, bingMap.SearchRequestType.searchRequest);
    };

    this.onDeviceTypeSelectionChange = function () {
        var deviceType = $(this).val();
        $('#incumbentType').val(deviceType);

        if ($('.ws-finder .wrap .channels .channel-list ul li').length > 0) {
            var availableChannels = self.AvailableChannels.getChannelsByDeviceType(deviceType);

            if (availableChannels) {
                $('.ws-finder .wrap .channels .channel-list ul li').removeClass('low high').addClass('none');

                availableChannels.forEach(function (item, index) {
                    $('.ws-finder .wrap .channels .channel-list ul li #' + item.Id).parent('li').removeClass('none contours-available').addClass(item.CssClass);
                });

                if (incumbentCollection) {
                    incumbentCollection.forEach(function (item, index) {
                        if (item.ContourPoints.length > 0) {
                            $('.ws-finder .wrap .channels .channel-list ul .none #' + item.Channel).parent('li').addClass('contours-available');
                        }
                    });
                }

                var highPowerChannelCount = $('.ws-finder .wrap .channels .channel-list ul li.high').length;
                var lowPowerChannelCount = $('.ws-finder .wrap .channels .channel-list ul li.low').length;
                var notAvailableChannelCount = $('.ws-finder .wrap .channels .channel-list ul li.none').length;

                $('.total-channels-info ul .high button').text(highPowerChannelCount);
                $('.total-channels-info ul .low button').text(lowPowerChannelCount);
                $('.total-channels-info ul .not-available button').text(notAvailableChannelCount);

                $('.total-channels-info ul .high button').attr('title', "Available High (" + highPowerChannelCount + ")");
                $('.total-channels-info ul .low button').attr('title', "Available Low (" + lowPowerChannelCount + ")");
                $('.total-channels-info ul .not-available button').attr('title', "Not Available (" + notAvailableChannelCount + ")");
            }
        }
    }

    this.onAccordianClick = function (e) {

        if (e.target.className === "icon info noBg") {
            return;
        }

        var accordian = $(this).parent('.accordion');

        $(".accordion").not(accordian).addClass("collapse");

        accordian.toggleClass('collapse');

        bingMap.ClearMapEntities();

        var searchRequest = {
        };
        var requestType = null;

        $('.summary-pane').removeClass('left collapsed').addClass('bottom');

        if (!$('.ws-finder').hasClass('collapse')) {

            var channelPowerInfoElement = $('.channels .total-channels-info ul');

            var lowPowerChannelCount = parseInt(channelPowerInfoElement.find('.low button').text());
            var highPowerChannelCount = parseInt(channelPowerInfoElement.find('.high button').text());
            var notAvailableChannelCount = parseInt(channelPowerInfoElement.find('.not-available button').text());

            // If all low,High and Not-available channel count equal 0 means there were no search operation performed before, that case just skip location update on Map.
            if (lowPowerChannelCount == 0 && highPowerChannelCount == 0 && notAvailableChannelCount == 0) {
                var pushpinOptions = {
                    zIndex: 100,
                    icon: '/Content/Images/PushPin.png',
                    height: 36,
                    width: 26
                };

                var loc = new Microsoft.Maps.Location(parseFloat($('#latitude').val()), parseFloat($('#longitude').val()));

                pushPin = bingMap.addPushPin(loc, pushpinOptions);
                bingMap.addInfobox(loc, { title: $('#countryRegion').val(), pushpin: pushPin });
            }
            else {
                // fall-back  on recent search location.
                searchRequest = {
                    where: $('#latitude').val() + ',' + $('#longitude').val(),
                    callback: onLocationFound,
                    errorCallback: onLocationSearchFailed,
                    userData: {
                        callback: updateMapEntites
                    }
                }

                requestType = bingMap.SearchRequestType.searchRequest;
            }

            // Remove selection for all the selected channels in protected area.
            $('.protected-area .channels .channel-list ul li.selected').removeClass('selected').addClass('none');
            $('.summary-pane').removeClass('left bottom expanded').addClass('collapsed');

            $('#summaryPane').show();
        }
        else if (!$('.protected-area').hasClass('collapse')) {
            searchRequest = {
                where: $('#countryRegion').val(),
                callback: bingMap.onGeocodeSuccess,
                errorCallback: bingMap.onSerachFailed
            };

            // Case to handle invalid search followed by channel selection in ProtectedEntity type.
            // Idea is fall-back on previous country region used before the invalid search result.
            if (searchRequest.where === "" && $('#latitude').val() === "" && $('#longitude').val() === "") {
                searchRequest.where = $('#regionName').text();
            }

            requestType = bingMap.SearchRequestType.geocodeRequest;

            bingMap.ClearMapEntities();
            $('.wrap .channels .channel-list ul .selected').removeClass('selected').addClass('none');
            $('#summarySection #detailsTable table tr.selected').removeClass('selected');

            // Collapse summary pane an hide.
            BingMapFullView();
            $('#summaryPane').hide();
        }

        bingMap.locationFinder(searchRequest, requestType);
        var headerHeight = $('#left-pane header').outerHeight();
        var accordionsLabelHeight = 0;

        $('#left-pane').find('.accordion').each(function () {
            accordionsLabelHeight = accordionsLabelHeight + $(this).find('label:first').outerHeight(true);
        });

        var maxScrollHeight = $(window).height() - (headerHeight + accordionsLabelHeight);
        leftPanelSlimScrollBar(maxScrollHeight, true);
    }

    this.onProtectedEntitySelectionChanged = function () {
        bingMap.ClearMapEntities();
        enableAjaxCaching = false;
        // TODO: If a channel is selected for the previous protected entity, should that channel
        // selection be retained for the current new selection?

        // Remove selection for all the selected channels in protected area.
        $('.protected-area .channels .channel-list ul li.selected').removeClass('selected').addClass('none');
    }

    this.onProtectedEntityChannelSelected = function () {
        var protectedEntityType = $('input[type="radio"][name="protectedNav"]:checked').val();
        var countryRegion = $("#countryRegion").val()
        var channel = $(this).children('button').text();

        // Case to handle invalid search followed by channel selection in ProtectedEntity type.
        // Idea is fall-back previous country region used before the invalid search result.
        if (countryRegion === "") {
            countryRegion = $('#regionName').text();
        }

        // Case to handle protected entity is not selected and trying to select a channel.
        if ($('input[type="radio"][name="protectedNav"]:checked').length === 0) {
            return false;
        }

        $(this).toggleClass('none selected');

        // Check if current element has 'none' class, which means new selection else the channel is deselected .
        if ($(this).hasClass('selected')) {
            $('#preloader').show();
            self.GetOrSetProtectedEntityDataCache(channel, countryRegion, protectedEntityType, DrawContours)
        }
        else if ($(this).hasClass('none')) {
            ClearMapEntitiesFromMapperCollection(channel, channelRequestType.ProtectedEntityChannelRequest);
        }
    }

    this.GetOrSetProtectedEntityDataCache = function (channel, countryRegion, entityType, callback) {
        var hashKey = countryRegion + "|" + entityType + "|" + channel;

        if (!self.GetOrSetProtectedEntityDataCache.cache[hashKey]) {
            ajaxRequest(
                "Get",
                "/WSFinder/GetProtectedIncumbents",
            {
                incumbentType: entityType, countryRegion: countryRegion, channel: channel
            },
                function (json) {
                    $('#preloader').hide();

                    if (json) {
                        enableAjaxCaching = true;
                        self.GetOrSetProtectedEntityDataCache.cache[hashKey] = json;
                        callback(self.GetOrSetProtectedEntityDataCache.cache[hashKey], channelRequestType.ProtectedEntityChannelRequest);
                    }
                },
                function (xhr, ajaxOptions, thrownError) {
                    //TODO: Error handling logic.
                    $('#preloader').hide();
                },
                 enableAjaxCaching);
        }
        else {
            $('#preloader').hide();
            callback(self.GetOrSetProtectedEntityDataCache.cache[hashKey], channelRequestType.ProtectedEntityChannelRequest);
        }
    }

    this.GetOrSetProtectedEntityDataCache.cache = {};

    this.AvailableChannels = function () {
        var channelAvailabilityLookup;
        var powerTransitionPoint;

        var initialize = function (channelLookup, powerValue) {
            channelAvailabilityLookup = channelLookup;
            powerTransitionPoint = powerValue;
        }

        var getChannels = function (deviceType) {
            if (channelAvailabilityLookup) {
                var channels = channelAvailabilityLookup[deviceType];

                if (!getChannels.cache[deviceType]) {
                    var channelInfoList = [];

                    channels.forEach(function (item, index) {
                        var cssClass = "none";
                        if (item.MaxPowerDBm >= powerTransitionPoint) {
                            cssClass = "high";
                        }
                        else {
                            cssClass = "low";
                        }

                        var channelInfo = {
                            Id: item.ChannelId,
                            CssClass: cssClass
                        };

                        channelInfoList.push(channelInfo)
                    });

                    getChannels.cache[deviceType] = channelInfoList;
                }

                return getChannels.cache[deviceType];
            }
        }

        var reset = function () {
            var channelAvailabilityLookup = {};
            getChannels.cache = {};
        }

        getChannels.cache = {};

        return {
            initialize: initialize,
            getChannelsByDeviceType: getChannels,
            reset: reset
        };
    }();

    this.customScrollBar = function () {
        var setLeftOffset = function (offset) {
            $(".slimScrollDiv").css({ left: offset + "px" });
        }

        var setRightOffset = function (offset) {
            $(".slimScrollDiv").css({ right: offset + "px" });
        }

        var setTopOffset = function (offset) {
            $(".slimScrollDiv").css({ top: offset + "px" });
        }

        var setBottomOffset = function (offset) {
            $(".slimScrollDiv").css({ bottom: offset + "px" });
        }

        var initialize = function (options, elementIdentifier, offset) {
            var domElement = elementIdentifier;

            $(domElement).slimScroll(options);

            if (offset) {

                if (offset['left']) {
                    setLeftOffset(offset['left']);
                }

                if (offset['right']) {
                    setRightOffset(offset['right']);
                }

                if (offset['top']) {
                    setTopOffset(offset['top']);
                }

                if (offset['bottom']) {
                    setBottomOffset(offset['bottom']);
                }
            }
        }
        var destroy = function (elementIdentifier) {
            var domElement = elementIdentifier;
            $(domElement).slimScroll({ destroy: true });
        }

        //TODO: Following implementation has be
        //changed according to the need.
        var resize = function (elementidentifier) {
            var me = $(elementidentifier);
            me.css("height", $(window).height() + 'px');
            $(".slimscrolldiv").css("height", $(window).height() + 'px');
            var height = math.max((me.outerheight() / me[0].scrollheight) * me.outerheight(), 30);
            $(".slimscrollbar").css({ height: height + 'px' });
        }

        return {
            initialize: initialize,
            destroy: destroy
        };
    }();

    var summaryPaneResize = function () {

        if ($('.table-body').parent('.slimScrollDiv').length === 0) {
            return;
        }

        var summaryPaneWidth = $('.summary-pane .container .title').outerWidth() - $('.summary-pane .container').css('padding-left');

        var summaryPaneTitleHeight = $('.summary-pane .container .title').outerHeight();
        var summaryTableHeaderHeight = $('.summary-pane').find('.details .table-header').outerHeight();

        var outerHeight = $('.summary-pane .container').outerHeight() - (summaryPaneTitleHeight + summaryTableHeaderHeight);

        $('.table-body').css("height", outerHeight + 'px');
        $('.table-body').parent('.slimScrollDiv').css("height", outerHeight + 'px');

        $('.table-body').css("width", summaryPaneWidth + 'px');
        $('.table-body').parent('.slimScrollDiv').css("width", summaryPaneWidth + 'px');

        var tableBodyOuterHeigth = $('.table-body').outerHeight();

        var height = Math.max((tableBodyOuterHeigth / $('.table-body')[0].scrollHeight) * tableBodyOuterHeigth, 30);

        $('.table-body').parent('.slimScrollDiv .slimScrollBar').css({ height: height + 'px' });

        $('.table-body').parent('.slimScrollDiv .slimScrollBar').css({ width: summaryPaneWidth + 'px' });
    }

    this.onChannelLinkClicked = function () {
        if (!$(this).hasClass('none')) {
            return false;
        }

        var channel = $(this).children('button').text();
        var detailsTableRow = $('#summarySection #detailsTable table #' + channel);
        DrawContourForSearchLocationChannel(channel, $(this), detailsTableRow);
    }

    this.onWhitespaceDetailRowSelected = function () {
        var channel = $(this).children('#channel').text();
        var channelLink = $('.channels .channel-list ul .none.contours-available #' + channel).parent('li');
        DrawContourForSearchLocationChannel(channel, channelLink, $(this));
    }

    this.UpdateLayout = function (enableProtectedAreas) {
        $('#preloader').hide();
        $('#summaryPane').show();

        var searchPane = $('#whitespacePanel #searchPane').html();
        var whitespaceSummary = $('#whitespacePanel #summarySection').html();

        $('#summaryPane').show();

        $('#whitespacePanel').empty();

        $('#summarySection').empty().html(whitespaceSummary);

        $('#leftPaneContentPlaceHolder').empty().html(searchPane);
        $('#findIncumbents').click(whitespaceFinder.onFindIncumbentsClick);
        $('#location').change(whitespaceFinder.onLocationTextBox);
        $('#requireAntennaHeight').change(whitespaceFinder.onAntennaHeightCheckboxChecked);

        if (!$('.summary-pane').hasClass('collapsed')) {
            $('#summarySection .details').hide();
            $(this).find('#summaryPaneTitle li:first input').prop('checked', true);
            $('#summarySection #summaryTable').show();
        }

        $('.summary-pane').find('#channelsDetail').next().show();

        if (!enableProtectedAreas) {
            $('.summary-pane').find('#channelsDetail').next().hide();
        }

        $('input[name = "deviceType"]').click(whitespaceFinder.onDeviceTypeSelectionChange);
        $('input[name="protectedNav"]').click(whitespaceFinder.onProtectedEntitySelectionChanged);
        $("#leftPaneContentPlaceHolder .search").keypress(whitespaceFinder.onEnterKeyPressed);
        $('.protected-area .channels .channel-list ul li').click(whitespaceFinder.onProtectedEntityChannelSelected);

        $('#wsFinder,#wsProtect').next('label').click(whitespaceFinder.onAccordianClick);

        /// Enable channel link click only if
        /// 1) User has already searched for a region
        if ($('.ws-finder .wrap .channels .channel-list ul .none').length > 0) {
            $('.ws-finder .wrap .channels .channel-list ul li').click(whitespaceFinder.onChannelLinkClicked);
            $('.summary-pane .container #summarySection #detailsTable table tr').click(whitespaceFinder.onWhitespaceDetailRowSelected);
        }

        var headerHeight = $('#left-pane header').outerHeight();
        var accordionsLabelHeight = 0;

        $('#left-pane').find('.accordion').each(function () {
            accordionsLabelHeight = accordionsLabelHeight + $(this).find('label:first').outerHeight(true);
        });

        var maxScrollHeight = $(window).height() - (headerHeight + accordionsLabelHeight);
        leftPanelSlimScrollBar(maxScrollHeight, false);

        // Initialize scrollbar for search pane controls.
        //self.customScrollBar.initialize(
        //        {
        //            width: $('#left-pane').find('.accordion').width() + 'px',
        //            height: $(window).height() - (headerHeight + accordionsLabelHeight) + "px",
        //            wheelStep: 5
        //        },
        //        '.ws-finder .wrap #finder-controls,.ws-protectedArea .wrap #protecteArea-controls'
        //       );

        var summaryPaneTitleHeight = $('.summary-pane .container .title').outerHeight();
        var summaryTableHeaderHeight = $('.summary-pane').find('.details .table-header').outerHeight();

        self.customScrollBar.initialize(
            {
                width: $('.summary-pane .container .title').width() + 'px',
                height: $('.summary-pane .container').outerHeight() - (summaryPaneTitleHeight + summaryTableHeaderHeight) + 'px',
                wheelStep: 5
            },
        '.table-body');


        var region = $('#regionName').text();
        if (region == "United States") {
            $('.summary-pane').find('#downloadData').show();
            $('.summary-pane').find('#registerIncumbent').show();
        }
        else {
            $('.summary-pane').find('#downloadData').hide();
            $('.summary-pane').find('#registerIncumbent').hide();
        }


        HelpControl.reset();
    }
    function leftPanelSlimScrollBar(maxScrollHeight, isAcordianPaneClick) {
        $('.accordion').each(function () {


            var controlHeight = $(this).find('.wrap').outerHeight(true);
            //var scrollHeight = 200;
            var scrollHeight = controlHeight;

            if (controlHeight > maxScrollHeight) {
                scrollHeight = maxScrollHeight;
            }

            var parentId = $(this)[0].parentElement.id
            if (parentId != "rightPaneContainer") {
                var domSelector = '#' + parentId;
                for (var index = 0; index < $(this)[0].classList.length; index++) {
                    var item = $(this)[0].classList[index];
                    if (domSelector == '#') {
                        domSelector = '.' + item;
                    }
                    else {
                        if (index == 0) {
                            domSelector = domSelector + ' .' + item;
                        }
                        else {
                            domSelector = domSelector + '.' + item;
                        }
                    }
                }
                domSelector = domSelector + ' .wrap';
                var collapseSearch = domSelector.search('collapse');

                if (domSelector.search('collapse') == -1) {

                    self.customScrollBar.initialize(
                        {
                            width: $('#left-pane').find('.accordion').width() + 'px',
                            height: scrollHeight + "px",
                            wheelStep: 5
                        },
                        domSelector
                       );
                }
                else {
                    if (isAcordianPaneClick) {
                        self.customScrollBar.destroy(domSelector);
                    }
                }
            }
        })
    }

    function DrawContourForSearchLocationChannel(channel, channelLink, detailRow) {
        detailRow.toggleClass('selected');

        if (($('#summarySection #detailsTable table #' + channel + ".selected").length >= 1 && !channelLink.hasClass('selected')) || $('#summarySection #detailsTable table #' + channel + ".selected").length == 0) {
            channelLink.toggleClass('selected');
        }

        if (channelLink.hasClass('selected') && detailRow.hasClass('selected')) {

            var incumbentList = [];

            incumbentCollection.forEach(function (item, index) {
                if (item.Channel === parseInt(channel)) {
                    incumbentList.push(item);
                }
            });

            DrawContours(incumbentList, channelRequestType.SearchLocationChannelRequest);
        }
        else {
            if (detailRow.length > 1) {
                ClearMapEntitiesFromMapperCollection(channel, channelRequestType.SearchLocationChannelRequest);
            }
            else {
                var callsign = detailRow.children('#callsign').text();
                var incumbentType = detailRow.children('#incumbentType').text();
                var latitude = detailRow.children("#latitude").text();
                var longitude = detailRow.children("#longitude").text();

                ClearMapEntities(channel, channelRequestType.SearchLocationChannelRequest, incumbentType, callsign, latitude, longitude);
            }
        }
    }

    function GetIndexCollectionOfChannelToEntityMapper(channel, requestType) {
        var mapperIndexCollection = [];

        //TODO: Performance enhancement required here if there are too many entries in the list.
        channelToEntityMapper.forEach(function (item, index) {
            if (item.ChannelNumber === parseInt(channel) && item.RequestType === requestType) {
                mapperIndexCollection.push(index);
            }
        })

        return mapperIndexCollection;
    }

    function GetIndexOfChannelToEntityMapper(channel, requestType, incumbentType, callsign, latitude, longitude) {
        var mapperIndex = -1;

        var lat = parseFloat(parseFloat(latitude).toFixed(12));
        var long = parseFloat(parseFloat(longitude).toFixed(12));

        channelToEntityMapper.forEach(function (item, index) {
            if (item.ChannelNumber === parseInt(channel) && item.RequestType === requestType && item.IncumbentType === incumbentType && item.Callsign == callsign && item.Latitude === lat && item.Longitude === long) {
                mapperIndex = index;
                return;
            }
        })

        return mapperIndex;
    }

    function ClearMapEntitiesFromMapperCollection(channel, requestType) {
        var mapperIndexCollection = GetIndexCollectionOfChannelToEntityMapper(channel, requestType);

        if (mapperIndexCollection.length > 0) {

            mapperIndexCollection.forEach(function (item, index) {
                // Remove all the entities for a given channel from the EntityCollection of Bing map instance.
                channelToEntityMapper[item].MapEntityCollection.forEach(function (entity, index) {
                    bingMap.RemoveEntity(entity);
                });
            })


            // Remove the entry from the channelToEntityMapper.
            channelToEntityMapper.splice(mapperIndexCollection[0], mapperIndexCollection.length);
        }
    }

    function ClearMapEntities(channel, requestType, incumbentType, callsign, latitude, longitude) {
        var mapperIndex = GetIndexOfChannelToEntityMapper(channel, requestType, incumbentType, callsign, latitude, longitude);

        if (mapperIndex != -1) {

            // Remove all the entities for a given channel from the EntityCollection of Bing map instance.
            channelToEntityMapper[mapperIndex].MapEntityCollection.forEach(function (item, index) {
                bingMap.RemoveEntity(item);
            });

            // Remove the entry from the channelToEntityMapper.
            channelToEntityMapper.splice(mapperIndex, 1);
        }
    }

    function DrawContours(incumbentList, requestType) {

        if (incumbentList && incumbentList.length >= 0) {
            incumbentList = RemoveDuplicateCallSigns(incumbentList);

            for (var index = 0; index < incumbentList.length; index++) {
                var viewBoundary = [];
                var pushpin = null;

                var latitude = parseFloat(incumbentList[index].TransmitLocation.Latitude.toFixed(12));
                var longitude = parseFloat(incumbentList[index].TransmitLocation.Longitude.toFixed(12))

                var channelInfo = {
                    ChannelNumber: incumbentList[index].Channel,
                    MapEntityCollection: [],
                    RequestType: requestType,
                    IncumbentType: incumbentList[index].IncumbentType,
                    Callsign: incumbentList[index].CallSign,
                    Latitude: latitude,
                    Longitude: longitude
                };

                var mapperIndex = GetIndexOfChannelToEntityMapper(incumbentList[index].Channel, requestType, incumbentList[index].IncumbentType, incumbentList[index].CallSign, latitude, longitude);

                // Check if there already exist an entry for a given channel, if not add a new entry.
                if (mapperIndex === -1) {
                    channelToEntityMapper.push(channelInfo);
                    mapperIndex = channelToEntityMapper.length - 1;
                }

                if (incumbentList[index].TransmitLocation) {

                    var location = new Microsoft.Maps.Location(incumbentList[index].TransmitLocation.Latitude, incumbentList[index].TransmitLocation.Longitude);

                    var customPushpinHtml = "<div>" + incumbentList[index].CallSign + "</div>";

                    //TODO: Updated pushpin custom HTML style as per the requirement.
                    var pushpinOptions = {
                        width: null,
                        height: null,
                        htmlContent: "" + customPushpinHtml + "",
                        typeName: 'micro'
                    }

                    pushpin = bingMap.addPushPin(location, pushpinOptions);

                    //TODO: Updated infobox custom HTML style as per the requirement.
                    var infoBoxOption = {
                        width: 100,
                        height: 100,
                        title: incumbentList[index].CallSign,
                        description: "" + incumbentList[index].Channel + "",
                        showPointer: false,
                        pushpin: pushpin,
                        showCloseButton: false
                    };

                    var infoBox = bingMap.addInfobox(location, infoBoxOption);

                    channelToEntityMapper[mapperIndex].MapEntityCollection.push(pushpin);
                    channelToEntityMapper[mapperIndex].MapEntityCollection.push(infoBox);
                }

                var polygon = null;

                var color = Microsoft.Maps.Color.fromHex(colorCodingScheme[incumbentList[index].IncumbentType]);

                var polygonOptions = {
                    fillColor: new Microsoft.Maps.Color(80, color.r, color.g, color.b),
                    strokeColor: new Microsoft.Maps.Color(80, 0, 0, 0),
                    strokeThickness: parseInt(1)
                };

                if ((incumbentList[index].IncumbentType === "LPAux" || incumbentList[index].IncumbentType === "UnlicensedLPAux") && incumbentList[index].ContourPoints.length === 1) {
                    var center = new Microsoft.Maps.Location(incumbentList[index].ContourPoints[0].Latitude, incumbentList[index].ContourPoints[0].Longitude);
                    polygon = bingMap.DrawCircle(center, 1000, 1000, polygonOptions);

                    channelToEntityMapper[mapperIndex].MapEntityCollection.push(polygon);
                }
                else {
                    for (var contourIndex = 0; contourIndex < incumbentList[index].ContourPoints.length; contourIndex++) {
                        var countourPoint = new Microsoft.Maps.Location(incumbentList[index].ContourPoints[contourIndex].Latitude, incumbentList[index].ContourPoints[contourIndex].Longitude);
                        viewBoundary.push(countourPoint);
                    }

                    if (viewBoundary.length > 0) {
                        polygon = bingMap.DrawPolygon(viewBoundary, polygonOptions);

                        channelToEntityMapper[mapperIndex].MapEntityCollection.push(polygon);
                    }
                }
            }
        }
    }

    function RemoveDuplicateCallSigns(incumbentList) {
        callsignArray = [];

        for (var index = 0; index < incumbentList.length; index++) {
            var entryExists = false;

            callsignArray.forEach(function (item) {
                if (incumbentList[index].Channel == item.Channel && incumbentList[index].Type == item.Type && incumbentList[index].CallSign == item.CallSign && incumbentList[index].TransmitLocation.Latitude == item.TransmitLocation.Latitude && incumbentList[index].TransmitLocation.Longitude == item.TransmitLocation.Longitude) {
                    entryExists = true;
                    return;
                }
            });

            if (!entryExists) {
                callsignArray.push(incumbentList[index]);
            }
        }

        return callsignArray;
    }

    var AlignBingMapRight = function () {
        $('#mapDiv').show();
        $('#rightPaneContainer').removeClass('bottom-view collapse-view').addClass('left-view');
    };

    var AlignBingMapTop = function () {
        $('#mapDiv').show();
        $('#rightPaneContainer').removeClass('left-view collapse-view').addClass('bottom-view');
    };

    var CollaspseBingMap = function () {
        $('#rightPaneContainer').removeClass('left-view bottom-view');
        $('#mapDiv').hide();
    };

    var BingMapFullView = function () {
        $('#mapDiv').show();
        $('#rightPaneContainer').removeClass('left-view bottom-view collapse-view');
    };

    var onSearchSuccess = function (latitude, longitude, countryRegion) {
        updateSearchResults(latitude, longitude, countryRegion)

        $('.wrap section #locationSearchForm').submit();
    };

    var updateSearchResults = function (latitude, longitude, countryRegion) {
        $('#latitude').val(latitude);
        $('#longitude').val(longitude);

        $('#countryRegion').val(countryRegion);
        $('#regionName').val(countryRegion);
    };

    var onLocationFound = function (searchResult, userData) {
        if (searchResult) {
            var searchRegion = searchResult.searchRegion;
            bingMap.ClearMapEntities();

            if (searchRegion) {

                if (searchRegion.matchCode == Microsoft.Maps.Search.MatchCode.none) {
                    $('#preloader').hide();
                    $('#locationValMsg').text(invalidSearchMsg);
                    $('#locationValMsg').removeClass('field-validation-valid').addClass('field-validation-error');

                    updateSearchResults("", "", "");

                    return;
                }

                if (searchRegion.address.countryRegion) {

                    var searchLocation = searchRegion.address.locality;

                    // If the searchResult address property doesn't contain locality information or it is already part of searchRegion.explictLocation.name.
                    if (searchLocation === "" || searchRegion.explicitLocation.name.toLowerCase().search(searchLocation.toLowerCase()) != -1) {
                        searchLocation = searchRegion.explicitLocation.name;
                    }
                    else {
                        searchLocation = searchRegion.address.locality + ", " + searchRegion.explicitLocation.name
                    }

                    if (searchRegion.address.countryRegion.toLowerCase() != $('#regionName').text().toLowerCase()) {
                        userData.callback = onCountryRegionChanged;
                    }

                    userData.callback(
                        searchRegion.explicitLocation.location.latitude,
                        searchRegion.explicitLocation.location.longitude,
                        searchRegion.address.countryRegion,
                        searchLocation);
                }
                else {
                    var reverseGeocoderequest = {
                        location: searchRegion.explicitLocation.location,
                        count: 10,
                        callback: geocodeCallback,
                        errorCallback: onSerachFailed,
                        userData: userData
                    }

                    bingMap.locationFinder(reverseGeocoderequest, bingMap.SearchRequestType.reverseGeocodeRequest);
                }
            }
            else {
                $('#locationValMsg').text(invalidSearchMsg);
                $('#locationValMsg').removeClass('field-validation-valid').addClass('field-validation-error');
            }
        }
    };

    var onLocationSearchFailed = function (searchRequest) {
        onRegionFound(null, null, searchRequest.request.where);
        $('#preloader').hide();
    }

    function onAntennaHeightCheckboxChecked() {
        var antennaHeightRequired = $(this).prop("checked");
        $('#antennaHeight').prop('disabled', !antennaHeightRequired);
    }

    var geocodeCallback = function (result, userData) {
        if (result && result.name) {
            var searchRequest = {
                where: result.name,
                callback: onLocationFound,
                errorCallback: onLocationSearchFailed,
                userData: {
                    callback: userData.callback
                }
            }

            bingMap.locationFinder(searchRequest, bingMap.SearchRequestType.searchRequest)
        }
        else {
            // Unknown location.
        }
    };

    var onSerachFailed = function () {
        ////TODO: code to handle search failure.
    }

    var onError = function () {
        ////TODO: code to handle search failure.
    }

    var ajaxRequest = function (type, url, data, successCallback, errorCallback, enableCaching) {
        ServerManager.AjaxRequest(
        {
            url: url,
            data: data,
            type: type,
            traditional: true,
            success: successCallback,
            error: errorCallback,
            cache: enableCaching
        });
    };

    var updateMapEntites = function (latitude, longitude, countryRegion, searchLocation) {
        var location = $('#location').val();
        var antennaHeight = $('.antenna-height').val();

        var selectedIncumbentType = $('input[type="radio"][name="deviceType"]:checked').val();
        $('#incumbentType').val(selectedIncumbentType);

        updateSearchResults(latitude, longitude, countryRegion);

        var data = {
            Location: location, CountryRegion: countryRegion, Latitude: latitude, Longitude: longitude, AntennaHeight: antennaHeight
        };

        // Considered 200 KM of area around the search location to ensure full view of all the contours can be seen on the Map.
        // [Why 200 KM ?]: An approximate distance from the search location to incumbent [device] transmit location will be 100 KM + approximate distance that 
        // incumbent can span from the transmit location is assumed to be 100 KM.
        var location = new Microsoft.Maps.Location(latitude, longitude);
        ResetMapView(location, 200000, 75000);

        var pushpinOptions = {
            zIndex: 100,
            icon: '/Content/Images/PushPin.png',
            height: 36,
            width: 26
        };

        var pushpin = bingMap.addPushPin(location, pushpinOptions);

        bingMap.addInfobox(location, { title: searchLocation, pushpin: pushpin });
    }

    var onRegionFound = function (latitude, longitude, countryRegion, searchLocation) {
        updateMapEntites(latitude, longitude, countryRegion, searchLocation);

        $('.wrap section #locationSearchForm').submit();
    };

    var onCountryRegionChanged = function (latitude, longitude, countryRegion, searchLocation) {
        var antennaHeight = $('.antenna-height').val();

        var data = {
            Location: searchLocation, CountryRegion: countryRegion, Latitude: latitude, Longitude: longitude, AntennaHeight: antennaHeight
        };

        ajaxRequest(
            "GET",
            "/WSFinder/GetCountryRegionSpecificControls",
           data,
            function (html) {
                if (html) {
                    $('#whitespacePanel').html(html)

                    var searchPane = $('#whitespacePanel #searchPane').html();
                    var whitespaceSummary = $('#whitespacePanel #summarySection').html();

                    $('#whitespacePanel').empty();

                    $('#summarySection').empty().html(whitespaceSummary);

                    $('#leftPaneContentPlaceHolder').empty().html(searchPane);

                    if ($('.ws-protectedArea') || $('.ws-protectedArea').length == 0) {
                        $('.summary-pane').find('#channelsDetail').next().hide();
                    }
                    else {
                        $('.summary-pane').find('#channelsDetail').next().show();
                    }

                    // TODO: Following event re-attaching logic has been repeated twice move it one common function.
                    $('#findIncumbents').click(whitespaceFinder.onFindIncumbentsClick);
                    $('#location').change(whitespaceFinder.onLocationTextBox);
                    $('#requireAntennaHeight').change(whitespaceFinder.onAntennaHeightCheckboxChecked);

                    if (!$('.summary-pane').hasClass('collapsed')) {
                        $('#summarySection .details').hide();
                        $(this).find('#summaryPaneTitle li:first input').prop('checked', true);
                        $('#summarySection #summaryTable').show();
                    }

                    $('input[name = "deviceType"]').click(whitespaceFinder.onDeviceTypeSelectionChange);
                    $('input[name="protectedNav"]').click(whitespaceFinder.onProtectedEntitySelectionChanged);
                    $("#leftPaneContentPlaceHolder .search").keypress(whitespaceFinder.onEnterKeyPressed);
                    $('.protected-area .channels .channel-list ul li').click(whitespaceFinder.onProtectedEntityChannelSelected);

                    if (searchLocation === "") {
                        searchLocation = countryRegion;
                    }

                    $('#location').val(searchLocation);

                    HelpControl.reset();

                    onRegionFound(latitude, longitude, countryRegion, searchLocation);
                }
            },
            onError,
    false);
    }

    function ResetMapView(location, radius, segments) {
        var circle = bingMap.DrawCircle(location, radius, segments, {
            polygonOptions: {
                fillColor: new Microsoft.Maps.Color(50, 255, 255, 255),
                strokeColor: new Microsoft.Maps.Color(50, 255, 255, 255),
                strokeThickness: parseInt(1)
            }
        });

        bingMap.RemoveEntity(circle);

        bingMap.SetMapView(Microsoft.Maps.LocationRect.fromLocations(circle.getLocations()));
    }

    var onError = function (xhr, status, error) {
        ////TODO: code to handle search failure.
        var obj = error;
    }
};
