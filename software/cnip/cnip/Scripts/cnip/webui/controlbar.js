var mainControlBar = new ol.control.Bar();
myMap.addControl(mainControlBar);
var nestedControlBar = new ol.control.Bar({ toggleOne: true, group: true });
mainControlBar.addControl(nestedControlBar);
var selectControl = new ol.control.Toggle(
    {
        html: '<i class="fas fa-mouse-pointer"></i>',
        className: 'select',
        title: 'Select',
        interaction: select,
        active: true,
        onToggle: function (active) {
            setActiveInteraction(null,'toggleSelect', active);
        }
    });
nestedControlBar.addControl(selectControl);
var measureLineControl = new ol.control.Toggle(
    {
        html: '<i class="fas fa-ruler"></i>',
        className: 'draw',
        title: 'Measure Line',
        active: false,
        onToggle: function (active) {
            setActiveInteraction(null,'toggleMeasureLine', active);
        }
    });
nestedControlBar.addControl(measureLineControl);
var measureAreaControl = new ol.control.Toggle(
    {
        html: '<i class="fas fa-ruler-combined"></i>',
        className: 'draw',
        title: 'Measure Area',
        active: false,
        onToggle: function (active) {
            setActiveInteraction(null,'toggleMeasureArea', active);
        }
    });
nestedControlBar.addControl(measureAreaControl);
var downloadPngControl = new ol.control.Button(
    {
        html: '<i class="fa fa-download"></i>',
        className: 'download',
        title: 'Download PNG',
        handleClick: function () {
            if (actionRunning) { return; }
            myMap.once('postrender', function (event) {
                hideAllDialogs();
                showProgress('Exporting PNG');
                setTimeout(function () {
                    closeProgress();
                    if (browser === 'edge') {
                        html2canvas(myMap.getTargetElement(), { ignoreElements: ignoreElements }).then(function (canvas) {
                            let base64image = canvas.toDataURL("image/png");
                            let block = base64image.split(";");
                            let mimeType = block[0].split(":")[1];
                            let realData = block[1].split(",")[1];
                            let canvasBlob = b64toBlob(realData, mimeType);
                            saveAs(canvasBlob, "map.png");
                        });
                    }
                    else {
                        domtoimage.toBlob(myMap.getTargetElement(), { filter: selectElements })
                            .then(function (blob) {
                                saveAs(blob, 'map.png');
                            });
                    }
                    unHideAllDialogs();
                }, 500);
            });
            myMap.render();
        }
    });
function selectElements(element) {
    return (element.className ? element.className.indexOf('ol-control') === -1 : true)
        && element.id !== 'activeInteraction';
}
function ignoreElements(element) {
    return (element.className ? element.className.indexOf('ol-control') !== -1 : false)
        || element.id === 'activeInteraction';
}
function b64toBlob(b64Data, contentType, sliceSize) {
    contentType = contentType || '';
    sliceSize = sliceSize || 512;
    let byteCharacters = atob(b64Data);
    let byteArrays = [];
    for (let offset = 0; offset < byteCharacters.length; offset += sliceSize) {
        let slice = byteCharacters.slice(offset, offset + sliceSize);
        let byteNumbers = new Array(slice.length);
        for (let i = 0; i < slice.length; i++) {
            byteNumbers[i] = slice.charCodeAt(i);
        }
        let byteArray = new Uint8Array(byteNumbers);
        byteArrays.push(byteArray);
    }
    let blob = new Blob(byteArrays, { type: contentType });
    return blob;
}
mainControlBar.addControl(downloadPngControl);
var myLocationControl = new ol.control.Button(
    {
        html: '<i class="fas fa-map-marker-alt"></i>',
        className: 'mylocation',
        title: 'My Location',
        handleClick: function () {
            if (actionRunning) { return; }
            //let geolocation = new ol.Geolocation();
            //geolocation.setTracking(true);
            //geolocation.on('change', function () {
            //    myMap.getView().animate({
            //        duration: 700,
            //        center: ol.proj.fromLonLat(geolocation.getPosition())
            //    });
            //    geolocation.setTracking(false);
            //});
            navigator.geolocation.getCurrentPosition(function (pos) {
                myMap.getView().animate({
                    duration: 700,
                    center: ol.proj.fromLonLat([pos.coords.longitude, pos.coords.latitude])
                });
            }, function (error) {
                
            }, {
                    enableHighAccuracy: true,
                    timeout: 5000,
                    maximumAge: 0
                });
        }
    });
mainControlBar.addControl(myLocationControl);
myLocationControl.button_.click();
var fullscreenControl = new ol.control.FullScreen({
    source: 'fullscreen'
});
mainControlBar.addControl(fullscreenControl);
var searchGpsControl = new ol.control.SearchGPS({
});
myMap.addControl(searchGpsControl);
searchGpsControl.button.title = 'Search GPS';
searchGpsControl.on('select', function (event) {
    if (actionRunning) { return; }
    myMap.getView().animate({
        center: event.search.coordinate,
        zoom: Math.max(myMap.getView().getZoom(), 5)
    });
});