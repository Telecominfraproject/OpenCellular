var ddsCount = 0;
var ddsContentData = [];
var ddsContentExtension = [];
var ddsContentChart = [];
var ddsContentChartPngString = [];
function ddsCreate(title, loadObj) {
    let dsDiv = document.createElement('div');
    let dsTable = document.createElement('table');
    let dsTBody = document.createElement('tbody');
    let dsTr = document.createElement('tr');
    let dsTd1 = document.createElement('td');
    let dsTitleDiv = document.createElement('div');
    dsTd1.appendChild(dsTitleDiv);
    let dsTd21 = document.createElement('td');
    let dsMinDiv = document.createElement('div');
    let dsMinButton = document.createElement('button');
    let dsMinButtonI = document.createElement('i');
    dsMinButton.appendChild(dsMinButtonI);
    dsMinDiv.appendChild(dsMinButton);
    dsTd21.appendChild(dsMinDiv);
    let dsTd22 = document.createElement('td');
    let dsMaxDiv = document.createElement('div');
    let dsMaxButton = document.createElement('button');
    let dsMaxButtonI = document.createElement('i');
    dsMaxButton.appendChild(dsMaxButtonI);
    dsMaxDiv.appendChild(dsMaxButton);
    dsTd22.appendChild(dsMaxDiv);
    let dsTd2 = document.createElement('td');
    let dsCloseDiv = document.createElement('div');
    let dsCloseButton = document.createElement('button');
    let dsCloseButtonI = document.createElement('i');
    dsCloseButton.appendChild(dsCloseButtonI);
    dsCloseDiv.appendChild(dsCloseButton);
    dsTd2.appendChild(dsCloseDiv);
    dsTr.appendChild(dsTd1);
    dsTr.appendChild(dsTd21);
    dsTr.appendChild(dsTd22);
    dsTr.appendChild(dsTd2);
    dsTBody.appendChild(dsTr);
    dsTable.appendChild(dsTBody);
    dsDiv.appendChild(dsTable);
    let dsBodyDiv = document.createElement('div');
    let dsBodyTabLinkDiv = document.createElement('div');
    let dsBodyTabContentDiv = document.createElement('div');
    let dsBodyTabLinks = [];
    let dsBodyTabContents = [];
    for (let i = 0; i < loadObj.length; i++) {
        dsBodyTabLinks.push(document.createElement('button'));
        dsBodyTabContents.push(document.createElement('div'));
        dsBodyTabLinkDiv.appendChild(dsBodyTabLinks[i]);
        dsBodyTabContentDiv.appendChild(dsBodyTabContents[i]);
    }
    dsBodyTopButton = document.createElement('button');
    let dsBodyTopButtonI = document.createElement('i');
    dsBodyTopButton.appendChild(dsBodyTopButtonI);
    dsBodyTabLinkDiv.appendChild(dsBodyTopButton);
    dsBodyTabLinkDiv.appendChild(document.createElement('hr'));
    dsBodyDiv.appendChild(dsBodyTabLinkDiv);
    dsBodyDiv.appendChild(dsBodyTabContentDiv);
    dsDiv.appendChild(dsBodyDiv);
    document.getElementById('myMap').insertBefore(dsDiv, document.getElementsByClassName('ol-viewport')[0]);
    dsDiv.id = 'dds' + ddsCount.toString();
    dsDiv.className = 'datasheet panel panel-primary draggable';
    dsDiv.style.display = 'block';
    dsDiv.onclick = function (e) {
        setDialogZIndex(e, this.value);
    };
    dsDiv.value = dsDiv.id;
    dsTable.style.width = '100%';
    dsTd1.style.width = '94%';
    dsTd21.style.width = '2%';
    dsTd22.style.width = '2%';
    dsTd2.style.width = '2%';
    dsTitleDiv.id = 'ch' + dsDiv.id + 'Title';
    dsTitleDiv.className = 'panel-heading draggable-handler';
    dsTitleDiv.innerHTML = '&nbsp;&nbsp;&nbsp;' + title + '...';
    dsMinDiv.className = 'panel-action';
    dsMinButton.className = 'panel-action-button';
    dsMinButton.title = 'Minimize';
    dsMinButtonI.className = 'fas fa-window-minimize';
    dsMinButton.onclick = function (e) {
        minimizeDialog(e, 'dds' + this.value.toString());
    };
    dsMinButton.value = ddsCount;
    dsMaxDiv.className = 'panel-action';
    dsMaxButton.className = 'panel-action-button';
    dsMaxButton.title = 'Restore';
    dsMaxButtonI.className = 'fas fa-window-restore';
    dsMaxButton.onclick = function (e) {
        restoreDialog(e, 'dds' + this.value.toString());
    };
    dsMaxButton.value = ddsCount;
    dsCloseDiv.className = 'panel-action';
    dsCloseButton.className = 'panel-action-button';
    dsCloseButton.title = 'Close';
    dsCloseButtonI.className = 'fas fa-window-close';
    dsCloseButton.onclick = function (e) {
        if (e) { e.stopPropagation(); }
        ddsDestroy(this.value);
    };
    dsCloseButton.value = ddsCount;
    dsBodyDiv.className = 'panel-body';
    dsBodyDiv.style.width = (Number(dsDiv.style.width.replace('px')) - 6).toString() + 'px';
    dsBodyDiv.style.height = (Number(dsDiv.style.height.replace('px')) - 37).toString() + 'px';
    dsBodyTabLinkDiv.style.overflow = 'hidden';
    dsBodyTabContentDiv.style.overflow = 'auto';
    dsBodyTabContentDiv.style.height = (360 - (Math.ceil(loadObj.length / 5) - 1) * 27.14).toString() + 'px';
    dsBodyTopButtonI.className = 'fas fa-file-csv'; //fas fa-chart-line
    dsBodyTopButtonI.id = 'ch' + dsDiv.id + 'TopButton'; //fas fa-chart-line
    dsBodyTopButton.className = 'pageTopButton';
    dsBodyTopButton.title = 'Download Content';
    dsBodyTopButton.onclick = function (e) {
        if (e) { e.stopPropagation(); }
        ddsDownloadActiveContent(this.value);
    };
    dsBodyTopButton.value = ddsCount;
    ddsContentData.push([ddsCount]);
    ddsContentExtension.push([ddsCount]);
    ddsContentChart.push([ddsCount]);
    ddsContentChartPngString.push([ddsCount]);
    for (let i = 0; i < loadObj.length; i++) {
        dsBodyTabLinks[i].className = 'pageTabLink';
        dsBodyTabLinks[i].onclick = function (e) {
            if (e) { e.stopPropagation(); }
            ddsOpenPage(this.value, this.index, this);
        };
        dsBodyTabLinks[i].value = ddsCount;
        dsBodyTabLinks[i].index = i;
        dsBodyTabLinks[i].id = 'ch' + dsDiv.id + 'TabLink' + i.toString();
        dsBodyTabLinks[i].style.display = 'block';
        dsBodyTabLinks[i].innerHTML = loadObj[i].title;
        dsBodyTabLinks[i].style.fontSize = '12px';
        dsBodyTabContents[i].className = 'pageTabContent';
        dsBodyTabContents[i].id = 'ch' + dsDiv.id + 'TabContent' + i.toString();
        dsBodyTabContents[i].style.padding = loadObj[i].contentPadding;
        dsBodyTabContents[i].style.fontSize = '12px';
        ddsContentData[ddsCount][i] = '';
        ddsContentExtension[ddsCount][i] = '';
        ddsContentChart[ddsCount][i] = '';
        ddsContentChartPngString[ddsCount][i] = '';
        if (loadObj[i].contentType === 'chart') {
            ddsContentChartPngString[ddsCount][i] = loadObj[i].pngString;
            dsBodyTabContents[i].contentType = 'chart';
            let contentTitleDiv = document.createElement('div');
            let chDiv = document.createElement('div');
            let chCanvas = document.createElement('canvas');
            dsBodyTabContents[i].appendChild(contentTitleDiv);
            chDiv.appendChild(chCanvas);
            dsBodyTabContents[i].appendChild(chDiv);
            contentTitleDiv.style.textAlign = 'center';
            contentTitleDiv.style.color =
                loadObj[i].contentTitle.includes('Passed') ? 'green' :
                    loadObj[i].contentTitle.includes('Failed') ? 'red' : 'blue';
            contentTitleDiv.innerHTML = loadObj[i].contentTitle;
            chCanvas.id = 'ch' + dsDiv.id + 'Chart' + i.toString();
            chCanvas.width = '570';
            chCanvas.height = '342'; // should be 60% of width
            chCanvas.style.msUserSelect = 'none';
            chCanvas.style.userSelect = 'none';
            chCanvas.style.webkitUserSelect = 'none';
            let xDataset = [];
            let dataString = loadObj[i].dataString; //rate&value&r,g,b
            dataString.split('$').forEach(function (d) {
                xDataset.push({
                    Label: d.split('&')[0],
                    Data: d.split('&')[1],
                    BackgroundColor: getColorFromLabel(d.split('&')[2], true),
                    BorderColor: getColorFromLabel(d.split('&')[2], false)
                });
            });
            let chartData = {
                labels: xDataset.map(d => d.Label),
                datasets: [
                    {
                        type: 'bar',
                        label: loadObj[i].title,
                        data: xDataset.map(d => d.Data),
                        backgroundColor: xDataset.map(d => d.BackgroundColor),
                        borderColor: xDataset.map(d => d.BorderColor)
                    }]
            };
            let chartContext = document.getElementById(chCanvas.id).getContext('2d');
            ddsContentChart[ddsCount][i] = new Chart(chartContext, {
                type: 'bar',
                data: chartData,
                options: {
                    animation: {
                        duration: 0
                    },
                    responsive: false,
                    tooltips: {
                        enabled: true,
                        mode: 'index',
                        intersect: false,
                        position: 'average'
                    },
                    legend: {
                        display: false
                    },
                    scales: {
                        xAxes: [{
                            gridLines: {
                                drawOnChartArea: false
                            }
                        }],
                        yAxes: [{
                            gridLines: {
                                drawOnChartArea: false
                            },
                            scaleLabel: {
                                display: true,
                                labelString: '% Polygon Area Covered'
                            }
                        }]
                    }
                }
            });
        } else {
            dsBodyTabContents[i].contentType = 'table';
            dsBodyTabContents[i].innerHTML = loadObj[i].contentHTML;
            ddsContentData[ddsCount][i] = loadObj[i].contentData;
            ddsContentExtension[ddsCount][i] = loadObj[i].contentExtension;
        }
    }
    if (dsBodyTabLinks[0]) { dsBodyTabLinks[0].click(); }
    setDialogZIndex(null, dsDiv.id);
    ddsCount += 1;
    // set draggable
    setDraggable();
}
function ddsOpenPage(ddsId, tabContentId, tabLink) {
    $('button[id^="chdds' + ddsId.toString() + 'TabLink"]').css({ 'background-color': '', 'color': '' });
    $('div[id^="chdds' + ddsId.toString() + 'TabContent"]').css('display', 'none');
    $('#chdds' + ddsId.toString() + 'TabContent' + tabContentId.toString()).css('display', 'block');
    if (document.getElementById('chdds' + ddsId.toString() + 'TabContent' + tabContentId.toString()).contentType === 'chart') {
        document.getElementById('chdds' + ddsId.toString() + 'TopButton').className = 'fas fa-chart-line';
        if (ddsContentChartPngString[ddsId][tabContentId] !== '') {
            loadRaster(ddsContentChartPngString[ddsId][tabContentId]);
        }
    } else {
        document.getElementById('chdds' + ddsId.toString() + 'TopButton').className = 'fas fa-file-csv';
    }
    tabLink.style.backgroundColor = '#555';
    tabLink.style.color = 'white';
}
function ddsDestroyAll() {
    for (let i = 0; i < ddsCount; i++) {
        for (let j = 0; j < $('div[id^="chdds' + i.toString() + 'TabContent"]').length; j++) {
            if (ddsContentChart[i][j].hasOwnProperty('destroy')) { ddsContentChart[i][j].destroy; }
        }
    }
    for (let i = 0; i < ddsCount; i++) {
        let dds = document.getElementById('dds' + i.toString());
        if (dds) {
            while (dds.firstChild) {
                dds.firstChild.remove();
            }
            document.getElementById('myMap').removeChild(dds);
        }
    }
    ddsCount = 0; ddsContentChart = [];
    ddsContentData = []; ddsContentExtension = [];
}
function ddsDestroy(ddsId) {
    for (let j = 0; j < $('div[id^="chdds' + ddsId.toString() + 'TabContent"]').length; j++) {
        if (ddsContentChart[ddsId][j].hasOwnProperty('destroy')) { ddsContentChart[i][j].destroy; }
        ddsContentChart[ddsId][j] = '';
        ddsContentData[ddsId][j] = '';
        ddsContentExtension[ddsId][j] = '';
    }
    let dds = document.getElementById('dds' + ddsId.toString());
    if (dds) {
        while (dds.firstChild) {
            dds.firstChild.remove();
        }
        document.getElementById('myMap').removeChild(dds);
    }
}
function ddsDownloadActiveContent(ddsId) {
    let tabContent = $('div[id^="chdds' + ddsId.toString() + 'TabContent"]');
    for (i = 0; i < tabContent.length; i++) {
        if (tabContent[i].style.display === 'block') {
            if (tabContent[i].contentType === 'chart') {
                let base64image = document.getElementById('chdds' + ddsId.toString() + 'Chart' + i.toString()).toDataURL("image/png");
                let block = base64image.split(";");
                let mimeType = block[0].split(":")[1];
                let realData = block[1].split(",")[1];
                let canvasBlob = b64toBlob(realData, mimeType);
                saveAs(canvasBlob, "chart.png");
            } else {
                let blob = new Blob([ddsContentData[ddsId][i]], { type: "text/plain;charset=utf-8" });
                saveAs(blob, $('#chdds' + ddsId.toString() + 'TabLink' + i.toString()).html() +
                    ddsContentExtension[ddsId][i]);
            }
        }
    }
}
function getColorFromLabel(label, background) {
    let color = 'rgba(0,0,0,0)';
    let rgb = label.split(',');
    color = 'rgba('
        + rgb[0] + ','
        + rgb[1] + ','
        + rgb[2] + ','
        + (background ? '0.8' : '1')
        + ')';
    return color;
}