function showPredict(e, predict) {
    if (e) { e.stopPropagation(); }
    if (!document.getElementById('pcDiv')) {
        let pcDiv = document.createElement('div');
        let pcTable = document.createElement('table');
        let pcTBody = document.createElement('tbody');
        let pcTr = document.createElement('tr');
        let pcTd1 = document.createElement('td');
        let pcTitleDiv = document.createElement('div');
        pcTd1.appendChild(pcTitleDiv);
        let pcTd2 = document.createElement('td');
        let pcCloseDiv = document.createElement('div');
        let pcCloseButton = document.createElement('button');
        let pcCloseButtonI = document.createElement('i');
        pcCloseButton.appendChild(pcCloseButtonI);
        pcCloseDiv.appendChild(pcCloseButton);
        pcTd2.appendChild(pcCloseDiv);
        pcTr.appendChild(pcTd1);
        pcTr.appendChild(pcTd2);
        pcTBody.appendChild(pcTr);
        pcTable.appendChild(pcTBody);
        pcDiv.appendChild(pcTable);
        let pcBodyDiv = document.createElement('div');
        pcBodyDiv.appendChild(document.createTextNode('Select Sites'));

        let pcContainer = document.createElement('div');
        //
        let pcList1Div = document.createElement('div');
        let pcList1 = document.createElement('select');
        let sites = myNetworkLayer.getSource().getFeatures();
        pcList1Option = [];
        for (let i = 0; i < sites.length; i++) {
            pcList1Option.push(document.createElement('option'));
            pcList1.appendChild(pcList1Option[i]);
        }
        pcList1Div.appendChild(document.createTextNode('Available Sites'));
        pcList1Div.appendChild(pcList1);
        pcContainer.appendChild(pcList1Div);

        let pcTransferDiv = document.createElement('div');
        let buttonRight = document.createElement('button');
        let buttonLeft = document.createElement('button');
        pcTransferDiv.appendChild(buttonRight);
        pcTransferDiv.appendChild(document.createElement('br'));
        pcTransferDiv.appendChild(document.createElement('br'));
        pcTransferDiv.appendChild(buttonLeft);
        pcContainer.appendChild(pcTransferDiv);

        let pcList2 = document.createElement('select');
        let pcList2Div = document.createElement('div');
        pcList2Div.appendChild(document.createTextNode('Selected Sites'));
        pcList2Div.appendChild(pcList2);
        pcContainer.appendChild(pcList2Div);

        pcBodyDiv.appendChild(pcContainer);

        pcButtonRun = document.createElement('button');
        pcBodyDiv.appendChild(pcButtonRun);
        pcButtonCancel = document.createElement('button');
        pcBodyDiv.appendChild(pcButtonCancel);
        pcDiv.appendChild(pcBodyDiv);

        document.getElementById('myMap').insertBefore(pcDiv,
            document.getElementsByClassName('ol-viewport')[0]);
        pcDiv.id = 'pcDiv';
        pcDiv.className = 'predictBestCan panel panel-primary draggable';
        pcDiv.style.display = 'block';
        pcDiv.onclick = function (e) {
            if (e) { e.stopPropagation(); }
            setDialogZIndex(e, 'pcDiv');
        };
        pcTable.style.width = '100%';
        pcTd1.style.width = '98%';
        pcTd2.style.width = '2%';
        pcTitleDiv.id = pcDiv.id + 'Title';
        pcTitleDiv.className = 'panel-heading draggable-handler';
        if (predict === 'Coverage') {
            pcTitleDiv.innerHTML = '&nbsp;&nbsp;&nbsp;Predict Coverage Options...';
        }
        if (predict === 'RadioPlan') {
            pcTitleDiv.innerHTML = '&nbsp;&nbsp;&nbsp;Predict Radio Plan Options...';
        }
        if (predict === 'Links') {
            pcTitleDiv.innerHTML = '&nbsp;&nbsp;&nbsp;Predict Links Options...';
        }
        pcCloseDiv.className = 'panel-action';
        pcCloseButton.className = 'panel-action-button';
        pcCloseButton.title = 'Close';
        pcCloseButtonI.className = 'fas fa-window-close';
        pcCloseButton.onclick = function (e) {
            if (e) { e.stopPropagation(); }
            closePredict();
        };
        pcDiv.style.height = '243px';
        pcBodyDiv.className = 'panel-body';
        pcBodyDiv.style.width =
            (Number(pcDiv.style.width.replace('px')) - 6).toString() + 'px';
        pcBodyDiv.style.height =
            (Number(pcDiv.style.height.replace('px')) - 37).toString() + 'px';
        pcBodyDiv.style.padding = '15px 10px';
        pcBodyDiv.style.color = 'var(--fontcolor)';
        pcBodyDiv.style.fontSize = '12px';
        pcButtonRun.className = 'btn btn-default';
        pcButtonRun.style.position = 'absolute';
        pcButtonRun.style.right = '10px';
        pcButtonRun.style.bottom = '10px';
        pcButtonRun.title = 'Run';
        pcButtonRun.innerHTML = 'Run';
        pcButtonRun.onclick = function (e) {
            if (e) { e.stopPropagation(); }
            var testSites = [];
            testSites = $('#lstBox2pc option').map(function () {
                return $(this).val();
            }).get();
            if (testSites) {
                if (testSites.length > 50) {
                    testSites = testSites.slice(0, 50);
                }
            }
            closePredict();
            if (testSites.length > 0) {
                if (predict === 'Coverage') {
                    predictCoverage(testSites);
                }
                if (predict === 'RadioPlan') {
                    predictRadioPlan(testSites);
                }
                if (predict === 'Links') {
                    predictLinks(testSites);
                }
            }
        };
        pcButtonCancel.className = 'btn btn-default';
        pcButtonCancel.style.position = 'absolute';
        pcButtonCancel.style.right = '70px';
        pcButtonCancel.style.bottom = '10px';
        pcButtonCancel.title = 'Cancel';
        pcButtonCancel.innerHTML = 'Cancel';
        pcButtonCancel.onclick = function (e) {
            if (e) { e.stopPropagation(); }
            closePredict();
        };
        pcContainer.className = 'transferContainer';
        pcList1.className = 'listBox';
        pcList1.id = 'lstBox1pc';
        pcList1.multiple = 'multiple';
        pcList2.id = 'lstBox2pc';
        pcList2.className = 'listBox';
        pcList2.multiple = 'multiple';
        for (let i = 0; i < sites.length; i++) {
            pcList1Option[i].innerHTML = sites[i].getProperties().sitename;
            pcList1Option[i].value = sites[i].getProperties().siteid;
        }
        pcTransferDiv.className = 'transferButtons';
        buttonRight.id = 'btnRightpc';
        buttonRight.innerHTML = ' > ';
        buttonRight.value = ' > ';
        buttonRight.type = 'button';
        buttonLeft.id = 'btnLeftpc';
        buttonLeft.innerHTML = ' < ';
        buttonLeft.value = ' < ';
        buttonLeft.type = 'button';
        buttonRight.className = 'btn btn-default';
        buttonLeft.className = 'btn btn-default';

        // set draggable
        $(document).ready(function () {
            $('#btnRightpc').click(function (e) {
                let selectedOpts = $('#lstBox1pc option:selected');
                if (selectedOpts.length === 0) {
                    e.preventDefault();
                }
                $('#lstBox2pc').append($(selectedOpts).clone());
                $(selectedOpts).remove();
                e.preventDefault();
            });
            $('#btnLeftpc').click(function (e) {
                let selectedOpts = $('#lstBox2pc option:selected');
                if (selectedOpts.length === 0) {
                    e.preventDefault();
                }
                $('#lstBox1pc').append($(selectedOpts).clone());
                $(selectedOpts).remove();
                e.preventDefault();
            });
        });
        setDraggable();
        setDialogZIndex(null, pcDiv.id);
    }
}
function closePredict() {
    let pcd = document.getElementById('pcDiv');
    if (pcd) {
        while (pcd.firstChild) {
            pcd.firstChild.remove();
        }
        document.getElementById('myMap').removeChild(pcd);
    }
}