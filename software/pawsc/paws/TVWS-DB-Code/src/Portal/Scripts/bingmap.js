// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

function BingMap(credentials, mapControl) {
    var map = null;
    var searchManager = null;
    var bingSpatialDataServiceBaseUrl = "http://platform.bing.com/geo/spatial/v1/public/Geodata?SpatialFilter=";
    var safeCharacters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-";
    var mapCredentials = credentials;
    var mapDiv = mapControl;
    var geoLocationProvider = null;
    var deferred = $.Deferred();

    var self = this;

    this.SearchRequestType = {
        geocodeRequest: 1,
        searchRequest: 2,
        reverseGeocodeRequest: 3
    };

    this.LoadMap = (function () {
        if (!map) {
            Microsoft.Maps.loadModule('Microsoft.Maps.Themes.BingTheme', {
                callback: function () {
                    map = GetMapObject();
                    deferred.resolve();
                }
            });
        }
    }());

    this.UnloadMap = function () {
        if (map) {
            map.dispose();
        };
    };

    this.locationFinder = function (request, searchRequestType) {
        registerSerachModule(request, searchRequestType);
    };

    this.findAddress = function (request, drawBorder, searchRequestType) {
        if (map.entities) {
            map.entities.clear();
        }

        if (drawBorder) {
            Microsoft.Maps.loadModule('Microsoft.Maps.AdvancedShapes', {
                callback: function () {
                    getBoundary(request.where);
                }
            });
        }

        registerSerachModule(request, searchRequestType);
    };

    this.LoadPushPinsForRegions = function (regions, pushPinIcon) {
        //[Temporary Fix]: Method being called more often at the time of page load, so following
        // operation has to be deferred until map object get initialized.
        deferred.then(function () {
            var locations = [];

            if (map.entities) {
                map.entities.clear();
            }

            for (var index = 0; index < regions.length; index++) {

                var location = new Microsoft.Maps.Location(regions[index].Latitude, regions[index].Longitude);

                var pushpinOptions = {
                    text: (index + 1),
                    zIndex: 100,
                    icon: pushPinIcon,
                    height: 36,
                    width: 26
                };

                var pushpin = self.addPushPin(location, pushpinOptions);

                self.RegionInfobox.initializeInfobox(regions[index].Name, pushpin)

                locations.push(location);
            }

            var viewBoundaries = Microsoft.Maps.LocationRect.fromLocations(locations);
            map.setView({ zoom: 1, bounds: viewBoundaries });
        })
    };

    this.addPushPin = function (location, pushpinOptions) {

        var pushpin = new Microsoft.Maps.Pushpin(location, pushpinOptions);

        map.entities.push(pushpin);

        return pushpin;
    };

    this.addInfobox = function (location, infoboxOptions) {
        var infobox = new Microsoft.Maps.Infobox(location, infoboxOptions);

        map.entities.push(infobox);

        return infobox;
    };

    this.ZoomtoLocation = function (latitude, longitude, zoomlevel) {
        var Loc = new Microsoft.Maps.Location(latitude, longitude);
        var viewOptions = { center: Loc, zoom: zoomlevel };
        map.setView(viewOptions);
    };

    this.ClearMapEntities = function () {
        map.entities.clear();
    };

    this.SetViewBoundary = function (locations) {
        var viewBoundaries = Microsoft.Maps.LocationRect.fromLocations(locations);
        map.setView({ bounds: viewBoundaries });
    }

    this.SetMapView = function (locationRect) {
        map.setView({ bounds: locationRect });
    }

    this.DrawPolygon = function (vertices, polygonOptions) {
        var polygon = new Microsoft.Maps.Polygon(vertices, polygonOptions);
        //map.setView({ bounds: Microsoft.Maps.LocationRect.fromLocations(vertices) });
        map.entities.push(polygon);

        return polygon;
    }

    this.DrawCircle = function (center, radiusInMeters, segments, polygonOptions) {

        geoLocationProvider = new Microsoft.Maps.GeoLocationProvider(map);

        geoLocationProvider.addAccuracyCircle(center, radiusInMeters, segments, { polygonOptions: polygonOptions });

        return map.entities.get(map.entities.getLength() - 1);
    }

    this.RemoveEntity = function (entity) {
        return map.entities.remove(entity);
    }

    this.RemoveEntityAt = function (index) {
        return map.entities.removeAt(index);
    }

    this.InsertEntityAt = function (entity, index) {
        map.entities.insertAt(entity, index);
    }

    this.GetIndexofEntity = function (entity) {
        return map.entities.indexOf(entity);
    }

    this.GetEntityAt = function (index) {
        return map.entities.get(index);
    }

    this.PushEntity = function (entity) {
        map.entities.push(entity);
    }

    this.PopEntity = function () {
        return map.entities.pop();
    }

    this.GetMapBounds = function () {
        return map.getBounds();
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

    this.RegionInfobox = function () {

        var getInfoboxHtml = function (region, pushpin, callback) {
            ajaxRequest(
                "POST",
                "/Home/GetRegionChannelsDetailView",
                { regionName: region },
                function (html) {
                    if (html) {
                        var infoboxHtml = $(html);
                        infoboxHtml.find('.infobox-stalk').remove();

                        var infoboxOptions = {
                            width: 200,
                            height: 100,
                            showCloseButton: true,
                            zIndex: 0,
                            offset: new Microsoft.Maps.Point(0, 0),
                            showPointer: true,
                            htmlContent: html,
                            pushpin: pushpin
                        };

                        var infobox = self.addInfobox(pushpin.getLocation(), infoboxOptions);

                        Microsoft.Maps.Events.addHandler(pushpin, 'mouseover', function (e) {
                            var currentInfobox = e.target._infobox;

                            var htmlObj = $(currentInfobox.getHtmlContent());
                            var infoboxHtml = currentInfobox.getHtmlContent();

                            htmlObj.find('.infobox-stalk').remove();

                            infoboxHtml = htmlObj.html();

                            infobox.setHtmlContent('<div class="infobox">' + infoboxHtml + '</div>');
                        });

                        Microsoft.Maps.Events.addHandler(pushpin, 'click', function (e) {
                            var currentInfobox = e.target._infobox;
                            var htmlObj = $(currentInfobox.getHtmlContent());
                            var infoboxHtml = currentInfobox.getHtmlContent();

                            if ($(infoboxHtml).find('.infobox-stalk').length === 0) {

                                $('<div class="infobox-stalk"></div>').insertAfter(htmlObj.find('.infobox-body'));
                                infoboxHtml = '<div class="infobox">' + htmlObj.html() + '</div>';
                            }

                            infobox.setHtmlContent(infoboxHtml)
                        });

                        if (callback) {
                            callback();
                        }
                    }
                },
                function () {
                    // If ajax request fails to get region specific infobox, then use default infobox.
                    var infoboxOptions = {
                        title: region,
                        offset: new Microsoft.Maps.Point(10, 0),
                        pushpin: pushpin,
                        id: region
                    };

                    self.addInfobox(pushpin.getLocation(), infoboxOptions);

                    if (callback) {
                        callback();
                    }
                },
                true);
        }

        var initializeInfobox = function (region, pushpin, callback) {
            getInfoboxHtml(region, pushpin, callback);
        }

        getInfoboxHtml.cache = {};

        return {
            initializeInfobox: initializeInfobox
        };
    }();


    var GetMapObject = function () {
        return new Microsoft.Maps.Map(mapDiv, {
            credentials: mapCredentials,
            mapTypeId: Microsoft.Maps.MapTypeId.road,
            theme: new Microsoft.Maps.Themes.BingTheme(),
            enableSearchLogo: false,
            enableClickableLogo: false,
            showMapTypeSelector: false,
            labelOverlay: Microsoft.Maps.LabelOverlay.hidden
        });
    }

    var CreateSearchManager = function () {
        map.addComponent('searchManager', new Microsoft.Maps.Search.SearchManager(map));
        searchManager = map.getComponent('searchManager');
    }

    var getBoundary = function (address) {

        var levelOfDetail = 3; // Specifies the best, 
        var culture = "en-us";
        var entityType = "CountryRegion";
        var getAllPolygons = false;
        var getEntityMetadata = true;

        map.getCredentials(function (credentials) {

            // For more info refer: http://msdn.microsoft.com/en-us/library/dn306801.aspx.
            var boundaryUrl = bingSpatialDataServiceBaseUrl
            + "GetBoundary(" + "'" + address + "'" + "," + levelOfDetail + ",'" + entityType + "'," + 1 + ',' + getAllPolygons + ',' + getEntityMetadata + ",'" + culture + "')" + "&$format=json&key="
            + credentials;

            ServerManager.GetJsonPData({
                url: boundaryUrl,
                success: boundaryCallback,
                error: function (xhr, status, error) {
                    //alert("Error Occurred");
                }
            });

        });
    };

    var registerSerachModule = function (requestData, searchRequestType) {
        if (!map) {
            this.LoadMap();
        }

        Microsoft.Maps.loadModule('Microsoft.Maps.Search', {
            callback: function () {
                switch (searchRequestType) {
                    case self.SearchRequestType.searchRequest:
                        searchRequest(requestData)
                        break;
                    case self.SearchRequestType.geocodeRequest:
                        geocodeRequest(requestData)
                        break;
                    case self.SearchRequestType.reverseGeocodeRequest:
                        reverseGeocodeRequest(requestData);
                        break;
                }
            }
        });
    };

    var loadSearchManagerModule = function () {
        if (!searchManager) {
            map.addComponent('searchManager', new Microsoft.Maps.Search.SearchManager(map));
            searchManager = map.getComponent('searchManager');
        }
    };

    var geocodeRequest = function (request) {
        loadSearchManagerModule();

        request.bounds = map.getBounds();

        searchManager.geocode(request);
    };

    var reverseGeocodeRequest = function (request) {
        loadSearchManagerModule();

        searchManager.reverseGeocode(request);
    };

    var searchRequest = function (request) {
        loadSearchManagerModule();

        searchManager.search(request);
    };

    this.onSearchSuccess = function (searchResult, userData) {
        if (searchResult) {
            var searchRegion = searchResult.searchRegion;

            if (searchRegion) {

                map.setView({ bounds: searchRegion.mapBounds });

                var pushPin = new Microsoft.Maps.Pushpin(searchRegion.explicitLocation.location, null);
                var infoBox = new Microsoft.Maps.Infobox(searchRegion.explicitLocation.location, { title: searchRegion.explicitLocation.name, pushpin: pushPin });

                map.entities.push(pushPin);
                map.entities.push(infoBox);
                pushPin.setOptions({ state: Microsoft.Maps.EntityState.selected });

                if (!searchRegion.address.countryRegion) {
                    var reverseGeocoderequest = {
                        location: searchRegion.explicitLocation.location,
                        count: 10,
                        callback: reverseGeocodeCallback,
                        errorCallback: onSerachFailed,
                        userData: infobox
                    }

                    self.locationFinder(reverseGeocodeCallback, self.SearchRequestType.reverseGeocodeRequest);
                }
            }
            else {
                $('#errorMessage').show();
            }
        }
    };

    this.onGeocodeSuccess = function (geoCodeResult, userData) {
        if (geoCodeResult) {
            var topResult = geoCodeResult.results && geoCodeResult.results[0];
            var infoBox = null;
            if (topResult) {
                map.setView({ bounds: topResult.bestView });

                if (userData && userData.callback) {
                    userData.callback(topResult);
                }
                else {

                    var pushpinOptions = {
                        zIndex: 100,
                        icon: '/Content/Images/PushPin.png',
                        height: 36,
                        width: 26
                    };

                    var pushPin = new Microsoft.Maps.Pushpin(topResult.location, pushpinOptions);
                    infoBox = new Microsoft.Maps.Infobox(topResult.location, { title: topResult.name, pushpin: pushPin });

                    map.entities.push(pushPin);
                    map.entities.push(infoBox);

                    pushPin.setOptions({ state: Microsoft.Maps.EntityState.selected });
                }

                // Check if topResult.name contains only a region name / countryRegion.
                if (geoCodeResult.parsedAddress.countryRegion == null || geoCodeResult.parsedAddress.countryRegion == "") {

                    // Check if the pin title is just the lat/long coordinates -- if so, then clear out the title field so that it will get overwritten by the reverseGeoCodeRequest.
                    var searchResult = topResult.name.split(",");
                    if (searchResult.length == 2 && searchResult[0].trim() == topResult.location.latitude && searchResult[1].trim() == topResult.location.longitude) {
                        infoBox._title = "";
                    }

                    var request = {
                        location: topResult.location,
                        count: 10,
                        callback: reverseGeocodeCallback,
                        errorCallback: self.onSerachFailed,
                        userData: infoBox
                    }

                    self.locationFinder(request, self.SearchRequestType.reverseGeocodeRequest)
                }
                //else if (geoCodeResult.parsedAddress.countryRegion.toLowerCase() != "") {//$('.finderAddHeader').text().toLowerCase()) { TODO: match it with HTML finder text value.
                //    //DisplayError();
                //}
            }
            else {
                DisplayNoResultError();
            }
        }
    };

    var reverseGeocodeCallback = function (result, userData) {
        if (result) {
            var countryRegion = result.name.split(",");

            var pinTitle = userData._title;

            // if the caller did not pass in a pin title, then use the friendly name that is returned from bing.
            if (pinTitle == null || pinTitle == "") {
                pinTitle = result.name;
            }
            userData.setOptions({ title: pinTitle });

            if (countryRegion[countryRegion.length - 1].trim().toLowerCase() != "") {//$('.finderAddHeader').text().toLowerCase()) {TODO: match it with HTML finder text value.
                DisplayReverseGeocodeError(result.name);
            }
        }
        else {
            DisplayNoResultError();
        }
    };

    this.onSerachFailed = function (result, userData) {
        //TODO: Display error messages here.
    };

    var DisplayReverseGeocodeError = function (locationTxt) {
        //TODO: Error message to be displayed when reverse geocode failed.
    };

    var DisplayNoResultError = function () {
        //TODO: Custom error to suggest no results found for the search.
    };

    var boundaryCallback = function (result) {

        var entity = result.d.results[0];
        var entityMetadata = entity.EntityMetadata;
        var entityName = entity.Name.EntityName;
        var primitives = entity.Primitives;

        var polygoncolor = null;
        var strokecolor = null;
        var numOfVertices = 0;

        polygoncolor = new Microsoft.Maps.Color(30, 0, 255, 0);
        strokecolor = new Microsoft.Maps.Color(0, 0, 0, 0);

        // TODO: Following logic may lead to decrease in performance when the amount of data obtained from Spatial Data Service is more. 
        // Need to find a right fix here. 
        var polygonArray = new Array();
        for (var i = 0; i < primitives.length; i++) {
            var ringStr = primitives[i].Shape;
            var ringArray = ringStr.split(",");

            for (var j = 1; j < ringArray.length; j++) {
                var array = ParseEncodedValue(ringArray[j]);

                if (array.length > numOfVertices) {
                    numOfVertices = array.length;
                }

                var polygon = new Microsoft.Maps.Polygon(array, { fillColor: polygoncolor, strokeColor: strokecolor, strokeThickness: 1 });

                map.entities.push(polygon)
            }
        }
    };

    // Point Compression Algorithm. 
    //For more Info refer:http://msdn.microsoft.com/en-us/library/jj158958.aspx.
    var ParseEncodedValue = function (value) {
        var list = new Array();
        var index = 0;
        var xsum = 0;
        var ysum = 0;
        var max = 4294967296;

        while (index < value.length) {
            var n = 0;
            var k = 0;

            while (1) {
                if (index >= value.length) {
                    return null;
                }
                var b = safeCharacters.indexOf(value.charAt(index++));
                if (b == -1) {
                    return null;
                }
                var tmp = ((b & 31) * (Math.pow(2, k)));

                var ht = tmp / max;
                var lt = tmp % max;

                var hn = n / max;
                var ln = n % max;

                var nl = (lt | ln) >>> 0;
                n = (ht | hn) * max + nl;
                k += 5;
                if (b < 32) break;
            }

            var diagonal = parseInt((Math.sqrt(8 * n + 5) - 1) / 2);
            n -= diagonal * (diagonal + 1) / 2;
            var ny = parseInt(n);
            var nx = diagonal - ny;
            nx = (nx >> 1) ^ -(nx & 1);
            ny = (ny >> 1) ^ -(ny & 1);
            xsum += nx;
            ysum += ny;
            var lat = ysum * 0.00001;
            var lon = xsum * 0.00001
            list.push(new Microsoft.Maps.Location(lat, lon));
        }

        return list;
    };
};
