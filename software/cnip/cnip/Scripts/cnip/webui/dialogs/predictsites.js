function showPredictSites(e) {
    if (e) { e.stopPropagation(); }
    if (!document.getElementById('pnDiv')) {
        let psDiv = document.createElement('div');
        let psTable = document.createElement('table');
        let psTBody = document.createElement('tbody');
        let psTr = document.createElement('tr');
        let psTd1 = document.createElement('td');
        let psTitleDiv = document.createElement('div');
        psTd1.appendChild(psTitleDiv);
        let psTd2 = document.createElement('td');
        let psCloseDiv = document.createElement('div');
        let psCloseButton = document.createElement('button');
        let psCloseButtonI = document.createElement('i');
        psCloseButton.appendChild(psCloseButtonI);
        psCloseDiv.appendChild(psCloseButton);
        psTd2.appendChild(psCloseDiv);
        psTr.appendChild(psTd1);
        psTr.appendChild(psTd2);
        psTBody.appendChild(psTr);
        psTable.appendChild(psTBody);
        psDiv.appendChild(psTable);
        let psBodyDiv = document.createElement('div');
        let psPolygonDiv = document.createElement('div');
        let psTechnologyDiv = document.createElement('div');
        psBodyDiv.appendChild(document.createTextNode('Select Polygon'));
        psBodyDiv.appendChild(psPolygonDiv);
        psBodyDiv.appendChild(document.createTextNode('Select Technology'));
        psBodyDiv.appendChild(psTechnologyDiv);
        psButtonCancel = document.createElement('button');
        psBodyDiv.appendChild(psButtonCancel);
        psButtonRun = document.createElement('button');
        psBodyDiv.appendChild(psButtonRun);
        psDiv.appendChild(psBodyDiv);
        document.getElementById('myMap').insertBefore(psDiv,
            document.getElementsByClassName('ol-viewport')[0]);
        psDiv.id = 'pnDiv';
        psDiv.className = 'predictSites panel panel-primary draggable';
        psDiv.style.display = 'block';
        psDiv.onclick = function (e) {
            if (e) { e.stopPropagation(); }
            setDialogZIndex(e, 'pnDiv');
        };
        psTable.style.width = '100%';
        psTd1.style.width = '98%';
        psTd2.style.width = '2%';
        psTitleDiv.id = psDiv.id + 'Title';
        psTitleDiv.className = 'panel-heading draggable-handler';
        psTitleDiv.innerHTML = '&nbsp;&nbsp;&nbsp;Predict Sites Options...';
        psCloseDiv.className = 'panel-action';
        psCloseButton.className = 'panel-action-button';
        psCloseButton.title = 'Close';
        psCloseButtonI.className = 'fas fa-window-close';
        psCloseButton.onclick = function (e) {
            if (e) { e.stopPropagation(); }
            closePredictSites();
        };
        psBodyDiv.className = 'panel-body';
        psBodyDiv.style.width =
            (Number(psDiv.style.width.replace('px')) - 6).toString() + 'px';
        psBodyDiv.style.height =
            (Number(psDiv.style.height.replace('px')) - 37).toString() + 'px';
        psBodyDiv.style.padding = '15px 10px';
        psBodyDiv.style.color = 'var(--fontcolor)';
        psBodyDiv.style.fontSize = '12px';
        psPolygonDiv.id = 'pnPolygon';
        psTechnologyDiv.id = 'pnTechnology';
        psButtonRun.className = 'btn btn-default';
        psButtonRun.style.position = 'absolute';
        psButtonRun.style.right = '10px';
        psButtonRun.style.bottom = '10px';
        psButtonRun.title = 'Run';
        psButtonRun.innerHTML = 'Run';
        psButtonRun.onclick = function (e) {
            if (e) { e.stopPropagation(); }
            var testPolygon = undefined;
            var testTechnology = undefined;
            testTechnology = document.getElementById('pnTechnology').value;
            let feature = myPolygonsLayer.getSource().getFeatureByProperty('polygonname',
                document.getElementById('pnPolygon').value);
            if (feature) {
                if (feature.get('polygonid') !== undefined) {
                    testPolygon = feature.get('polygonid');
                }
            }
            closePredictSites();
            if (testPolygon) {
                analysePredictSites(testPolygon, testTechnology);
            }
        };
        psButtonCancel.className = 'btn btn-default';
        psButtonCancel.style.position = 'absolute';
        psButtonCancel.style.right = '70px';
        psButtonCancel.style.bottom = '10px';
        psButtonCancel.title = 'Cancel';
        psButtonCancel.innerHTML = 'Cancel';
        psButtonCancel.onclick = function (e) {
            if (e) { e.stopPropagation(); }
            closePredictSites();
        };
        let polygons = myPolygonsLayer.getSource().getFeatures();
        let polygonsList = "";
        for (let i = 0; i < polygons.length; i++) {
            polygonsList += polygons[i].getProperties().polygonname + '\t';
        }
        polygonsList = polygonsList.substr(0, polygonsList.length - 1);
        initSelectOpt(psPolygonDiv, '100%', polygonsList, '');
        initSelectOpt(psTechnologyDiv, '100%', '2G\t4G', '');
        // set draggable
        setDraggable();
        setDialogZIndex(null, psDiv.id);
    }
}
function closePredictSites() {
    let psd = document.getElementById('pnDiv');
    if (psd) {
        while (psd.firstChild) {
            psd.firstChild.remove();
        }
        document.getElementById('myMap').removeChild(psd);
    }
}