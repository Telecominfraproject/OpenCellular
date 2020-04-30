$('#notesDiv').collapse({
    toggle: true
});
var notesTree = document.getElementById('notesUl');
initSingleSlider(document.getElementById('notesOpacity'), '', '0', '1', '0.1', '1');
document.getElementById('notesOpacity').addEventListener('change', setNotesStyle);
loadNotesTreeFromFile();
/* note functions*/
function setNotesStyle(e) {
    if (e) { e.stopPropagation(); }
    let custStyle = function (feature) {
        let noteOpacity = Number($('#notesOpacity').val());
        return [new ol.style.Style({
            image: feature.get('notetype') === 'Public' ? new ol.style.Icon({
                scale: 1,
                src: '../img/notePublic.png',
                opacity: noteOpacity
            }) : new ol.style.Icon({
                scale: 1,
                src: '../img/notePrivate.png',
                opacity: noteOpacity
            }),
            text: new ol.style.Text({
                offsetY: 25,
                text: feature.get('notename'),
                font: '14px "Liberation Sans", "Lucida Sans", "Lucida Sans Regular", "Lucida Grande", "Lucida Sans Unicode", Geneva, Verdana, sans-serif',
                fill: new ol.style.Fill({ color: 'rgba(0,0,0,'+noteOpacity+')' }),
                stroke: new ol.style.Stroke({ color: 'rgba(255,255,255,' + noteOpacity + ')', width: 2 })
            })
        })];
    };
    myNotesLayer.setStyle(custStyle);
    myMap.render();
}
function loadNotesTreeFromFile() {
    let httpRequest = new XMLHttpRequest();
    httpRequest.open('GET', '../' + vectorsPath + 'mynotes.geojson', true);
    httpRequest.send();
    httpRequest.onload = function () {
        let source = JSON.parse(httpRequest.responseText);
        notesTree.innerHTML = '';
        if (typeof source.features !== 'undefined') {
            for (let i = 0; i < source.features.length; i++) {
                notesTree.innerHTML = notesTree.innerHTML +
                    '<li class="visualLi" onclick="showNoteOptionsByIdx(event,' + i.toString() + ');"><i class="fas fa-flag"></i>&nbsp;' +
                    truncate(source.features[i].properties.notename,27) + '</li>';
            }
        }
    };
}
function loadNotesTreeFromSource() {
    let features = myNotesLayer.getSource().getFeatures();
    notesTree.innerHTML = '';
    for (let i = 0; i < features.length; i++) {
        notesTree.innerHTML = notesTree.innerHTML +
            '<li class="visualLi" onclick="showNoteOptionsByIdx(event,' + i.toString() + ');"><i class="fas fa-flag"></i>&nbsp;' +
            truncate(features[i].getProperties().notename,27) + '</li>';
    }
}
function showNoteOptionsByIdx(e, Idx) {
    if (e) { e.stopPropagation(); }
    if (actionRunning) { return; }
    document.getElementById('noteOptions').style.display = 'block';
    document.getElementById('noteOptions').style.left = 'calc(50vw - 451px / 2)';
    document.getElementById('noteOptions').style.top = 'calc(50vh - 236px / 2)';
    document.getElementById('noteOptions').style.width = '451px';
    document.getElementById('noteOptions').style.height = '236px';
    document.getElementById('noteOptionsFrame').contentWindow.userDisplay = true;
    document.getElementById('noteOptionsFrame').contentWindow.refreshNoteData('', Idx);
    setDialogZIndex(null, 'noteOptions');
}
myNotesLayer.getSource().on('addfeature', function (event) {
    if (event.feature.get('noteid') === undefined) {
        let newid = getNewIdx(myNotesLayer.getSource(), 'noteid');
        event.feature.setProperties({
            'noteid': newid,
            'notename': 'new note ' + newid,
            'notetype': 'Private',
            'description': 'new note'
        });
        loadNotesTreeFromSource();
        document.getElementById('noteOptionsFrame').contentWindow.refreshNoteData('add');
    }
});
function getNoteDataByIdx(Idx) {
    return getDataByIdx(myNotesLayer.getSource(), Idx);
}
function getNoteIdxById(noteid) {
    return getIdxById(myNotesLayer.getSource(), 'noteid', noteid);
}
function zoomToNoteByIdx(Idx) {
    zoomToFeatureByIdx(myNotesLayer.getSource(), Idx);
}
function updateNoteDataByIdx(Idx, notename, notetype, description) {
    if (Idx) {
        if (Number(Idx) > -1) {
            let feature = myNotesLayer.getSource().getFeatures()[Idx];
            feature.set('notename', notename.toString());
            feature.set('notetype', notetype.toString());
            feature.set('description', description.toString());
            myMap.render();
            loadNotesTreeFromSource();
        }
    }
}
function getNotesCount() {
    return getFeaturesCount(myNotesLayer.getSource());
}
function deleteNoteByIdx(Idx) {
    deleteFeatureByIdx(myNotesLayer.getSource(), Idx);
    loadNotesTreeFromSource();
    clearInteractions();
}
function downloadNotesCSV(e) {
    if (e) { e.stopPropagation(); }
    if (actionRunning) { return; }
    showProgress('Exporting CSV');
    setTimeout(function () {
        download.CSV(myNotesLayer.getSource(), 'mynotes');
        closeProgress();
    }, 500);
}
function downloadNotesJson(e) {
    if (e) { e.stopPropagation(); }
    if (actionRunning) { return; }
    showProgress('Exporting JSON');
    setTimeout(function () {
        download.JSON(myNotesLayer.getSource(), 'mynotes');
        closeProgress();
    }, 500);
}
function downloadNotesKML(e) {
    if (e) { e.stopPropagation(); }
    if (actionRunning) { return; }
    showProgress('Exporting KML');
    setTimeout(function () {
        download.KML(myNotesLayer.getSource(), 'mynotes');
        closeProgress();
    }, 500);
}
