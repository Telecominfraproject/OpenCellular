var lyrMapTilerBasic = new ol.layer.Tile({
    title: 'Basic',
    type: 'base',
    visible: false,
    source: new ol.source.XYZ({
        url: 'https://api.maptiler.com/maps/basic/{z}/{x}/{y}.png?key=aIokLGbrqIsRfBnnPIFP',
        crossOrigin: 'anonymous',
        wrapX: false
    })
});
var lyrMapTilerBright = new ol.layer.Tile({
    title: 'Bright',
    type: 'base',
    visible: false,
    source: new ol.source.XYZ({
        url: 'https://api.maptiler.com/maps/bright/{z}/{x}/{y}.png?key=aIokLGbrqIsRfBnnPIFP',
        crossOrigin: 'anonymous',
        wrapX: false
    })
});
var lyrMapTilerDarkmatter = new ol.layer.Tile({
    title: 'Darkmatter',
    type: 'base',
    visible: false,
    source: new ol.source.XYZ({
        url: 'https://api.maptiler.com/maps/darkmatter/{z}/{x}/{y}.png?key=aIokLGbrqIsRfBnnPIFP',
        crossOrigin: 'anonymous',
        wrapX: false
    })
});
var lyrMapTilerPastel = new ol.layer.Tile({
    title: 'Pastel',
    type: 'base',
    visible: false,
    source: new ol.source.XYZ({
        url: 'https://api.maptiler.com/maps/pastel/{z}/{x}/{y}.png?key=aIokLGbrqIsRfBnnPIFP',
        crossOrigin: 'anonymous',
        wrapX: false
    })
});
var lyrMapTilerPositron = new ol.layer.Tile({
    title: 'Positron',
    type: 'base',
    visible: false,
    source: new ol.source.XYZ({
        url: 'https://api.maptiler.com/maps/positron/{z}/{x}/{y}.png?key=aIokLGbrqIsRfBnnPIFP',
        crossOrigin: 'anonymous',
        wrapX: false
    })
});
var lyrMapTilerHybrid = new ol.layer.Tile({
    title: 'Hybrid',
    type: 'base',
    visible: false,
    source: new ol.source.XYZ({
        url: 'https://api.maptiler.com/maps/hybrid/{z}/{x}/{y}.jpg?key=aIokLGbrqIsRfBnnPIFP',
        crossOrigin: 'anonymous',
        wrapX: false
    })
});
var lyrMapTilerStreets = new ol.layer.Tile({
    title: 'Streets',
    type: 'base',
    visible: true,
    source: new ol.source.XYZ({
        url: 'https://api.maptiler.com/maps/streets/{z}/{x}/{y}.png?key=aIokLGbrqIsRfBnnPIFP',
        crossOrigin: 'anonymous',
        wrapX: false
    })
});
var lyrMapTilerTopo = new ol.layer.Tile({
    title: 'Topo',
    type: 'base',
    visible: false,
    source: new ol.source.XYZ({
        url: 'https://api.maptiler.com/maps/topo/{z}/{x}/{y}.png?key=aIokLGbrqIsRfBnnPIFP',
        crossOrigin: 'anonymous',
        wrapX: false
    })
});
var lyrMapTilerTopographique = new ol.layer.Tile({
    title: 'Topographique',
    type: 'base',
    visible: false,
    source: new ol.source.XYZ({
        url: 'https://api.maptiler.com/maps/topographique/{z}/{x}/{y}.png?key=aIokLGbrqIsRfBnnPIFP',
        crossOrigin: 'anonymous',
        wrapX: false
    })
});
var lyrMapTilerVoyager = new ol.layer.Tile({
    title: 'Voyager',
    type: 'base',
    visible: false,
    source: new ol.source.XYZ({
        url: 'https://api.maptiler.com/maps/voyager/{z}/{x}/{y}.png?key=aIokLGbrqIsRfBnnPIFP',
        crossOrigin: 'anonymous',
        wrapX: false
    })
});
var MapTilerLayers = new ol.layer.Group({
    title: 'Maptiler',
    fold: 'open',
    layers: [
        lyrMapTilerVoyager,
        lyrMapTilerTopographique,
        lyrMapTilerTopo,
        lyrMapTilerStreets,
        lyrMapTilerHybrid,
        lyrMapTilerPositron,
        lyrMapTilerPastel,
        lyrMapTilerDarkmatter,
        lyrMapTilerBright,
        lyrMapTilerBasic
    ]
});