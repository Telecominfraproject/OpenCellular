/* global variables*/
var userDisplay = false;
var siteData;
var sitesCount;
var currSite;
/* initialize controls*/
document.getElementById('defaultOpen').click();
initSelectOpt(document.getElementById('antennamodel'), '100%', 'LT OD9-5 890-950 MHz\tLT OD9-6 865-945 MHz\tLT OD9-6 860-960 MHz\tLT OD9-8 865-945 MHz\tLT OD9-8 860-960 MHz\tLT OD9-11 860-960 MHz\tLT OD9-11D1 860-960 MHz', 'LT OD9-5 890-950 MHz');
/* setup listeners*/
document.getElementById('sitename').addEventListener('change', siteNameChange);
document.getElementById('longitude').addEventListener('change', longitudeChange);
document.getElementById('latitude').addEventListener('change', latitudeChange);
document.getElementById('height').addEventListener('change', heightChange);
document.getElementById('region').addEventListener('change', propertyChange);
document.getElementById('country').addEventListener('change', propertyChange);
document.getElementById('city').addEventListener('change', propertyChange);
document.getElementById('district').addEventListener('change', propertyChange);
document.getElementById('province').addEventListener('change', propertyChange);
document.getElementById('address').addEventListener('change', propertyChange);
document.getElementById('comments').addEventListener('change', propertyChange);
document.getElementById('rfcn').addEventListener('change', rfcnChange);
document.getElementById('rfid').addEventListener('change', rfidChange);
document.getElementById('rfpower').addEventListener('change', rfPowerChange);
document.getElementById('hba').addEventListener('change', hbaChange);
document.getElementById('antennamodel').addEventListener('change', antennaModelChange);
document.getElementById('azimuth').addEventListener('change', azimuthChange);
document.getElementById('feederloss').addEventListener('change', feederLossChange);
/* siteoptions functions*/
function siteNameChange(e) {
    document.getElementById('sitename').value = fixStringName(document.getElementById('sitename').value);
    saveSiteData(e);
}
function propertyChange(e) {
    document.getElementById('region').value = fixStringName(document.getElementById('region').value);
    document.getElementById('country').value = fixStringName(document.getElementById('country').value);
    document.getElementById('city').value = fixStringName(document.getElementById('city').value);
    document.getElementById('district').value = fixStringName(document.getElementById('district').value);
    document.getElementById('province').value = fixStringName(document.getElementById('province').value);
    document.getElementById('address').value = fixStringName(document.getElementById('address').value);
    document.getElementById('comments').value = fixStringName(document.getElementById('comments').value);
    saveSiteData(e);
}
function longitudeChange(e) {
    document.getElementById('longitude').value = Number(document.getElementById('longitude').value);
    if (document.getElementById('longitude').value.toString() !== siteData[2].toString()) {
        window.parent.deleteLinksBySiteId(siteData[0].toString());
    }
    saveSiteData(e);
}
function latitudeChange(e) {
    document.getElementById('latitude').value = Number(document.getElementById('latitude').value);
    if (document.getElementById('latitude').value.toString() !== siteData[3].toString()) {
        window.parent.deleteLinksBySiteId(siteData[0].toString());
    }
    saveSiteData(e);
}
function feederLossChange(e) {
    document.getElementById('feederloss').value = Number(document.getElementById('feederloss').value);
    if (Number(document.getElementById('feederloss').value) > 50) {
        document.getElementById('feederloss').value = 50;
    } else if (Number(document.getElementById('feederloss').value) < 0) {
        document.getElementById('feederloss').value = 0;
    }
    saveSiteData(e);
}
function azimuthChange(e) {
    document.getElementById('azimuth').value = Number(document.getElementById('azimuth').value);
    if (Number(document.getElementById('azimuth').value) > 359) {
        document.getElementById('azimuth').value = 359;
    } else if (Number(document.getElementById('azimuth').value) < 0) {
        document.getElementById('azimuth').value = 0;
    }
    saveSiteData(e);
}
function rfidChange(e) {
    document.getElementById('rfid').value = Number(document.getElementById('rfid').value);
    switch (document.getElementById('technology').value) {
        case '2G':
            if (Number(document.getElementById('rfid').value) > 77) {
                document.getElementById('rfid').value = 77;
            } else if (Number(document.getElementById('rfid').value) < 0) {
                document.getElementById('rfid').value = 0;
            }
            break;
        case '4G':
            if (Number(document.getElementById('rfid').value) > 503) {
                document.getElementById('rfid').value = 503;
            } else if (Number(document.getElementById('rfid').value) < 0) {
                document.getElementById('rfid').value = 0;
            }
            break;
        default: break;
    }
    saveSiteData(e);
}
function rfcnChange(e) {
    document.getElementById('rfcn').value = Number(document.getElementById('rfcn').value);
    switch (document.getElementById('technology').value) {
        case '2G':
            if (Number(document.getElementById('rfcn').value) > 124) {
                document.getElementById('rfcn').value = 124;
            } else if (Number(document.getElementById('rfcn').value) < 1) {
                document.getElementById('rfcn').value = 1;
            }
            document.getElementById('dlfrequency').value = window.parent.getGSMDlfrequency(document.getElementById('rfcn').value);
            document.getElementById('ulfrequency').value = window.parent.getGSMUlfrequency(document.getElementById('rfcn').value);
            break;
        case '4G':
            if (Number(document.getElementById('rfcn').value) > 3799) {
                document.getElementById('rfcn').value = 3799;
            } else if (Number(document.getElementById('rfcn').value) < 3450) {
                document.getElementById('rfcn').value = 3450;
            }
            document.getElementById('dlfrequency').value = window.parent.getLTEDlfrequency(document.getElementById('rfcn').value);
            document.getElementById('ulfrequency').value = window.parent.getLTEUlfrequency(document.getElementById('rfcn').value);
            break;
        default: break;
    }
    saveSiteData(e);
}
function rfPowerChange(e) {
    document.getElementById('rfpower').value = Number(document.getElementById('rfpower').value);
    if (Number(document.getElementById('rfpower').value) > 40) {
        document.getElementById('rfpower').value = 40;
    } else if (Number(document.getElementById('rfpower').value) < 1) {
        document.getElementById('rfpower').value = 1;
    }
    saveSiteData(e);
}
function hbaChange(e) {
    document.getElementById('hba').value = Number(document.getElementById('hba').value);
    if (Number(document.getElementById('hba').value) > Number(document.getElementById('height').value)) {
        document.getElementById('height').value = document.getElementById('hba').value;
    }
    saveSiteData(e);
}
function heightChange(e) {
    document.getElementById('height').value = Number(document.getElementById('height').value);
    if (Number(document.getElementById('height').value) < Number(document.getElementById('hba').value)) {
        document.getElementById('hba').value = document.getElementById('height').value;
    }
    saveSiteData(e);
}
function antennaModelChange(e) {
    let antennaOptions = window.parent.getAntennaOptions(document.getElementById('antennamodel').value);
    document.getElementById('antennatype').value = antennaOptions.antennaType;
    document.getElementById('polarization').value = antennaOptions.polarization;
    document.getElementById('vbeamwidth').value = antennaOptions.vBeamWidth;
    document.getElementById('hbeamwidth').value = antennaOptions.hBeamWidth;
    document.getElementById('downtilt').value = antennaOptions.downTilt;
    document.getElementById('antennagain').value = antennaOptions.antennaGain;
    document.getElementById('azimuth').value = '0';
    if (document.getElementById('antennatype').value === 'Omni') {
        document.getElementById('azimuth').readonly = true;
        document.getElementById('azimuth').disabled = true;
    } else {
        document.getElementById('azimuth').readonly = false;
        document.getElementById('azimuth').disabled = false;
    }
    saveSiteData(e);
}
function fillSiteData(siteData) {
    // site
    document.getElementById('siteid').value = siteData[0] === undefined ? '' : siteData[0].toString();
    document.getElementById('sitename').value = siteData[1] === undefined ? '' : siteData[1].toString();
    document.getElementById('sitename1').value = siteData[1] === undefined ? '' : siteData[1].toString();
    document.getElementById('longitude').value = siteData[2] === undefined ? '' : siteData[2].toString();
    document.getElementById('latitude').value = siteData[3] === undefined ? '' : siteData[3].toString();
    // tower
    document.getElementById('height').value = siteData[4] === undefined ? '' : siteData[4].toString();
    // property
    document.getElementById('region').value = siteData[5] === undefined ? '' : siteData[5].toString();
    document.getElementById('country').value = siteData[6] === undefined ? '' : siteData[6].toString();
    document.getElementById('city').value = siteData[7] === undefined ? '' : siteData[7].toString();
    document.getElementById('district').value = siteData[8] === undefined ? '' : siteData[8].toString();
    document.getElementById('province').value = siteData[9] === undefined ? '' : siteData[9].toString();
    document.getElementById('address').value = siteData[10] === undefined ? '' : siteData[10].toString();
    document.getElementById('comments').value = siteData[11] === undefined ? '' : siteData[11].toString();
    // radio
    document.getElementById('technology').value = siteData[12] === undefined ? '' : siteData[12].toString();
    document.getElementById('technology1').value = siteData[12] === undefined ? '' : siteData[12].toString();
    document.getElementById('band').value = siteData[13] === undefined ? '' : siteData[13].toString();
    document.getElementById('bandwidth').value = siteData[14] === undefined ? '' : siteData[14].toString();
    document.getElementById('cellid').value = siteData[15] === undefined ? '' : siteData[15].toString();
    document.getElementById('lac').value = siteData[16] === undefined ? '' : siteData[16].toString();
    document.getElementById('rfcn').value = siteData[17] === undefined ? '' : siteData[17].toString();
    document.getElementById('rfid').value = siteData[18] === undefined ? '' : siteData[18].toString();
    document.getElementById('dlfrequency').value = siteData[19] === undefined ? '' : siteData[19].toString();
    document.getElementById('ulfrequency').value = siteData[20] === undefined ? '' : siteData[20].toString();
    document.getElementById('rfpower').value = siteData[21] === undefined ? '' : siteData[21].toString();
    // antenna
    document.getElementById('hba').value = siteData[22] === undefined ? '' : siteData[22].toString();
    document.getElementById('azimuth').value = siteData[23] === undefined ? '' : siteData[23].toString();
    document.getElementById('antennamodel').setOpt(siteData[24] === undefined ? '' : siteData[24].toString());
    document.getElementById('antennatype').value = siteData[25] === undefined ? '' : siteData[25].toString();
    document.getElementById('polarization').value = siteData[26] === undefined ? '' : siteData[26].toString();
    document.getElementById('vbeamwidth').value = siteData[27] === undefined ? '' : siteData[27].toString();
    document.getElementById('hbeamwidth').value = siteData[28] === undefined ? '' : siteData[28].toString();
    document.getElementById('downtilt').value = siteData[29] === undefined ? '' : siteData[29].toString();
    document.getElementById('antennagain').value = siteData[30] === undefined ? '' : siteData[30].toString();
    document.getElementById('feederloss').value = siteData[31] === undefined ? '' : siteData[31].toString();
    if (document.getElementById('antennatype').value === 'Omni') {
        document.getElementById('azimuth').readonly = true;
        document.getElementById('azimuth').disabled = true;
    } else {
        document.getElementById('azimuth').readonly = false;
        document.getElementById('azimuth').disabled = false;
    }
}
function refreshSiteData(refreshType, Idx = null) {
    let currSiteId = siteData === undefined ? 0 : siteData[0].toString();
    siteData = undefined; sitesCount = '0';
    sitesCount = window.parent.getSitesCount();
    if (Idx !== null) { currSite = Idx.toString(); }
    else {
        if (refreshType === 'start') { currSite = '0'; }
        else if (refreshType === 'add') { /* update */
            currSite = (Number(sitesCount) - 1).toString();
        }
        else if (refreshType === 'cursor') { /* update */
            currSite = window.parent.getSiteIdxById(currSiteId).toString();
        }
        else if (refreshType === 'delete') /*delete*/ {
            if (Number(currSite) > Number(sitesCount) - 1) {
                currSite = (Number(sitesCount) - 1).toString();
            }
        }
    }
    siteData = window.parent.getSiteDataByIdx(currSite).split('\t');
    fillSiteData(siteData);
}
function refreshSiteDataById(siteid) {
    refreshSiteData('', window.parent.getSiteIdxById(siteid).toString());
}
function nextSite(e) {
    if (e) { e.stopPropagation(); }
    if (Number(sitesCount) - 1 > Number(currSite)) {
        refreshSiteData('', (Number(currSite) + 1).toString());
    }
}
function previousSite(e) {
    if (e) { e.stopPropagation(); }
    if (Number(currSite) > 0) {
        refreshSiteData('', (Number(currSite) - 1).toString());
    }
}
function saveSiteData(e) {
    if (e) { e.stopPropagation(); }
    window.parent.updateSiteDataByIdx({
        //site
        idx: currSite,
        sitename: document.getElementById('sitename').value,
        longitude: document.getElementById('longitude').value,
        latitude: document.getElementById('latitude').value,
        // tower
        height: document.getElementById('height').value,
        // property
        region: document.getElementById('region').value,
        country: document.getElementById('country').value,
        city: document.getElementById('city').value,
        district: document.getElementById('district').value,
        province: document.getElementById('province').value,
        address: document.getElementById('address').value,
        comments: document.getElementById('comments').value,
        //radio
        technology: document.getElementById('technology').value,
        band: document.getElementById('band').value,
        bandwidth: document.getElementById('bandwidth').value,
        cellid: document.getElementById('cellid').value,
        lac: document.getElementById('lac').value,
        rfcn: document.getElementById('rfcn').value,
        rfid: document.getElementById('rfid').value,
        dlfrequency: document.getElementById('dlfrequency').value,
        ulfrequency: document.getElementById('ulfrequency').value,
        rfpower: document.getElementById('rfpower').value,
        //antenna
        hba: document.getElementById('hba').value,
        azimuth: document.getElementById('azimuth').value,
        antennamodel: document.getElementById('antennamodel').value,
        antennatype: document.getElementById('antennatype').value,
        polarization: document.getElementById('polarization').value,
        vbeamwidth: document.getElementById('vbeamwidth').value,
        hbeamwidth: document.getElementById('hbeamwidth').value,
        downtilt: document.getElementById('downtilt').value,
        antennagain: document.getElementById('antennagain').value,
        feederloss: document.getElementById('feederloss').value
    });
    refreshSiteData('cursor');
    window.parent.document.getElementById('linkOptionsFrame').contentWindow.refreshLinkData('cursor');
    window.parent.document.getElementById('linkProfileFrame').contentWindow.refreshLinkData('cursor');
}
function deleteSite(e) {
    if (e) { e.stopPropagation(); }
    window.parent.deleteSiteByIdx(currSite);
    refreshSiteData('delete');
    window.parent.document.getElementById('linkOptionsFrame').contentWindow.refreshLinkData('delete');
    window.parent.document.getElementById('linkProfileFrame').contentWindow.refreshLinkData('delete');
}
function zoomToSite(e) {
    if (e) { e.stopPropagation(); }
    window.parent.document.getElementById('siteOptions').style.display = 'none';
    window.parent.zoomToSiteByIdx(currSite);
}