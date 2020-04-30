var thunderforestAttributions = [
    'Tiles &copy; <a href="http://www.thunderforest.com/">Thunderforest</a>',
    ol.source.OSM.ATTRIBUTION
];
var lyrThunderForestCycleMap = new ol.layer.Tile({
    title: 'Cycle',
    type: 'base',
    visible: false,
    source: new ol.source.OSM(
        {
            'url': 'http://tile.thunderforest.com/cycle/{z}/{x}/{y}.png?apikey=ec248890a6ed43b8841ea243819f8d0d',
            attributions: thunderforestAttributions,
            wrapX: false
        })
});
var lyrThunderForestTransport = new ol.layer.Tile({
    title: 'Transport',
    type: 'base',
    visible: false,
    source: new ol.source.OSM(
        {
            'url': 'http://tile.thunderforest.com/transport/{z}/{x}/{y}.png?apikey=ec248890a6ed43b8841ea243819f8d0d',
            wrapX: false
        })
});
var lyrThunderForestTransportDark = new ol.layer.Tile({
    title: 'Transport Dark',
    type: 'base',
    visible: false,
    source: new ol.source.OSM(
        {
            'url': 'http://tile.thunderforest.com/transport-dark/{z}/{x}/{y}.png?apikey=ec248890a6ed43b8841ea243819f8d0d',
            wrapX: false
        })
});
var lyrThunderForestLandscape = new ol.layer.Tile({
    title: 'Landscape',
    type: 'base',
    visible: false,
    source: new ol.source.OSM(
        {
            'url': 'http://tile.thunderforest.com/landscape/{z}/{x}/{y}.png?apikey=ec248890a6ed43b8841ea243819f8d0d',
            wrapX: false
        })
});
var lyrThunderForestOutdoors = new ol.layer.Tile({
    title: 'Outdoors',
    type: 'base',
    visible: false,
    source: new ol.source.OSM(
        {
            'url': 'http://tile.thunderforest.com/outdoors/{z}/{x}/{y}.png?apikey=ec248890a6ed43b8841ea243819f8d0d',
            wrapX: false
        })
});
var ThunderForestLayers = new ol.layer.Group({
    title: 'Thunder Forest',
    fold: 'close',
    layers: [
        lyrThunderForestTransportDark,
        lyrThunderForestTransport,
        lyrThunderForestCycleMap,
        lyrThunderForestLandscape,
        lyrThunderForestOutdoors
    ]
});