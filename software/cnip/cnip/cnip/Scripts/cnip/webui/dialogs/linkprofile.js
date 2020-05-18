/* global variables*/
var userDisplay = false;
var linkData;
var siteAData;
var siteBData;
var linksCount;
var currLinkIdx;
var elevChart;
var deleteLinkIdx = -1;

initSingleSlider(document.getElementById('dheighta'), '', '0', '100', '0.5', '0');
initSingleSlider(document.getElementById('dheightb'), '', '0', '100', '0.5', '0');
initSingleSlider(document.getElementById('dfrequencya'), '', '1', '15', '0.1', '5');
initSingleSlider(document.getElementById('dchannelwidtha'), '', '10', '100', '10', '10');
initSingleSlider(document.getElementById('doutputpowera'), '', '27', '47', '0.5', '30');
initSingleSlider(document.getElementById('dantennagaina'), '', '19', '51', '0.1', '34');
initSingleSlider(document.getElementById('dlossesa'), '', '0', '51', '0.1', '0');
initSingleSlider(document.getElementById('dfrequencyb'), '', '1', '15', '0.1', '5');
initSingleSlider(document.getElementById('dchannelwidthb'), '', '10', '100', '10', '10');
initSingleSlider(document.getElementById('doutputpowerb'), '', '27', '47', '0.5', '30');
initSingleSlider(document.getElementById('dantennagainb'), '', '19', '51', '0.1', '34');
initSingleSlider(document.getElementById('dlossesb'), '', '0', '51', '0.1', '0');

document.getElementById('linkname').addEventListener('change', linkNameChange);
document.getElementById('dheighta').addEventListener('change', heightAChange);
document.getElementById('dfrequencya').addEventListener('change', frequencyAChange);
document.getElementById('dchannelwidtha').addEventListener('change', channelwidthAChange);
document.getElementById('doutputpowera').addEventListener('change', outputpowerAChange);
document.getElementById('dantennagaina').addEventListener('change', antennagainAChange);
document.getElementById('dlossesa').addEventListener('change', lossesAChange);
document.getElementById('dheightb').addEventListener('change', heightBChange);
document.getElementById('dfrequencyb').addEventListener('change', frequencyBChange);
document.getElementById('dchannelwidthb').addEventListener('change', channelwidthBChange);
document.getElementById('doutputpowerb').addEventListener('change', outputpowerBChange);
document.getElementById('dantennagainb').addEventListener('change', antennagainBChange);
document.getElementById('dlossesb').addEventListener('change', lossesBChange);

function frequencyAChange(e) {
    document.getElementById('frequencya').value = Number(document.getElementById('dfrequencya').value);
    saveLinkData(e);
}
function channelwidthAChange(e) {
    document.getElementById('channelwidtha').value = Number(document.getElementById('dchannelwidtha').value);
    saveLinkData(e);
}
function outputpowerAChange(e) {
    document.getElementById('outputpowera').value = Number(document.getElementById('doutputpowera').value);
    saveLinkData(e);
}
function antennagainAChange(e) {
    document.getElementById('antennagaina').value = Number(document.getElementById('dantennagaina').value);
    saveLinkData(e);
}
function lossesAChange(e) {
    document.getElementById('lossesa').value = Number(document.getElementById('dlossesa').value);
    saveLinkData(e);
}
function frequencyBChange(e) {
    document.getElementById('frequencyb').value = Number(document.getElementById('dfrequencyb').value);
    saveLinkData(e);
}
function channelwidthBChange(e) {
    document.getElementById('channelwidthb').value = Number(document.getElementById('dchannelwidthb').value);
    saveLinkData(e);
}
function outputpowerBChange(e) {
    document.getElementById('outputpowerb').value = Number(document.getElementById('doutputpowerb').value);
    saveLinkData(e);
}
function antennagainBChange(e) {
    document.getElementById('antennagainb').value = Number(document.getElementById('dantennagainb').value);
    saveLinkData(e);
}
function lossesBChange(e) {
    document.getElementById('lossesb').value = Number(document.getElementById('dlossesb').value);
    saveLinkData(e);
}
function linkNameChange(e) {
    document.getElementById('linkname').value = fixStringName(document.getElementById('linkname').value);
    saveLinkData(e);
}
function heightAChange(e) {
    // height of site a siteAData[4];
    document.getElementById('heighta').value = Number(document.getElementById('dheighta').value);
    if (Number(siteAData[4] === undefined ? '' : siteAData[4].toString()) < Number(document.getElementById('heighta').value)) {
        window.parent.updateSiteHeightById((siteAData[0] === undefined ? '' : siteAData[0].toString()), document.getElementById('heighta').value);
    }
    saveLinkData(e, true);
}
function heightBChange(e) {
    // height of site b siteBData[4];
    document.getElementById('heightb').value = Number(document.getElementById('dheightb').value);
    if (document.getElementById('linktype').innerHTML === 'public') {
        if (Number(siteBData[4] === undefined ? '' : siteBData[4].toString()) < Number(document.getElementById('heightb').value)) {
            document.getElementById('heightb').value = Number(siteBData[4] === undefined ? '' : siteBData[4].toString());
        }
    }
    else {
        if (Number(siteBData[4] === undefined ? '' : siteBData[4].toString()) < Number(document.getElementById('heightb').value)) {
            window.parent.updateSiteHeightById((siteBData[0] === undefined ? '' : siteBData[0].toString()), document.getElementById('heightb').value);
        }
    }
    saveLinkData(e, true);
}
/* char js global setup */
Chart.defaults.global.pointHitDetectionRadius = 1;
Chart.defaults.LineWithLine = Chart.defaults.line;
Chart.controllers.LineWithLine = Chart.controllers.line.extend({
    draw: function (ease) {
        Chart.controllers.line.prototype.draw.call(this, ease);
        if (this.chart.tooltip._active && this.chart.tooltip._active.length) {
            let activePoint = this.chart.tooltip._active[0],
                ctx = this.chart.ctx,
                x = activePoint.tooltipPosition().x,
                topY = this.chart.legend.bottom,
                bottomY = this.chart.chartArea.bottom;
            ctx.save();
            ctx.beginPath();
            ctx.moveTo(x, topY);
            ctx.lineTo(x, bottomY);
            ctx.lineWidth = 1;
            ctx.strokeStyle = '#07C';
            ctx.stroke();
            ctx.restore();
        }
    }
});
var customTooltips = function (tooltip) {
    let tooltipEl = document.getElementById('chartjs-tooltip');
    if (!tooltipEl) {
        tooltipEl = document.createElement('div');
        tooltipEl.id = 'chartjs-tooltip';
        tooltipEl.innerHTML = '<table></table>';
        this._chart.canvas.parentNode.appendChild(tooltipEl);
    }
    if (tooltip.opacity === 0) {
        tooltipEl.style.opacity = 0;
        return;
    }
    tooltipEl.classList.remove('above', 'below', 'no-transform');
    if (tooltip.yAlign) {
        tooltipEl.classList.add(tooltip.yAlign);
    } else {
        tooltipEl.classList.add('no-transform');
    }
    function getBody(bodyItem) {
        return bodyItem.lines;
    }
    if (tooltip.body) {
        let titleLines = tooltip.title || [];
        let bodyLines = tooltip.body.map(getBody);
        let tDistance, tElevation, tLosHeight, tFreznelZone;
        titleLines.forEach(function (title) {
            tDistance = round(Number(title), 2);
        });
        bodyLines.forEach(function (body, i) {
            if (body) {
                if (body[0].includes('Elevation')) tElevation = Number(body[0].split(':')[1]);
                if (body[0].includes('LosHeight')) tLosHeight = Number(body[0].split(':')[1]);
                if (body[0].includes('FresnelZone')) tFresnelZone = Number(body[0].split(':')[1]);
            }
        });
        let innerHtml = '<thead>';
        innerHtml += '<tr><th></th></tr>';
        innerHtml += '</thead><tbody>';
        if (tDistance !== undefined && tElevation !== undefined && tLosHeight !== undefined && tFresnelZone !== undefined) {
            innerHtml += '<tr><td style="background-color:rgba(0,0,0,0.7);color:white;font-size:12px;" >Distance (Km): ' + tDistance.toString() + '</td></tr>';
            innerHtml += '<tr><td style="background-color:rgba(0,0,0,0.7);color:white;font-size:12px;" >Elevation (m): ' + tElevation.toString() + '</td></tr>';
            innerHtml += '<tr><td style="background-color:rgba(0,0,0,0.7);color:white;font-size:12px;" >Los Height (m): ' + round(tLosHeight - tElevation, 2).toString() + '</td></tr>';
            innerHtml += '<tr><td style="background-color:rgba(0,0,0,0.7);color:white;font-size:12px;" >Fresnel Height (m): ' + round(tFresnelZone - tElevation, 2).toString() + '</td></tr>';
        }
        innerHtml += '</tbody>';
        let tableRoot = tooltipEl.querySelector('table');
        tableRoot.innerHTML = innerHtml;
    }
    let positionY = this._chart.canvas.offsetTop;
    let positionX = this._chart.canvas.offsetLeft;
    tooltipEl.style.opacity = 1;
    tooltipEl.style.left = positionX + tooltip.caretX + 161 + 'px';
    tooltipEl.style.top = positionY + tooltip.caretY - 50 + 'px';
    tooltipEl.style.fontFamily = tooltip._bodyFontFamily;
    tooltipEl.style.fontSize = tooltip.bodyFontSize + 'px';
    tooltipEl.style.fontStyle = tooltip._bodyFontStyle;
    tooltipEl.style.padding = tooltip.yPadding + 'px ' + tooltip.xPadding + 'px';
};
/* linkprofile functions*/
function fillLinkData(linkData, refreshGraph) {

    document.getElementById('dheighta').setValue(linkData[5] === undefined ? '' : linkData[5].toString());
    document.getElementById('dchannelwidtha').setValue(linkData[7] === undefined ? '' : linkData[7].toString());
    document.getElementById('dfrequencya').setValue(linkData[8] === undefined ? '' : linkData[8].toString());
    document.getElementById('doutputpowera').setValue(linkData[9] === undefined ? '' : linkData[9].toString());
    document.getElementById('dantennagaina').setValue(linkData[10] === undefined ? '' : linkData[10].toString());
    document.getElementById('dlossesa').setValue(linkData[11] === undefined ? '' : linkData[11].toString());
    document.getElementById('dheightb').setValue(linkData[14] === undefined ? '' : linkData[14].toString());
    document.getElementById('dchannelwidthb').setValue(linkData[16] === undefined ? '' : linkData[16].toString());
    document.getElementById('dfrequencyb').setValue(linkData[17] === undefined ? '' : linkData[17].toString());
    document.getElementById('doutputpowerb').setValue(linkData[18] === undefined ? '' : linkData[18].toString());
    document.getElementById('dantennagainb').setValue(linkData[19] === undefined ? '' : linkData[19].toString());
    document.getElementById('dlossesb').setValue(linkData[20] === undefined ? '' : linkData[20].toString());

    document.getElementById('linkid').innerHTML = linkData[0].toString();
    document.getElementById('linkname').value = linkData[1] === undefined ? '' : linkData[1].toString();
    document.getElementById('linktype').innerHTML = linkData[2] === undefined ? '' : linkData[2].toString();
    let linktype = linkData[2] === undefined ? '' : linkData[2].toString();
    if (linktype === 'public') {
        document.getElementById('sitebp').innerHTML = 'Site B (Public)';
        document.getElementById('sitebp').style.width = '100px';
    } else {
        document.getElementById('sitebp').innerHTML = 'Site B';
        document.getElementById('sitebp').style.width = '50px';
    }
    document.getElementById('siteida').value = linkData[3] === undefined ? '' : linkData[3].toString();
    document.getElementById('sitenamea').value = siteAData[1] === undefined ? '' : siteAData[1].toString();
    document.getElementById('siteheighteleva').value =
        round(siteAData[4] === undefined ? '' : siteAData[4].toString(), 0).toString() + ' / ' +
        round(Number(linkData[4] === undefined ? '' : linkData[4].toString()), 0).toString();
    document.getElementById('heighta').value = linkData[5] === undefined ? '' : linkData[5].toString();
    document.getElementById('channelwidtha').value = linkData[7] === undefined ? '' : linkData[7].toString();
    document.getElementById('frequencya').value = linkData[8] === undefined ? '' : linkData[8].toString();
    document.getElementById('outputpowera').value = linkData[9] === undefined ? '' : linkData[9].toString();
    document.getElementById('antennagaina').value = linkData[10] === undefined ? '' : linkData[10].toString();
    document.getElementById('lossesa').value = linkData[11] === undefined ? '' : linkData[11].toString();
    document.getElementById('siteidb').value = linkData[12] === undefined ? '' : linkData[12].toString();
    document.getElementById('sitenameb').value = siteBData[1] === undefined ? '' : siteBData[1].toString();
    document.getElementById('siteheightelevb').value =
        round(siteBData[4] === undefined ? '' : siteBData[4].toString(), 0).toString() + ' / ' +
        round(Number(linkData[13] === undefined ? '' : linkData[13].toString()), 0).toString();
    document.getElementById('heightb').value = linkData[14] === undefined ? '' : linkData[14].toString();
    document.getElementById('channelwidthb').value = linkData[16] === undefined ? '' : linkData[16].toString();
    document.getElementById('frequencyb').value = linkData[17] === undefined ? '' : linkData[17].toString();
    document.getElementById('outputpowerb').value = linkData[18] === undefined ? '' : linkData[18].toString();
    document.getElementById('antennagainb').value = linkData[19] === undefined ? '' : linkData[19].toString();
    document.getElementById('lossesb').value = linkData[20] === undefined ? '' : linkData[20].toString();
    document.getElementById('distance').innerHTML =
        round(Number(linkData[21] === undefined ? '' : linkData[21].toString()), 1).toString() + ' Km';
    let xDistanceKm = Number(linkData[21] === undefined ? '' : linkData[21].toString());
    let deviceheightAm = Number(linkData[5] === undefined ? '' : linkData[5].toString()) +
        Number(linkData[4] === undefined ? '' : linkData[4].toString());
    let deviceheightBm = Number(linkData[14] === undefined ? '' : linkData[14].toString()) +
        Number(linkData[13] === undefined ? '' : linkData[13].toString());
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
    document.getElementById('headinga').value =
        round(Number(linkData[6] === undefined ? '' : linkData[6].toString()), 2).toString() + ' / ' + tiltAd;
    document.getElementById('headingb').value =
        round(Number(linkData[15] === undefined ? '' : linkData[15].toString()), 2).toString() + ' / ' + tiltBd;
    let channelwidthMHz = Number(linkData[7] === undefined ? '' : linkData[7].toString());
    let xFrequencyGHz = Number(linkData[8] === undefined ? '' : linkData[8].toString());
    let lossesAdB = Number(linkData[11] === undefined ? '' : linkData[11].toString());
    let lossesBdB = Number(linkData[20] === undefined ? '' : linkData[20].toString());
    let antennagainAdBi = Number(linkData[10] === undefined ? '' : linkData[10].toString());
    let antennagainBdBi = Number(linkData[19] === undefined ? '' : linkData[19].toString());
    let outputpowerAdBm = Number(linkData[9] === undefined ? '' : linkData[9].toString());
    let outputpowerBdBm = Number(linkData[18] === undefined ? '' : linkData[18].toString());
    let outputpowerAdBW = outputpowerAdBm - 30;
    let outputpowerBdBW = outputpowerBdBm - 30;
    let freespacelossdB = (20 * Math.log10(xDistanceKm)) + (20 * Math.log10(xFrequencyGHz * 1000)) + 32.44;
    let erpAdBm = outputpowerAdBm + antennagainAdBi - lossesAdB;
    let erpBdBm = outputpowerBdBm + antennagainBdBi - lossesBdB;
    let erpAdBW = outputpowerAdBW + antennagainAdBi - lossesAdB;
    let erpBdBW = outputpowerBdBW + antennagainBdBi - lossesBdB;
    let rxlevAdBm = erpBdBm - freespacelossdB + antennagainAdBi;
    let rxlevBdBm = erpAdBm - freespacelossdB + antennagainBdBi;
    let rxlevAdBW = erpBdBW - freespacelossdB + antennagainAdBi;
    let rxlevBdBW = erpAdBW - freespacelossdB + antennagainBdBi;
    let johnsonNoisedBm = -174 + (10 * Math.log10(channelwidthMHz * 1000 * 1000));
    let johnsonNoisedBW = johnsonNoisedBm - 30;
    let snrAdB = rxlevAdBW - johnsonNoisedBW;
    let snrBdB = rxlevBdBW - johnsonNoisedBW;
    let snrnumberA = Math.pow(10, snrAdB / 10);
    let snrnumberB = Math.pow(10, snrBdB / 10);
    let datarateAMbps = ((channelwidthMHz * 1000 * 1000) * Math.log2(1 + snrnumberA)) / 1000000;
    let datarateBMbps = ((channelwidthMHz * 1000 * 1000) * Math.log2(1 + snrnumberB)) / 1000000;
    if (document.getElementById('connectivityInfo').innerHTML === 'Link Obstructed') {
        document.getElementById('effca').innerHTML = '';
        document.getElementById('ratea').innerHTML = '';
        document.getElementById('leva').innerHTML = '';
        document.getElementById('effcb').innerHTML = '';
        document.getElementById('rateb').innerHTML = '';
        document.getElementById('levb').innerHTML = '';
    }
    else {
        document.getElementById('effca').innerHTML = '';
        document.getElementById('ratea').innerHTML = round(datarateAMbps, 0);
        document.getElementById('leva').innerHTML = round(rxlevAdBm, 0);
        document.getElementById('effcb').innerHTML = '';
        document.getElementById('rateb').innerHTML = round(datarateBMbps, 0);
        document.getElementById('levb').innerHTML = round(rxlevBdBm, 0);
    }
    if (refreshGraph) {
        let losBreak = false;
        let xElevations = linkData[22] === undefined ? '' : linkData[22].toString().split('@');
        let xDataset = [];
        let minYaxis = 50000;
        let fzoneBreak = false;
        let f6zoneBreak = false;
        for (let x = 0; x < xElevations.length; x++) {
            let xElevationm = Number(xElevations[x].split('&')[3]);
            let xCdistanceKm = Number(xElevations[x].split('&')[2]);
            if (minYaxis > xElevationm) { minYaxis = xElevationm; }
            let xLoS = ((x * (deviceheightBm - deviceheightAm)) /
                (xElevations.length)) + deviceheightAm;
            let xMaxx = xElevations.length - 1;
            let xFresnelFactor = 17.3 * Math.sqrt((xCdistanceKm *
                (xDistanceKm - xCdistanceKm)) / (xDistanceKm * xFrequencyGHz));
            let xFresnelZone = xLoS - xFresnelFactor;
            let xFZTop = xLoS + xFresnelFactor;
            let xFZ6 = xLoS - (xFresnelFactor * 0.6);
            let xFZ6Top = xLoS + (xFresnelFactor * 0.6);
            if (xLoS - xElevationm < 0) { losBreak = true; }
            if (xFresnelZone - xElevationm < 0) { fzoneBreak = true; }
            if (xFZ6 - xElevationm < 0) { f6zoneBreak = true; }
            xDataset.push({
                Tower: (x === 0 ? deviceheightAm : x === xMaxx ? deviceheightBm : xElevationm),
                TowerBackgroundColor: (x === 0 ? 'rgba(57,204,100,1)' :
                    x === xMaxx ? 'rgba(57,204,100,1)' : 'rgba(57,204,100,0)'),
                TowerBorderColor: (x === 0 ? 'rgba(57,204,100,1)' :
                    x === xMaxx ? 'rgba(57,204,100,1)' : 'rgba(57,204,100,0)'),
                TowerBorderWidth: (x === 0 ? 6 : x === xMaxx ? 6 : 3),
                TowerBarThickness: (x === 0 ? 6 : x === xMaxx ? 6 : 3),
                Location: [Number(xElevations[x].split('&')[0]), Number(xElevations[x].split('&')[1])],
                Distance: xCdistanceKm,
                Elevation: xElevationm,
                LoS: xLoS,
                FresnelZone: xFresnelZone,
                FZTop: xFZTop,
                FZ6: xFZ6,
                FZ6Top: xFZ6Top
            });
        }
        if (!losBreak) {
            document.getElementById('effca').innerHTML = '';
            document.getElementById('ratea').innerHTML = round(datarateAMbps, 0);
            document.getElementById('leva').innerHTML = round(rxlevAdBm, 0);
            document.getElementById('effcb').innerHTML = '';
            document.getElementById('rateb').innerHTML = round(datarateBMbps, 0);
            document.getElementById('levb').innerHTML = round(rxlevBdBm, 0);
            document.getElementById('connectivityInfo').innerHTML = '';
        }
        else {
            document.getElementById('effca').innerHTML = '';
            document.getElementById('ratea').innerHTML = '';
            document.getElementById('leva').innerHTML = '';
            document.getElementById('effcb').innerHTML = '';
            document.getElementById('rateb').innerHTML = '';
            document.getElementById('levb').innerHTML = '';
            document.getElementById('connectivityInfo').innerHTML = 'Link Obstructed';
        }
        let chartData = {
            labels: xDataset.map(d => d.Distance),
            datasets: [
                {
                    type: 'bar',
                    label: 'Towers',
                    data: xDataset.map(d => d.Tower),
                    backgroundColor: xDataset.map(d => d.TowerBackgroundColor),
                    borderColor: xDataset.map(d => d.TowerBorderColor),
                    borderWidth: xDataset.map(d => d.TowerBorderWidth),
                    barThickness: xDataset.map(d => d.TowerBarThickness)
                }, {
                    label: 'Location',
                    data: xDataset.map(d => d.Location),
                    showLine: false
                }, {
                    label: 'Elevation',
                    data: xDataset.map(d => d.Elevation),
                    borderWidth: 0,
                    pointRadius: 0
                }, {
                    label: 'LosHeight',
                    data: xDataset.map(d => d.LoS),
                    borderWidth: 2,
                    pointRadius: 1,
                    borderColor: losBreak ? 'rgba(255, 0, 0, 0.7)' : 'rgba(114,119,122,1)',
                    fill: false
                }, {
                    label: 'FresnelZone',
                    data: xDataset.map(d => d.FresnelZone),
                    borderWidth: 1,
                    pointRadius: 0,
                    fill: false,
                    borderColor: fzoneBreak ? 'rgba(255,165,0,1)' : 'rgba(0,255,0,1)'
                }, {
                    label: 'FZTop',
                    data: xDataset.map(d => d.FZTop),
                    borderWidth: 1,
                    pointRadius: 0,
                    fill: false,
                    borderColor: fzoneBreak ? 'rgba(255,165,0,1)' : 'rgba(0,255,0,1)'
                }, {
                    label: 'FZ6',
                    data: xDataset.map(d => d.FZ6),
                    borderWidth: 1,
                    pointRadius: 0,
                    fill: false,
                    borderColor: f6zoneBreak ? 'rgba(255,165,0,1)' : 'rgba(0,255,0,1)',
                    borderDash: [7, 7]
                }, {
                    label: 'FZ6Top',
                    data: xDataset.map(d => d.FZ6Top),
                    borderWidth: 1,
                    pointRadius: 0,
                    fill: false,
                    borderColor: f6zoneBreak ? 'rgba(255,165,0,1)' : 'rgba(0,255,0,1)',
                    borderDash: [7, 7]
                }]
        };
        let elevChartContext = document.getElementById('chart1').getContext('2d');
        if (elevChart) { elevChart.destroy(); }
        elevChart = new Chart(elevChartContext, {
            type: 'LineWithLine',
            data: chartData,
            options: {
                animation: {
                    duration: 0
                },
                responsive: false,
                tooltips: {
                    enabled: false,
                    mode: 'index',
                    intersect: false,
                    custom: customTooltips
                },
                hover: {
                    mode: 'index',
                    intersect: false
                },
                onHover: function (e, elements) {
                    if (e) { e.stopPropagation(); }
                    if (elements) {
                        if (elements.length > 0) {
                            if (elements[0]._index) {
                                $("#chart1").css("cursor", elements[0] ? "pointer" : "default");
                                window.parent.highlightPoint(
                                    elevChart.data.datasets[1].data[elements[0]._index]);
                            }
                        }
                    }
                },
                legend: {
                    display: false
                },
                scales: {
                    xAxes: [{
                        display: false
                    }],
                    yAxes: [{
                        display: false,
                        ticks: {
                            min: minYaxis
                        }
                    }]
                }
            }
        });
    }
}
function saveLinkData(e, refreshGraph = false) {
    if (e) { e.stopPropagation(); }
    window.parent.updateLinkDataByIdx({
        idx: currLinkIdx,
        linkname: document.getElementById('linkname').value,
        heighta: document.getElementById('heighta').value,
        channelwidtha: document.getElementById('channelwidtha').value,
        frequencya: document.getElementById('frequencya').value,
        outputpowera: document.getElementById('outputpowera').value,
        antennagaina: document.getElementById('antennagaina').value,
        lossesa: document.getElementById('lossesa').value,
        heightb: document.getElementById('heightb').value,
        channelwidthb: document.getElementById('channelwidthb').value,
        frequencyb: document.getElementById('frequencyb').value,
        outputpowerb: document.getElementById('outputpowerb').value,
        antennagainb: document.getElementById('antennagainb').value,
        lossesb: document.getElementById('lossesb').value
    });
    window.parent.document.getElementById('linkOptionsFrame').contentWindow.refreshLinkData('cursor');
    window.parent.document.getElementById('siteOptionsFrame').contentWindow.refreshSiteData('cursor');
    refreshLinkData('cursor', null, refreshGraph);
}
function refreshLinkData(refreshType, Idx = null, refreshGraph = true) {
    let currLinkId = linkData === undefined ? '0' : linkData[0].toString();
    linkData = undefined; linksCount = '0';
    linksCount = window.parent.getLinksCount();
    if (Idx !== null) { currLinkIdx = Idx.toString(); } else {
        if (refreshType === 'start') {
            if (Number(linksCount) > 0) {
                currLinkIdx = window.parent.sLinksTree[0][0].toString();
            } else { currLinkIdx = '-1'; }
        }
        else if (refreshType === 'add') { /* add */
            if (Number(linksCount) > 0) {
                currLinkIdx = window.parent.sLinksTree[Number(linksCount) - 1][0].toString();
            } else { currLinkIdx = '-1'; }
        }
        else if (refreshType === 'cursor') { /* update */
            if (Number(linksCount) > 0) {
                currLinkIdx = window.parent.getLinkIdxById(currLinkId).toString();
            } else { currLinkIdx = '-1'; }
        }
        else if (refreshType === 'delete') /*delete*/ {
            if (Number(linksCount) > 0) {
                if (deleteLinkIdx > Number(linksCount) - 1) {
                    currLinkIdx = window.parent.sLinksTree[Number(linksCount) - 1][0].toString();
                } else {
                    if (deleteLinkIdx > -1) {
                        currLinkIdx = window.parent.sLinksTree[deleteLinkIdx][0].toString();
                    } else { currLinkIdx = '-1'; }                    
                }
            } else { currLinkIdx = '-1'; }
        }
    }
    linkData = window.parent.getLinkProfileByIdx(currLinkIdx).split('\t');
    let linktype = linkData[2] === undefined ? '' : linkData[2].toString();
    let siteida = linkData[3] === undefined ? -1 : linkData[3].toString();
    let siteidb = linkData[12] === undefined ? -1 : linkData[12].toString();
    let email = linkData[24] === undefined ? '' : linkData[24].toString();
    siteAData = window.parent.getSiteDataById(siteida).split('\t');
    if (linktype === 'public') {
        siteBData = window.parent.getPublicSiteData(siteidb, email).split('\t');
    } else {
        siteBData = window.parent.getSiteDataById(siteidb).split('\t');
    }
    fillLinkData(linkData, refreshGraph);
}
function refreshLinkDataById(linkid) {
    refreshLinkData('', window.parent.getLinkIdxById(linkid).toString());
}
function pop(item) {
    switch (item) {
        case 'heighta':
            document.getElementById('ddfrequencya').style.display = 'none';
            document.getElementById('ddchannelwidtha').style.display = 'none';
            document.getElementById('ddoutputpowera').style.display = 'none';
            document.getElementById('ddantennagaina').style.display = 'none';
            document.getElementById('ddlossesa').style.display = 'none';
            document.getElementById('ddfrequencyb').style.display = 'none';
            document.getElementById('ddchannelwidthb').style.display = 'none';
            document.getElementById('ddoutputpowerb').style.display = 'none';
            document.getElementById('ddantennagainb').style.display = 'none';
            document.getElementById('ddlossesb').style.display = 'none';
            document.getElementById('ddheightb').style.display = 'none';
            if (document.getElementById('ddheighta').style.display === 'none') {
                document.getElementById('ddheighta').style.display = 'block';
            } else {
                document.getElementById('ddheighta').style.display = 'none';
            }
            break;
        case 'heightb':
            document.getElementById('ddfrequencya').style.display = 'none';
            document.getElementById('ddchannelwidtha').style.display = 'none';
            document.getElementById('ddoutputpowera').style.display = 'none';
            document.getElementById('ddantennagaina').style.display = 'none';
            document.getElementById('ddlossesa').style.display = 'none';
            document.getElementById('ddfrequencyb').style.display = 'none';
            document.getElementById('ddchannelwidthb').style.display = 'none';
            document.getElementById('ddoutputpowerb').style.display = 'none';
            document.getElementById('ddantennagainb').style.display = 'none';
            document.getElementById('ddlossesb').style.display = 'none';
            document.getElementById('ddheighta').style.display = 'none';
            if (document.getElementById('ddheightb').style.display === 'none') {
                document.getElementById('ddheightb').style.display = 'block';
            } else {
                document.getElementById('ddheightb').style.display = 'none';
            }
            break;
        case 'frequencya':
            document.getElementById('ddchannelwidtha').style.display = 'none';
            document.getElementById('ddoutputpowera').style.display = 'none';
            document.getElementById('ddantennagaina').style.display = 'none';
            document.getElementById('ddlossesa').style.display = 'none';
            document.getElementById('ddfrequencyb').style.display = 'none';
            document.getElementById('ddchannelwidthb').style.display = 'none';
            document.getElementById('ddoutputpowerb').style.display = 'none';
            document.getElementById('ddantennagainb').style.display = 'none';
            document.getElementById('ddlossesb').style.display = 'none';
            document.getElementById('ddheighta').style.display = 'none';
            document.getElementById('ddheightb').style.display = 'none';
            if (document.getElementById('ddfrequencya').style.display === 'none') {
                document.getElementById('ddfrequencya').style.display = 'block';
            } else {
                document.getElementById('ddfrequencya').style.display = 'none';
            }
            break;
        case 'channelwidtha':
            document.getElementById('ddfrequencya').style.display = 'none';
            document.getElementById('ddoutputpowera').style.display = 'none';
            document.getElementById('ddantennagaina').style.display = 'none';
            document.getElementById('ddlossesa').style.display = 'none';
            document.getElementById('ddfrequencyb').style.display = 'none';
            document.getElementById('ddchannelwidthb').style.display = 'none';
            document.getElementById('ddoutputpowerb').style.display = 'none';
            document.getElementById('ddantennagainb').style.display = 'none';
            document.getElementById('ddlossesb').style.display = 'none';
            document.getElementById('ddheighta').style.display = 'none';
            document.getElementById('ddheightb').style.display = 'none';
            if (document.getElementById('ddchannelwidtha').style.display === 'none') {
                document.getElementById('ddchannelwidtha').style.display = 'block';
            } else {
                document.getElementById('ddchannelwidtha').style.display = 'none';
            }
            break;
        case 'outputpowera':
            document.getElementById('ddchannelwidtha').style.display = 'none';
            document.getElementById('ddfrequencya').style.display = 'none';
            document.getElementById('ddantennagaina').style.display = 'none';
            document.getElementById('ddlossesa').style.display = 'none';
            document.getElementById('ddfrequencyb').style.display = 'none';
            document.getElementById('ddchannelwidthb').style.display = 'none';
            document.getElementById('ddoutputpowerb').style.display = 'none';
            document.getElementById('ddantennagainb').style.display = 'none';
            document.getElementById('ddlossesb').style.display = 'none';
            document.getElementById('ddheighta').style.display = 'none';
            document.getElementById('ddheightb').style.display = 'none';
            if (document.getElementById('ddoutputpowera').style.display === 'none') {
                document.getElementById('ddoutputpowera').style.display = 'block';
            } else {
                document.getElementById('ddoutputpowera').style.display = 'none';
            }
            break;
        case 'antennagaina':
            document.getElementById('ddchannelwidtha').style.display = 'none';
            document.getElementById('ddoutputpowera').style.display = 'none';
            document.getElementById('ddfrequencya').style.display = 'none';
            document.getElementById('ddlossesa').style.display = 'none';
            document.getElementById('ddfrequencyb').style.display = 'none';
            document.getElementById('ddchannelwidthb').style.display = 'none';
            document.getElementById('ddoutputpowerb').style.display = 'none';
            document.getElementById('ddantennagainb').style.display = 'none';
            document.getElementById('ddlossesb').style.display = 'none';
            document.getElementById('ddheighta').style.display = 'none';
            document.getElementById('ddheightb').style.display = 'none';
            if (document.getElementById('ddantennagaina').style.display === 'none') {
                document.getElementById('ddantennagaina').style.display = 'block';
            } else {
                document.getElementById('ddantennagaina').style.display = 'none';
            }
            break;
        case 'lossesa':
            document.getElementById('ddchannelwidtha').style.display = 'none';
            document.getElementById('ddoutputpowera').style.display = 'none';
            document.getElementById('ddantennagaina').style.display = 'none';
            document.getElementById('ddfrequencya').style.display = 'none';
            document.getElementById('ddfrequencyb').style.display = 'none';
            document.getElementById('ddchannelwidthb').style.display = 'none';
            document.getElementById('ddoutputpowerb').style.display = 'none';
            document.getElementById('ddantennagainb').style.display = 'none';
            document.getElementById('ddlossesb').style.display = 'none';
            document.getElementById('ddheighta').style.display = 'none';
            document.getElementById('ddheightb').style.display = 'none';
            if (document.getElementById('ddlossesa').style.display === 'none') {
                document.getElementById('ddlossesa').style.display = 'block';
            } else {
                document.getElementById('ddlossesa').style.display = 'none';
            }
            break;
        case 'frequencyb':
            document.getElementById('ddchannelwidthb').style.display = 'none';
            document.getElementById('ddoutputpowerb').style.display = 'none';
            document.getElementById('ddantennagainb').style.display = 'none';
            document.getElementById('ddlossesb').style.display = 'none';
            document.getElementById('ddfrequencya').style.display = 'none';
            document.getElementById('ddchannelwidtha').style.display = 'none';
            document.getElementById('ddoutputpowera').style.display = 'none';
            document.getElementById('ddantennagaina').style.display = 'none';
            document.getElementById('ddlossesa').style.display = 'none';
            document.getElementById('ddheighta').style.display = 'none';
            document.getElementById('ddheightb').style.display = 'none';
            if (document.getElementById('ddfrequencyb').style.display === 'none') {
                document.getElementById('ddfrequencyb').style.display = 'block';
            } else {
                document.getElementById('ddfrequencyb').style.display = 'none';
            }
            break;
        case 'channelwidthb':
            document.getElementById('ddfrequencyb').style.display = 'none';
            document.getElementById('ddoutputpowerb').style.display = 'none';
            document.getElementById('ddantennagainb').style.display = 'none';
            document.getElementById('ddlossesb').style.display = 'none';
            document.getElementById('ddfrequencya').style.display = 'none';
            document.getElementById('ddchannelwidtha').style.display = 'none';
            document.getElementById('ddoutputpowera').style.display = 'none';
            document.getElementById('ddantennagaina').style.display = 'none';
            document.getElementById('ddlossesa').style.display = 'none';
            document.getElementById('ddheighta').style.display = 'none';
            document.getElementById('ddheightb').style.display = 'none';
            if (document.getElementById('ddchannelwidthb').style.display === 'none') {
                document.getElementById('ddchannelwidthb').style.display = 'block';
            } else {
                document.getElementById('ddchannelwidthb').style.display = 'none';
            }
            break;
        case 'outputpowerb':
            document.getElementById('ddchannelwidthb').style.display = 'none';
            document.getElementById('ddfrequencyb').style.display = 'none';
            document.getElementById('ddantennagainb').style.display = 'none';
            document.getElementById('ddlossesb').style.display = 'none';
            document.getElementById('ddfrequencya').style.display = 'none';
            document.getElementById('ddchannelwidtha').style.display = 'none';
            document.getElementById('ddoutputpowera').style.display = 'none';
            document.getElementById('ddantennagaina').style.display = 'none';
            document.getElementById('ddlossesa').style.display = 'none';
            document.getElementById('ddheighta').style.display = 'none';
            document.getElementById('ddheightb').style.display = 'none';
            if (document.getElementById('ddoutputpowerb').style.display === 'none') {
                document.getElementById('ddoutputpowerb').style.display = 'block';
            } else {
                document.getElementById('ddoutputpowerb').style.display = 'none';
            }
            break;
        case 'antennagainb':
            document.getElementById('ddchannelwidthb').style.display = 'none';
            document.getElementById('ddoutputpowerb').style.display = 'none';
            document.getElementById('ddfrequencyb').style.display = 'none';
            document.getElementById('ddlossesb').style.display = 'none';
            document.getElementById('ddfrequencya').style.display = 'none';
            document.getElementById('ddchannelwidtha').style.display = 'none';
            document.getElementById('ddoutputpowera').style.display = 'none';
            document.getElementById('ddantennagaina').style.display = 'none';
            document.getElementById('ddlossesa').style.display = 'none';
            document.getElementById('ddheighta').style.display = 'none';
            document.getElementById('ddheightb').style.display = 'none';
            if (document.getElementById('ddantennagainb').style.display === 'none') {
                document.getElementById('ddantennagainb').style.display = 'block';
            } else {
                document.getElementById('ddantennagainb').style.display = 'none';
            }
            break;
        case 'lossesb':
            document.getElementById('ddchannelwidthb').style.display = 'none';
            document.getElementById('ddoutputpowerb').style.display = 'none';
            document.getElementById('ddantennagainb').style.display = 'none';
            document.getElementById('ddfrequencyb').style.display = 'none';
            document.getElementById('ddfrequencya').style.display = 'none';
            document.getElementById('ddchannelwidtha').style.display = 'none';
            document.getElementById('ddoutputpowera').style.display = 'none';
            document.getElementById('ddantennagaina').style.display = 'none';
            document.getElementById('ddlossesa').style.display = 'none';
            document.getElementById('ddheighta').style.display = 'none';
            document.getElementById('ddheightb').style.display = 'none';
            if (document.getElementById('ddlossesb').style.display === 'none') {
                document.getElementById('ddlossesb').style.display = 'block';
            } else {
                document.getElementById('ddlossesb').style.display = 'none';
            }
            break;
        default: break;
    }
}