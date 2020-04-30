function showPredictBestCan(e) {
    if (e) { e.stopPropagation(); }
    if (!document.getElementById('pbcDiv')) {
        let bcDiv = document.createElement('div');
        let bcTable = document.createElement('table');
        let bcTBody = document.createElement('tbody');
        let bcTr = document.createElement('tr');
        let bcTd1 = document.createElement('td');
        let bcTitleDiv = document.createElement('div');
        bcTd1.appendChild(bcTitleDiv);
        let bcTd2 = document.createElement('td');
        let bcCloseDiv = document.createElement('div');
        let bcCloseButton = document.createElement('button');
        let bcCloseButtonI = document.createElement('i');
        bcCloseButton.appendChild(bcCloseButtonI);
        bcCloseDiv.appendChild(bcCloseButton);
        bcTd2.appendChild(bcCloseDiv);
        bcTr.appendChild(bcTd1);
        bcTr.appendChild(bcTd2);
        bcTBody.appendChild(bcTr);
        bcTable.appendChild(bcTBody);
        bcDiv.appendChild(bcTable);
        let bcBodyDiv = document.createElement('div');
        let bcPolygonDiv = document.createElement('div');
        bcBodyDiv.appendChild(document.createTextNode('Select Polygon'));
        bcBodyDiv.appendChild(bcPolygonDiv);
        bcBodyDiv.appendChild(document.createTextNode('Select Candidate Sites'));

        let bcContainer = document.createElement('div');
        //
        let bcList1Div = document.createElement('div');
        let bcList1 = document.createElement('select');
        let sites = myNetworkLayer.getSource().getFeatures();
        bcList1Option = [];
        for (let i = 0; i < sites.length; i++) {
            bcList1Option.push(document.createElement('option'));
            bcList1.appendChild(bcList1Option[i]);
        }
        bcList1Div.appendChild(document.createTextNode('Available Sites'));
        bcList1Div.appendChild(bcList1);
        bcContainer.appendChild(bcList1Div);

        let bcTransferDiv = document.createElement('div');
        let buttonRight = document.createElement('button');
        let buttonLeft = document.createElement('button');
        bcTransferDiv.appendChild(buttonRight);
        bcTransferDiv.appendChild(document.createElement('br'));
        bcTransferDiv.appendChild(document.createElement('br'));
        bcTransferDiv.appendChild(buttonLeft);
        bcContainer.appendChild(bcTransferDiv);

        let bcList2 = document.createElement('select');
        let bcList2Div = document.createElement('div');
        bcList2Div.appendChild(document.createTextNode('Selected Sites'));
        bcList2Div.appendChild(bcList2);
        bcContainer.appendChild(bcList2Div);

        bcBodyDiv.appendChild(bcContainer);

        bcButtonRun = document.createElement('button');
        bcBodyDiv.appendChild(bcButtonRun);
        bcButtonCancel = document.createElement('button');
        bcBodyDiv.appendChild(bcButtonCancel);
        bcDiv.appendChild(bcBodyDiv);

        document.getElementById('myMap').insertBefore(bcDiv,
            document.getElementsByClassName('ol-viewport')[0]);
        bcDiv.id = 'pbcDiv';
        bcDiv.className = 'predictBestCan panel panel-primary draggable';
        bcDiv.style.display = 'block';
        bcDiv.onclick = function (e) {
            if (e) { e.stopPropagation(); }
            setDialogZIndex(e, 'pbcDiv');
        };
        bcTable.style.width = '100%';
        bcTd1.style.width = '98%';
        bcTd2.style.width = '2%';
        bcTitleDiv.id = bcDiv.id + 'Title';
        bcTitleDiv.className = 'panel-heading draggable-handler';
        bcTitleDiv.innerHTML = '&nbsp;&nbsp;&nbsp;Best Candidate Options...';
        bcCloseDiv.className = 'panel-action';
        bcCloseButton.className = 'panel-action-button';
        bcCloseButton.title = 'Close';
        bcCloseButtonI.className = 'fas fa-window-close';
        bcCloseButton.onclick = function (e) {
            if (e) { e.stopPropagation(); }
            closePredictBestCan();
        };
        bcBodyDiv.className = 'panel-body';
        bcBodyDiv.style.width =
            (Number(bcDiv.style.width.replace('px')) - 6).toString() + 'px';
        bcBodyDiv.style.height =
            (Number(bcDiv.style.height.replace('px')) - 37).toString() + 'px';
        bcBodyDiv.style.padding = '15px 10px';
        bcBodyDiv.style.color = 'var(--fontcolor)';
        bcBodyDiv.style.fontSize = '12px';
        bcPolygonDiv.id = 'pbcPolygon';
        bcButtonRun.className = 'btn btn-default';
        bcButtonRun.style.position = 'absolute';
        bcButtonRun.style.right = '10px';
        bcButtonRun.style.bottom = '10px';
        bcButtonRun.title = 'Run';
        bcButtonRun.innerHTML = 'Run';
        bcButtonRun.onclick = function (e) {
            if (e) { e.stopPropagation(); }
            var testSites = [];
            var testPolygon = undefined;
            testSites = $('#lstBox2 option').map(function () {
                return $(this).val();
            }).get();
            if (testSites) {
                if (testSites.length > 50) {
                    testSites = testSites.slice(0, 50);
                }
            }
            let feature = myPolygonsLayer.getSource().getFeatureByProperty('polygonname',
                document.getElementById('pbcPolygon').value);
            if (feature) {
                if (feature.get('polygonid') !== undefined) {
                    testPolygon = feature.get('polygonid');
                }
            }
            closePredictBestCan();
            if (testPolygon) {
                if (testSites.length > 0) {
                    analyseBestCan(testSites, testPolygon);
                }
            }
        };
        bcButtonCancel.className = 'btn btn-default';
        bcButtonCancel.style.position = 'absolute';
        bcButtonCancel.style.right = '70px';
        bcButtonCancel.style.bottom = '10px';
        bcButtonCancel.title = 'Cancel';
        bcButtonCancel.innerHTML = 'Cancel';
        bcButtonCancel.onclick = function (e) {
            if (e) { e.stopPropagation(); }
            closePredictBestCan();
        };
        bcContainer.className = 'transferContainer';
        bcList1.className = 'listBox';
        bcList1.id = 'lstBox1';
        bcList1.multiple = 'multiple';
        bcList2.id = 'lstBox2';
        bcList2.className = 'listBox';
        bcList2.multiple = 'multiple';
        for (let i = 0; i < sites.length; i++) {
            bcList1Option[i].innerHTML = sites[i].getProperties().sitename;
            bcList1Option[i].value = sites[i].getProperties().siteid;
        }
        bcTransferDiv.className = 'transferButtons';
        buttonRight.id = 'btnRight';
        buttonRight.innerHTML = ' > ';
        buttonRight.value = ' > ';
        buttonRight.type = 'button';
        buttonLeft.id = 'btnLeft';
        buttonLeft.innerHTML = ' < ';
        buttonLeft.value = ' < ';
        buttonLeft.type = 'button';
        buttonRight.className = 'btn btn-default';
        buttonLeft.className = 'btn btn-default';

        let polygons = myPolygonsLayer.getSource().getFeatures();
        let polygonslst = "";
        for (let i = 0; i < polygons.length; i++) {
            polygonslst += polygons[i].getProperties().polygonname + '\t';
        }
        polygonslst = polygonslst.substr(0, polygonslst.length - 1);
        initSelectOpt(bcPolygonDiv, '100%', polygonslst, '');
        // set draggable
        $(document).ready(function () {
            $('#btnRight').click(function (e) {
                let selectedOpts = $('#lstBox1 option:selected');
                if (selectedOpts.length === 0) {
                    e.preventDefault();
                }
                $('#lstBox2').append($(selectedOpts).clone());
                $(selectedOpts).remove();
                e.preventDefault();
            });
            $('#btnLeft').click(function (e) {
                let selectedOpts = $('#lstBox2 option:selected');
                if (selectedOpts.length === 0) {
                    e.preventDefault();
                }
                $('#lstBox1').append($(selectedOpts).clone());
                $(selectedOpts).remove();
                e.preventDefault();
            });
        });
        setDraggable();
        setDialogZIndex(null, bcDiv.id);
    }
}
function closePredictBestCan() {
    let bcd = document.getElementById('pbcDiv');
    if (bcd) {
        while (bcd.firstChild) {
            bcd.firstChild.remove();
        }
        document.getElementById('myMap').removeChild(bcd);
    }
}