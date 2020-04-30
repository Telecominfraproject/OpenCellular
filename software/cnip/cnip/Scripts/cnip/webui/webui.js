// about
var about = document.getElementById('about');
about.innerHTML = '<img alt="" style="width:113px;height:30px;" src="../img/cnip.png" />';
about.style.display = 'block';
// action status
var actionRunning = false;
// initialize map
var myMap = new ol.Map({
    loadTilesWhileAnimating: false,
    loadTilesWhileInteracting: false,
    interactions: ol.interaction.defaults({ altShiftDragRotate: false, pinchRotate: false }),
    target: 'myMap',
    view: new ol.View({
        center: ol.proj.fromLonLat([67.056848, 24.808224]),
        zoom: 15
    }),
    layers: [
        baseLayers,
        overlayLayers
    ]
});
var sidebar = new ol.control.Sidebar({ element: 'sidebar', position: 'left' });
var toc = document.getElementById('layers');
ol.control.LayerSwitcher.renderPanel(myMap, toc);
myMap.addControl(sidebar);
myMap.addControl(new ol.control.ScaleLine({ units: 'metric' }));
myMap.getControls().forEach(function (control) {
    if (control instanceof ol.control.Attribution) {
        myMap.removeControl(control);
    }
}, this);
// pointer move functionality
var highlightLayer = new ol.layer.Vector({
    source: new ol.source.Vector(),
    map: myMap
});
highlightLayer.setStyle(highlightStyle);
var highlightFeature;
//var featureTip = $('#featureTip');
//featureTip.tooltip({ animation: false, trigger: 'manual' });
var noteInfo = document.getElementById('noteInfo');
var featureInfo = document.getElementById('featureInfo');
var locationInfo = document.getElementById('locationInfo');
var displayFeatureInfo = function (e) {
    let pixel = myMap.getEventPixel(e.originalEvent);
    let features = []; let feature = undefined;
    let layers = []; let layer = undefined;
    myMap.forEachFeatureAtPixel(pixel,
        function (feature, layer) { layers.push(layer); features.push(feature); });
    let layerIndex = 0;
    features.forEach(function (f) {
        if (f.get('sitename') !== undefined || f.get('linkname') !== undefined || f.get('notename') !== undefined) {
            feature = f; layer = layers[layerIndex];
        }
        layerIndex += 1;
    });
    if (feature === undefined) {
        layerIndex = 0;
        features.forEach(function (f) {
            feature = f; layer = layers[layerIndex];
        });
        layerIndex += 1;
    }
    if (!addLink.getActive() && !deleteSiteActive && !deleteLinkActive && !deleteNoteActive && !deletePolygonActive) {
        if (feature) {
            let properties = [];
            if (feature.getGeometry().getType() === 'Polygon') {
                let geom = feature.clone().getGeometry().transform('EPSG:3857', 'EPSG:4326');
                let polygon = turf.polygon(geom.getCoordinates());
                let area = round(turf.area(polygon) / 1000000, 2);
                let centerOfMass = turf.getCoord(turf.centerOfMass(polygon));
                let center = turf.getCoord(turf.center(polygon));
                properties.push('<tr><td style="text-align:left;padding-left:5px;">Calculated</td><td>&nbsp;&nbsp;</td><td style="text-align:left;padding-right:5px;"></td></tr>');
                properties.push('<tr><td style="text-align:left;padding-left:5px;">Area</td><td>&nbsp;:&nbsp;</td><td style="text-align:left;padding-right:5px;">' + area.toString() + ' Km²' + '</td></tr>');
                properties.push('<tr><td style="text-align:left;padding-left:5px;">Center</td><td>&nbsp;:&nbsp;</td><td style="text-align:left;padding-right:5px;">' + round(center[0], 6).toString() + ',' + round(center[1], 6).toString() + '</td></tr>');
                properties.push('<tr><td style="text-align:left;padding-left:5px;">Center of Mass</td><td>&nbsp;:&nbsp;</td><td style="text-align:left;padding-right:5px;">' + round(centerOfMass[0], 6).toString() + ',' + round(centerOfMass[1], 6).toString() + '</td></tr>');
                properties.push('<tr><td style="text-align:left;padding-left:5px;"></td><td>&nbsp;&nbsp;</td><td style="text-align:left;padding-right:5px;"></td></tr>');
            }
            feature.getKeys().forEach(function (key) {
                if (key !== 'geometry' && key !== 'elevstr' && !key.toString().includes('id')) {
                    if (feature.get(key)) {
                        properties.push('<tr><td style="text-align:left;padding-left:5px;">' + titleCase(key) +
                            '</td><td>&nbsp;:&nbsp;</td><td style="text-align:left;padding-right:5px;">' +
                            (isNumber(feature.get(key)) ? round(parseFloat(feature.get(key)), 6) :
                                feature.get(key).toString().length > 15 ?
                                    feature.get(key).toString().substr(0, 15) : feature.get(key)) +
                            '</td></tr>');
                    }
                }
            });
            featureInfo.innerHTML = '<table><tbody><tr><td style="color:var(--facecolor);background-color:var(--barcolor);padding-left:5px;">Quick Info</td><td style="color:var(--facecolor);background-color:var(--barcolor)"></td><td style="color:var(--facecolor);background-color:var(--barcolor)"></td></tr>' + properties.join('') + '</tbody></table>' || '(unknown)';
            featureInfo.style.opacity = 1;
            if (layer === myNotesLayer || layer === publicNotesLayer) {
                noteInfo.style.left = (pixel[0] + 5).toString() + 'px';
                noteInfo.style.top = (pixel[1] - 15).toString() + 'px';
                noteInfo.innerHTML = '<span style="background-color: #fff2ab;">+ Description&nbsp;' +
                    '&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;' +
                    '&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;...&nbsp;&nbsp;</span><br/>' +
                    '<span style="background-color: #fff7d1;">' + truncate(feature.get('description'), 255) + '</span>';
                noteInfo.style.opacity = 1;
            }
            //featureTip.css({ left: pixel[0] + 'px', top: pixel[1] - 15 + 'px' });
            //featureTip.attr('data-original-title', feature.get('sitename') === undefined ?
            //    feature.get('polygonname') === undefined ? feature.get('linkname') :
            //        feature.get('polygonname') : feature.get('sitename')).tooltip('show');
        } else {
            featureInfo.innerHTML = ''; featureInfo.style.opacity = 0;
            noteInfo.innerHTML = ''; noteInfo.style.opacity = 0;
            //featureTip.attr('data-original-title', '').tooltip('hide');
        }
    }
    if (feature !== highlightFeature) {
        if (highlightFeature) {
            highlightLayer.getSource().clear();
        }
        if (feature) {
            if (!addLink.getActive() && !deleteSiteActive && !deleteLinkActive && !deleteNoteActive && !deletePolygonActive) {
                highlightLayer.getSource().addFeature(feature);
            } else {
                if (addLink.getActive() || deleteSiteActive) {
                    if (layer === myNetworkLayer) {
                        highlightLayer.getSource().addFeature(feature);
                    }
                }
                if (deleteNoteActive) {
                    if (layer === myNotesLayer) {
                        highlightLayer.getSource().addFeature(feature);
                    }
                }
                if (deleteLinkActive) {
                    if (layer === myLinksLayer) {
                        highlightLayer.getSource().addFeature(feature);
                    }
                }
                if (deletePolygonActive) {
                    if (layer === myPolygonsLayer) {
                        highlightLayer.getSource().addFeature(feature);
                    }
                }
            }
        }
        highlightFeature = feature;
    }
};
function highlightPoint(loc) {
    let feature = new ol.Feature(new ol.geom.Point(ol.proj.fromLonLat(loc)));
    feature.setStyle(highlightPointStyle);
    highlightLayer.getSource().clear();
    highlightLayer.getSource().addFeature(feature);
    highlightFeature = feature;
}
var displayElevation = function (e) {
    let lonlat = ol.proj.toLonLat(e.coordinate);
    let elev = ''; //Ajax_runccmd("GetElevationFromLonLat", "{'longitude':'" + lonlat[0] + "','latitude':'" + lonlat[1] + "'}");
    locationInfo.style.opacity = 1;
    locationInfo.innerHTML =
        ol.coordinate.format(lonlat, 'lon {x}\xB0 lat {y}\xB0 ', 6);//elev ', 6) + elev;
};
var setPointerStyle = function (e) {
    if (modifyNote.getActive()) {
        let hit = e.map.hasFeatureAtPixel(e.pixel, {
            layerFilter: function (layer) {
                return layer === myNotesLayer;
            }
        });
        e.map.getTargetElement().style.cursor = hit ? 'move' : '';
    }
    if (modifySite.getActive()) {
        let hit = e.map.hasFeatureAtPixel(e.pixel, {
            layerFilter: function (layer) {
                return layer === myNetworkLayer;
            }
        });
        e.map.getTargetElement().style.cursor = hit ? 'move' : '';
    }
};
$('#sidebar div ul li a').click(function () {
    toggleSelect();
});

