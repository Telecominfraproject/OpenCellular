$('#polygonsDiv').collapse({
    toggle: true
});
var polygonsTree = document.getElementById('polygonsUl');

var fileupload = document.getElementById("FileUpload1");
var button = document.getElementById("btnFileUpload");
button.onclick = function () {
    fileupload.click();
};
fileupload.onchange = function (evt) {
    if (evt) { evt.stopPropagation(); }
    if (getActionRunning()) { return; }
    if (evt.target.files) {
        var reader = new FileReader();
        var file = evt.currentTarget.files[0];
        var filename = file.name;
        if (!filename.includes('json') ||
            !filename.includes('geojson')) {
            showMessage("Polygon Upload", "Please provide polygons in Geojson format.");
        }
        else {
            reader.readAsText(file, "UTF-8");
            reader.onload = function (evt) {
                showProgress('Loading Polygons');
                setTimeout(function () {
                    // save current polygons
                    uploadDataToUrl(myPolygonsLayer.getSource().getJSONString(),
                        vectorsPath + 'mypolygons.geojson');
                    Ajax_runccmd("UploadPolygonsToDB", "");
                    myMap.render();
                    loadPolygonsTreeFromFile();
                    // load user polygons
                    uploadDataToUrl(evt.target.result,
                        vectorsPath + "t" + filename);
                    Ajax_runccmd("UploadUserPolygonsToDB",
                        "{'polygonname':'t" + filename + "'}");
                    // reload polygons from server
                    Ajax_runccmd("GetPolygonsFromDB", "");
                    Ajax_loadSourceFromFile(myPolygonsLayer.getSource(),
                        "../" + vectorsPath + "mypolygons.geojson");
                    myMap.render();
                    loadPolygonsTreeFromFile();
                    closeProgress();
                }, 500);
            };
        }
        fileupload.value = null;
    }
};
initSingleSlider(document.getElementById('polygonsFillOpacity'), '', '0', '1', '0.1', '0.2');
initSingleSlider(document.getElementById('polygonsStrokeOpacity'), '', '0', '1', '0.1', '1');
document.getElementById('polygonsFillOpacity').addEventListener('change', setPolygonsStyle);
document.getElementById('polygonsStrokeOpacity').addEventListener('change', setPolygonsStyle);
loadPolygonsTreeFromFile();
/* polygon functions*/
function setPolygonsStyle(e) {
    if (e) { e.stopPropagation(); }
    let custStyle = function (feature) {
        let fillOpacity = Number($('#polygonsFillOpacity').val());
        let strokeOpacity = Number($('#polygonsStrokeOpacity').val());
        return [new ol.style.Style({
            fill: new ol.style.Fill({
                color: colorWithAlpha('#ffffff', fillOpacity)
            }),
            stroke: new ol.style.Stroke({
                color: colorWithAlpha('#ffcc33', strokeOpacity),
                width: 3
            }),
            text: new ol.style.Text({
                text: feature.get('polygonname'),
                font: '14px "Liberation Sans", "Lucida Sans", "Lucida Sans Regular", "Lucida Grande", "Lucida Sans Unicode", Geneva, Verdana, sans-serif',
                fill: new ol.style.Fill({ color: 'rgba(0,0,0,' + strokeOpacity + ')' }),
                stroke: new ol.style.Stroke({ color: 'rgba(255,255,255,' + strokeOpacity + ')', width: 2 })
            })
        })];
    };
    myPolygonsLayer.setStyle(custStyle);
    myMap.render();
}
function loadPolygonsTreeFromFile() {
    let httpRequest = new XMLHttpRequest();
    httpRequest.open('GET', '../' + vectorsPath + 'mypolygons.geojson', true);
    httpRequest.send();
    httpRequest.onload = function () {
        let source = JSON.parse(httpRequest.responseText);
        polygonsTree.innerHTML = '';
        if (typeof source.features !== 'undefined') {
            for (let i = 0; i < source.features.length; i++) {
                polygonsTree.innerHTML = polygonsTree.innerHTML +
                    '<li class="visualLi" onclick="showPolygonOptionsByIdx(event,' +
                    i.toString() + ');"><i class="fas fa-draw-polygon"></i>&nbsp;' +
                    truncate(source.features[i].properties.polygonname, 27) + '</li>';
            }
        }
    };
}
function loadPolygonsTreeFromSource() {
    let features = myPolygonsLayer.getSource().getFeatures();
    polygonsTree.innerHTML = '';
    for (let i = 0; i < features.length; i++) {
        polygonsTree.innerHTML = polygonsTree.innerHTML +
            '<li class="visualLi" onclick="showPolygonOptionsByIdx(event,' +
            i.toString() + ');"><i class="fas fa-draw-polygon"></i>&nbsp;' +
            truncate(features[i].getProperties().polygonname, 27) + '</li>';
    }
}
function showPolygonOptionsByIdx(e, Idx) {
    if (e) { e.stopPropagation(); }
    if (actionRunning) { return; }
    document.getElementById('polyOptions').style.display = 'block';
    document.getElementById('polyOptions').style.left = 'calc(50vw - 351px / 2)';
    document.getElementById('polyOptions').style.top = 'calc(50vh - 236px / 2)';
    document.getElementById('polyOptions').style.width = '351px';
    document.getElementById('polyOptions').style.height = '236px';
    document.getElementById('polyOptionsFrame').contentWindow.userDisplay = true;
    document.getElementById('polyOptionsFrame').contentWindow.refreshPolygonData('', Idx);
    setDialogZIndex(null, 'polyOptions');
}
myPolygonsLayer.getSource().on('addfeature', function (event) {
    if (event.feature.get('polygonid') === undefined) {
        let newid = getNewIdx(myPolygonsLayer.getSource(), 'polygonid');
        event.feature.setProperties({ 'polygonid': newid, 'polygonname': 'new polygon ' + newid });
        loadPolygonsTreeFromSource();
        document.getElementById('polyOptionsFrame').contentWindow.refreshPolygonData('add');
    }
});
function getPolygonDataByIdx(Idx) {
    return getDataByIdx(myPolygonsLayer.getSource(), Idx);
}
function getPolygonIdxById(polygonid) {
    return getIdxById(myPolygonsLayer.getSource(), 'polygonid', polygonid);
}
function zoomToPolygonByIdx(Idx) {
    zoomToFeatureByIdx(myPolygonsLayer.getSource(), Idx);
}
function updatePolygonDataByIdx(Idx, polygonname) {
    if (Idx) {
        if (Number(Idx) > -1) {
            let feature = myPolygonsLayer.getSource().getFeatures()[Idx];
            feature.set('polygonname', polygonname.toString());
            myMap.render();
            loadPolygonsTreeFromSource();
        }
    }
}
function getPolygonsCount() {
    return getFeaturesCount(myPolygonsLayer.getSource());
}
function deletePolygonByIdx(Idx) {
    deleteFeatureByIdx(myPolygonsLayer.getSource(), Idx);
    loadPolygonsTreeFromSource();
    clearInteractions();
}
function downloadPolygonsCSV(e) {
    if (e) { e.stopPropagation(); }
    if (actionRunning) { return; }
    showProgress('Exporting CSV');
    setTimeout(function () {
        download.CSV(myPolygonsLayer.getSource(), 'mypolygons');
        closeProgress();
    }, 500);
}
function downloadPolygonsJson(e) {
    if (e) { e.stopPropagation(); }
    if (actionRunning) { return; }
    showProgress('Exporting JSON');
    setTimeout(function () {
        download.JSON(myPolygonsLayer.getSource(), 'mypolygons');
        closeProgress();
    }, 500);
}
function downloadPolygonsKML(e) {
    if (e) { e.stopPropagation(); }
    if (actionRunning) { return; }
    showProgress('Exporting KML');
    setTimeout(function () {
        download.KML(myPolygonsLayer.getSource(), 'mypolygons');
        closeProgress();
    }, 500);
}
