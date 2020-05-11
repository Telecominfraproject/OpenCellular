var myLegend = new ol.control.Legend({
    title: 'Legend',
    collapsed: true,
    margin: 5,
    size: [20, 10]
});
myMap.addControl(myLegend);
myLegend.addRow({ title: 'Legend' });
function setLegend(thematicString) {
    // ,&$#@>
    // result : thematicString>pngString>polygonsString@polygonsString
    // thematicString
    // rate&r,g,b$rate&r,g,b
    // pngString
    // png,1,2,3,4
    // polygonsString
    // polygonid,polygonname#siteid,sitename#png,1,2,3,4#
    // cttrue,ctext1234#rate&value&r,g,b$rate&value&r,g,b#bestcandidate
    clearLegend();
    if (thematicString) {
        if (thematicString.length > 0) {
            let rows = thematicString.split('$');
            let lastTitleValue = '+s';
            for (let i = 0; i < rows.length; i++) {
                let titleValue = rows[i].split('&')[0];
                let rgb = rows[i].split('&')[1].split(',');
                myLegend.addRow(
                    {
                        title: allLetter(titleValue) ? titleValue : lastTitleValue + ' to ' + titleValue,
                        typeGeom: 'Point',
                        style: new ol.style.Style({
                            image: new ol.style.RegularShape({
                                points: 4,
                                radius: 5,
                                stroke: new ol.style.Stroke({ color: [rgb[0], rgb[1], rgb[2], 1], width: 1.5 }),
                                fill: new ol.style.Fill({ color: [rgb[0], rgb[1], rgb[2], 0.85] })
                            })
                        })
                    }
                );
                lastTitleValue = titleValue;
            }
            myLegend.show();
        }
    }
}
function clearLegend() {
    while (myLegend.getLength() > 0) {
        for (let i = 0; i < myLegend.getLength(); i++) {
            myLegend.removeRow(i);
        }
    }
}
function showLegend() {
    let button = document.getElementById('myMap').querySelector(
        'div.ol-legend.ol-unselectable.ol-control.ol-collapsed button'
    );
    if (button) {
        fireClick(button);
    }
}
var fireClick = function (node) {
    if (typeof MouseEvent === 'function') {
        var mevt = new MouseEvent('click', {
            bubbles: false,
            cancelable: true
        });
        node.dispatchEvent(mevt);
    } else if (document.createEvent) {
        // Fallback
        var evt = document.createEvent('MouseEvents');
        evt.initEvent('click', false, false);
        node.dispatchEvent(evt);
    } else if (document.createEventObject) {
        node.fireEvent('onclick');
    } else if (typeof node.onclick === 'function') {
        node.onclick();
    }
};