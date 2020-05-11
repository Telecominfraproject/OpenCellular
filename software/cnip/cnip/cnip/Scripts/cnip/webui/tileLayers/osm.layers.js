var osmCopyright = '© <a href="https://www.openstreetmap.org/copyright">OpenStreetMap contributors<a>';
var topoCopyright = 'Kartendaten: © <a href="https://openstreetmap.org/copyright">OpenStreetMap</a>-Mitwirkende, SRTM | Kartendarstellung: © <a href="http://opentopomap.org">OpenTopoMap</a> (<a href="https://creativecommons.org/licenses/by-sa/3.0/">CC-BY-SA</a>)';

var lyrOpenStreetMap = new ol.layer.Tile({
    title: 'Street',
    type: 'base',
    visible: true,
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