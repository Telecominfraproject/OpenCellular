//// context menu *********************************
//var contextmenu_items = [
//    {
//        text: 'Center map here',
//        classname: 'bold',
//        icon: '../img/center.png',
//        callback: center
//    },
//    //{
//    //    text: 'Some Actions',
//    //    icon: '../img/view_list.png',
//    //    items: [
//    //        {
//    //            text: 'Center map here',
//    //            icon: '../img/center.png',
//    //            callback: center
//    //        },
//    //        {
//    //            text: 'Add Site',
//    //            icon: '../img/site2G.png',
//    //            callback: marker
//    //        }
//    //    ]
//    //},
//    , '-',
//    {
//        text: 'Add Site',
//        icon: '../img/site2G.png',
//        callback: marker
//    },
//    //'-' // this is a separator
//];
//var propertiesItem = {
//    text: 'Site Options',
//    icon: '../img/options.png',
//    callback: showsiteOptions
//};
//var removeMarkerItem = {
//    text: 'Delete Site',
//    icon: '../img/delete.png',
//    callback: removeMarker
//};
//var contextmenu = new ContextMenu({
//    width: 180,
//    items: contextmenu_items
//});
//myMap.addControl(contextmenu);
//contextmenu.on('open', function (evt) {
//    var feature = myMap.forEachFeatureAtPixel(evt.pixel, function (ft, l) {
//        return ft;
//    });
//    if (feature /*&& feature.get('type') === 'removable'*/) {
//        contextmenu.clear();
//        propertiesItem.data = {
//            marker: feature
//        };
//        contextmenu.push(propertiesItem);
//        contextmenu.push('-');
//        removeMarkerItem.data = {
//            marker: feature
//        };
//        contextmenu.push(removeMarkerItem);
//    } else {
//        contextmenu.clear();
//        contextmenu.extend(contextmenu_items);
//        //contextmenu.extend(contextmenu.getDefaultItems());
//    }
//});
//function showsiteOptions(obj) {
//    var siteid = obj.data.marker.get('siteid');
//    document.getElementById('siteOptions').style.display = 'block';
//    document.getElementById('siteOptionsFrame').contentWindow.showsite(siteid);
//}
//function center(obj) {
//    let view = myMap.getView();
//    view.animate({
//        duration: 700,
//        center: obj.coordinate
//    });
//}
//function removeMarker(obj) {
//    var siteid = obj.data.marker.get('siteid');
//    deleteSite(siteid);
//    createmynetwork();
//    myNetworkLayer.getSource().clear();
//    document.getElementById('siteOptionsFrame').contentWindow.refreshsitedata();
//}
//function marker(obj) {
//    var coord4326 = ol.proj.transform(obj.coordinate, 'EPSG:3857', 'EPSG:4326');
//    var longitude = ol.coordinate.format(coord4326, '{x}', 6);
//    var latitude = ol.coordinate.format(coord4326, '{y}', 6);
//    AddNewSite(longitude, latitude);
//    createmynetwork();
//    myNetworkLayer.getSource().clear();
//    document.getElementById('siteOptionsFrame').contentWindow.refreshsitedata();
//}