$('#resultsDiv').collapse({
    toggle: true
});
var raster;
var resultsTree = document.getElementById('resultsUl');
initSingleSlider(document.getElementById('rasterOpacity'), '', '0', '1', '0.1', '0.8');
document.getElementById('rasterOpacity').addEventListener('change', setRasterStyle);
loadResultsTreeFromFile();
/* polygon functions*/

function setRasterVisiblity() {
    if (raster) {
        if (document.getElementById("rasterVisibility").checked === true) {
            raster.setVisible(true);
        } else {
            raster.setVisible(false);
        }
        myMap.render();
        ol.control.LayerSwitcher.renderPanel(myMap, toc);
    }
}
function setLinksVisiblity() {
    if (myLinksLayer) {
        if (document.getElementById("linksVisibility").checked === true) {
            myLinksLayer.setVisible(true);
        } else {
            myLinksLayer.setVisible(false);
        }
        myMap.render();
        ol.control.LayerSwitcher.renderPanel(myMap, toc);
    }
}

function setPolygonsVisiblity() {
    if (myPolygonsLayer) {
        if (document.getElementById("polygonsVisibility").checked === true) {
            myPolygonsLayer.setVisible(true);
        } else {
            myPolygonsLayer.setVisible(false);
        }
        myMap.render();
        ol.control.LayerSwitcher.renderPanel(myMap, toc);
    }
}

function setNotesVisiblity() {
    if (myNotesLayer) {
        if (document.getElementById("notesVisibility").checked === true) {
            myNotesLayer.setVisible(true);
        } else {
            myNotesLayer.setVisible(false);
        }
        myMap.render();
        ol.control.LayerSwitcher.renderPanel(myMap, toc);
    }
}
function setSitesVisiblity() {
    if (myNetworkLayer) {
        if (document.getElementById("sitesVisibility").checked === true) {
            myNetworkLayer.setVisible(true);
        } else {
            myNetworkLayer.setVisible(false);
        }
        myMap.render();
        ol.control.LayerSwitcher.renderPanel(myMap, toc);
    }
}

function setPublicNotesVisiblity() {
    if (publicNotesLayer) {
        if (document.getElementById("publicNotesVisibility").checked === true) {
            publicNotesLayer.setVisible(true);
        } else {
            publicNotesLayer.setVisible(false);
        }
        myMap.render();
        ol.control.LayerSwitcher.renderPanel(myMap, toc);
    }
}

function setPublicSitesVisiblity() {
    if (publicNetworkLayer) {
        if (document.getElementById("publicSitesVisibility").checked === true) {
            publicNetworkLayer.setVisible(true);
        } else {
            publicNetworkLayer.setVisible(false);
        }
        myMap.render();
        ol.control.LayerSwitcher.renderPanel(myMap, toc);
    }
}

function setRasterStyle(e) {
    if (e) { e.stopPropagation(); }
    if (raster) {
        let opacity = Number($('#rasterOpacity').val());
        raster.setOpacity(opacity);
        myMap.render();
    }
}
function loadResultsTreeFromFile() {
    let httpRequest = new XMLHttpRequest();
    httpRequest.open('GET', '../' + vectorsPath + 'results.geojson', true);
    httpRequest.send();
    httpRequest.onload = function () {
        let source = JSON.parse(httpRequest.responseText);
        resultsTree.innerHTML = '';
        if (typeof source.features !== 'undefined') {
            for (let i = 0; i < source.features.length; i++) {
                resultsTree.innerHTML = resultsTree.innerHTML +
                    '<li class="visualLi">' +
                    '<button class="toolsButton" style="width:auto;height:auto;"' +
                    ' onclick="showResultByIdx(event,' + i.toString() + ');">' +
                    '<i class="fas fa-poll" style="font-size:14px !important;"></i>' +
                    '&nbsp;' + truncate(source.features[i].properties.resultname, 23) +
                    '</button>' +
                    '&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;' +
                    '<button class="toolsButton" style="width:auto;height:auto;float:right;"' +
                    ' onclick="deleteResultByIdx(event,' + i.toString() + ');">' +
                    '<i class="fas fa-trash-alt" style="font-size:14px !important;"></i>' +
                    '</button>' +
                    '</li>';
            }
        }
    };
}
function loadResultsTreeFromSource() {
    let features = resultsLayer.getSource().getFeatures();
    resultsTree.innerHTML = '';
    for (let i = 0; i < features.length; i++) {
        resultsTree.innerHTML = resultsTree.innerHTML +
            '<li class="visualLi">' +
            '<button class="toolsButton" style="width:auto;height:auto;"' +
            ' onclick="showResultByIdx(event,' + i.toString() + ');">' +
            '<i class="fas fa-poll" style="font-size:14px !important;"></i>' +
            '&nbsp;' + truncate(features[i].getProperties().resultname, 23) +
            '</button>' +
            '&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;' +
            '<button class="toolsButton" style="width:auto;height:auto;float:right;"' +
            ' onclick="deleteResultByIdx(event,' + i.toString() + ');">' +
            '<i class="fas fa-trash-alt" style="font-size:14px !important;"></i>' +
            '</button>' +
            '</li>';
    }
}
var deleteResultIdx = null;
function deleteResultByIdx(e, Idx) {
    if (e) { e.stopPropagation(); }
    if (getActionRunning()) { return; }
    deleteResultIdx = Idx;
    document.getElementById("yesMessageBox").addEventListener("click", deleteResult);
    document.getElementById("okcancelMessageBox").addEventListener("click", removeDeleteResult);
    let feature = resultsLayer.getSource().getFeatures()[deleteResultIdx];
    showMessage('Delete Result', 'Are you sure you want to permanently delete<br/>&nbsp;&nbsp;&nbsp;"'
        + feature.get('resultname') + '" ?', true);
}
function removeDeleteResult() {
    document.getElementById("yesMessageBox").removeEventListener("click", deleteResult);
    document.getElementById("okcancelMessageBox").removeEventListener("click", removeDeleteResult);
}
function deleteResult() {
    showProgress('Deleting Result');
    setTimeout(function () {
        let feature = resultsLayer.getSource().getFeatures()[deleteResultIdx];
        let result = feature.get('resultstring').split('>');
        let pngString = result[1];
        let polygonsString = result[2];
        let pngs = [];
        pngs.push(pngString.split(',')[0]);
        if (polygonsString.length > 0) {
            polygonsString.split('@').forEach(function (ps) {
                pngs.push(ps.split('#')[2].split(',')[0]);
            });
        }
        pngs = [...new Set(pngs)];
        Ajax_runccmd("DeleteResult", "{'resultid':'" +
            feature.get('resultid') + "','pngs':'" + pngs.join(',') + "'}");
        loadResultsFromDB();
        deleteResultIdx = null;
        document.getElementById("yesMessageBox").removeEventListener("click", deleteResult);
        document.getElementById("okcancelMessageBox").removeEventListener("click", removeDeleteResult);
        closeProgress();
    }, 500);
}
function showResultByIdx(e, Idx) {
    if (e) { e.stopPropagation(); }
    if (actionRunning) { return; }
    showResult(resultsLayer.getSource().getFeatures()[Idx].get('resultstring'));
}
function showResult(resultstring) {
    if (resultstring) {
        if (resultstring.length == 0) {
            return;
        }
        let result = resultstring.split('>');
        let thematicString = result[0];
        let pngString = result[1];
        let polygonsString = result[2];
        setLegend(thematicString);
        if (polygonsString.length === 0) {
            loadRaster(pngString);
        }
        showPolygonsCoverageReport(polygonsString);
    }
}
function loadResultsFromDB() {
    Ajax_runccmd("GetResultsFromDB", "");
    Ajax_loadSourceFromFile(resultsLayer.getSource(),
        "../" + vectorsPath + "results.geojson");
    loadResultsTreeFromSource();
}
function loadRaster(pngString) {
    // ,&$#@>
    // result : thematicString>pngString>polygonsString@polygonsString

    // thematicString
    // rate&r,g,b$rate&r,g,b

    // pngString
    // png,1,2,3,4

    // polygonsString
    // polygonid,polygonname#siteid,sitename#png,1,2,3,4#cttrue,ctext1234#rate&value&r,g,b$rate&value&r,g,b#bestcandidate
    if (pngString) {
        if (pngString.length > 0) {
            let png = pngString.split(',');
            let extent = [png[1], png[2], png[3], png[4]];
            if (raster) {
                overlayLayers.getLayers().pop(raster);
            }
            raster = new ol.layer.Image({
                title: 'Result',
                visible: true,
                name: 'Result',
                opacity: 0.8,
                crossOrigin: '',
                source: new ol.source.ImageStatic({
                    url: '../' + resultsPath + png[0],
                    imageExtent: extent,
                    projection: 'EPSG:4326'
                })
            });
            document.getElementById('rasterOpacity').setValue('0.8');
            overlayLayers.getLayers().push(raster);
            myNetworkLayer.setZIndex(9);
            publicNetworkLayer.setZIndex(6);
            myLinksLayer.setZIndex(5);
            myNotesLayer.setZIndex(4);
            publicNotesLayer.setZIndex(3);
            myPolygonsLayer.setZIndex(2);
            raster.setZIndex(1);
            myMap.updateSize();
            myMap.render();
            document.getElementById("rasterVisibility").checked = true;
            ol.control.LayerSwitcher.renderPanel(myMap, toc);
        }
    }
}
function showPolygonsCoverageReport(polygonsString) {
    // ,&$#@>
    // result : thematicString>pngString>polygonsString@polygonsString

    // thematicString
    // rate&r,g,b$rate&r,g,b

    // pngString
    // png,1,2,3,4

    // polygonsString
    // polygonid,polygonname#siteid,sitename#png,1,2,3,4#cttrue,ctext1234#rate&value&r,g,b$rate&value&r,g,b#bestcandidate
    if (polygonsString) {
        if (polygonsString.length > 0) {
            let loadObj = []; let i = 0; let bestCandidate = 0; let bestCandidateCT = 0;
            let reportTitle = 'Polygons Coverage Report';
            polygonsString.split('@').forEach(function (ps) {
                if (i < 20) {
                    let coverageTestExt = Number(ps.split('#')[3].split(',')[1]);
                    if (bestCandidateCT < coverageTestExt) {
                        bestCandidateCT = coverageTestExt;
                        bestCandidate = i;
                    }
                    i += 1;
                }
            });
            i = 0;
            polygonsString.split('@').forEach(function (ps) {
                if (i < 20) {
                    let polygonid = ps.split('#')[0].split(',')[0];
                    let polygonname = ps.split('#')[0].split(',')[1];
                    let sites = ps.split('#')[1].split('&');
                    let siteids = '', sitenames = '';
                    sites.forEach(function (site) {
                        siteids += site.split(',')[0] + ', ';
                        sitenames += site.split(',')[1] + ', ';
                    });
                    siteids = siteids.substr(0, siteids.length - 2);
                    sitenames = sitenames.substr(0, sitenames.length - 2);
                    let pngString = ps.split('#')[2];
                    let coverageTest = ps.split('#')[3].split(',')[0];
                    let coverageTestExt = ps.split('#')[3].split(',')[1];
                    let dataString = ps.split('#')[4];
                    let bestCandidateType = ps.split('#')[5];
                    let tabTitle = '';
                    coverageTest =
                        coverageTest.toLowerCase() === 'true' ? 'Coverage Test: <b>Passed</b>' :
                            coverageTest.toLowerCase() === 'false' ? 'Coverage Test: <b>Failed</b>' :
                                coverageTest.toLowerCase() === 'n/a' ? 'Coverage Test: <b>Not Applicable</b>' :
                                    'Coverage Test: <b>Failed</b>';
                    if (bestCandidateType !== '') {
                        if (bestCandidate === i) {
                            coverageTest = '<span style="color: #000000; background-color: #ffff00;"><strong>&lt;Best Candidate&gt;</strong></span>' + ' <b>' + polygonname + '</b>,  <b>' + sitenames + '</b>, ' + coverageTest;
                        } else {
                            coverageTest = '<b>' + polygonname + '</b>, <b>' + sitenames + '</b>, ' + coverageTest;
                        }
                        tabTitle = 'Polygon Id: ' + polygonid + ', Site Id: ' + siteids;
                        reportTitle = 'Best Candidate Report';
                    } else {
                        coverageTest = 'Site Ids: <b>' + siteids + '</b>, ' + coverageTest;
                        tabTitle = 'Polygon Id: ' + polygonid + ', ' + polygonname;
                    }
                    loadObj.push({
                        title: tabTitle,
                        contentType: 'chart', contentTitle: coverageTest,
                        contentPadding: '0', dataString: dataString,
                        pngString: pngString
                    });
                    i += 1;
                }
            });
            ddsCreate(reportTitle, loadObj);
        }
    }
}
