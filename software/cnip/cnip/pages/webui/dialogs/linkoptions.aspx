<%@ Page Language="C#" AutoEventWireup="true" CodeBehind="linkoptions.aspx.cs" Inherits="cnip.pages.linkoptions" %>

<!DOCTYPE html>
<html xmlns="http://www.w3.org/1999/xhtml" style="">
<head>
    <title></title>
    <!-- jQuery -->
    <script src="../../../Scripts/jquery.min.js"></script>
    <!-- jsapi -->
    <script src="https://www.google.com/jsapi"></script>
    <!-- bootstrap -->
    <link href="../../../Content/bootstrap.min.css" rel="stylesheet" />
    <script src="../../../Scripts/bootstrap.min.js"></script>
    <!-- font awesome -->
    <link href="../../../Content/all.css" rel="stylesheet" />
    <!-- no ui slider -->
    <script src="../../../Scripts/nouislider.min.js"></script>
    <!-- cnip styles -->
    <link href="../../../Content/cnip/cnip.css" rel="stylesheet" />
    <style>
        * {
            font-size: 12px;
        }

        .textInput {
            margin: 3px 0px 0px 0px;
        }

        .selectBar {
            margin: 3px 0px 0px 0px;
        }

        .selectDiv {
            margin-top: 3px;
        }

        span {
            margin-top: 3px;
            display: inline-block;
        }
    </style>
</head>
<body>
    <button class="pageTabLink" onclick="openPage(event,'General', this);" id="defaultOpen">General</button>
    <button class="pageTabLink" onclick="openPage(event,'Microwave', this);">Microwave</button>
    <hr />
    <div id="General" class="pageTabContent">
        <table>
            <tr>
                <td style="width: 48%;">Link                   
                    <hr />
                    <table style="display: none;">
                        <tr>
                            <td style="width: 40%;">
                                <span>Id</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="linkid" type="text" readonly="readonly" disabled="disabled" class="textInput" />
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Name</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="linkname" type="text" class="textInput" />
                            </td>
                        </tr>
                    </table>
                </td>
                <td style="width: 2%;"></td>
                <td style="width: 48%;">&nbsp;                  
                    <hr />
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Type</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="linktype" type="text" readonly="readonly" disabled="disabled" class="textInput" />
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Distance (Km)</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="distance" type="text" readonly="readonly" disabled="disabled" class="textInput" />
                            </td>
                        </tr>
                    </table>
                </td>
            </tr>
            <tr>
                <td style="width: 48%;">Site A                   
                    <hr />
                    <table style="display: none;">
                        <tr>
                            <td style="width: 40%;">
                                <span>Id</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="siteida" type="text" readonly="readonly" disabled="disabled" class="textInput" />
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Name</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="sitenamea" type="text" readonly="readonly" disabled="disabled" class="textInput" />
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Tower Height (m)</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="heighta" type="text" readonly="readonly" disabled="disabled" class="textInput" />
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Elevation (m)</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="locheighta" type="text" readonly="readonly" disabled="disabled" class="textInput" />
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Bearing °</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="bearinga" type="text" readonly="readonly" disabled="disabled" class="textInput" />
                            </td>
                        </tr>
                    </table>
                </td>
                <td style="width: 2%;"></td>
                <td style="width: 48%;">
                    <p id="sitebtd1" style="margin: 0 0 0 0;">Site B</p>
                    <hr />
                    <table style="display: none;">
                        <tr>
                            <td style="width: 40%;">
                                <span>Id</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="siteidb" type="text" readonly="readonly" disabled="disabled" class="textInput" />
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Name</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="sitenameb" type="text" readonly="readonly" disabled="disabled" class="textInput" />
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Tower Height (m)</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="heightb" type="text" readonly="readonly" disabled="disabled" class="textInput" />
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Elevation (m)</span>

                            </td>
                            <td style="width: 60%;">
                                <input id="locheightb" type="text" readonly="readonly" disabled="disabled" class="textInput" />
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Bearing °</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="bearingb" type="text" readonly="readonly" disabled="disabled" class="textInput" />
                            </td>
                        </tr>
                    </table>
                    <p id="user" style="margin: 0 0 0 0;">User</p>
                    <hr id="userhr" />
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span id="spanname">Name</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="name" type="text" readonly="readonly" disabled="disabled" class="textInput" />
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span id="spanemail">Email</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="email" type="text" readonly="readonly" disabled="disabled" class="textInput" />
                            </td>
                        </tr>
                    </table>
                </td>
            </tr>
        </table>
    </div>
    <div id="Microwave" class="pageTabContent">
        <table>
            <tr>
                <td style="width: 48%;">Link                   
                    <hr />
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Name</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="linkname1" type="text" readonly="readonly" disabled="disabled" class="textInput" />
                            </td>
                        </tr>
                    </table>
                </td>
                <td style="width: 2%;"></td>
                <td style="width: 48%;"></td>
            </tr>
            <tr>
                <td style="width: 48%;">Site A                   
                    <hr />
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Frequency (GHz)</span>
                            </td>
                            <td style="width: 60%;">
                                <div id="frequencya"></div>
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Channel Width (MHz)</span>
                            </td>
                            <td style="width: 60%;">
                                <div id="channelwidtha"></div>
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Output Power (dBm)</span>
                            </td>
                            <td style="width: 60%;">
                                <div id="outputpowera"></div>
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Antenna Gain (dBi)</span>
                            </td>
                            <td style="width: 60%;">
                                <div id="antennagaina"></div>
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Losses (dB)</span>
                            </td>
                            <td style="width: 60%;">
                                <div id="lossesa"></div>
                            </td>
                        </tr>
                    </table>
                </td>
                <td style="width: 2%"></td>
                <td style="width: 48%;">
                    <p id="sitebtd2" style="margin: 0 0 0 0;">Site B</p>
                    <hr />
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Frequency (GHz)</span>
                            </td>
                            <td style="width: 60%;">
                                <div id="frequencyb"></div>
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Channel Width (MHz)</span>
                            </td>
                            <td style="width: 60%;">
                                <div id="channelwidthb"></div>
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Output Power (dBm)</span>
                            </td>
                            <td style="width: 60%;">
                                <div id="outputpowerb"></div>
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Antenna Gain (dBi)</span>
                            </td>
                            <td style="width: 60%;">
                                <div id="antennagainb"></div>
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Losses (dB)</span>
                            </td>
                            <td style="width: 60%;">
                                <div id="lossesb"></div>
                            </td>
                        </tr>
                    </table>
                </td>
            </tr>
        </table>
    </div>
    <button class="pageBottomButton" style="left: 10px; color: red;" title="Delete" onclick="deleteLink(event);"><i class="fas fa-trash-alt"></i></button>
    <button class="pageBottomButton" style="right: 10px;" title="Next" onclick="nextLink(event);"><i class="fas fa-angle-double-right"></i></button>
    <button class="pageBottomButton" style="right: 35px;" title="Previous" onclick="previousLink(event);"><i class="fas fa-angle-double-left"></i></button>
    <button class="pageBottomButton" style="right: 60px; color: rgb(3,169,244);" title="Locate" onclick="zoomToLink(event);"><i class="fas fa-map-marker-alt"></i></button>
    <script src="../../../Scripts/cnip/webui/generic.js"></script>
    <script src="../../../Scripts/cnip/webui/dialogs/linkoptions.js"></script>
</body>
</html>
