/* global variables*/
var userDisplay = false;
var noteData;
var notesCount;
var currNoteIdx;
var deleteNoteIdx = -1;
/* setup listeners*/
$('#description').keypress(function (event) {
    if (event.keyCode === 13) {
        event.preventDefault();
    }
});
document.getElementById('defaultOpen').click();
initSelectOpt(document.getElementById('notetype'), '100%', 'public\tprivate', 'public');
document.getElementById('notename').addEventListener('change', noteNameChange);
document.getElementById('notetype').addEventListener('change', saveNoteData);
document.getElementById('description').addEventListener('change', descriptionChange);
function noteNameChange(e) {
    document.getElementById('notename').value = fixStringName(document.getElementById('notename').value);
    saveNoteData(e);
}
function descriptionChange(e) {
    document.getElementById('description').value = fixStringName(document.getElementById('description').value);
    saveNoteData(e);
}
/* noteoptions functions*/
function fillNoteData(noteData) {
    document.getElementById('noteid').value = noteData[0].toString();
    document.getElementById('notename').value = noteData[1] === undefined ? '' : noteData[1].toString();
    document.getElementById('notetype').setOpt(noteData[2] === undefined ? '' : noteData[2].toString());
    document.getElementById('description').value = noteData[3] === undefined ? '' : noteData[3].toString();
}
function refreshNoteData(refreshType, Idx = null) {
    let currNoteId = noteData === undefined ? '0' : noteData[0].toString();
    noteData = undefined; notesCount = '0';
    notesCount = window.parent.getNotesCount();
    if (Idx !== null) { currNoteIdx = Idx.toString(); }
    else {
        if (refreshType === 'start') {
            if (Number(notesCount) > 0) {
                currNoteIdx = window.parent.sNotesTree[0][0].toString();
            } else { currNoteIdx = '-1'; }
        }
        else if (refreshType === 'add') { /* add */
            if (Number(notesCount) > 0) {
                currNoteIdx = window.parent.sNotesTree[Number(notesCount) - 1][0].toString();
            } else { currNoteIdx = '-1'; }
        }
        else if (refreshType === 'cursor') { /* update */
            if (Number(notesCount) > 0) {
                currNoteIdx = window.parent.getNoteIdxById(currNoteId).toString();
            } else { currNoteIdx = '-1'; }
        }
        else if (refreshType === 'delete') /*delete*/ {
            if (Number(notesCount) > 0) {
                if (deleteNoteIdx > Number(notesCount) - 1) {
                    currNoteIdx = window.parent.sNotesTree[Number(notesCount) - 1][0].toString();
                } else {
                    if (deleteNoteIdx > -1) {
                        currNoteIdx = window.parent.sNotesTree[deleteNoteIdx][0].toString();
                    } else { currNoteIdx = '-1'; }
                }
            } else { currNoteIdx = '-1'; }
        }
    }
    noteData = window.parent.getNoteDataByIdx(currNoteIdx).split('\t');
    fillNoteData(noteData);
}
function refreshNoteDataById(noteid) {
    refreshNoteData('', window.parent.getNoteIdxById(noteid).toString());
}
function nextNote(e) {
    if (e) { e.stopPropagation(); }
    let sortedIdx = window.parent.sNotesTree.findIndex(x => x[0].toString() === currNoteIdx);
    if (sortedIdx < Number(notesCount) - 1) {
        refreshNoteData('', (window.parent.sNotesTree[sortedIdx + 1][0]).toString());
    }
}
function previousNote(e) {
    if (e) { e.stopPropagation(); }
    let sortedIdx = window.parent.sNotesTree.findIndex(x => x[0].toString() === currNoteIdx);
    if (sortedIdx > 0) {
        refreshNoteData('', (window.parent.sNotesTree[sortedIdx - 1][0]).toString());
    }
}
function saveNoteData(e) {
    if (e) { e.stopPropagation(); }
    let notename = document.getElementById('notename').value;
    let notetype = document.getElementById('notetype').value;
    let description = document.getElementById('description').value;
    window.parent.updateNoteDataByIdx(currNoteIdx, notename, notetype, description);
    refreshNoteData('cursor');
}
function deleteNote(e) {
    if (e) { e.stopPropagation(); }
    deleteNoteIdx = window.parent.sNotesTree.findIndex(x => x[0].toString() === currNoteIdx);
    window.parent.deleteNoteByIdx(currNoteIdx);
    refreshNoteData('delete');
}
function zoomToNote(e) {
    if (e) { e.stopPropagation(); }
    window.parent.document.getElementById('noteOptions').style.display = 'none';
    window.parent.zoomToNoteByIdx(currNoteIdx);
}