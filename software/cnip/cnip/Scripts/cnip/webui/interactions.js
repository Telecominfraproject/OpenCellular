// Select interaction
var select = new ol.interaction.Select({
    condition: ol.events.condition.singleClick,
    hitTolerance: 10,
    wrapX: false,
    toggleCondition: ol.events.condition.shiftKeyOnly,
    style: selectStyle
});
myMap.addInteraction(select);
select.setActive(false);
selection = select.getFeatures();
selection.on('add', function (e) {
    if (actionRunning) { return; }
    let feature = e.target.getArray()[0];
    if (feature.get('linkid') !== undefined) {
        if (e) { e.stopPropagation(); }
        //display link profile
        document.getElementById('linkProfile').style.display = 'block';
        document.getElementById('linkProfileFrame').contentWindow.refreshLinkDataById(feature.get('linkid'));
        document.getElementById('linkProfileFrame').contentWindow.userDisplay = true;
        setDialogZIndex(null, 'linkProfile');
        clearInteractions();
    }
    if (feature.get('noteid') !== undefined) {
        if (feature.get('email') === undefined) {
            if (e) { e.stopPropagation(); }
            document.getElementById('noteOptions').style.display = 'block';
            document.getElementById('noteOptionsFrame').contentWindow.refreshNoteDataById(feature.get('noteid'));
            document.getElementById('noteOptionsFrame').contentWindow.userDisplay = true;
            setDialogZIndex(null, 'noteOptions');
            clearInteractions();
        }
    }
    if (feature.get('polygonid') !== undefined) {
        if (e) { e.stopPropagation(); }
        document.getElementById('polyOptions').style.display = 'block';
        document.getElementById('polyOptionsFrame').contentWindow.refreshPolygonDataById(feature.get('polygonid'));
        document.getElementById('polyOptionsFrame').contentWindow.userDisplay = true;
        setDialogZIndex(null, 'polyOptions');
        clearInteractions();
    }
    if (feature.get('siteid') !== undefined) {
        if (feature.get('email') === undefined) {
            if (e) { e.stopPropagation(); }
            document.getElementById('siteOptions').style.display = 'block';
            document.getElementById('siteOptionsFrame').contentWindow.refreshSiteDataById(feature.get('siteid'));
            document.getElementById('siteOptionsFrame').contentWindow.userDisplay = true;
            setDialogZIndex(null, 'siteOptions');
            clearInteractions();
        }
    }
});
// ******* Link interactions
var addLink = new ol.interaction.Select({
    condition: ol.events.condition.singleClick,
    hitTolerance: 1,
    wrapX: false,
    layers: function (layer) { return layer === myNetworkLayer; },
    style: drawLinkSiteStyle
});
myMap.addInteraction(addLink);
addLink.setActive(false);
var siteASelected = undefined;
var pointCount = 0;
var addedLink = addLink.getFeatures();
addedLink.on('add', function (e) {
    if (!siteASelected) {
        if (e) { e.stopPropagation(); }
        drawLink.setActive(true);
        let feature = e.target.getArray()[0];
        siteASelected = feature;
        let coordinate = feature.getGeometry().getCoordinates();
        let clickEvent = $.Event('click');
        clickEvent.map = myMap;
        clickEvent.coordinate = coordinate;
        drawLink.startDrawing_(clickEvent);
    }
    else {
        if (e) { e.stopPropagation(); }
        drawLink.finishDrawing();
        drawLink.setActive(false);
        let feature = e.target.getArray()[0];
        createLink({
            siteida: siteASelected.get('siteid'),
            locationA: ol.proj.toLonLat(siteASelected.getGeometry().getCoordinates()),
            siteidb: feature.get('siteid'),
            locationB: ol.proj.toLonLat(feature.getGeometry().getCoordinates())
        });
        document.getElementById('linkOptionsFrame').contentWindow.refreshLinkData('add');
        clearInteractions();
    }
});
//document.addEventListener('click', function (e) {
//    //if (siteASelected) {
//    //    if (e) { e.stopPropagation(); }
//    //    pointCount = pointCount + 1;
//    //    if (pointCount > 1) {
//    //        drawLink.removeLastPoint();
//    //        pointCount = pointCount - 1;
//    //    }
//    //}
//});
var drawLink = new ol.interaction.Draw({ source: myLinksLayer.getSource(), type: 'LineString', style: drawLinkLineStyle });
myMap.addInteraction(drawLink);
drawLink.setActive(false);
var drawLinkTip = new ol.Overlay.Tooltip();
myMap.addOverlay(drawLinkTip);
drawLink.on('drawstart', function () { drawLinkTip.setInfo('Select site B to create link'); drawLinkTip.setFeature.bind(drawLinkTip); pointCount = 1; });
drawLink.on(['change:active', 'drawend'], function () { drawLinkTip.setInfo(''); drawLinkTip.removeFeature.bind(drawLinkTip); });
var selectLink = new ol.interaction.Select({
    condition: ol.events.condition.singleClick,
    hitTolerance: 5,
    wrapX: false,
    toggleCondition: ol.events.condition.shiftKeyOnly,
    layers: function (layer) { return layer === myLinksLayer; },
    style: selectStyle
});
myMap.addInteraction(selectLink);
selectLink.setActive(false);
var deleteLinkActive = false;
var snapLink = new ol.interaction.Snap({ source: myLinksLayer.getSource() });
myMap.addInteraction(snapLink);
snapLink.setActive(false);
var selectedLink = selectLink.getFeatures();
selectedLink.on('add', function (e) {
    if (deleteLinkActive === true) {
        if (e) { e.stopPropagation(); }
        selectLink.getFeatures().forEach(function (feature) {
            myLinksLayer.getSource().removeFeature(feature);
            myMap.render();
        });
        loadLinksTreeFromSource();
        document.getElementById('linkOptionsFrame').contentWindow.refreshLinkData('delete');
        document.getElementById('linkProfileFrame').contentWindow.refreshLinkData('delete');
        // clear all tools here
        clearInteractions();
    }
});


// ******* Polygon interactions
var selectPolygon = new ol.interaction.Select({
    condition: ol.events.condition.singleClick,
    hitTolerance: 5,
    wrapX: false,
    toggleCondition: ol.events.condition.shiftKeyOnly,
    layers: function (layer) { return layer === myPolygonsLayer; },
    style: selectStyle
});
myMap.addInteraction(selectPolygon);
selectPolygon.setActive(false);
var drawPolygon = new ol.interaction.Draw({ source: myPolygonsLayer.getSource(), type: 'Polygon', style: drawPolygonStyle });
myMap.addInteraction(drawPolygon);
drawPolygon.setActive(false);
document.addEventListener('keydown', function (e) {
    if (e.which === 27) {
        if (drawPolygon.getActive()) {
            if (e) { e.stopPropagation(); }
            drawPolygon.removeLastPoint();
        }
        if (addLink.getActive()) {
            drawLink.removeLastPoint();
            drawLink.setActive(false);
            clearInteractions();
        }
    }
});
var drawPolygonTip = new ol.Overlay.Tooltip();
myMap.addOverlay(drawPolygonTip);
drawPolygon.on('drawstart', function () { drawPolygonTip.setInfo('Click ESC to remove last point'); drawPolygonTip.setFeature.bind(drawPolygonTip); });
drawPolygon.on(['change:active', 'drawend'], function () { drawPolygonTip.setInfo(''); drawPolygonTip.removeFeature.bind(drawPolygonTip); });
var modifyPolygon = new ol.interaction.Modify({ features: selectPolygon.getFeatures() });
myMap.addInteraction(modifyPolygon);
modifyPolygon.setActive(false);
var transformPolygon = new ol.interaction.Transform({ features: selectPolygon.getFeatures() });
myMap.addInteraction(transformPolygon);
transformPolygon.setActive(false);
var deletePolygonActive = false;
var snapPolygon = new ol.interaction.Snap({ source: myPolygonsLayer.getSource() });
myMap.addInteraction(snapPolygon);
snapPolygon.setActive(false);
var selectedPolygon = selectPolygon.getFeatures();
selectedPolygon.on('add', function (e) {
    if (transformPolygon.getActive() === true) {
        transformPolygon.setSelection(selectPolygon.getFeatures());
    }
    if (deletePolygonActive === true) {
        if (e) { e.stopPropagation(); }
        selectPolygon.getFeatures().forEach(function (feature) {
            myPolygonsLayer.getSource().removeFeature(feature);
            myMap.render();
        });
        loadPolygonsTreeFromSource();
        document.getElementById('polyOptionsFrame').contentWindow.refreshPolygonData('delete');
        // clear all tools here
        clearInteractions();
    }
});


// ******* Site interactions
var selectSite = new ol.interaction.Select({
    condition: ol.events.condition.singleClick,
    hitTolerance: 5,
    wrapX: false,
    toggleCondition: ol.events.condition.shiftKeyOnly,
    layers: function (layer) { return layer === myNetworkLayer; },
    style: selectStyle
});
myMap.addInteraction(selectSite);
selectSite.setActive(false);
var drawSite2G = new ol.interaction.Draw({ source: myNetworkLayer.getSource(), type: 'Point', style: drawSiteStyle });
myMap.addInteraction(drawSite2G);
drawSite2G.setActive(false);
var drawSite4G = new ol.interaction.Draw({ source: myNetworkLayer.getSource(), type: 'Point', style: drawSiteStyle });
myMap.addInteraction(drawSite4G);
drawSite4G.setActive(false);
var modifySite = new ol.interaction.Modify({ features: selectSite.getFeatures() });
myMap.addInteraction(modifySite);
modifySite.setActive(false);
var deleteSiteActive = false;
var snapSite = new ol.interaction.Snap({ source: myNetworkLayer.getSource() });
myMap.addInteraction(snapSite);
snapSite.setActive(false);
var selectedSite = selectSite.getFeatures();
selectedSite.on('add', function (e) {
    if (deleteSiteActive === true) {
        if (e) { e.stopPropagation(); }
        selectSite.getFeatures().forEach(function (feature) {
            deleteLinksBySiteId(feature.get('siteid'));
            myNetworkLayer.getSource().removeFeature(feature);
            myMap.render();
        });
        loadSitesTreeFromSource();
        document.getElementById('siteOptionsFrame').contentWindow.refreshSiteData('delete');
        document.getElementById('linkOptionsFrame').contentWindow.refreshLinkData('delete');
        document.getElementById('linkProfileFrame').contentWindow.refreshLinkData('delete');
        // clear all tools here
        clearInteractions();
    }
});
modifySite.on('modifyend', function (e) {
    let feature = e.features.getArray()[0];
    if (feature.get('siteid') !== undefined) {
        if (e) { e.stopPropagation(); }
        let coordinates3857 = feature.getGeometry().getCoordinates();
        let coordinates4326 = ol.proj.transform(coordinates3857, 'EPSG:3857', 'EPSG:4326');
        feature.setProperties({
            'longitude': coordinates4326[0],
            'latitude': coordinates4326[1]
        });
        myMap.render();
        deleteLinksBySiteId(feature.get('siteid'));
        loadSitesTreeFromSource();
        document.getElementById('siteOptionsFrame').contentWindow.refreshSiteData('cursor');
        document.getElementById('linkOptionsFrame').contentWindow.refreshLinkData('delete');
        document.getElementById('linkProfileFrame').contentWindow.refreshLinkData('delete');
        // clear all tools here
        clearInteractions();
    }
});

// notes interactions
// ******* Note interactions
var selectNote = new ol.interaction.Select({
    condition: ol.events.condition.singleClick,
    hitTolerance: 5,
    wrapX: false,
    toggleCondition: ol.events.condition.shiftKeyOnly,
    layers: function (layer) { return layer === myNotesLayer; },
    style: selectStyle
});
myMap.addInteraction(selectNote);
selectNote.setActive(false);
var drawNote = new ol.interaction.Draw({
    source: myNotesLayer.getSource(),
    type: 'Point', style: drawNoteStyle
});
myMap.addInteraction(drawNote);
drawNote.setActive(false);
var modifyNote = new ol.interaction.Modify({ features: selectNote.getFeatures() });
myMap.addInteraction(modifyNote);
modifyNote.setActive(false);
var deleteNoteActive = false;
var snapNote = new ol.interaction.Snap({ source: myNotesLayer.getSource() });
myMap.addInteraction(snapNote);
snapNote.setActive(false);
var selectedNote = selectNote.getFeatures();
selectedNote.on('add', function (e) {
    if (deleteNoteActive === true) {
        if (e) { e.stopPropagation(); }
        selectNote.getFeatures().forEach(function (feature) {
            myNotesLayer.getSource().removeFeature(feature);
            myMap.render();
        });
        loadNotesTreeFromSource();
        document.getElementById('noteOptionsFrame').contentWindow.refreshNoteData('delete');
        // clear all tools here
        clearInteractions();
    }
});




// measure line && area interactions
var measureLine = new ol.interaction.Draw({ type: 'LineString' });
myMap.addInteraction(measureLine);
measureLine.setActive(false);
var measureArea = new ol.interaction.Draw({ type: 'Polygon' });
myMap.addInteraction(measureArea);
measureArea.setActive(false);
var measureTip = new ol.Overlay.Tooltip();
myMap.addOverlay(measureTip);
measureLine.on('drawstart', measureTip.setFeature.bind(measureTip));
measureLine.on(['change:active', 'drawend'], measureTip.removeFeature.bind(measureTip));
measureArea.on('drawstart', measureTip.setFeature.bind(measureTip));
measureArea.on(['change:active', 'drawend'], measureTip.removeFeature.bind(measureTip));