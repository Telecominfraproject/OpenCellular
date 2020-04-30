<%@ Page Language="C#" AutoEventWireup="true" CodeBehind="webui.aspx.cs" Inherits="cnip.pages.webui" %>

<!DOCTYPE html>
<html xmlns="http://www.w3.org/1999/xhtml">
<head runat="server">
    <meta name="viewport" content="initial-scale=1.0, user-scalable=no, width=device-width" />
    <!-- title -->
    <title>CNIP</title>
    <link rel="manifest" href="../manifest.json" />
    <link rel="shortcut icon" href="https://drive.google.com/uc?id=1CE4IwHGAZakAtwNuTJkIWy1K2rE0pdm2" />
    <!-- jQuery -->
    <script src="../Scripts/jquery.min.js"></script>
    <!-- jsapi -->
    <script src="https://www.google.com/jsapi"></script>
    <!-- bootstrap -->
    <link href="../Content/bootstrap.min.css" rel="stylesheet" />
    <script src="../Scripts/bootstrap.min.js"></script>
    <!-- FileSaver -->
    <script src="../Scripts/FileSaver.min.js"></script>
    <!-- font awesome -->
    <link href="../Content/all.css" rel="stylesheet" />
    <!-- open layers -->
    <script src="../Scripts/ol-debug.js"></script>
    <link href="../Scripts/ol.css" rel="stylesheet" />
    <!-- The line below is only needed for old environments like Internet Explorer and Android 4.x -->
    <script src="../Scripts/polyfill.min.js"></script>
    <!-- ol-ext -->
    <link href="../Scripts/ol-ext.min.css" rel="stylesheet" />
    <script src="../Scripts/ol-ext.min.js"></script>
    <!-- ol layerswitcher sidebar -->
    <link href="../Scripts/ol3-sidebar.css" rel="stylesheet" />
    <script src="../Scripts/ol3-sidebar.js"></script>
    <!-- ol layerswitcher -->
    <link href="../Scripts/ol-layerswitcher.css" rel="stylesheet" />
    <script src="../Scripts/ol-layerswitcher.js"></script>
    <!-- ol context menu -->
    <script src="../Scripts/ol-contextmenu.js"></script>
    <link href="../Scripts/ol-contextmenu.min.css" rel="stylesheet" />
    <!-- Papaparse -->
    <script src="../Scripts/papaparse.min.js"></script>
    <!-- turf -->
    <script src="../Scripts/turf.min.js"></script>
    <!-- dom to image -->
    <script src="../Scripts/dom-to-image.min.js"></script>
    <!-- html to canvas -->
    <script src="../Scripts/html2canvas.min.js"></script>
    <!-- no ui slider -->
    <script src="../Scripts/nouislider.min.js"></script>
    <!-- chart js -->
    <script src="../Scripts/Chart.min.js"></script>
    <!-- cnip ol vector extension -->
    <script src="../Scripts/cnip/webui/ol.vector.extension.js"></script>
    <!-- cnip styles -->
    <link href="../Content/cnip/cnip.css" rel="stylesheet" />
    <!-- webui styles -->
    <link href="../Content/cnip/webui.css" rel="stylesheet" />
</head>
<body>
    <div id="fullscreen" class="fullscreen">
        <div class="hidden-content page-disabler"></div>
        <div class="hidden-content page-enabler">
            <div id="messageBoxBar" class="panel panel-primary draggable">
                <div id="messageBoxTitle" class="panel-heading draggable-handler" style="text-align: left;">
                </div>
                <div class="panel-body" style="text-align: left;">
                    <div id="messageBoxContent">
                    </div>
                    <div style="position: absolute; right: 90px; bottom: 10px;">
                        <button type="button" id="yesMessageBox" class="btn btn-default" style="width: 70px;">Yes</button>
                    </div>
                    <div style="position: absolute; right: 10px; bottom: 10px;">
                        <button type="button" id="okcancelMessageBox" class="btn btn-default" style="width: 70px;">Ok</button>
                    </div>
                </div>
            </div>
        </div>
        <div id="main">
            <!-- START OF SIDEBAR DIV -->
            <div id="sidebar" class="sidebar collapsed">
                <!-- Nav tabs -->
                <div class="sidebar-tabs">
                    <ul role="tablist">
                        <li><a href="#myNetwork" role="tab" title="My Network"><i class="fas fa-broadcast-tower" style="color: rgb(33,163,246);"></i></a></li>
                        <li><a href="#myPolygons" role="tab" title="My Polygons"><i class="fas fa-draw-polygon" style="color: rgb(127,58,183);"></i></a></li>
                        <li><a href="#myLinks" role="tab" title="My Links"><i class="fas fa-link" style="color: rgb(246,67,54);"></i></a></li>
                        <li><a href="#myNotes" role="tab" title="My Notes"><i class="fas fa-flag" style="color: rgb(255,227,0);"></i></a></li>
                        <%--<li><a href="#myClutter" role="tab" title="My Clutter"><i class="fa fa-icicles" style="color: rgb(164,85,72);"></i></a></li>--%>
                        <li><a href="#myResults" role="tab" title="My Results"><i class="fas fa-poll" style="color: rgb(164,85,72);"></i></a></li>
                        <li><a href="#toolbox" role="tab" title="Toolbox"><i class="fas fa-tools" style="color: rgb(255,152,0);"></i></a></li>
                        <li><a href="#settings" role="tab" title="Settings"><i class="fas fa-cog" style="color: rgb(0,150,136);"></i></a></li>
                        <li><a href="#layerSelection" role="tab" title="Layers"><i class="fas fa-layer-group" style="color: rgb(255,87,34);"></i></a></li>
                    </ul>
                    <ul role="tablist">
                        <li><a href="userguide.aspx" role="tab" target="_blank" title="User Guide"><i class="fas fa-book"></i></a></li>
                        <li><a href="#account" role="tab" title="Account"><i class="fas fa-user" style="color: rgb(76,175,80)"></i></a></li>
                    </ul>
                </div>
                <!-- Tab panes -->
                <div class="sidebar-content">
                    <div class="sidebar-pane" id="myNetwork">
                        <!--#include virtual="webui/sidebar/html/mynetwork.html" -->
                    </div>
                    <div class="sidebar-pane" id="myPolygons">
                        <!--#include virtual="webui/sidebar/html/mypolygons.html" -->
                    </div>
                    <div class="sidebar-pane" id="myLinks">
                        <!--#include virtual="webui/sidebar/html/mylinks.html" -->
                    </div>
                    <div class="sidebar-pane" id="myNotes">
                        <!--#include virtual="webui/sidebar/html/mynotes.html" -->
                    </div>
                    <%--<div class="sidebar-pane" id="myClutter">
                    <!--#include virtual="webui/sidebar/html/myclutter.html" -->
                    </div>--%>
                    <div class="sidebar-pane" id="myResults">
                    <!--#include virtual="webui/sidebar/html/myresults.html" -->
                    </div>
                    <div class="sidebar-pane" id="toolbox">
                        <!--#include virtual="webui/sidebar/html/toolbox.html" -->
                    </div>
                    <div class="sidebar-pane" id="settings">
                        <!--#include virtual="webui/sidebar/html/settings.html" -->
                    </div>
                    <div class="sidebar-pane" id="layerSelection">
                        <h2 class="sidebar-header" style="color: rgb(255,87,34);">Layers<span class="sidebar-close"><i class="fa fa-caret-left"></i></span></h2>
                        <!-- !!! HERE WILL GO THE CONTENT OF LAYERSWITCHER !!! -->
                        <div id="layers" class="layer-switcher"></div>
                    </div>
                    <div class="sidebar-pane" id="account">
                        <!--#include virtual="webui/sidebar/html/account.html" -->
                    </div>
                </div>
            </div>
            <!-- END OF SIDEBAR DIV -->
            <div id="myMap" tabindex="0">
                <div id="about"></div>
                <div id="activeInteraction"></div>
                <div id="featureInfo"></div>
                <div id="noteInfo"></div>
                <%--<div id="featureTip"></div>--%>
                <div id="locationInfo"></div>
                <!--#include virtual="webui/dialogs/html/dialogs.html" -->
            </div>
        </div>
    </div>
    <script>
        var username = '<%=GetUsername()%>';
        var email = '<%=GetEmail()%>';
        var vectorsPath = 'users/' + '<%=GetPuid()%>' + '/vectorLayers/';
        var tempPath = 'users/' + '<%=GetPuid()%>' + '/temp/';
        var resultsPath = 'users/' + '<%=GetPuid()%>' + '/results/';
        var LAC = '<%=GetPuid()%>';
        window.cniphost = {
            "environment": "cnip",
            "api": {
                "host": "https://cnip.com",
                "path": "/api/v1"
            }
        };
    </script>
    <script src="../Scripts/cnip/webui/generic.js"></script>
    <script src="../Scripts/cnip/webui/tileLayers/thunderforest.layers.js"></script>
    <script src="../Scripts/cnip/webui/tileLayers/bingmaps.layers.js"></script>
    <script src="../Scripts/cnip/webui/tileLayers/googlemaps.layers.js"></script>
    <script src="../Scripts/cnip/webui/tileLayers/stamen.layers.js"></script>
    <script src="../Scripts/cnip/webui/tileLayers/osm.layers.js"></script>
    <script src="../Scripts/cnip/webui/tileLayers/maptiler.layers.js"></script>
    <script src="../Scripts/cnip/webui/tileLayers/mapzen.layers.js"></script>
    <script src="../Scripts/cnip/webui/vectorLayers/vector.layers.js"></script>
    <script src="../Scripts/cnip/webui/vectorrequests.js"></script>
    <script src="../Scripts/cnip/webui/styles.js"></script>
    <script src="../Scripts/cnip/webui/webui.js"></script>
    <script src="../Scripts/cnip/webui/interactions.js"></script>
    <script src="../Scripts/cnip/webui/activeinteraction.js"></script>
    <script src="../Scripts/cnip/webui/controlbar.js"></script>
    <script src="../Scripts/cnip/webui/legend.js"></script>
    <script src="../Scripts/cnip/webui/rendermap.js"></script>
    <script src="../Scripts/cnip/webui/contextmenu.js"></script>
    <script src="../Scripts/cnip/webui/dialogs.js"></script>
    <script src="../Scripts/cnip/webui/dds.js"></script>
    <script src="../Scripts/cnip/webui/sidebar/mynetwork.js"></script>
    <script src="../Scripts/cnip/webui/sidebar/mypolygons.js"></script>
    <script src="../Scripts/cnip/webui/sidebar/mylinks.js"></script>
    <script src="../Scripts/cnip/webui/sidebar/myclutter.js"></script>
    <script src="../Scripts/cnip/webui/sidebar/mynotes.js"></script>
    <script src="../Scripts/cnip/webui/sidebar/myresults.js"></script>
    <script src="../Scripts/cnip/webui/sidebar/toolbox.js"></script>
    <script src="../Scripts/cnip/webui/sidebar/settings.js"></script>
    <script src="../Scripts/cnip/webui/sidebar/account.js"></script>
    <script src="../Scripts/cnip/webui/dialogs/predictbestcan.js"></script>
    <script src="../Scripts/cnip/webui/dialogs/predictsites.js"></script>
    <script src="../Scripts/cnip/webui/dialogs/predict.js"></script>
</body>
</html>
