/* global variables*/
var userDisplay = false;
var linkData;
var siteAData;
var siteBData;
var linksCount;
var currLink;
/* initialize controls*/
initSingleSlider(document.getElementById('frequencya'), '', '1', '15', '0.1', '5');
initSelectOpt(document.getElementById('channelwidtha'), '100%', '10\t20\t30\t40\t50\t60\t80\t100', '20');
initSingleSlider(document.getElementById('outputpowera'), '', '27', '47', '0.5', '30');
initSingleSlider(document.getElementById('antennagaina'), '', '19', '51', '0.1', '34');
initSingleSlider(document.getElementById('lossesa'), '', '0', '51', '0.1', '0');
initSingleSlider(document.getElementById('frequencyb'), '', '1', '15', '0.1', '5');
initSelectOpt(document.getElementById('channelwidthb'), '100%', '10\t20\t30\t40\t50\t60\t80\t100', '20');
initSingleSlider(document.getElementById('outputpowerb'), '', '27', '47', '0.5', '30');
initSingleSlider(document.getElementById('antennagainb'), '', '19', '51', '0.1', '34');
initSingleSlider(document.getElementById('lossesb'), '', '0', '51', '0.1', '0');
/* setup listeners*/
document.getElementById('defaultOpen').click();
document.getElementById('linkname').addEventListener('change', linkNameChange);
document.getElementById('frequencya').addEventListener('change', saveLinkData);
document.getElementById('channelwidtha').addEventListener('change', saveLinkData);
document.getElementById('outputpowera').addEventListener('change', saveLinkData);
document.getElementById('antennagaina').addEventListener('change', saveLinkData);
document.getElementById('lossesa').addEventListener('change', saveLinkData);
document.getElementById('frequencyb').addEventListener('change', saveLinkData);
document.getElementById('channelwidthb').addEventListener('change', saveLinkData);
document.getElementById('outputpowerb').addEventListener('change', saveLinkData);
document.getElementById('antennagainb').addEventListener('change', saveLinkData);
document.getElementById('lossesb').addEventListener('change', saveLinkData);
/* linkoptions functions*/
function linkNameChange(e) {
    document.getElementById('linkname').value = fixStringName(document.getElementById('linkname').value);
    saveLinkData(e);
}
function fillLinkData(linkData) {
    document.getElementById('linkid').value = linkData[0].toString();
    document.getElementById('linkname').value = linkData[1] === undefined ? '' : linkData[1].toString();
    document.getElementById('linkname1').value = linkData[1] === undefined ? '' : linkData[1].toString();
    document.getElementById('linktype').value = linkData[2] === undefined ? '' : linkData[2].toString();
    let linktype = linkData[2] === undefined ? '' : linkData[2].toString();
    if (linktype === 'public') {
        document.getElementById('sitebtd1').innerText = 'Site B (Public)';
        document.getElementById('sitebtd2').innerText = 'Site B (Public)';
        document.getElementById('user').hidden = false;
        document.getElementById('userhr').hidden = false;
        document.getElementById('name').hidden = false;
        document.getElementById('email').hidden = false;
        document.getElementById('spanname').hidden = false;
        document.getElementById('spanemail').hidden = false;
    } else {
        document.getElementById('sitebtd1').innerText = 'Site B';
        document.getElementById('sitebtd2').innerText = 'Site B';
        document.getElementById('user').hidden = true;
        document.getElementById('userhr').hidden = true;
        document.getElementById('name').hidden = true;
        document.getElementById('email').hidden = true;
        document.getElementById('spanname').hidden = true;
        document.getElementById('spanemail').hidden = true;
    }
    document.getElementById('siteida').value = linkData[3] === undefined ? '' : linkData[3].toString();
    document.getElementById('sitenamea').value = siteAData[1] === undefined ? '' : siteAData[1].toString();
    document.getElementById('heighta').value = siteAData[4] === undefined ? '' : siteAData[4].toString();
    document.getElementById('locheighta').value = linkData[4] === undefined ? '' : linkData[4].toString();
    document.getElementById('bearinga').value = linkData[5] === undefined ? '' : linkData[5].toString();
    document.getElementById('channelwidtha').setOpt(linkData[6] === undefined ? '' : linkData[6].toString());
    document.getElementById('frequencya').setValue(linkData[7] === undefined ? '' : linkData[7].toString());
    document.getElementById('outputpowera').setValue(linkData[8] === undefined ? '' : linkData[8].toString());
    document.getElementById('antennagaina').setValue(linkData[9] === undefined ? '' : linkData[9].toString());
    document.getElementById('lossesa').setValue(linkData[10] === undefined ? '' : linkData[10].toString());
    document.getElementById('siteidb').value = linkData[11] === undefined ? '' : linkData[11].toString();
    document.getElementById('sitenameb').value = siteBData[1] === undefined ? '' : siteBData[1].toString();
    document.getElementById('heightb').value = siteBData[4] === undefined ? '' : siteBData[4].toString();
    document.getElementById('locheightb').value = linkData[12] === undefined ? '' : linkData[12].toString();
    document.getElementById('bearingb').value = linkData[13] === undefined ? '' : linkData[13].toString();
    document.getElementById('channelwidthb').setOpt(linkData[14] === undefined ? '' : linkData[14].toString());
    document.getElementById('frequencyb').setValue(linkData[15] === undefined ? '' : linkData[15].toString());
    document.getElementById('outputpowerb').setValue(linkData[16] === undefined ? '' : linkData[16].toString());
    document.getElementById('antennagainb').setValue(linkData[17] === undefined ? '' : linkData[17].toString());
    document.getElementById('lossesb').setValue(linkData[18] === undefined ? '' : linkData[18].toString());
    document.getElementById('distance').value = linkData[19] === undefined ? '' : linkData[19].toString();
    document.getElementById('name').value = linkData[20] === undefined ? '' : linkData[20].toString();
    document.getElementById('email').value = linkData[21] === undefined ? '' : linkData[21].toString();
}
function refreshLinkData(refreshType, Idx = null) {
    let currLinkId = linkData === undefined ? 0 : linkData[0].toString();
    linkData = undefined; linksCount = '0';
    linksCount = window.parent.getLinksCount();
    if (Idx !== null) { currLink = Idx.toString(); } else {
        if (refreshType === 'start') { currLink = '0'; }
        else if (refreshType === 'add') {
            currLink = (Number(linksCount) - 1).toString();
        }
        else if (refreshType === 'cursor') {
            currLink = window.parent.getLinkIdxById(currLinkId).toString();
        }
        else if (refreshType === 'delete') /*delete*/ {
            if (Number(currLink) > Number(linksCount) - 1) {
                currLink = (Number(linksCount) - 1).toString();
            }
        }
    }
    linkData = window.parent.getLinkDataByIdx(currLink).split('\t');
    let siteida = linkData[3] === undefined ? -1 : linkData[3].toString();
    let siteidb = linkData[11] === undefined ? -1 : linkData[11].toString();
    let linktype = linkData[2] === undefined ? '' : linkData[2].toString();
    let email = linkData[21] === undefined ? '' : linkData[21].toString();
    siteAData = window.parent.getSiteDataById(siteida).split('\t');
    if (linktype === 'public') {
        siteBData = window.parent.getPublicSiteData(siteidb, email).split('\t');
    } else {
        siteBData = window.parent.getSiteDataById(siteidb).split('\t');
    }
    fillLinkData(linkData);
}
function refreshLinkDataById(linkid) {
    refreshLinkData('', window.parent.getLinkIdxById(linkid).toString());
}
function nextLink(e) {
    if (e) { e.stopPropagation(); }
    if (Number(linksCount) - 1 > Number(currLink)) {
        refreshLinkData('', (Number(currLink) + 1).toString());
    }
}
function previousLink(e) {
    if (e) { e.stopPropagation(); }
    if (Number(currLink) > 0) {
        refreshLinkData('', (Number(currLink) - 1).toString());
    }
}
function saveLinkData(e) {
    if (e) { e.stopPropagation(); }
    window.parent.updateLinkDataByIdx({
        idx: currLink,
        linkname: document.getElementById('linkname').value,
        channelwidtha: document.getElementById('channelwidtha').value,
        frequencya: document.getElementById('frequencya').value,
        outputpowera: document.getElementById('outputpowera').value,
        antennagaina: document.getElementById('antennagaina').value,
        lossesa: document.getElementById('lossesa').value,
        channelwidthb: document.getElementById('channelwidthb').value,
        frequencyb: document.getElementById('frequencyb').value,
        outputpowerb: document.getElementById('outputpowerb').value,
        antennagainb: document.getElementById('antennagainb').value,
        lossesb: document.getElementById('lossesb').value
    });
    window.parent.document.getElementById('linkProfileFrame').contentWindow.refreshLinkData('cursor');
    refreshLinkData('cursor');
}
function deleteLink(e) {
    if (e) { e.stopPropagation(); }
    window.parent.deleteLinkByIdx(currLink);
    window.parent.document.getElementById('linkProfileFrame').contentWindow.refreshLinkData('delete');
    refreshLinkData('delete');
}
function zoomToLink(e) {
    if (e) { e.stopPropagation(); }
    window.parent.document.getElementById('linkOptions').style.display = 'none';
    window.parent.zoomToLinkByIdx(currLink);
}