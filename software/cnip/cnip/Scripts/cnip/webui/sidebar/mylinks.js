$('#linksDiv').collapse({
    toggle: true
});
var linksTree = document.getElementById('linksUl');
initSingleSlider(document.getElementById('linksStrokeOpacity'), '', '0', '1', '0.1', '1');
document.getElementById('linksStrokeOpacity').addEventListener('change', setLinksStyle);
loadLinksTreeFromFile();
/* link functions*/
function setLinksStyle(e) {
    if (e) { e.stopPropagation(); }
    let custStyle = function (feature) {
        let strokeOpacity = Number($('#linksStrokeOpacity').val());
        return [new ol.style.Style({
            stroke: new ol.style.Stroke({
                color: feature.get('linktype') === 'internal' ?
                    colorWithAlpha('#00ffff', strokeOpacity) :
                    colorWithAlpha('#ffe300', strokeOpacity),
                lineDash: [7, 7],
                width: 4
            }),
            text: new ol.style.Text({
                text: feature.get('linkname'),
                font: '14px "Liberation Sans", "Lucida Sans", "Lucida Sans Regular", "Lucida Grande", "Lucida Sans Unicode", Geneva, Verdana, sans-serif',
                fill: new ol.style.Fill({ color: 'rgba(0,0,0,' + strokeOpacity + ')' }),
                stroke: new ol.style.Stroke({ color: 'rgba(0,0,0,' + strokeOpacity + ')', width: 2 })
            })
        })];
    };
    myLinksLayer.setStyle(custStyle);
    myMap.render();
}
function loadLinksTreeFromFile() {
    let httpRequest = new XMLHttpRequest();
    httpRequest.open('GET', '../' + vectorsPath + 'mylinks.geojson', true);
    httpRequest.send();
    httpRequest.onload = function () {
        let source = JSON.parse(httpRequest.responseText);
        linksTree.innerHTML = '';
        if (typeof source.features !== 'undefined') {
            for (let i = 0; i < source.features.length; i++) {
                linksTree.innerHTML = linksTree.innerHTML +
                    '<li class="visualLi" onclick="showLinkOptionsByIdx(event,' +
                    i.toString() + ');"><i class="fas fa-link"></i>&nbsp;' +
                    source.features[i].properties.linkname + '</li>';
            }
        }
    };
}
function loadLinksTreeFromSource() {
    let features = myLinksLayer.getSource().getFeatures();
    linksTree.innerHTML = '';
    for (let i = 0; i < features.length; i++) {
        linksTree.innerHTML = linksTree.innerHTML +
            '<li class="visualLi" onclick="showLinkOptionsByIdx(event,' +
            i.toString() + ');"><i class="fas fa-link"></i>&nbsp;' +
            features[i].getProperties().linkname + '</li>';
    }
}
function showLinkOptionsByIdx(e, Idx) {
    if (e) { e.stopPropagation(); }
    if (actionRunning) { return; }
    document.getElementById('linkOptions').style.display = 'block';
    document.getElementById('linkOptions').style.left = 'calc(50vw - 651px / 2)';
    document.getElementById('linkOptions').style.top = 'calc(50vh - 432px / 2)';
    document.getElementById('linkOptions').style.width = '651px';
    document.getElementById('linkOptions').style.height = '432px';
    document.getElementById('linkOptionsFrame').contentWindow.userDisplay = true;
    document.getElementById('linkOptionsFrame').contentWindow.refreshLinkData('', Idx);
    setDialogZIndex(null, 'linkOptions');
}
function getLinkDataByIdx(Idx) {
    return getDataByIdx(myLinksLayer.getSource(), Idx);
}
function getLinkProfileByIdx(Idx) {
    let returnString = '';
    if (Idx) {
        if (Number(Idx) > -1) {
            let feature = myLinksLayer.getSource().getFeatures()[Idx];
            let keys = feature.getKeys();
            for (let j = 0; j < keys.length; j++) {
                if (keys[j] !== 'geometry') {
                    returnString += feature.get(keys[j]).toString() + '\t';
                }
            }
            returnString = returnString.substring(0, returnString.length - 1);
        }
    }
    return returnString;
}
function getPublicSiteData(siteidb, email) {
    let feature = getPublicSite(siteidb, email);
    let returnString = '';
    if (feature) {
        let keys = feature.getKeys();
        for (let j = 0; j < keys.length; j++) {
            if (keys[j] !== 'geometry') {
                returnString += feature.get(keys[j]).toString() + '\t';
            }
        }
        returnString = returnString.substring(0, returnString.length - 1);
    }
    return returnString;
}
function getPublicSite(siteidb, email) {
    if (siteidb) {
        if (Number(siteidb) > -1) {
            if (email) {
                let features = publicNetworkLayer.getSource().getFeatures();
                for (let i = 0; i < features.length; i++) {
                    if (features[i].get('siteid') === siteidb && features[i].get('email') === email) {
                        return features[i];
                        break;
                    }
                }
            }
        }
    }
}
function getLinkIdxById(linkid) {
    return getIdxById(myLinksLayer.getSource(), 'linkid', linkid);
}
function zoomToLinkByIdx(Idx) {
    zoomToFeatureByIdx(myLinksLayer.getSource(), Idx);
}
function updateLinkDataByIdx(options) {
    if (options.idx) {
        if (Number(options.idx) > -1) {
            let feature = myLinksLayer.getSource().getFeatures()[options.idx];
            feature.set('linkname', options.linkname.toString());
            feature.set('channelwidtha', options.channelwidtha.toString());
            feature.set('frequencya', options.frequencya.toString());
            feature.set('outputpowera', options.outputpowera.toString());
            feature.set('antennagaina', options.antennagaina.toString());
            feature.set('lossesa', options.lossesa.toString());
            feature.set('channelwidthb', options.channelwidthb.toString());
            feature.set('frequencyb', options.frequencyb.toString());
            feature.set('outputpowerb', options.outputpowerb.toString());
            feature.set('antennagainb', options.antennagainb.toString());
            feature.set('lossesb', options.lossesb.toString());
            myMap.render();
            loadLinksTreeFromSource();
        }
    }
}
function getLinksCount() {
    return getFeaturesCount(myLinksLayer.getSource());
}
function deleteLinkByIdx(Idx) {
    deleteFeatureByIdx(myLinksLayer.getSource(), Idx);
    loadLinksTreeFromSource();
    clearInteractions();
}
function deleteLinksBySiteId(siteid) {
    let feature = undefined;
    feature = myLinksLayer.getSource().getFeatureByProperty('siteida', siteid);
    while (feature) {
        myLinksLayer.getSource().removeFeature(feature);
        feature = myLinksLayer.getSource().getFeatureByProperty('siteida', siteid);
    }
    feature = myLinksLayer.getSource().getFeatureByProperty('siteidb', siteid);
    while (feature) {
        myLinksLayer.getSource().removeFeature(feature);
        feature = myLinksLayer.getSource().getFeatureByProperty('siteidb', siteid);
    }
    myMap.render();
    loadLinksTreeFromSource();
    clearInteractions();
}
myLinksLayer.getSource().on('addfeature', function (event) {
    if (event.feature.get('linkid') === undefined) {
        myLinksLayer.getSource().removeFeature(event.feature);
    }
});
function downloadLinksCSV(e) {
    if (e) { e.stopPropagation(); }
    if (actionRunning) { return; }
    showProgress('Exporting CSV');
    setTimeout(function () {
        download.CSV(myLinksLayer.getSource(), 'mylinks');
        closeProgress();
    }, 500);
}
function downloadLinksJson(e) {
    if (e) { e.stopPropagation(); }
    if (actionRunning) { return; }
    showProgress('Exporting JSON');
    setTimeout(function () {
        download.JSON(myLinksLayer.getSource(), 'mylinks');
        closeProgress();
    }, 500);
}
function downloadLinksKML(e) {
    if (e) { e.stopPropagation(); }
    if (actionRunning) { return; }
    showProgress('Exporting KML');
    setTimeout(function () {
        download.KML(myLinksLayer.getSource(), 'mylinks');
        closeProgress();
    }, 500);
}
function saveVectorsToServer(e) {
    if (e) { e.stopPropagation(); }
    if (getActionRunning()) { return; }
    showProgress('Saving Vectors');
    setTimeout(function () {
        saveAllVectors();
        closeProgress();
    }, 500);
}
function createLink(options) {
    let linkid = getNewIdx(myLinksLayer.getSource(), 'linkid');
    let elevationString = Ajax_runccmd("GetElevationStringFromLineString",
        "{'lineString':'LINESTRING(" +
        options.locationA[0] + " " +
        options.locationA[1] + "," +
        options.locationB[0] + " " +
        options.locationB[1] + ")'}").split('#');
    let distanceToLocationB = Ajax_runccmd("Distance",
        "{'longitude1':'" + options.locationA[0] +
        "','latitude1':'" + options.locationA[1] +
        "','longitude2':'" + options.locationB[0] +
        "','latitude2':'" + options.locationB[1] + "'}") / 1000;
    let feature = new ol.Feature({
        'linkid': linkid,
        'linkname': 'my link ' + linkid,
        'linktype': 'internal',
        'siteida': options.siteida,
        'locheighta': elevationString[0],
        'bearinga': round(turf.bearingToAzimuth(turf.bearing(
            turf.point(options.locationA), turf.point(options.locationB))), 2),
        'channelwidtha': document.getElementById('opt_channelwidth').value,
        'frequencya': document.getElementById('opt_frequency').value,
        'outputpowera': document.getElementById('opt_outputpower').value,
        'antennagaina': document.getElementById('opt_antennagain').value,
        'lossesa': document.getElementById('opt_losses').value,
        'siteidb': options.siteidb,
        'locheightb': elevationString[1],
        'bearingb': round(turf.bearingToAzimuth(turf.bearing(
            turf.point(options.locationB), turf.point(options.locationA))), 2),
        'channelwidthb': document.getElementById('opt_channelwidth').value,
        'frequencyb': document.getElementById('opt_frequency').value,
        'outputpowerb': document.getElementById('opt_outputpower').value,
        'antennagainb': document.getElementById('opt_antennagain').value,
        'lossesb': document.getElementById('opt_losses').value,
        'distance': distanceToLocationB,
        'elevstr': elevationString[2],
        geometry: new ol.geom.LineString(
            [options.locationA, options.locationB]).transform('EPSG:4326', 'EPSG:3857')
    });
    myLinksLayer.getSource().addFeature(feature);
    myMap.render();
    loadLinksTreeFromSource();
}