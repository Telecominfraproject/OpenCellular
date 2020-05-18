var stamenTonerCopyright = 'Map tiles by <a href="http://stamen.com">Stamen Design</a>, under <a href="http://creativecommons.org/licenses/by/3.0">CC BY 3.0</a>. Data by <a href="http://openstreetmap.org">OpenStreetMap</a>, under <a href="http://www.openstreetmap.org/copyright">ODbL</a>.';
var stamenWatercolorCopyright = 'Map tiles by <a href="http://stamen.com">Stamen Design</a>, under <a href="http://creativecommons.org/licenses/by/3.0">CC BY 3.0</a>. Data by <a href="http://openstreetmap.org">OpenStreetMap</a>, under <a href="http://creativecommons.org/licenses/by-sa/3.0">CC BY SA</a>.';

var lyrStamenToner = new ol.layer.Tile({
    title: 'Toner',
    type: 'base',
    visible: false,
    source: new ol.source.Stamen({
        layer: 'toner',
        wrapX: false
    })
});
var lyrStamenTonerLite = new ol.layer.Tile({
    title: 'Toner-Lite',
    type: 'base',
    visible: false,
    source: new ol.source.Stamen({
        layer: 'toner-lite',
        wrapX: false
    })
});
var lyrStamenTerrain = new ol.layer.Tile({
    title: 'Terrain',
    type: 'base',
    visible: false,
    source: new ol.source.Stamen({
        layer: 'terrain',
        wrapX: false
    })
});
var lyrStamenTerrainBackground = new ol.layer.Tile({
    title: 'Terrain Background',
    type: 'base',
    visible: false,
    source: new ol.source.Stamen({
        layer: 'terrain-background',
        wrapX: false
    })
});
var lyrStamenWaterColor = new ol.layer.Tile({
    title: 'Water Color',
    type: 'base',
    visible: false,
    source: new ol.source.Stamen({
        layer: 'watercolor',
        wrapX: false
    })
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