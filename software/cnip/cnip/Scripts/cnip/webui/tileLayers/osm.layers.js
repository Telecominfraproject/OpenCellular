var lyrOpenStreetMap = new ol.layer.Tile({
    title: 'Street',
    type: 'base',
    visible: false,
    source: new ol.source.OSM({
        wrapX: false
    })
});
var lyrOpenTopoMap = new ol.layer.Tile({
    title: 'Topo',
    type: 'base',
    visible: false,
    source: new ol.source.XYZ({
        url: 'https://{a-c}.tile.opentopomap.org/{z}/{x}/{y}.png',
        crossOrigin: 'anonymous',
        wrapX: false
    })
});
var OSMLayers = new ol.layer.Group({
    title: 'Open Street Map',
    fold: 'close',
    layers: [
        lyrOpenTopoMap,
        lyrOpenStreetMap
    ]
});