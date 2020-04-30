function getDataByIdx(source, Idx) {
    let returnString = '';
    if (Idx) {
        if (Number(Idx) > -1) {
            let feature = source.getFeatures()[Idx];
            let keys = feature.getKeys();
            for (let j = 0; j < keys.length; j++) {
                if (keys[j] !== 'geometry' && keys[j] !== 'elevstr') {
                    returnString += feature.get(keys[j]).toString() + '\t';
                }
            }
            returnString = returnString.substring(0, returnString.length - 1);
        }
    }
    return returnString;
}
function getIdxById(source, key, id) {
    let features = source.getFeatures();
    let returnString = '';
    for (let i = 0; i < features.length; i++) {
        if (features[i].get(key).toString() === id.toString()) {
            returnString = i.toString(); break;
        }
    }
    return returnString;
}
function getFeaturesCount(source) {
    return source.getFeatures().length;
}
function deleteFeatureByIdx(source, Idx) {
    if (Idx) {
        if (Number(Idx) > -1) {
            let feature = source.getFeatures()[Idx];
            source.removeFeature(feature);
            myMap.render();
        }
    }
}
function zoomToFeatureByIdx(source, Idx) {
    if (Idx) {
        if (Number(Idx) > -1) {
            let feature = source.getFeatures()[Idx];
            myMap.getView().animate({
                duration: 700,
                center: ol.extent.getCenter(feature.getGeometry().getExtent())
            });
        }
    }
}
function getNewIdx(source, key) {
    let features = source.getFeatures();
    let newIdx = 0; let eIdx = 0;
    features.forEach(function (feature) {
        eIdx = Number(feature.get(key));
        if (newIdx < eIdx) { newIdx = eIdx; }
    });
    return (newIdx + 1).toString();
}
var download = {
    KML: function (source, name) {
        let blob = new Blob([source.getKMLString(true)], { type: 'text/plain;charset=utf-8' });
        saveAs(blob, name + '.kml');
    },
    JSON: function (source, name) {
        let blob = new Blob([source.getJSONString(true)], { type: 'text/plain;charset=utf-8' });
        saveAs(blob, name + '.geojson');
    },
    CSV: function (source, name) {
        let csv = [];
        let header = {};
        let n = 0;
        let isPt = true;
        let features = JSON.parse(source.getJSONString()).features;
        features.forEach(function (f) {
            for (let p in f.properties) {
                if (header[p] === undefined) {
                    header[p] = n++;
                }
            }
            if (f.geometry.type !== 'Point') isPt = false;
        });
        features.forEach(function (f) {
            let row = [];
            for (let p in header) {
                row[header[p]] = f.properties[p];
            }
            if (isPt) {
                row.push(f.geometry.coordinates[0]);
                row.push(f.geometry.coordinates[1]);
            } else {
                row.push(f.geometry.type);
                row.push(JSON.stringify(f.geometry.coordinates));
            }
            csv.push(row);
        });
        let cols = [];
        for (let p in header) {
            cols.push(p);
        }
        if (isPt) {
            cols.push('lon');
            cols.push('lat');
        } else {
            cols.push('type');
            cols.push('coord');
        }
        csv.unshift(cols);
        result = Papa.unparse(csv);
        let blob = new Blob([result], { type: 'text/plain;charset=utf-8' });
        saveAs(blob, name + '.csv');
    }
};
function saveAllVectors() {
    uploadDataToUrl(myNetworkLayer.getSource().getJSONString(),
        vectorsPath + 'mynetwork.geojson');
    Ajax_runccmd("UploadNetworkToDB", "");
    myMap.render();
    loadSitesTreeFromFile();

    uploadDataToUrl(myPolygonsLayer.getSource().getJSONString(),
        vectorsPath + 'mypolygons.geojson');
    Ajax_runccmd("UploadPolygonsToDB", "");
    myMap.render();
    loadPolygonsTreeFromFile();

    uploadDataToUrl(myLinksLayer.getSource().getJSONString(),
        vectorsPath + 'mylinks.geojson');
    Ajax_runccmd("UploadLinksToDB", "");
    myMap.render();
    loadLinksTreeFromFile();

    uploadDataToUrl(myNotesLayer.getSource().getJSONString(),
        vectorsPath + 'mynotes.geojson');
    Ajax_runccmd("UploadNotesToDB", "");
    myMap.render();
    loadNotesTreeFromFile();
}
function loadAllVectors() {
    Ajax_runccmd("GetVectorsFromDB", "");

    Ajax_loadSourceFromFile(myNetworkLayer.getSource(),
        "../" + vectorsPath + "mynetwork.geojson");
    loadSitesTreeFromSource();

    Ajax_loadSourceFromFile(myPolygonsLayer.getSource(),
        "../" + vectorsPath + "mypolygons.geojson");
    loadPolygonsTreeFromSource();

    Ajax_loadSourceFromFile(myLinksLayer.getSource(),
        "../" + vectorsPath + "mylinks.geojson");
    loadLinksTreeFromSource();

    Ajax_loadSourceFromFile(myNotesLayer.getSource(),
        "../" + vectorsPath + "mynotes.geojson");
    loadNotesTreeFromSource();

    Ajax_loadSourceFromFile(resultsLayer.getSource(),
        "../" + vectorsPath + "results.geojson");
    loadResultsTreeFromSource();
}
function Ajax_loadSourceFromFile(source, url) {
    $.ajax({
        type: "GET",
        url: url,
        data: "",
        async: false,
        contentType: "application/json; charset=utf-8",
        dataType: "json",
        success: function (r) {
            source.clearFeatures();
            source.addFeatures(
                new ol.format.GeoJSON().readFeatures(r,
                    {
                        dataProjection: 'EPSG:4326',
                        featureProjection: 'EPSG:3857'
                    }
                ));
            myMap.render();
        },
        failure: function (r) {
            source.clearFeatures();
            myMap.render();
        },
        error: function (r) {
            source.clearFeatures();
            myMap.render();
        }
    });
}
function uploadDataToUrl(data, url) {
    Ajax_runccmd("UploadDataToUrl", "{'data':'" + data + "','url':'" + url + "'}");
}
function Ajax_runccmd(command, args) {
    let result;
    $.ajax({
        async: false,
        type: "POST",
        url: "webui.aspx/" + command,
        data: args,
        contentType: "application/json; charset=utf-8",
        dataType: "json",
        success: function (r) {
            result = r.d;
        },
        failure: function (r) {
            return "";
        },
        error: function (r) {
            return "";
        }
    });
    return result;
}