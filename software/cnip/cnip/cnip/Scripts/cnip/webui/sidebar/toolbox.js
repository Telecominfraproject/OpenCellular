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
                showNetworkReport(null, newid, Ajax_runccmd("GetRadioPlanResult", ""));
                showLinksReport(null, newid, Ajax_runccmd("GetLinksResult", ""));
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
                showNetworkReport(null, newid, Ajax_runccmd("GetRadioPlanResult", ""));
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
    let d = new Date();
    let resultid = Math.round(d.getTime() / 1000, 0).toString();
    showProgress('Predicting Radio Plan');
    setTimeout(function () {
        Ajax_runccmd("RadioPlan",
            "{" +
            "'resultid':'" + resultid + "'," +
            "'sites':'" + sites + "'" +
            "}");
        function processLoop() {
            let chk = getForecastRunning();
            if (chk === false) {
                loadAllVectors();
                showNetworkReport(null, resultid, Ajax_runccmd("GetRadioPlanResult", ""));
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
    let d = new Date();
    let resultid = Math.round(d.getTime() / 1000, 0).toString();
    showProgress('Predicting Links');
    setTimeout(function () {
        Ajax_runccmd("Links",
            "{" +
            "'resultid':'" + resultid + "'," +
            "'sites':'" + sites + "'" +
            "}");
        function processLoop() {
            let chk = getForecastRunning();
            if (chk === false) {
                loadAllVectors();
                showLinksReport(null, resultid, Ajax_runccmd("GetLinksResult", ""));
                closeProgress(); toggleSelect();
            } else {
                setTimeout(processLoop, 2000);
            }
        }
        processLoop();
    }, 500);
}
function showNetworkReport(e = null, testResultid = '', resultString = undefined) {
    if (e) { e.stopPropagation(); }
    let resultid = '';
    let sites = [];
    if (resultString) {
        resultid = resultString.split('@')[0];
    }
    if (resultid.toString() === testResultid.toString() && resultid.length > 0) {
        sites = resultString.split('@')[1].length > 0 ? resultString.split('@')[1].split(',') : [];
    }
    let features;
    if (sites.length > 0) {
        features = myNetworkLayer.getSource().getFeaturesByProperty('siteid', sites);
    }
    else {
        features = myNetworkLayer.getSource().getFeatures();
    }
    if (features.length > 0) {
        let header = [
            'Site Id', 'Site Name', 'Longitude', 'Latitude', 'Tower Height', 'Technology', 'Band', 'Bandwidth',
            'Cell ID', 'TAC / LAC', 'EARFCN / ARFCN', 'PCI / BSIC', 'DL Frequency', 'UL Frequency', 'RF Power',
            'Antenna Height', 'Azimuth', 'Antenna Model', 'Antenna Type', 'Polarization', 'V-Beamwidth', 'H-Beamwidth',
            'Downtilt', 'Antenna Gain', 'Feeder Loss'
        ];
        let th = '<th style="color:var(--facecolor);background-color:var(--barcolor);white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">';
        header = header.map(h => th + h + '</th>');
        header.unshift('<tr>');
        header.push('</tr>');
        let td = '<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">';
        let properties = [];
        features.forEach(function (f) {
            properties.push('<tr>');
            properties.push(td + f.get('siteid') + '</td>');
            properties.push(td + f.get('sitename') + '</td>');
            properties.push(td + round(f.get('longitude'), 6).toString() + '°' + '</td>');
            properties.push(td + round(f.get('latitude'), 6).toString() + '°' + '</td>');
            properties.push(td + round(f.get('height'), 0).toString() + ' m' + '</td>');
            properties.push(td + f.get('technology') + '</td>');
            properties.push(td + f.get('band') + '</td>');
            properties.push(td + f.get('bandwidth') + ' MHz' + '</td>');
            properties.push(td + f.get('cellid') + '</td>');
            properties.push(td + f.get('lac') + '</td>');
            properties.push(td + f.get('rfcn') + '</td>');
            properties.push(td + f.get('rfid') + '</td>');
            properties.push(td + f.get('dlfrequency') + ' MHz' + '</td>');
            properties.push(td + f.get('ulfrequency') + ' MHz' + '</td>');
            properties.push(td + f.get('rfpower') + ' W' + '</td>');
            properties.push(td + round(f.get('hba'), 0).toString() + ' m' + '</td>');
            properties.push(td + f.get('azimuth') + '°' + '</td>');
            properties.push(td + f.get('antennamodel') + '</td>');
            properties.push(td + f.get('antennatype') + '</td>');
            properties.push(td + f.get('polarization') + '</td>');
            properties.push(td + f.get('vbeamwidth') + '°' + '</td>');
            properties.push(td + f.get('hbeamwidth') + '°' + '</td>');
            properties.push(td + f.get('downtilt') + '°' + '</td>');
            properties.push(td + f.get('antennagain') + ' dBi' + '</td>');
            properties.push(td + f.get('feederloss') + ' dB' + '</td>');
            properties.push('</tr>');
        });
        let details = '<table id="table' + ddsCount.toString() + '" class="sortable"><thead>' + header.join('') + '</thead><tbody>' + properties.join('') + '</tbody></table>';
        let loadObj = [];
        loadObj.push({ title: 'Network details', contentType: 'table', contentHTML: details, contentPadding: '0', contentData: '', contentExtension: '.csv' });
        ddsCreate('Network Report', loadObj);
    }
}
function showLinksReport(e = null, testResultid = '', resultString = undefined) {
    if (e) { e.stopPropagation(); }
    let resultid = '';
    let links = [];
    let privateAdjustments = [];
    let publicAdjustments = [];
    let summaryHTML = '';
    let summaryText = '';
    if (resultString) {
        resultid = resultString.split('@')[0];
    }
    if (resultid.toString() === testResultid.toString() && resultid.length > 0) {
        links = resultString.split('@')[1].length > 0 ? resultString.split('@')[1].split(',') : [];
        privateAdjustments = resultString.split('@')[2].length > 0 ? resultString.split('@')[2].split(',') : [];
        publicAdjustments = resultString.split('@')[3].length > 0 ? resultString.split('@')[3].split(',') : [];
    }
    if (links.length > 0) {
        summaryHTML = '<div><b>Links created</b><br/>';
        summaryText = 'Links created' + '\r\n';
        let publicLink;
        for (let i = 0; i < links.length; i++) {
            publicLink = myLinksLayer.getSource().getFeatureByProperties({ 'linkid': links[i], 'linktype': 'public' });
            if (publicLink) {
                break;
            }
        }
        if (publicLink) {
            let publicSite = getPublicSite(publicLink.get('siteidb'), publicLink.get('email'));
            summaryHTML += '<br/><b>Public Link:</b><br/>Site Id: ' + publicSite.get('siteid') + ' Site Name: ' + publicSite.get('sitename') + ' from Public Network has been found,'
                + ' Link Id: ' + publicLink.get('linkid') + ' has been added to link Public Site,' +
                ' You can contact user: ' + publicSite.get('name') + ' on email address: ' + publicSite.get('email') + ' for correspondance.<br/>';
            summaryText += '\r\nPublic Link:\r\nSite Id:' + publicSite.get('siteid') + ' Site Name: ' + publicSite.get('sitename') + ' from Public Network has been found,'
                + ' Link Id: ' + publicLink.get('linkid') + ' has been added to link Public Site,' +
                ' You can contact user: ' + publicSite.get('name') + ' on email address: ' + publicSite.get('email') + ' for correspondance.\r\n';
            if (publicAdjustments.length > 0) {
                summaryHTML += '<br/><b>Links adjusted to meet ' + document.getElementById('opt_fresnelclearance').value + '% Fresnel Zone clearance requirement.</b><br/>';
                summaryText += '\r\nLinks adjusted to meet ' + document.getElementById('opt_fresnelclearance').value + '% Fresnel Zone clearance requirement.\r\n';
                for (let i = 0; i < publicAdjustments.length; i++) {
                    let adjustedSite = myNetworkLayer.getSource().getFeatureByProperty('siteid', publicAdjustments[i]);
                    summaryHTML += 'Site Id: ' + adjustedSite.get('siteid') + ' Site Name: ' + adjustedSite.get('sitename') + ', height has been adjusted to ' +
                        round(adjustedSite.get('height'), 0) + ' meters to link with Public Site<br/>';
                    summaryText += 'Site Id: ' + adjustedSite.get('siteid') + ' Site Name: ' + adjustedSite.get('sitename') + ', height has been adjusted to ' +
                        round(adjustedSite.get('height'), 0) + ' meters to link with Public Site\r\n';
                }
            }
        }
        if (privateAdjustments.length > 0) {
            if (publicAdjustments.length === 0) {
                summaryHTML += '<br/><b>Links adjusted to meet ' + document.getElementById('opt_fresnelclearance').value + '% Fresnel Zone clearance requirement.</b><br/>';
                summaryText += '\r\nLinks adjusted to meet ' + document.getElementById('opt_fresnelclearance').value + '% Fresnel Zone clearance requirement.\r\n';
            }
            for (i = 0; i < privateAdjustments.length; i++) {
                let adjustedSite = myNetworkLayer.getSource().getFeatureByProperty('siteid', privateAdjustments[i]);
                summaryHTML += 'Site Id: ' + adjustedSite.get('siteid') + ' Site Name: ' + adjustedSite.get('sitename') + ', height has been adjusted to ' +
                    round(adjustedSite.get('height'), 0) + ' meters.<br/>';
                summaryText += 'Site Id: ' + adjustedSite.get('siteid') + ' Site Name: ' + adjustedSite.get('sitename') + ', height has been adjusted to ' +
                    round(adjustedSite.get('height'), 0) + ' meters.\r\n';
            }
        }
        summaryHTML += '</div>';
    }
    let features;
    if (links.length > 0) {
        features = myLinksLayer.getSource().getFeaturesByProperty('linkid', links);
    }
    else {
        features = myLinksLayer.getSource().getFeatures();
    }
    if (features.length > 0) {
        let header = [
            'Link Id', 'Link Name', 'Link Type',
            'Site Id A', 'Site Name A', 'Height / Elevation A', 'Bearing / Tilt A', 'Frequency A',
            'Channel Width A', 'Output Power A', 'Antenna Gain A', 'Losses A',
            'Site Id B', 'Site Name B', 'Height / Elevation B', 'Bearing / Tilt B', 'Frequency B',
            'Channel Width B', 'Output Power B', 'Antenna Gain B', 'Losses B'
        ];
        let th = '<th style="color:var(--facecolor);background-color:var(--barcolor);white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">';
        header = header.map(h => th + h + '</th>');
        header.unshift('<tr>');
        header.push('</tr>');
        let td = '<td style="white-space:nowrap;border:thin solid #C0C0C0;vertical-align:middle;text-align:center;padding:5px;">';
        let properties = [];
        features.forEach(function (f) {
            let siteA = myNetworkLayer.getSource().getFeatureByProperty('siteid', f.get('siteida'));
            let siteB = undefined;
            if (f.get('linktype') === 'public') {
                siteB = getPublicSite(f.get('siteidb'), f.get('email'));
            } else {
                siteB = myNetworkLayer.getSource().getFeatureByProperty('siteid', f.get('siteidb'));
            }
            let xDistanceKm = Number(f.get('distance'));
            let deviceheightAm = Number(f.get('heighta')) + Number(f.get('locheighta'));
            let deviceheightBm = Number(f.get('heightb')) + Number(f.get('locheightb'));
            let tiltAd = 0; tiltBd = 0;
            if (deviceheightAm > deviceheightBm) {
                tiltAd = -1 * round(turf.radiansToDegrees(Math.atan((deviceheightAm - deviceheightBm) / (xDistanceKm * 1000))), 2);
                tiltBd = round(turf.radiansToDegrees(Math.atan((deviceheightAm - deviceheightBm) / (xDistanceKm * 1000))), 2);
            } else if (deviceheightAm < deviceheightBm) {
                tiltAd = round(turf.radiansToDegrees(Math.atan((deviceheightBm - deviceheightAm) / (xDistanceKm * 1000))), 2);
                tiltBd = -1 * round(turf.radiansToDegrees(Math.atan((deviceheightBm - deviceheightAm) / (xDistanceKm * 1000))), 2);
            } else {
                tiltAd = 0; tiltBd = 0;
            }
            properties.push('<tr>');
            properties.push(td + f.get('linkid') + '</td>');
            properties.push(td + f.get('linkname') + '</td>');
            properties.push(td + titleCase(f.get('linktype')) + '</td>');
            properties.push(td + f.get('siteida') + '</td>');
            properties.push(td + siteA.get('sitename') + '</td>');
            properties.push(td + round(parseFloat(f.get('heighta')), 1).toString() + ' m / ' + round(parseFloat(f.get('locheighta')), 0).toString() + ' m </td>');
            properties.push(td + round(parseFloat(f.get('bearinga')), 2).toString() + '° / ' + (tiltAd).toString() + '° </td>');
            properties.push(td + f.get('frequencya') + ' GHz</td>');
            properties.push(td + f.get('channelwidtha') + ' MHz</td>');
            properties.push(td + f.get('outputpowera') + ' dBm</td>');
            properties.push(td + f.get('antennagaina') + ' dBi</td>');
            properties.push(td + f.get('lossesa') + ' dB</td>');
            properties.push(td + f.get('siteidb') + '</td>');
            properties.push(td + siteB.get('sitename') + '</td>');
            properties.push(td + round(parseFloat(f.get('heightb')), 1).toString() + ' m / ' + round(parseFloat(f.get('locheightb')), 0).toString() + ' m </td>');
            properties.push(td + round(parseFloat(f.get('bearingb')), 2).toString() + '° / ' + (tiltBd).toString() + '° </td>');
            properties.push(td + f.get('frequencyb') + ' GHz</td>');
            properties.push(td + f.get('channelwidthb') + ' MHz</td>');
            properties.push(td + f.get('outputpowerb') + ' dBm</td>');
            properties.push(td + f.get('antennagainb') + ' dBi</td>');
            properties.push(td + f.get('lossesb') + ' dB</td>');
            properties.push('</tr>');
        });
        let details = '<table id="table' + ddsCount.toString() + '" class="sortable"><thead>' + header.join('') + '</thead><tbody>' + properties.join('') + '</tbody></table>';
        let loadObj = [];
        if (summaryHTML.length > 0) {
            loadObj.push({ title: 'Links summary', contentType: 'text', contentHTML: summaryHTML, contentPadding: '15px 10px', contentData: summaryText, contentExtension: '.txt' });
            loadObj.push({ title: 'Links details', contentType: 'table', contentHTML: details, contentPadding: '0', contentData: '', contentExtension: '.csv' });
            ddsCreate('Links Report', loadObj);
        }
        else {
            loadObj.push({ title: 'Links details', contentType: 'table', contentHTML: details, contentPadding: '0', contentData: '', contentExtension: '.csv' });
            ddsCreate('Links Report', loadObj);
        }
    }
}
var getForecastRunning = function () { return Ajax_runccmd("GetForecastRunning", "") === 'true'; };
var getActionRunning = function () { return getForecastRunning() || actionRunning; };

