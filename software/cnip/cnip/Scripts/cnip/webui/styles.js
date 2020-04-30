// pointer move functionality
var highlightStyle = function (feature, layer) {
    return [new ol.style.Style({
        image: feature.get('sitename') === undefined ?
            new ol.style.Icon({ scale: 1, src: '../img/note_highlight.png', opacity: 0.7 }) :
            new ol.style.Icon({ scale: .6, src: '../img/site_highlight.png', opacity: 0.7 }),
        stroke: new ol.style.Stroke({
            color: 'rgba(0,255,0,0.7)',
            width: 2
        }),
        fill: new ol.style.Fill({
            color: 'rgba(0,255,0,0.1)'
        })
    })];
};
var highlightPointStyle = new ol.style.Style({
    image: new ol.style.Circle({
        radius: 5,
        stroke: new ol.style.Stroke({
            color: 'rgba(0,255,0,1)',
            width: 5
        }),
        fill: new ol.style.Fill({
            color: 'rgba(0,255,0,1)'
        })
    })
});
// ******interactions ***********************//
// Select style
var selectStyle = function (feature, layer) {
    return [new ol.style.Style({
        image: feature.get('sitename') === undefined ?
            new ol.style.Icon({ scale: 1, src: '../img/note_selected.png', opacity: 0.7 }) :
            new ol.style.Icon({ scale: .6, src: '../img/site_selected.png', opacity: 0.7 }),
        text: new ol.style.Text({
            offsetY: feature.get('sitename') === undefined && feature.get('notename') === undefined ? 0 : 25,
            text: feature.get('sitename') === undefined ? feature.get('polygonname') === undefined ?
                feature.get('linkname') === undefined ?
                    feature.get('notename') : feature.get('linkname') :
                feature.get('polygonname') : feature.get('sitename'),
            font: '14px "Liberation Sans", "Lucida Sans", "Lucida Sans Regular", "Lucida Grande", "Lucida Sans Unicode", Geneva, Verdana, sans-serif',
            fill: new ol.style.Fill({ color: '#0099ff' }),
            stroke: new ol.style.Stroke({ color: 'rgba(255, 255, 255, 0.2)', width: 3 })
        }),
        fill: new ol.style.Fill({
            color: 'rgba(255, 255, 255, 0.2)'
        }),
        stroke: new ol.style.Stroke({
            color: '#0099ff',
            width: 2
        })
    })];
};


var drawSiteStyle = new ol.style.Style({
    image: new ol.style.Icon({ scale: .6, src: '../img/site_selected.png', opacity: 0.7 })
});
var drawNoteStyle = new ol.style.Style({
    image: new ol.style.Icon({ scale: 1, src: '../img/note_selected.png', opacity: 0.7 })
});
var drawLinkSiteStyle = new ol.style.Style({
    image: new ol.style.Icon({ scale: .6, src: '../img/site_link_add.png', opacity: 0.7 })
});
var drawLinkLineStyle = new ol.style.Style({
    stroke: new ol.style.Stroke({
        color: 'rgba(0, 255, 255, 1)',
        lineDash: [10, 10],
        width: 3
    })
});
var drawPolygonStyle = new ol.style.Style({
    image: new ol.style.Circle({
        fill: new ol.style.Fill({
            color: 'rgba(0, 153, 255, 1)'
        }),
        stroke: new ol.style.Stroke({
            color: 'rgba(255, 255, 255, 0.8)',
            width: 1.5
        }),
        radius: 6
    }),
    fill: new ol.style.Fill({
        color: 'rgba(255, 255, 255, 0.7)'
    }),
    stroke: new ol.style.Stroke({
        color: 'rgba(0, 153, 255, 1)',
        lineDash: [10, 10],
        width: 2
    })
});


var deleteSiteStyle = new ol.style.Style({
    image: new ol.style.Icon({ scale: 0.6, src: '../img/site_delete.png', opacity: 1 })
});
var deleteNoteStyle = new ol.style.Style({
    image: new ol.style.Icon({ scale: 1.3, src: '../img/note_delete.png', opacity: 1 })
});
var deleteLinkStyle = new ol.style.Style({
    stroke: new ol.style.Stroke({
        color: 'rgba(255, 0, 0, 1)',
        lineDash: [7, 7],
        width: 5
    })
});
var deletePolygonStyle = new ol.style.Style({
    stroke: new ol.style.Stroke({
        color: 'rgba(255,0,0,1)',
        width: 3
    }),
    fill: new ol.style.Fill({
        color: 'rgba(255,0,0,0.1)'
    })
});
