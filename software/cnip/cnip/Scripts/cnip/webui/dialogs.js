function _minimizeDialog(e) {
    e.style.left = 'calc(80vw)';
    e.style.top = 'calc(80vh)';
    e.style.width = '220px';
    e.style.height = '37px';
    e.style.overflow = 'hidden';
}
function minimizeDialog(e = null, dialogId) {
    if (e) { e.stopPropagation(); }
    let dds = document.getElementById(dialogId);
    _minimizeDialog(dds);
}
function restoreDialog(e = null, dialogId, width = '651px', height = '432px') {
    if (e) { e.stopPropagation(); }
    let dds = document.getElementById(dialogId);
    dds.style.left = 'calc(50vw - (' + width + ' / 2))';
    dds.style.top = 'calc(50vh - (' + height + ' / 2))';
    dds.style.width = width;
    dds.style.height = height;
    dds.style.overflow = 'auto';
}
function hideAllDialogs() {
    closePredictBestCan();
    closePredictSites();
    closePredict();
    [...document.getElementsByClassName('draggable')].forEach(function (e) {
        if (e.id !== 'progressBar' && e.id !== 'messageBoxBar') {
            e.style.display = 'none';
        }
    });
}
function unHideAllDialogs() {
    [...document.getElementsByClassName('draggable')].forEach(function (e) {
        if (e.id !== 'progressBar' && e.id !== 'messageBoxBar') {
            switch (e.id) {
                case 'siteOptions':
                    if (document.getElementById('siteOptionsFrame').contentWindow.userDisplay) { e.style.display = 'block'; } break;
                case 'polyOptions':
                    if (document.getElementById('polyOptionsFrame').contentWindow.userDisplay) { e.style.display = 'block'; } break;
                case 'noteOptions':
                    if (document.getElementById('noteOptionsFrame').contentWindow.userDisplay) { e.style.display = 'block'; } break;
                case 'linkOptions':
                    if (document.getElementById('linkOptionsFrame').contentWindow.userDisplay) { e.style.display = 'block'; } break;
                case 'linkProfile':
                    if (document.getElementById('linkProfileFrame').contentWindow.userDisplay) { e.style.display = 'block'; } break;
                default:
                    e.style.display = 'block'; break;
            }
        }
    });
}
function minimizeAllDialogs() {
    closePredictBestCan();
    closePredictSites();
    closePredict();
    [...document.getElementsByClassName('draggable')].forEach(function (e) {
        if (e.id !== 'progressBar' && e.id !== 'messageBoxBar') {
            switch (e.id) {
                case 'siteOptions':
                    if (document.getElementById('siteOptionsFrame').contentWindow.userDisplay) { _minimizeDialog(e); } break;
                case 'polyOptions':
                    if (document.getElementById('polyOptionsFrame').contentWindow.userDisplay) { _minimizeDialog(e); } break;
                case 'noteOptions':
                    if (document.getElementById('noteOptionsFrame').contentWindow.userDisplay) { _minimizeDialog(e); } break;
                case 'linkOptions':
                    if (document.getElementById('linkOptionsFrame').contentWindow.userDisplay) { _minimizeDialog(e); } break;
                case 'linkProfile':
                    if (document.getElementById('linkProfileFrame').contentWindow.userDisplay) { _minimizeDialog(e); } break;
                default:
                    _minimizeDialog(e); break;
            }
        }
    });
}
function closeAllDialogs() {
    document.getElementById('siteOptions').style.display = 'none';
    document.getElementById('polyOptions').style.display = 'none';
    document.getElementById('noteOptions').style.display = 'none';
    document.getElementById('linkOptions').style.display = 'none';
    document.getElementById('linkProfile').style.display = 'none';
    closePredictBestCan();
    closePredictSites();
    closePredict();
    ddsDestroyAll();
}
function setDialogZIndex(e = null, dialogId) {
    if (document.getElementById(dialogId)) {
        if (document.getElementById(dialogId).style.zIndex !== '199') {
            if (e) { e.stopPropagation(); }
            [...document.getElementsByClassName('draggable')].forEach(function (e) { e.style.zIndex = '99'; });
            document.getElementById(dialogId).style.zIndex = 199;
        }
    }
}
function showProgress(msg) {
    actionRunning = true;
    document.getElementById('progressBar').style.display = 'block';
    setDialogZIndex(null, 'progressBar');
    document.getElementById('progressTitle').innerHTML = '&nbsp;&nbsp;&nbsp;' + msg + '...';
    document.body.style.cursor = 'wait';
}
function closeProgress() {
    sleep(1000);
    document.getElementById('progressBar').style.display = 'none';
    document.getElementById('progressBar').style.zIndex = 99;
    document.body.style.cursor = 'default';
    actionRunning = false;
}
// message box setup
document.getElementById("yesMessageBox").addEventListener("click", function () {
    document.querySelector(".page-enabler").classList.add("hidden-content");
    document.querySelector(".page-disabler").classList.add("hidden-content");
    document.getElementById("main").removeEventListener("focus", function (e) { e.preventDefault(); });
    document.getElementById('messageBoxBar').style.zIndex = 99;
});
document.getElementById("okcancelMessageBox").addEventListener("click", function () {
    document.querySelector(".page-enabler").classList.add("hidden-content");
    document.querySelector(".page-disabler").classList.add("hidden-content");
    document.getElementById("main").removeEventListener("focus", function (e) { e.preventDefault(); });
    document.getElementById('messageBoxBar').style.zIndex = 99;
});
function showMessage(title, msg, request=false) {
    setDialogZIndex(null, 'messageBoxBar');
    if (request) {
        document.getElementById("yesMessageBox").style.display = 'block';
        document.getElementById("okcancelMessageBox").innerHTML = 'No';
    } else {
        document.getElementById("yesMessageBox").style.display = 'none';
        document.getElementById("okcancelMessageBox").innerHTML = 'Ok';
    }
    document.getElementById('messageBoxBar').style.left = 'calc(50vw - 403px / 2)';
    document.getElementById('messageBoxBar').style.top = 'calc(50vh - 203px / 2)';
    document.getElementById('messageBoxBar').style.width = '403px';
    document.getElementById('messageBoxBar').style.height = '203px';
    document.getElementById('messageBoxTitle').innerHTML = '&nbsp;&nbsp;&nbsp;' + title + '...';
    document.getElementById('messageBoxContent').innerHTML = '<br />&nbsp;&nbsp;&nbsp;' + msg;
    document.querySelector(".page-enabler").classList.remove("hidden-content");
    document.querySelector(".page-disabler").classList.remove("hidden-content");
    document.getElementById("main").addEventListener("focus", function (e) { e.preventDefault(); });
}