<%@ Page Language="C#" AutoEventWireup="true" CodeBehind="siteoptions.aspx.cs" Inherits="cnip.pages.siteoptions" %>

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
    <button class="pageTabLink" onclick="openPage(event,'Transmitter', this);">Transmitter</button>
    <hr />
    <div id="General" class="pageTabContent">
        <table>
            <tr>
                <td style="width: 48%; vertical-align: top">Site                   
                    <hr />
                    <table style="display:none;">
                        <tr>
                            <td style="width: 40%;">
                                <span>Id</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="siteid" type="text" readonly="readonly" disabled="disabled" class="textInput" />
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Name</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="sitename" type="text" class="textInput" />
                            </td>
                        </tr>
                    </table>
                    Location                   
                    <hr />
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Longitude °</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="longitude" type="number" class="textInput" />
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Latitude °</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="latitude" type="number" class="textInput" />
                            </td>
                        </tr>
                    </table>
                    Technology
                    <hr />
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Name</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="technology" type="text" class="textInput" readonly="readonly" disabled="disabled" />
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Band</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="band" type="text" class="textInput" readonly="readonly" disabled="disabled" />
                            </td>
                        </tr>
                    </table>
                </td>
                <td style="width: 2%;"></td>
                <td style="width: 48%; vertical-align: top">Tower                   
                    <hr />
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Height (m)</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="height" type="number" class="textInput" />
                            </td>
                        </tr>
                    </table>
                    Property                   
                    <hr />
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Region</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="region" type="text" class="textInput" />
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Country</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="country" type="text" class="textInput" />
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>City</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="city" type="text" class="textInput" />
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>District</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="district" type="text" class="textInput" />
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Province</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="province" type="text" class="textInput" />
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Address</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="address" type="text" class="textInput" />
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Comments</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="comments" type="text" class="textInput" />
                            </td>
                        </tr>
                    </table>
                </td>
            </tr>
        </table>
    </div>
    <div id="Transmitter" class="pageTabContent">
        <table>
            <tr>
                <td style="width: 48%;">Site                   
                    <hr />
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Name</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="sitename1" type="text" class="textInput" readonly="readonly" disabled="disabled" />
                            </td>
                        </tr>
                    </table>
                    Radio                   
                    <hr />
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Technology</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="technology1" type="text" class="textInput" readonly="readonly" disabled="disabled" />
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Bandwidth (MHz)</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="bandwidth" type="text" class="textInput" readonly="readonly" disabled="disabled" />
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Cell Id</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="cellid" type="number" class="textInput" readonly="readonly" disabled="disabled" />
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>TAC / LAC</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="lac" type="number" class="textInput" readonly="readonly" disabled="disabled" />
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>EARFCN / ARFCN</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="rfcn" type="number" class="textInput" />
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>PCI / BSIC</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="rfid" type="number" class="textInput" />
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>DL Frequency (MHz)</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="dlfrequency" type="number" class="textInput" readonly="readonly" disabled="disabled" />
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>UL Frequency (MHz)</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="ulfrequency" type="number" class="textInput" readonly="readonly" disabled="disabled" />
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>RF Power (W)</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="rfpower" type="number" class="textInput" />
                            </td>
                        </tr>
                    </table>
                </td>
                <td style="width: 2%"></td>
                <td style="width: 48%">Antenna
                    <hr />
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Height (m)</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="hba" type="number" class="textInput" />
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Azimuth °</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="azimuth" type="number" class="textInput" />
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Model</span>
                            </td>
                            <td style="width: 60%;">
                                <div id="antennamodel"></div>
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Type</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="antennatype" type="text" class="textInput" readonly="readonly" disabled="disabled" />
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Polarization</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="polarization" type="text" class="textInput" readonly="readonly" disabled="disabled" />
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>V-Beamwidth °</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="vbeamwidth" type="number" class="textInput" readonly="readonly" disabled="disabled" />
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>H-Beamwidth °</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="hbeamwidth" type="number" class="textInput" readonly="readonly" disabled="disabled" />
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Downtilt °</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="downtilt" type="number" class="textInput" readonly="readonly" disabled="disabled" />
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Gain (dBi)</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="antennagain" type="number" class="textInput" readonly="readonly" disabled="disabled" />
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Feeder Loss (dB)</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="feederloss" type="number" min="0" max="50" step="0.01" class="textInput" />
                            </td>
                        </tr>
                    </table>
                </td>
            </tr>
        </table>
    </div>
    <button class="pageBottomButton" style="left: 10px; color: red;" title="Delete" onclick="deleteSite(event);"><i class="fas fa-trash-alt"></i></button>
    <button class="pageBottomButton" style="right: 10px;" title="Next" onclick="nextSite(event);"><i class="fas fa-angle-double-right"></i></button>
    <button class="pageBottomButton" style="right: 35px;" title="Previous" onclick="previousSite(event);"><i class="fas fa-angle-double-left"></i></button>
    <button class="pageBottomButton" style="right: 60px; color: rgb(3,169,244);" title="Locate" onclick="zoomToSite(event);"><i class="fas fa-map-marker-alt"></i></button>
    <script src="../../../Scripts/cnip/webui/generic.js"></script>
    <script src="../../../Scripts/cnip/webui/dialogs/siteoptions.js"></script>
</body>
</html>
