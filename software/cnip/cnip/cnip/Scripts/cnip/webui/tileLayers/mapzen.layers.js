var lyrMapzenTerrarium = new ol.layer.Tile({
    title: 'Terrarium',
    type: 'base',
    visible: false,
    source: new ol.source.XYZ({
        url: 'https://s3.amazonaws.com/elevation-tiles-prod/terrarium/{z}/{x}/{y}.png',
        crossOrigin: 'anonymous',
        wrapX: false
    })
});
var lyrMapzenNormal = new ol.layer.Tile({
    title: 'Normal',
    type: 'base',
    visible: false,
    source: new ol.source.XYZ({
        url: 'https://s3.amazonaws.com/elevation-tiles-prod/normal/{z}/{x}/{y}.png',
        crossOrigin: 'anonymous',
        wrapX: false
    })
});
var MapzenLayers = new ol.layer.Group({
    title: 'Mapzen',
    fold: 'close',
    layers: [
        lyrMapzenTerrarium,
        lyrMapzenNormal
    ]
});