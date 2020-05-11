var lyrBingMapsAerial = new ol.layer.Tile({
    source: new ol.source.BingMaps({
        imagerySet: 'Aerial',
        key: 'AjabU18xKFl_N3WgLVzRwdDDjcaQTIftpixGXL3Dk93ucbLfm27eEdiQts5lQ0Bt',
        crossOrigin: 'anonymous',
        wrapX: false
    }),
    title: 'Aerial',
    type: 'base',
    visible: false
});
var lyrBingMapsAerialWithLabels = new ol.layer.Tile({
    source: new ol.source.BingMaps({
        imagerySet: 'AerialWithLabels',
        key: 'AjabU18xKFl_N3WgLVzRwdDDjcaQTIftpixGXL3Dk93ucbLfm27eEdiQts5lQ0Bt',
        crossOrigin: 'anonymous',
        wrapX: false
    }),
    title: 'Aerial With Labels',
    type: 'base',
    visible: false
});
var lyrBingMapsRoad = new ol.layer.Tile({
    source: new ol.source.BingMaps({
        imagerySet: 'Road',
        key: 'AjabU18xKFl_N3WgLVzRwdDDjcaQTIftpixGXL3Dk93ucbLfm27eEdiQts5lQ0Bt',
        crossOrigin: 'anonymous',
        wrapX: false
    }),
    title: 'Road',
    type: 'base',
    visible: false
});
var lyrBingMapsRoadOnDemand = new ol.layer.Tile({
    source: new ol.source.BingMaps({
        imagerySet: 'RoadOnDemand',
        key: 'AjabU18xKFl_N3WgLVzRwdDDjcaQTIftpixGXL3Dk93ucbLfm27eEdiQts5lQ0Bt',
        crossOrigin: 'anonymous',
        wrapX: false
    }),
    title: 'Road On Demand',
    type: 'base',
    visible: false
});
var BingMapsLayers = new ol.layer.Group({
    title: 'Bing Maps',
    fold: 'close',
    layers: [
        lyrBingMapsRoadOnDemand,
        lyrBingMapsRoad,
        lyrBingMapsAerialWithLabels,
        lyrBingMapsAerial
    ]
});