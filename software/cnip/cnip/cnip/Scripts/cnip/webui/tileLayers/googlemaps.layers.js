var lyrGoogleMapsRoadNames = new ol.layer.Tile({
    title: "Road Names",
    source: new ol.source.TileImage({
        url: 'http://mt1.google.com/vt/lyrs=h&x={x}&y={y}&z={z}',
        wrapX: false}),
    type: 'base',
    visible: false
});
var lyrGoogleMapsRoadmap = new ol.layer.Tile({
    title: "Road Map",
    source: new ol.source.TileImage({
        url: 'http://mt1.google.com/vt/lyrs=m&x={x}&y={y}&z={z}',
        wrapX: false}),
    type: 'base',
    visible: false
});
var lyrGoogleMapsSatellite = new ol.layer.Tile({
    title: "Satellite",
    source: new ol.source.TileImage({
        url: 'http://mt1.google.com/vt/lyrs=s&hl=pl&&x={x}&y={y}&z={z}',
        wrapX: false }),
    type: 'base',
    visible: false
});
var lyrGoogleMapsHybrid = new ol.layer.Tile({
    title: "Satellite & Roads",
    source: new ol.source.TileImage({
        url: 'http://mt1.google.com/vt/lyrs=y&x={x}&y={y}&z={z}',
        wrapX: false}),
    type: 'base',
    visible: false
});
var lyrGoogleMapsTerrain = new ol.layer.Tile({
    title: "Terrain",
    source: new ol.source.TileImage({
        url: 'http://mt1.google.com/vt/lyrs=t&x={x}&y={y}&z={z}',
        wrapX: false }),
    type: 'base',
    visible: false
});
var lyrGoogleMapsHybrid2 = new ol.layer.Tile({
    title: "Terrain & Roads",
    source: new ol.source.TileImage({
        url: 'http://mt1.google.com/vt/lyrs=p&x={x}&y={y}&z={z}',
        wrapX: false}),
    type: 'base',
    visible: false
});
var lyrGoogleMapsOnlyRoad = new ol.layer.Tile({
    title: "Road without Building",
    source: new ol.source.TileImage({
        url: 'http://mt1.google.com/vt/lyrs=r&x={x}&y={y}&z={z}',
        wrapX: false}),
    type: 'base',
    visible: false
});
var GoogleMapsLayers = new ol.layer.Group({
    title: 'Google Maps',
    fold: 'close',
    layers: [
        lyrGoogleMapsHybrid2,
        lyrGoogleMapsHybrid,
        lyrGoogleMapsTerrain,
        lyrGoogleMapsOnlyRoad,
        lyrGoogleMapsRoadNames,
        lyrGoogleMapsRoadmap,
        lyrGoogleMapsSatellite
    ]
});