/* global variables*/
var userDisplay = false;
var noteData;
var notesCount;
var currNote;
/* setup listeners*/
$('#description').keypress(function (event) {
    if (event.keyCode === 13) {
        event.preventDefault();
    }
});
document.getElementById('defaultOpen').click();
initSelectOpt(document.getElementById('notetype'), '100%', 'Public\tPrivate', 'Public');
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
    let currNoteId = noteData === undefined ? 0 : noteData[0].toString();
    noteData = undefined; notesCount = '0';
    notesCount = window.parent.getNotesCount();
    if (Idx !== null) { currNote = Idx.toString(); }
    else {
        if (refreshType === 'start') { currNote = '0'; }
        else if (refreshType === 'add') {
            currNote = (Number(notesCount) - 1).toString();
        }
        else if (refreshType === 'cursor') {
            currNote = window.parent.getNoteIdxById(currNoteId).toString();
        }
        else if (refreshType === 'delete') /*delete*/ {
            if (Number(currNote) > Number(notesCount) - 1) {
                currNote = (Number(notesCount) - 1).toString();
            }
        }
    }
    noteData = window.parent.getNoteDataByIdx(currNote).split('\t');
    fillNoteData(noteData);
}
function refreshNoteDataById(noteid) {
    refreshNoteData('', window.parent.getNoteIdxById(noteid).toString());
}
function nextNote(e) {
    if (e) { e.stopPropagation(); }
    if (Number(notesCount) - 1 > Number(currNote)) {
        refreshNoteData('', (Number(currNote) + 1).toString());
    }
}
function previousNote(e) {
    if (e) { e.stopPropagation(); }
    if (Number(currNote) > 0) {
        refreshNoteData('', (Number(currNote) - 1).toString());
    }
}
function saveNoteData(e) {
    if (e) { e.stopPropagation(); }
    let notename = document.getElementById('notename').value;
    let notetype = document.getElementById('notetype').value;
    let description = document.getElementById('description').value;
    window.parent.updateNoteDataByIdx(currNote, notename, notetype, description);
    refreshNoteData('cursor');
}
function deleteNote(e) {
    if (e) { e.stopPropagation(); }
    window.parent.deleteNoteByIdx(currNote);
    refreshNoteData('delete');
}
function zoomToNote(e) {
    if (e) { e.stopPropagation(); }
    window.parent.document.getElementById('noteOptions').style.display = 'none';
    window.parent.zoomToNoteByIdx(currNote);
}