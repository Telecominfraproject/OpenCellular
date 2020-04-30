$('#analysisDiv').collapse({
    toggle: true
});
$('#predictDiv').collapse({
    toggle: true
});
$('#reportDiv').collapse({
    toggle: true
});
function analyseBestCan(sites, polygonid) {
    if (getActionRunning()) { return; }
    _saveSettings();
    saveAllVectors();
    minimizeAllDialogs();
    showProgress('Analysing Candidates');
    setTimeout(function () {
        let newid = Ajax_runccmd("GetNewResultId", "");
        Ajax_runccmd("BestCandidate",
            "{" +
            "'resultid':'" + newid + "'," +
            "'sites':'" + sites + "'," +
            "'polygonid':'" + polygonid + "'" +
            "}");
        function processLoop() {
            let chk = getForecastRunning();
            if (chk === false) {
                showResult(Ajax_runccmd("GetResultString",
                    "{'resultid':'" + newid + "'}"));
                loadAllVectors(); closeProgress(); toggleSelect();
            } else {
                setTimeout(processLoop, 2000);
            }
        }
        processLoop();
    }, 500);
}
function analysePredictSites(polygonid, technology) {
    if (getActionRunning()) { return; }
    _saveSettings();
    saveAllVectors();
    minimizeAllDialogs();
    showProgress('Predicting Sites');
    setTimeout(function () {
        let newid = Ajax_runccmd("GetNewResultId", "");
        Ajax_runccmd("PredictSites",
            "{" +
            "'resultid':'" + newid + "'," +
            "'polygonid':'" + polygonid + "'," +
            "'technology':'" + technology + "'" +
            "}");
        function processLoop() {
            let chk = getForecastRunning();
            if (chk === false) {
                showResult(Ajax_runccmd("GetResultString",
                    "{'resultid':'" + newid + "'}"));
                loadAllVectors();
                showNetworkReport(null);
                showLinksReport(null, Ajax_runccmd("GetLinksResult", ""));
                closeProgress(); toggleSelect();
            } else {
                setTimeout(processLoop, 2000);
            }
        }
        processLoop();
    }, 500);
}
function predictCoverage(sites) {
    if (getActionRunning()) { return; }
    _saveSettings();
    saveAllVectors();
    minimizeAllDialogs();
    showProgress('Predicting Coverage');
    setTimeout(function () {
        let newid = Ajax_runccmd("GetNewResultId", "");
        Ajax_runccmd("PredictCoverage",
            "{" +
            "'resultid':'" + newid + "'," +
            "'sites':'" + sites + "'" +
            "}");
        function processLoop() {
            let chk = getForecastRunning();
            if (chk === false) {
                showResult(Ajax_runccmd("GetResultString",
                    "{'resultid':'" + newid + "'}"));
                loadAllVectors();
                showNetworkReport(null);
                closeProgress(); toggleSelect();
            } else {
                setTimeout(processLoop, 2000);
            }
        }
        processLoop();
    }, 500);
}
function predictRadioPlan(sites) {
    if (getActionRunning()) { return; }
    _saveSettings();
    saveAllVectors();
    minimizeAllDialogs();
    showProgress('Predicting Radio Plan');
    setTimeout(function () {
        Ajax_runccmd("RadioPlan",
            "{" +
            "'sites':'" + sites + "'" +
            "}");
        function processLoop() {
            let chk = getForecastRunning();
            if (chk === false) {
                loadAllVectors();
                showNetworkReport(null);
                closeProgress(); toggleSelect();
            } else {
                setTimeout(processLoop, 2000);
            }
        }
        processLoop();
    }, 500);
}
function predictLinks(sites) {
    if (getActionRunning()) { return; }
    _saveSettings();
    saveAllVectors();
    minimizeAllDialogs();
    showProgress('Predicting Links');
    setTimeout(function () {
        Ajax_runccmd("Links",
            "{" +
            "'sites':'" + sites + "'" +
            "}");
        function processLoop() {
            let chk = getForecastRunning();
            if (chk === false) {
                loadAllVectors();
                showLinksReport(null, Ajax_runccmd("GetLinksResult", ""));
                closeProgress(); toggleSelect();
            } else {
                setTimeout(processLoop, 2000);
            }
        }
        processLoop();
    }, 500);
}
function showNetworkReport(e = null) {
    if (e) { e.stopPropagation(); }
    let header = ['Site Id', 'Site Name', 'Longitude', 'Latitude', 'Tower Height', 'Technology', 'Band', 'Bandwidth',
        'Cell ID', 'TAC / LAC', 'EARFCN / ARFCN', 'PCI / BSIC', 'DL Frequency', 'UL Frequency', 'RF Power',
        'Antenna Height', 'Azimuth', 'Antenna Model', 'Antenna Type', 'Polarization', 'V-Beamwidth', 'H-Beamwidth', 'Downtilt', 'Antenna Gain', 'Feeder Loss'];
    let csvString;
    csvString = header.join(',') + '\r\n';
    header = header.map(h => '<td style="color:var(--facecolor);background-color:var(--barcolor);white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + h + '</td>');
    header.unshift('<tr>');
    header.push('</tr>');
    let properties = [];
    let features = myNetworkLayer.getSource().getFeatures();
    features.forEach(function (f) {
        properties.push('<tr>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + f.get('siteid') + '</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + f.get('sitename') + '</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + round(f.get('longitude'), 6).toString() + '°' + '</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + round(f.get('latitude'), 6).toString() + '°' + '</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + round(f.get('height'), 0).toString() + ' m' + '</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + f.get('technology') + '</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + f.get('band') + '</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + f.get('bandwidth') + ' MHz' + '</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + f.get('cellid') + '</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + f.get('lac') + '</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + f.get('rfcn') + '</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + f.get('rfid') + '</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + f.get('dlfrequency') + ' MHz' + '</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + f.get('ulfrequency') + ' MHz' + '</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + f.get('rfpower') + ' W' + '</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + round(f.get('hba'), 0).toString() + ' m' + '</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + f.get('azimuth') + '°' + '</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + f.get('antennamodel') + '</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + f.get('antennatype') + '</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + f.get('polarization') + '</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + f.get('vbeamwidth') + '°' + '</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + f.get('hbeamwidth') + '°' + '</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + f.get('downtilt') + '°' + '</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + f.get('antennagain') + ' dBi' + '</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + f.get('feederloss') + ' dB' + '</td>');
        properties.push('</tr>');
    });
    csvString += properties.join(',');
    while (csvString.includes('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">')) {
        csvString = csvString.replace('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">', '');
    }
    while (csvString.includes('</td>')) {
        csvString = csvString.replace('</td>', '');
    }
    while (csvString.includes('<tr>,')) {
        csvString = csvString.replace('<tr>,', '');
    }
    while (csvString.includes('</tr>,')) {
        csvString = csvString.replace('</tr>,', '\r\n');
    }
    while (csvString.includes('</tr>')) {
        csvString = csvString.replace('</tr>', '');
    }
    let details = '<table><tbody>' + header.join('') + properties.join('') + '</tbody></table>';
    let loadObj = [];
    loadObj.push({ title: 'Network details', contentType: 'table', contentHTML: details, contentPadding: '0', contentData: csvString, contentExtension: '.csv' });
    ddsCreate('Network Report', loadObj);
}
function showLinksReport(e = null, transmsg = undefined) {
    if (e) { e.stopPropagation(); }
    let summary = '';
    let modifiedLinks = undefined;
    if (transmsg) {
        let modified = transmsg.split('@');
        let internal = modified[0];
        let external = modified[1];
        modifiedLinks = {
            internal: internal.length > 0 ? internal.split(',') : [],
            external: external.length > 0 ? external.split(',') : []
        }
    }
    if (modifiedLinks) {
        summary = '<div><b>Links created</b><br/>';
        let publicLink = myLinksLayer.getSource().getFeatureByProperty('linktype', 'public');
        if (publicLink) {
            let publicSite = getPublicSite(publicLink.get('siteidb'), publicLink.get('email'));
            summary += '<br/><b>Public Link:</b><br/>Site Id: ' + publicSite.get('siteid') + ' Site Name: ' + publicSite.get('sitename') + ' from Public Network has been found,'
                + ' Link Id: ' + publicLink.get('linkid') + ' has been added to link Public Site,' +
                ' You can contact user: ' + publicSite.get('name') + ' on email address: ' + publicSite.get('email') + ' for correspondance.<br/>';
            if (modifiedLinks.external.length > 0) {
                summary += '<br/><b>Links adjusted to meet ' + document.getElementById('opt_fresnelclearance').value + '% Fresnel Zone clearance requirement:</b><br/>';
                let adjustedSite = myNetworkLayer.getSource().getFeatureByProperty('siteid', modifiedLinks.external[0]);
                summary += 'Site Id: ' + adjustedSite.get('siteid') + ' Site Name: ' + adjustedSite.get('sitename') + ', height has been adjusted to ' +
                    round(adjustedSite.get('height'), 0) + ' meters to link with Public Site<br/>';
            }
        }
        if (modifiedLinks.internal.length > 0) {
            if (modifiedLinks.external.length === 0) {
                summary += '<br/><b>Links adjusted to meet ' + document.getElementById('opt_fresnelclearance').value + '% Fresnel Zone clearance requirement:</b><br/>';
            }
            for (i = 0; i < modifiedLinks.internal.length; i++) {
                let adjustedSite = myNetworkLayer.getSource().getFeatureByProperty('siteid', modifiedLinks.internal[i]);
                summary += 'Site Id: ' + adjustedSite.get('siteid') + ' Site Name: ' + adjustedSite.get('sitename') + ', height has been adjusted to ' +
                    round(adjustedSite.get('height'), 0) + ' meters, internal adjustment<br/>';
            }
        }
        summary += '</div>';
    }
    let textString = summary;
    while (textString.includes('<div>')) {
        textString = textString.replace('<div>', '');
    }
    while (textString.includes('</div>')) {
        textString = textString.replace('</div>', '');
    }
    while (textString.includes('<br/>')) {
        textString = textString.replace('<br/>', '\r\n');
    }
    while (textString.includes('<b>')) {
        textString = textString.replace('<b>', '');
    }
    while (textString.includes('</b>')) {
        textString = textString.replace('</b>', '');
    }

    let header = ['Link Id', 'Link Name', 'Link Type',
        'Site Id A', 'Site Name A', 'Height / Elevation A', 'Bearing / Tilt A', 'Frequency A', 'Channel Width A', 'Output Power A', 'Antenna Gain A', 'Losses A',
        'Site Id B', 'Site Name B', 'Height / Elevation B', 'Bearing / Tilt B', 'Frequency B', 'Channel Width B', 'Output Power B', 'Antenna Gain B', 'Losses B'];
    let csvString;
    csvString = header.join(',') + '\r\n';
    header = header.map(h => '<td style="color:var(--facecolor);background-color:var(--barcolor);white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + h + '</td>');
    header.unshift('<tr>');
    header.push('</tr>');
    let properties = [];
    let features = myLinksLayer.getSource().getFeatures();
    features.forEach(function (f) {
        let siteA = myNetworkLayer.getSource().getFeatureByProperty('siteid', f.get('siteida'));
        let siteB = undefined;
        if (f.get('linktype') === 'public') {
            siteB = getPublicSite(f.get('siteidb'), f.get('email'));
        } else {
            siteB = myNetworkLayer.getSource().getFeatureByProperty('siteid', f.get('siteidb'));
        }
        let xDistanceKm = Number(f.get('distance'));
        let deviceheightAm = Number(siteA.get('height')) + Number(f.get('locheighta'));
        let deviceheightBm = Number(siteB.get('height')) + Number(f.get('locheightb'));
        let tiltAd = 0; tiltBd = 0;
        if (deviceheightAm > deviceheightBm) {
            tiltAd = round(turf.radiansToDegrees(Math.atan(deviceheightAm / (xDistanceKm * 1000))), 2);
            tiltBd = round(turf.radiansToDegrees(-1 * Math.atan((deviceheightAm - deviceheightBm) / (xDistanceKm * 1000))), 2);
        }
        else {
            tiltAd = round(turf.radiansToDegrees(-1 * Math.atan((deviceheightBm - deviceheightAm) / (xDistanceKm * 1000))), 2);
            tiltBd = round(turf.radiansToDegrees(Math.atan(deviceheightBm / (xDistanceKm * 1000))), 2);
        }
        properties.push('<tr>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + f.get('linkid') + '</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + f.get('linkname') + '</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + titleCase(f.get('linktype')) + '</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + f.get('siteida') + '</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + siteA.get('sitename') + '</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + round(parseFloat(siteA.get('height')), 0).toString() + ' m / ' + round(parseFloat(f.get('locheighta')), 0).toString() + ' m </td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + round(parseFloat(f.get('bearinga')), 2).toString() + '° / ' + (tiltAd).toString() + '° </td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + f.get('frequencya') + ' GHz</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + f.get('channelwidtha') + ' MHz</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + f.get('outputpowera') + ' dBm</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + f.get('antennagaina') + ' dBi</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + f.get('lossesa') + ' dB</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + f.get('siteidb') + '</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + siteB.get('sitename') + '</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + round(parseFloat(siteB.get('height')), 0).toString() + ' m / ' + round(parseFloat(f.get('locheightb')), 0).toString() + ' m </td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + round(parseFloat(f.get('bearingb')), 2).toString() + '° / ' + (tiltBd).toString() + '° </td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + f.get('frequencyb') + ' GHz</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + f.get('channelwidthb') + ' MHz</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + f.get('outputpowerb') + ' dBm</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + f.get('antennagainb') + ' dBi</td>');
        properties.push('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">' + f.get('lossesb') + ' dB</td>');
        properties.push('</tr>');
    });
    csvString += properties.join(',');
    while (csvString.includes('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">')) {
        csvString = csvString.replace('<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">', '');
    }
    while (csvString.includes('</td>')) {
        csvString = csvString.replace('</td>', '');
    }
    while (csvString.includes('<tr>,')) {
        csvString = csvString.replace('<tr>,', '');
    }
    while (csvString.includes('</tr>,')) {
        csvString = csvString.replace('</tr>,', '\r\n');
    }
    while (csvString.includes('</tr>')) {
        csvString = csvString.replace('</tr>', '');
    }
    let details = '<table><tbody>' + header.join('') + properties.join('') + '</tbody></table>';
    let loadObj = [];
    if (modifiedLinks) {
        loadObj.push({ title: 'Links summary', contentType: 'text', contentHTML: summary, contentPadding: '15px 10px', contentData: textString, contentExtension: '.txt' });
        loadObj.push({ title: 'Links details', contentType: 'table', contentHTML: details, contentPadding: '0', contentData: csvString, contentExtension: '.csv' });
        ddsCreate('Links Report', loadObj);
    }
    else {
        loadObj.push({ title: 'Links details', contentType: 'table', contentHTML: details, contentPadding: '0', contentData: csvString, contentExtension: '.csv' });
        ddsCreate('Links Report', loadObj);
    }
}
var getForecastRunning = function () { return Ajax_runccmd("GetForecastRunning", "") === 'true'; };
var getActionRunning = function () { return getForecastRunning() || actionRunning; };

