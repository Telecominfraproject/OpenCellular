/* global variables*/
var userDisplay = false;
var polygonData;
var polygonsCount;
var currPolygonIdx;
var deletePolygonIdx = -1;
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
    let currPolygonId = polygonData === undefined ? '0' : polygonData[0].toString();
    polygonData = undefined; polygonsCount = '0';
    polygonsCount = window.parent.getPolygonsCount();
    if (Idx !== null) { currPolygonIdx = Idx.toString(); }
    else {
        if (refreshType === 'start') {
            if (Number(polygonsCount) > 0) {
                currPolygonIdx = window.parent.sPolygonsTree[0][0].toString();
            } else { currPolygonIdx = '-1'; }
        }
        else if (refreshType === 'add') { /* add */
            if (Number(polygonsCount) > 0) {
                currPolygonIdx = window.parent.sPolygonsTree[Number(polygonsCount) - 1][0].toString();
            } else { currPolygonIdx = '-1'; }
        }
        else if (refreshType === 'cursor') { /* update */
            if (Number(polygonsCount) > 0) {
                currPolygonIdx = window.parent.getPolygonIdxById(currPolygonId).toString();
            } else { currPolygonIdx = '-1'; }
        }
        else if (refreshType === 'delete') /*delete*/ {
            if (Number(polygonsCount) > 0) {
                if (deletePolygonIdx > Number(polygonsCount) - 1) {
                    currPolygonIdx = window.parent.sPolygonsTree[Number(polygonsCount) - 1][0].toString();
                } else {
                    if (deletePolygonIdx > -1) {
                        currPolygonIdx = window.parent.sPolygonsTree[deletePolygonIdx][0].toString();
                    } else { currPolygonIdx = '-1'; }
                }
            } else { currPolygonIdx = '-1'; }
        }
    }
    polygonData = window.parent.getPolygonDataByIdx(currPolygonIdx).split('\t');
    fillPolygonData(polygonData);
}
function refreshPolygonDataById(polygonid) {
    refreshPolygonData('', window.parent.getPolygonIdxById(polygonid).toString());
}
function nextPolygon(e) {
    if (e) { e.stopPropagation(); }
    let sortedIdx = window.parent.sPolygonsTree.findIndex(x => x[0].toString() === currPolygonIdx);
    if (sortedIdx < Number(polygonsCount) - 1) {
        refreshPolygonData('', (window.parent.sPolygonsTree[sortedIdx + 1][0]).toString());
    }
}
function previousPolygon(e) {
    if (e) { e.stopPropagation(); }
    let sortedIdx = window.parent.sPolygonsTree.findIndex(x => x[0].toString() === currPolygonIdx);
    if (sortedIdx > 0) {
        refreshPolygonData('', (window.parent.sPolygonsTree[sortedIdx - 1][0]).toString());
    }
}
function savePolygonData(e) {
    if (e) { e.stopPropagation(); }
    let polygonname = document.getElementById('polygonname').value;
    window.parent.updatePolygonDataByIdx(currPolygonIdx, polygonname);
    refreshPolygonData('cursor');
}
function deletePolygon(e) {
    if (e) { e.stopPropagation(); }
    deletePolygonIdx = window.parent.sPolygonsTree.findIndex(x => x[0].toString() === currPolygonIdx);
    window.parent.deletePolygonByIdx(currPolygonIdx);
    refreshPolygonData('delete');
}
function zoomToPolygon(e) {
    if (e) { e.stopPropagation(); }
    window.parent.document.getElementById('polyOptions').style.display = 'none';
    window.parent.zoomToPolygonByIdx(currPolygonIdx);
}