var lyrStamenToner = new ol.layer.Tile({
    title: 'Toner',
    type: 'base',
    visible: false,
    source: new ol.source.Stamen({
        layer: 'toner',
        wrapX: false})
});
var lyrStamenTonerLite = new ol.layer.Tile({
    title: 'Toner-Lite',
    type: 'base',
    visible: false,
    source: new ol.source.Stamen({
        layer: 'toner-lite',
        wrapX: false })
});
var lyrStamenTerrain = new ol.layer.Tile({
    title: 'Terrain',
    type: 'base',
    visible: false,
    source: new ol.source.Stamen({
        layer: 'terrain',
        wrapX: false })
});
var lyrStamenTerrainBackground = new ol.layer.Tile({
    title: 'Terrain Background',
    type: 'base',
    visible: false,
    source: new ol.source.Stamen({
        layer: 'terrain-background',
        wrapX: false})
});
var lyrStamenWaterColor = new ol.layer.Tile({
    title: 'Water Color',
    type: 'base',
    visible: false,
    source: new ol.source.Stamen({
        layer: 'watercolor',
        wrapX: false})
});
var StamenLayers = new ol.layer.Group({
    title: 'Stamen',
    fold: 'close',
    layers: [
        lyrStamenTerrainBackground,
        lyrStamenTerrain,
        lyrStamenTonerLite,
        lyrStamenToner,
        lyrStamenWaterColor
    ]
});