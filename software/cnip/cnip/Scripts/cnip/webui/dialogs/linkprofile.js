/* global variables*/
var userDisplay = false;
var linkData;
var siteAData;
var siteBData;
var linksCount;
var currLink;
var elevChart;
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
function fillLinkData(linkData) {
    document.getElementById('linkid').innerHTML = linkData[0].toString();
    document.getElementById('linkname').innerHTML = linkData[1] === undefined ? '' : linkData[1].toString();
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
    document.getElementById('deviceheighta').value =
        round(Number(siteAData[4] === undefined ? '' : siteAData[4].toString()), 0).toString() + ' / ' +
        round(Number(linkData[4] === undefined ? '' : linkData[4].toString()), 0).toString();
    document.getElementById('channelwidtha').value = linkData[6] === undefined ? '' : linkData[6].toString();
    document.getElementById('frequencya').value = linkData[7] === undefined ? '' : linkData[7].toString();
    document.getElementById('outputpowera').value = linkData[8] === undefined ? '' : linkData[8].toString();
    document.getElementById('antennagaina').value = linkData[9] === undefined ? '' : linkData[9].toString();
    document.getElementById('lossesa').value = linkData[10] === undefined ? '' : linkData[10].toString();
    document.getElementById('siteidb').value = linkData[11] === undefined ? '' : linkData[11].toString();
    document.getElementById('sitenameb').value = siteBData[1] === undefined ? '' : siteBData[1].toString();
    document.getElementById('deviceheightb').value =
        round(Number(siteBData[4] === undefined ? '' : siteBData[4].toString()), 0).toString() + ' / ' +
        round(Number(linkData[12] === undefined ? '' : linkData[12].toString()), 0).toString();
    document.getElementById('channelwidthb').value = linkData[14] === undefined ? '' : linkData[14].toString();
    document.getElementById('frequencyb').value = linkData[15] === undefined ? '' : linkData[15].toString();
    document.getElementById('outputpowerb').value = linkData[16] === undefined ? '' : linkData[16].toString();
    document.getElementById('antennagainb').value = linkData[17] === undefined ? '' : linkData[17].toString();
    document.getElementById('lossesb').value = linkData[18] === undefined ? '' : linkData[18].toString();
    document.getElementById('distance').innerHTML =
        round(Number(linkData[19] === undefined ? '' : linkData[19].toString()), 1).toString() + ' Km';
    let xDistanceKm = Number(linkData[19] === undefined ? '' : linkData[19].toString());
    let deviceheightAm = Number(siteAData[4] === undefined ? '' : siteAData[4].toString()) +
        Number(linkData[4] === undefined ? '' : linkData[4].toString());
    let deviceheightBm = Number(siteBData[4] === undefined ? '' : siteBData[4].toString()) +
        Number(linkData[12] === undefined ? '' : linkData[12].toString());
    let tiltAd = 0; tiltBd = 0;
    if (deviceheightAm > deviceheightBm) {
        tiltAd = round(turf.radiansToDegrees(Math.atan(deviceheightAm / (xDistanceKm * 1000))), 2);
        tiltBd = round(turf.radiansToDegrees(-1 * Math.atan((deviceheightAm - deviceheightBm) / (xDistanceKm * 1000))), 2);
    }
    else {
        tiltAd = round(turf.radiansToDegrees(-1 * Math.atan((deviceheightBm - deviceheightAm) / (xDistanceKm * 1000))), 2);
        tiltBd = round(turf.radiansToDegrees(Math.atan(deviceheightBm / (xDistanceKm * 1000))), 2);
    }
    document.getElementById('headinga').value =
        round(Number(linkData[5] === undefined ? '' : linkData[5].toString()), 2).toString() + ' / ' + tiltAd;
    document.getElementById('headingb').value =
        round(Number(linkData[13] === undefined ? '' : linkData[13].toString()), 2).toString() + ' / ' + tiltBd;
    let xFrequencyGHz = Number(linkData[7] === undefined ? '' : linkData[7].toString());
    let channelwidthMHz = Number(linkData[6] === undefined ? '' : linkData[6].toString());
    let lossesAdB = Number(linkData[10] === undefined ? '' : linkData[10].toString());
    let lossesBdB = Number(linkData[18] === undefined ? '' : linkData[18].toString());
    let antennagainAdBi = Number(linkData[9] === undefined ? '' : linkData[9].toString());
    let antennagainBdBi = Number(linkData[17] === undefined ? '' : linkData[17].toString());
    let outputpowerAdBm = Number(linkData[8] === undefined ? '' : linkData[8].toString());
    let outputpowerBdBm = Number(linkData[16] === undefined ? '' : linkData[16].toString());
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
    let xElevations = linkData[20] === undefined ? '' : linkData[20].toString().split('@');
    let xDataset = [];
    let minYaxis = 50000;
    let losBreak = false;
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
    linkData = window.parent.getLinkProfileByIdx(currLink).split('\t');
    let siteida = linkData[3] === undefined ? -1 : linkData[3].toString();
    let siteidb = linkData[11] === undefined ? -1 : linkData[11].toString();
    let linktype = linkData[2] === undefined ? '' : linkData[2].toString();
    let email = linkData[22] === undefined ? '' : linkData[22].toString();
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