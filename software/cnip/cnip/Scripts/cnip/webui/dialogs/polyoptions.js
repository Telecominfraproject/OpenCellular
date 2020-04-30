/* global variables*/
var userDisplay = false;
var polygonData;
var polygonsCount;
var currPolygon;
/* setup listeners*/
document.getElementById('defaultOpen').click();
document.getElementById('polygonname').addEventListener('change', polygonNameChange);
function polygonNameChange(e) {
    document.getElementById('polygonname').value = fixStringName(document.getElementById('polygonname').value);
    savePolygonData(e);
}
/* polyoptions functions*/
function fillPolygonData(polygonData) {
    document.getElementById('polygonid').value = polygonData[0].toString();
    document.getElementById('polygonname').value = polygonData[1] === undefined ? '' : polygonData[1].toString();
}
function refreshPolygonData(refreshType, Idx = null) {
    let currPolygonId = polygonData === undefined ? 0 : polygonData[0].toString();
    polygonData = undefined; polygonsCount = '0';
    polygonsCount = window.parent.getPolygonsCount();
    if (Idx !== null) { currPolygon = Idx.toString(); }
    else {
        if (refreshType === 'start') { currPolygon = '0'; }
        else if (refreshType === 'add') {
            currPolygon = (Number(polygonsCount) - 1).toString();
        }
        else if (refreshType === 'cursor') {
            currPolygon = window.parent.getPolygonIdxById(currPolygonId).toString();
        }
        else if (refreshType === 'delete') /*delete*/ {
            if (Number(currPolygon) > Number(polygonsCount) - 1) {
                currPolygon = (Number(polygonsCount) - 1).toString();
            }
        }
    }
    polygonData = window.parent.getPolygonDataByIdx(currPolygon).split('\t');
    fillPolygonData(polygonData);
}
function refreshPolygonDataById(polygonid) {
    refreshPolygonData('', window.parent.getPolygonIdxById(polygonid).toString());
}
function nextPolygon(e) {
    if (e) { e.stopPropagation(); }
    if (Number(polygonsCount) - 1 > Number(currPolygon)) {
        refreshPolygonData('', (Number(currPolygon) + 1).toString());
    }
}
function previousPolygon(e) {
    if (e) { e.stopPropagation(); }
    if (Number(currPolygon) > 0) {
        refreshPolygonData('', (Number(currPolygon) - 1).toString());
    }
}
function savePolygonData(e) {
    if (e) { e.stopPropagation(); }
    let polygonname = document.getElementById('polygonname').value;
    window.parent.updatePolygonDataByIdx(currPolygon, polygonname);
    refreshPolygonData('cursor');
}
function deletePolygon(e) {
    if (e) { e.stopPropagation(); }
    window.parent.deletePolygonByIdx(currPolygon);
    refreshPolygonData('delete');
}
function zoomToPolygon(e) {
    if (e) { e.stopPropagation(); }
    window.parent.document.getElementById('polyOptions').style.display = 'none';
    window.parent.zoomToPolygonByIdx(currPolygon);
}