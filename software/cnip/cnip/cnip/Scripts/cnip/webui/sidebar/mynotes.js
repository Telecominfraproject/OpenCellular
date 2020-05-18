var sNotesTree = [];

$('#notesDiv').collapse({
    toggle: true
});
var notesTree = document.getElementById('notesUl');
initSingleSlider(document.getElementById('notesOpacity'), '', '0', '1', '0.1', '1');
document.getElementById('notesOpacity').addEventListener('change', setNotesStyle);
loadNotesTreeFromFile();
/* note functions*/
function setNotesLabel(e) {
    setNotesStyle(e);
}
function setNotesStyle(e) {
    if (e) { e.stopPropagation(); }
    let custStyle = function (feature) {
        let noteOpacity = Number($('#notesOpacity').val());
        return [new ol.style.Style({
            image: feature.get('notetype') === 'public' ? new ol.style.Icon({
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
                text: document.getElementById("notesLabel").checked ? feature.get('notename') : '',
                font: '14px "Liberation Sans", "Lucida Sans", "Lucida Sans Regular", "Lucida Grande", "Lucida Sans Unicode", Geneva, Verdana, sans-serif',
                fill: new ol.style.Fill({ color: 'rgba(0,0,0,' + noteOpacity + ')' }),
                stroke: new ol.style.Stroke({ color: 'rgba(255,255,255,' + noteOpacity + ')', width: 2 }),
                overflow: false
            })
        })];
    };
    myNotesLayer.setStyle(custStyle);
    myMap.render();
}
function setNotesVisiblity() {
    if (myNotesLayer) {
        if (document.getElementById("notesVisibility").checked === true) {
            myNotesLayer.setVisible(true);
        } else {
            myNotesLayer.setVisible(false);
        }
        myMap.render();
        ol.control.LayerSwitcher.renderPanel(myMap, toc);
    }
}
function setPublicNotesVisiblity() {
    if (publicNotesLayer) {
        if (document.getElementById("publicNotesVisibility").checked === true) {
            publicNotesLayer.setVisible(true);
        } else {
            publicNotesLayer.setVisible(false);
        }
        myMap.render();
        ol.control.LayerSwitcher.renderPanel(myMap, toc);
    }
}
function loadNotesTreeFromFile() {
    let httpRequest = new XMLHttpRequest();
    httpRequest.open('GET', '../' + vectorsPath + 'mynotes.geojson');
    httpRequest.onload = function () {
        let source = JSON.parse(httpRequest.responseText);
        function processLoop() {
            if (source.features.length === myNotesLayer.getSource().getFeatures().length) {
                loadNotesTreeFromSource();
            } else {
                setTimeout(processLoop, 500);
            }
        }
        processLoop();
    };
    httpRequest.send();
}
function loadNotesTreeFromSource() {
    sNotesTree = buildSortedTreeFromSource(myNotesLayer, 'noteid', 'notename');
    notesTree.innerHTML = '';
    for (let i = 0; i < sNotesTree.length; i++) {
        notesTree.innerHTML = notesTree.innerHTML +
            '<li class="visualLi" onclick="showNoteOptionsByIdx(event,' +
            sNotesTree[i][0].toString() + ');"><i class="fas fa-flag"></i>&nbsp;' +
            truncate(sNotesTree[i][2].toString(), 27) + '</li>';
    }
}
function showNoteOptionsByIdx(e, Idx) {
    if (e) { e.stopPropagation(); }
    if (actionRunning) { return; }
    document.getElementById('noteOptions').style.display = 'block';
    //document.getElementById('noteOptions').style.left = 'calc(50vw - 451px / 2)';
    //document.getElementById('noteOptions').style.top = 'calc(50vh - 236px / 2)';
    //document.getElementById('noteOptions').style.width = '451px';
    //document.getElementById('noteOptions').style.height = '236px';
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
            'notetype': 'private',
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
    return getIdxByProperty(myNotesLayer.getSource(), 'noteid', noteid);
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
