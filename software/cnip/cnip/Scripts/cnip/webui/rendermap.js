$('.ol-control.ol-bar button[title]').tooltip({
    placement: 'bottom', trigger: 'hover'
});
$('#activeInteraction').tooltip({
    placement: 'bottom', trigger: 'hover'
});
$('.ol-zoom button[title]').tooltip({
    placement: 'left', trigger: 'hover'
});
$('.ol-search button[title]').tooltip({
    placement: 'left', trigger: 'hover'
});
$('.ol-legend.ol-unselectable.ol-control.ol-collapsed').attr('title', 'Legend');
$('.ol-legend.ol-unselectable.ol-control.ol-collapsed').tooltip({
    placement: 'left', trigger: 'hover'
});
$('.ol-full-screen-false').html('<i class=\"fas fa-expand-arrows-alt\"></i>');

while (!select) { let i = 0; }
while (!addedLink) { let i = 0; }
myMap.on('pointermove', function (e) {
    if (actionRunning) { return; }
    if (e.dragging) {
        //featureTip.attr('data-original-title', '').tooltip('hide');
        locationInfo.style.opacity = 0; locationInfo.innerHTML = '';
        noteInfo.style.opacity = 0; noteInfo.innerHTML = '';
        return;
    }
    setPointerStyle(e);
    if (select.getActive()) {
        displayElevation(e);
    }
    if (select.getActive() || addLink.getActive() ||
        deleteSiteActive || deleteLinkActive ||
        deletePolygonActive || deleteNoteActive) {
        displayFeatureInfo(e);
    }
});
myMap.on('click', function (e) {
    if (siteASelected) {
        if (e) { e.stopPropagation(); }
        pointCount = pointCount + 1;
        if (pointCount > 1) {
            drawLink.removeLastPoint();
            pointCount = pointCount - 1;
        }
    }

    if (actionRunning) { return; }
    if (!select.getActive()) { return; }
    let pixel = myMap.getEventPixel(e.originalEvent);
    let feature = myMap.forEachFeatureAtPixel(pixel,
        function (feature) { return feature; });
    if (!feature) { return; }
    if (feature.get('linkid') !== undefined) {
        if (e) { e.stopPropagation(); }
        document.getElementById('linkOptionsFrame').contentWindow.refreshLinkDataById(feature.get('linkid'));
    }
    //if (feature.get('polygonid') !== undefined) {
    //    if (e) { e.stopPropagation(); }
    //    document.getElementById('polyOptionsFrame').contentWindow.refreshPolygonDataById(feature.get('polygonid'));
    //}
    //if (feature.get('siteid') !== undefined) {
    //    if (e) { e.stopPropagation(); }
    //    document.getElementById('siteOptionsFrame').contentWindow.refreshSiteDataById(feature.get('siteid'));
    //}
    //if (feature.get('noteid') !== undefined) {
    //    if (e) { e.stopPropagation(); }
    //    document.getElementById('noteOptionsFrame').contentWindow.refreshNoteDataById(feature.get('noteid'));
    //}
});

while (!document.getElementById('siteOptionsFrame').contentWindow.document) { let i = 0; }
while (!document.getElementById('polyOptionsFrame').contentWindow.document) { let i = 0; }
while (!document.getElementById('noteOptionsFrame').contentWindow.document) { let i = 0; }
while (!document.getElementById('linkOptionsFrame').contentWindow.document) { let i = 0; }
while (!document.getElementById('linkProfileFrame').contentWindow.document) { let i = 0; }
document.getElementById('siteOptionsFrame').contentWindow.document.onclick =
    function () {
        setDialogZIndex(null, 'siteOptions');
    };
document.getElementById('polyOptionsFrame').contentWindow.document.onclick =
    function () {
        setDialogZIndex(null, 'polyOptions');
    };
document.getElementById('noteOptionsFrame').contentWindow.document.onclick =
    function () {
        setDialogZIndex(null, 'noteOptions');
    };
document.getElementById('linkOptionsFrame').contentWindow.document.onclick =
    function () {
        setDialogZIndex(null, 'linkOptions');
    };
document.getElementById('linkProfileFrame').contentWindow.document.onclick =
    function () {
        setDialogZIndex(null, 'linkProfile');
    };

myMap.render();