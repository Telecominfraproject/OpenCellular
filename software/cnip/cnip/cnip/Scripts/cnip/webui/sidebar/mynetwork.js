var sNetworkTree = [];

$('#sitesDiv').collapse({
    toggle: true
});
var sitesTree = document.getElementById('sitesUl');

//https://www.plupload.com/examples/core

var fileupload1 = document.getElementById("FileUpload11");
var button1 = document.getElementById("btnFileUpload1");
button1.onclick = function () {
    fileupload1.click();
};
fileupload1.onchange = function (evt) {
    if (evt) { evt.stopPropagation(); }
    if (getActionRunning()) { return; }
    if (evt.target.files) {
        var reader = new FileReader();
        var file = evt.currentTarget.files[0];
        var filename = file.name;
        if (!filename.includes('json') ||
            !filename.includes('geojson')) {
            showMessage("Network Upload", "Please provide sites in Geojson format.");
        }
        else {
            reader.readAsText(file, "UTF-8");
            reader.onload = function (evt) {
                showProgress('Loading Network');
                setTimeout(function () {
                    // save current network
                    uploadDataToUrl(myNetworkLayer.getSource().getJSONString(),
                        vectorsPath + 'mynetwork.geojson');
                    Ajax_runccmd("UploadNetworkToDB", "");
                    myMap.render();
                    loadSitesTreeFromFile();
                    // load user network
                    uploadDataToUrl(evt.target.result,
                        vectorsPath + "t" + filename);
                    Ajax_runccmd("UploadUserNetworkToDB",
                        "{'sitename':'t" + filename + "'}");
                    // reload polygons from server
                    Ajax_runccmd("GetNetworkFromDB", "");
                    Ajax_loadSourceFromFile(myNetworkLayer.getSource(),
                        "../" + vectorsPath + "mynetwork.geojson");
                    myMap.render();
                    loadSitesTreeFromFile();
                    closeProgress();
                }, 500);
            };
        }
        fileupload.value = null;
    }
};

initSingleSlider(document.getElementById('sitesIconOpacity'), '', '0', '1', '0.1', '1');
document.getElementById('sitesIconOpacity').addEventListener('change', setSitesStyle);
loadSitesTreeFromFile();
/* site functions*/
function setSitesLabel(e) {
    setSitesStyle(e);
}
function setSitesStyle(e) {
    if (e) { e.stopPropagation(); }
    let custStyle = function (feature) {
        let iconOpacity = Number($('#sitesIconOpacity').val());
        return [new ol.style.Style({
            image: feature.get('technology') === '4G' ? new ol.style.Icon({
                scale: .6,
                src: '../img/site4G.png',
                opacity: iconOpacity
            }) : new ol.style.Icon({
                scale: .6,
                src: '../img/site2G.png',
                opacity: iconOpacity
            }),
            text: new ol.style.Text({
                offsetY: 25,
                text: document.getElementById("sitesLabel").checked ? feature.get('sitename') : '',
                font: '14px "Liberation Sans", "Lucida Sans", "Lucida Sans Regular", "Lucida Grande", "Lucida Sans Unicode", Geneva, Verdana, sans-serif',
                fill: new ol.style.Fill({ color: 'rgba(0,0,0,' + iconOpacity + ')' }),
                stroke: new ol.style.Stroke({ color: 'rgba(255,255,255,' + iconOpacity + ')', width: 2 }),
                overflow: false
            })
        })];
    };
    myNetworkLayer.setStyle(custStyle);
    myMap.render();
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
function loadSitesTreeFromFile() {
    let httpRequest = new XMLHttpRequest();
    httpRequest.open('GET', '../' + vectorsPath + 'mynetwork.geojson');
    httpRequest.onload = function () {
        let source = JSON.parse(httpRequest.responseText);
        function processLoop() {
            if (source.features.length === myNetworkLayer.getSource().getFeatures().length) {
                loadSitesTreeFromSource();
            } else {
                setTimeout(processLoop, 500);
            }
        }
        processLoop();
    };
    httpRequest.send();
}
function loadSitesTreeFromSource() {
    sNetworkTree = buildSortedTreeFromSource(myNetworkLayer, 'siteid', 'sitename', true);
    sitesTree.innerHTML = '';
    for (let i = 0; i < sNetworkTree.length; i++) {
        sitesTree.innerHTML = sitesTree.innerHTML +
            '<li class="visualLi" onclick="showSiteOptionsByIdx(event,' +
            sNetworkTree[i][0].toString() + ');"><i class="fas fa-broadcast-tower"' +
            (sNetworkTree[i][3].toString() === '4G' ?
                ' style="color:rgb(255,106,0);" ' : '') + '></i>&nbsp;' +
            truncate(sNetworkTree[i][2].toString(), 27) + '</li>';
    }
}
function buildSortedTreeFromSource(layer, id, name, tech = false) {
    let tree = [];
    let features = layer.getSource().getFeatures();
    for (let idx = 0; idx < features.length; idx++) {
        if (tech) {
            tree.push([idx, Number(features[idx].get(id)), features[idx].get(name), features[idx].get('technology')]);
        } else {
            tree.push([idx, Number(features[idx].get(id)), features[idx].get(name)]);
        }
    }
    tree = multiSort(tree, {
        1: 'asc'
    });
    return tree;
}
function showSiteOptionsByIdx(e, Idx) {
    if (e) { e.stopPropagation(); }
    if (actionRunning) { return; }
    document.getElementById('siteOptions').style.display = 'block';
    //document.getElementById('siteOptions').style.left = 'calc(50vw - 651px / 2)';
    //document.getElementById('siteOptions').style.top = 'calc(50vh - 432px / 2)';
    //document.getElementById('siteOptions').style.width = '651px';
    //document.getElementById('siteOptions').style.height = '432px';
    document.getElementById('siteOptionsFrame').contentWindow.userDisplay = true;
    document.getElementById('siteOptionsFrame').contentWindow.refreshSiteData('', Idx);
    setDialogZIndex(null, 'siteOptions');
}
myNetworkLayer.getSource().on('addfeature', function (event) {
    if (event.feature.get('siteid') === undefined) {
        let newid = getNewIdx(myNetworkLayer.getSource(), 'siteid');
        let coordinates3857 = event.feature.getGeometry().getCoordinates();
        let coordinates4326 = ol.proj.transform(
            coordinates3857, 'EPSG:3857', 'EPSG:4326');
        let antennaOptions = getAntennaOptions(
            document.getElementById('opt_antennamodel').value);
        event.feature.setProperties({
            //site
            'siteid': newid,
            'sitename': 'new site ' + newid,
            'longitude': coordinates4326[0],
            'latitude': coordinates4326[1],
            //tower
            'height': '',
            //property
            'region': '',
            'country': '',
            'city': '',
            'district': '',
            'province': '',
            'address': '',
            'comments': '',
            //radio
            'technology': drawSite2G.getActive() ? '2G' : '4G',
            'band': drawSite2G.getActive() ?
                document.getElementById('opt_gsmband').value :
                document.getElementById('opt_lteband').value,
            'bandwidth': drawSite2G.getActive() ?
                document.getElementById('opt_gsmbandwidth').value :
                document.getElementById('opt_ltebandwidth').value,
            'cellid': 10000 + parseInt(newid),
            'lac': LAC,
            'rfcn': '',
            'rfid': '',
            'dlfrequency': '',
            'ulfrequency': '',
            'rfpower': '',
            //antenna
            'hba': '',
            'azimuth': '0',
            'antennamodel': antennaOptions.antennaModel,
            'antennatype': antennaOptions.antennaType,
            'polarization': antennaOptions.polarization,
            'vbeamwidth': antennaOptions.vBeamWidth,
            'hbeamwidth': antennaOptions.hBeamWidth,
            'downtilt': antennaOptions.downTilt,
            'antennagain': antennaOptions.antennaGain,
            'feederloss': '0'
        });
        loadSitesTreeFromSource();
        document.getElementById('siteOptionsFrame').contentWindow.refreshSiteData('add');
    }
});
function getSiteDataByIdx(Idx) {
    return getDataByIdx(myNetworkLayer.getSource(), Idx);
}
function getSiteIdxById(siteid) {
    return getIdxByProperty(myNetworkLayer.getSource(), 'siteid', siteid);
}
function getSiteDataById(siteid) {
    return getSiteDataByIdx(getSiteIdxById(siteid));
}
function zoomToSiteByIdx(Idx) {
    zoomToFeatureByIdx(myNetworkLayer.getSource(), Idx);
}
function getPublicSiteData(siteidb, email) {
    let feature = getPublicSite(siteidb, email);
    let returnString = '';
    if (feature) {
        let keys = feature.getKeys();
        for (let j = 0; j < keys.length; j++) {
            if (keys[j] !== 'geometry') {
                if (feature.get(keys[j])) {
                    returnString += feature.get(keys[j]).toString() + '\t';
                }
                else {
                    returnString += '\t';
                }
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
                    }
                }
            }
        }
    }
}
function updateSiteHeightById(siteid, height) {
    let feature = myNetworkLayer.getSource().getFeatureByProperty('siteid', siteid);
    feature.set('height', height.toString());
    myMap.render();
}
function updateSiteDataByIdx(options) {
    if (options.idx) {
        if (Number(options.idx) > -1) {
            let feature = myNetworkLayer.getSource().getFeatures()[options.idx];
            //site
            feature.set('sitename', options.sitename.toString());
            feature.set('longitude', options.longitude.toString());
            feature.set('latitude', options.latitude.toString());
            //tower
            feature.set('height', options.height.toString());
            //property
            feature.set('region', options.region.toString());
            feature.set('country', options.country.toString());
            feature.set('city', options.city.toString());
            feature.set('district', options.district.toString());
            feature.set('province', options.province.toString());
            feature.set('address', options.address.toString());
            feature.set('comments', options.comments.toString());
            //radio
            feature.set('technology', options.technology.toString());
            feature.set('band', options.band.toString());
            feature.set('bandwidth', options.bandwidth.toString());
            feature.set('cellid', options.cellid.toString());
            feature.set('lac', options.lac.toString());
            feature.set('rfcn', options.rfcn.toString());
            feature.set('rfid', options.rfid.toString());
            feature.set('dlfrequency', options.dlfrequency.toString());
            feature.set('ulfrequency', options.ulfrequency.toString());
            feature.set('rfpower', options.rfpower.toString());
            //antenna
            feature.set('hba', options.hba.toString());
            feature.set('azimuth', options.azimuth.toString());
            feature.set('antennamodel', options.antennamodel.toString());
            feature.set('antennatype', options.antennatype.toString());
            feature.set('polarization', options.polarization.toString());
            feature.set('vbeamwidth', options.vbeamwidth.toString());
            feature.set('hbeamwidth', options.hbeamwidth.toString());
            feature.set('downtilt', options.downtilt.toString());
            feature.set('antennagain', options.antennagain.toString());
            feature.set('feederloss', options.feederloss.toString());
            //geometry
            feature.set('geometry', new ol.geom.Point(ol.proj.fromLonLat(
                [Number(options.longitude), Number(options.latitude)])));
            myMap.render();
            loadSitesTreeFromSource();
        }
    }
}
function getSitesCount() {
    return getFeaturesCount(myNetworkLayer.getSource());
}
function deleteSiteByIdx(Idx) {
    let siteData = getSiteDataByIdx(Idx).split('\t');
    if (siteData) { deleteLinksBySiteId(siteData[0].toString()); }
    deleteFeatureByIdx(myNetworkLayer.getSource(), Idx);
    loadSitesTreeFromSource();
    clearInteractions();
}
function downloadSitesCSV(e) {
    if (e) { e.stopPropagation(); }
    if (actionRunning) { return; }
    showProgress('Exporting CSV');
    setTimeout(function () {
        download.CSV(myNetworkLayer.getSource(), 'mynetwork');
        closeProgress();
    }, 500);
}
function downloadSitesJson(e) {
    if (e) { e.stopPropagation(); }
    if (actionRunning) { return; }
    showProgress('Exporting JSON');
    setTimeout(function () {
        download.JSON(myNetworkLayer.getSource(), 'mynetwork');
        closeProgress();
    }, 500);
}
function downloadSitesKML(e) {
    if (e) { e.stopPropagation(); }
    if (actionRunning) { return; }
    showProgress('Exporting KML');
    setTimeout(function () {
        download.KML(myNetworkLayer.getSource(), 'mynetwork');
        closeProgress();
    }, 500);
}
function getGSMDlfrequency(rfcn) {
    return 890 + (0.2 * Number(rfcn)) + 45;
}
function getGSMUlfrequency(rfcn) {
    return 890 + (0.2 * Number(rfcn));
}
function getLTEDlfrequency(rfcn) {
    return 925 + (0.1 * (Number(rfcn) - 3450));
}
function getLTEUlfrequency(rfcn) {
    return 880 + (0.1 * (Number(rfcn) + 18000 - 21450));
}
function getAntennaOptions(antennaModel) {
    let Options;
    switch (antennaModel.toString()) {
        case 'LT OD9-5 890-950 MHz':
            Options = {
                antennaModel: 'LT OD9-5 890-950 MHz',
                antennaType: 'Omni',
                polarization: 'Vertical',
                vBeamWidth: '36',
                hBeamWidth: '360',
                downTilt: '0',
                antennaGain: '5'
            };
            break;
        case 'LT OD9-6 865-945 MHz':
            Options = {
                antennaModel: 'LT OD9-6 865-945 MHz',
                antennaType: 'Omni',
                polarization: 'Vertical',
                vBeamWidth: '16',
                hBeamWidth: '360',
                downTilt: '0',
                antennaGain: '6'
            };
            break;
        case 'LT OD9-6 860-960 MHz':
            Options = {
                antennaModel: 'LT OD9-6 860-960 MHz',
                antennaType: 'Omni',
                polarization: 'Vertical',
                vBeamWidth: '16',
                hBeamWidth: '360',
                downTilt: '0',
                antennaGain: '6'
            };
            break;
        case 'LT OD9-8 865-945 MHz':
            Options = {
                antennaModel: 'LT OD9-8 865-945 MHz',
                antennaType: 'Omni',
                polarization: 'Vertical',
                vBeamWidth: '10',
                hBeamWidth: '360',
                downTilt: '0',
                antennaGain: '8'
            };
            break;
        case 'LT OD9-8 860-960 MHz':
            Options = {
                antennaModel: 'LT OD9-8 860-960 MHz',
                antennaType: 'Omni',
                polarization: 'Vertical',
                vBeamWidth: '10',
                hBeamWidth: '360',
                downTilt: '0',
                antennaGain: '8'
            };
            break;
        case 'LT OD9-11 860-960 MHz':
            Options = {
                antennaModel: 'LT OD9-11 860-960 MHz',
                antennaType: 'Omni',
                polarization: 'Vertical',
                vBeamWidth: '7',
                hBeamWidth: '360',
                downTilt: '0',
                antennaGain: '11'
            };
            break;
        case 'LT OD9-11D1 860-960 MHz':
            Options = {
                antennaModel: 'LT OD9-11D1 860-960 MHz',
                antennaType: 'Omni',
                polarization: 'Vertical',
                vBeamWidth: '7',
                hBeamWidth: '360',
                downTilt: '1',
                antennaGain: '11'
            };
            break;
        default:
            Options = {
                antennaModel: 'LT OD9-5 890-950 MHz',
                antennaType: 'Omni',
                polarization: 'Vertical',
                vBeamWidth: '36',
                hBeamWidth: '360',
                downTilt: '0',
                antennaGain: '5'
            };
            break;
    }
    return Options;
}
