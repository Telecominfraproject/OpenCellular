var publicNotesSource = new ol.source.Vector({
    url: '../' + vectorsPath + 'publicnotes.geojson',
    format: new ol.format.GeoJSON({
        extractStyles: true,
        extractAttributes: true
    }),
    wrapX: false
});
var publicNotesLayer = new ol.layer.Vector({
    title: 'Public Notes',
    visible: true,
    source: publicNotesSource,
    minResolution: 0,
    maxResolution: 1000,
    style: function (feature) {
        return [new ol.style.Style({
            image: feature.get('notetype') === 'Public' ? new ol.style.Icon({
                scale: 1,
                src: '../img/note_Public.png',
                opacity: 1
            }) : new ol.style.Icon({
                scale: 1,
                src: '../img/note_Public.png',
                opacity: 1
            }),
            text: new ol.style.Text({
                offsetY: 25,
                text: feature.get('notename'),
                font: '14px "Liberation Sans", "Lucida Sans", "Lucida Sans Regular", "Lucida Grande", "Lucida Sans Unicode", Geneva, Verdana, sans-serif',
                fill: new ol.style.Fill({ color: '#000' }),
                stroke: new ol.style.Stroke({ color: '#fff', width: 2 })
            })
        })];
    }
});
var myNotesSource = new ol.source.Vector({
    url: '../' + vectorsPath + 'mynotes.geojson',
    format: new ol.format.GeoJSON({
        extractStyles: true,
        extractAttributes: true
    }),
    wrapX: false
});
var myNotesLayer = new ol.layer.Vector({
    title: 'My Notes',
    visible: true,
    source: myNotesSource,
    minResolution: 0,
    maxResolution: 1000,
    style: function (feature) {
        return [new ol.style.Style({
            image: feature.get('notetype') === 'Public' ? new ol.style.Icon({
                scale: 1,
                src: '../img/notePublic.png',
                opacity: 1
            }) : new ol.style.Icon({
                scale: 1,
                src: '../img/notePrivate.png',
                opacity: 1
            }),
            text: new ol.style.Text({
                offsetY: 25,
                text: feature.get('notename'),
                font: '14px "Liberation Sans", "Lucida Sans", "Lucida Sans Regular", "Lucida Grande", "Lucida Sans Unicode", Geneva, Verdana, sans-serif',
                fill: new ol.style.Fill({ color: '#000' }),
                stroke: new ol.style.Stroke({ color: '#fff', width: 2 })
            })
        })];
    }
});
var publicNetworkSource = new ol.source.Vector({
    url: '../' + vectorsPath + 'publicnetwork.geojson',
    format: new ol.format.GeoJSON({
        extractStyles: true,
        extractAttributes: true
    }),
    wrapX: false
});
var publicNetworkLayer = new ol.layer.Vector({
    title: 'Public Network',
    visible: true,
    source: publicNetworkSource,
    minResolution: 0,
    maxResolution: 1000,
    style: function (feature) {
        return [new ol.style.Style({
            image: feature.get('technology') === '4G' ? new ol.style.Icon({
                scale: .6,
                src: '../img/site_Public.png',
                opacity: 1
            }) : new ol.style.Icon({
                scale: .6,
                src: '../img/site_Public.png',
                opacity: 1
            }),
            text: new ol.style.Text({
                offsetY: 25,
                text: feature.get('sitename'),
                font: '14px "Liberation Sans", "Lucida Sans", "Lucida Sans Regular", "Lucida Grande", "Lucida Sans Unicode", Geneva, Verdana, sans-serif',
                fill: new ol.style.Fill({ color: '#000' }),
                stroke: new ol.style.Stroke({ color: '#fff', width: 2 })
            })
        })];
    }
});
var myNetworkSource = new ol.source.Vector({
    url: '../' + vectorsPath + 'mynetwork.geojson',
    format: new ol.format.GeoJSON({
        extractStyles: true,
        extractAttributes: true
    }),
    wrapX: false
});
var myNetworkLayer = new ol.layer.Vector({
    title: 'My Network',
    visible: true,
    source: myNetworkSource,
    minResolution: 0,
    maxResolution: 1000,
    style: function (feature) {
        return [new ol.style.Style({
            image: feature.get('technology') === '4G' ? new ol.style.Icon({
                scale: .6,
                src: '../img/site4G.png',
                opacity: 1
            }) : new ol.style.Icon({
                scale: .6,
                src: '../img/site2G.png',
                opacity: 1
            }),
            text: new ol.style.Text({
                offsetY: 25,
                text: feature.get('sitename'),
                font: '14px "Liberation Sans", "Lucida Sans", "Lucida Sans Regular", "Lucida Grande", "Lucida Sans Unicode", Geneva, Verdana, sans-serif',
                fill: new ol.style.Fill({ color: '#000' }),
                stroke: new ol.style.Stroke({ color: '#fff', width: 2 })
            })
        })];
    }
});
var myPolygonsSource = new ol.source.Vector({
    url: '../' + vectorsPath + 'mypolygons.geojson',
    format: new ol.format.GeoJSON({
        extractStyles: true,
        extractAttributes: true
    }),
    wrapX: false
});
var myPolygonsLayer = new ol.layer.Vector({
    title: 'My Polygons',
    visible: true,
    source: myPolygonsSource,
    minResolution: 0,
    maxResolution: 1000,
    style: function (feature, resolution) {
        return [new ol.style.Style({
            fill: new ol.style.Fill({
                color: 'rgba(255, 255, 255, 0.2)'
            }),
            stroke: new ol.style.Stroke({
                color: '#ffcc33',
                width: 2
            }),
            text: new ol.style.Text({
                text: feature.get('polygonname'),
                font: '14px "Liberation Sans", "Lucida Sans", "Lucida Sans Regular", "Lucida Grande", "Lucida Sans Unicode", Geneva, Verdana, sans-serif',
                fill: new ol.style.Fill({ color: '#000' }),
                stroke: new ol.style.Stroke({ color: '#fff', width: 2 })
            })
        })];
    }
});
var myLinksSource = new ol.source.Vector({
    url: '../' + vectorsPath + 'mylinks.geojson',
    format: new ol.format.GeoJSON({
        extractStyles: true,
        extractAttributes: true
    }),
    wrapX: false
});
var myLinksLayer = new ol.layer.Vector({
    title: 'My Links',
    visible: true,
    source: myLinksSource,
    minResolution: 0,
    maxResolution: 1000,
    style: function (feature, resolution) {
        return [new ol.style.Style({
            stroke: new ol.style.Stroke({
                color: feature.get('linktype') === 'internal' ? 'rgba(0,255,255,1)' : 'rgba(255,0,220,1)',
                lineDash: [7, 7],
                width: 4
            }),
            text: new ol.style.Text({
                text: feature.get('linkname'),
                font: '14px "Liberation Sans", "Lucida Sans", "Lucida Sans Regular", "Lucida Grande", "Lucida Sans Unicode", Geneva, Verdana, sans-serif',
                fill: new ol.style.Fill({ color: '#000' }),
                stroke: new ol.style.Stroke({ color: '#fff', width: 2 })
            })
        })];
    }
});
var myClutterSource = new ol.source.Vector({
    url: '../' + vectorsPath + 'myclutter.geojson',
    format: new ol.format.GeoJSON({
        extractStyles: true,
        extractAttributes: true
    }),
    wrapX: false
});
var myClutterLayer = new ol.layer.Vector({
    title: 'My Clutter',
    visible: true,
    source: myClutterSource,
    minResolution: 0,
    maxResolution: 1000,
    style: function (feature, resolution) {
        return [new ol.style.Style({
            fill: new ol.style.Fill({
                color: 'rgba(255, 255, 255, 0.2)'
            }),
            stroke: new ol.style.Stroke({
                color: '#11f0ff',
                width: 2
            }),
            text: new ol.style.Text({
                text: feature.get('cluttertype'),
                font: '14px "Liberation Sans", "Lucida Sans", "Lucida Sans Regular", "Lucida Grande", "Lucida Sans Unicode", Geneva, Verdana, sans-serif',
                fill: new ol.style.Fill({ color: '#000' }),
                stroke: new ol.style.Stroke({ color: '#fff', width: 2 })
            })
        })];
    }
});
var resultsSource = new ol.source.Vector({
    url: '../' + vectorsPath + 'results.geojson',
    format: new ol.format.GeoJSON({
        extractStyles: true,
        extractAttributes: true
    }),
    wrapX: false
});
var resultsLayer = new ol.layer.Vector({
    source: resultsSource
});
var overlayLayers = new ol.layer.Group({
    title: 'Overlay Layers',
    fold: 'close',
    layers: [
        resultsLayer,
        myPolygonsLayer,
        publicNotesLayer,
        myNotesLayer,
        myLinksLayer,
        publicNetworkLayer,
        myNetworkLayer
    ]
});
var baseLayers = new ol.layer.Group({
    'title': 'Base Layers',
    fold: 'close',
    layers: [
        GoogleMapsLayers,
        BingMapsLayers,
        MapzenLayers,
        ThunderForestLayers,
        MapTilerLayers,
        StamenLayers,
        OSMLayers
    ]
});