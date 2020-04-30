<%@ Page Language="C#" AutoEventWireup="true" CodeBehind="linkprofile.aspx.cs" Inherits="cnip.pages.linkprofile" %>

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
    <!-- turf -->
    <script src="../../../Scripts/turf.min.js"></script>
    <!-- chart js -->
    <script src="../../../Scripts/Chart.min.js"></script>
    <!-- cnip styles -->
    <link href="../../../Content/cnip/cnip.css" rel="stylesheet" />
    <style>
        * {
            font-size: 14px;
            color: var(--fontcolor);
            background-color: var(--backcolor);
        }

        td {
            vertical-align: middle;
        }

        .textInput {
            margin: 0;
        }

        .auto-style1 {
            margin-left: 15px;
        }

        .auto-style2 {
            width: 20px;
            font-size: 12px;
        }

        .auto-style3 {
            width: 24px;
            border: none;
            font-size: 12px;
            color: var(--barcolor);
        }

        .auto-style4 {
            width: 33px;
        }

        .auto-style5 {
            width: 39px;
            font-size: 12px;
        }

        .auto-style6 {
            width: 114px;
            border: none;
            font-size: 12px;
            color: var(--barcolor);
        }

        .auto-style7 {
            width: 145px;
            font-size: 12px;
        }

        .auto-style8 {
            width: 80px;
            border: none;
            font-size: 12px;
            color: var(--barcolor);
        }

        canvas {
            -moz-user-select: none;
            -webkit-user-select: none;
            -ms-user-select: none;
        }

        #chartjs-tooltip {
            opacity: 1;
            position: absolute;
            border-radius: 3px;
            -webkit-transition: all .1s ease;
            transition: all .1s ease;
            pointer-events: none;
            -webkit-transform: translate(-50%, 0);
            transform: translate(-50%, 0);
            background-color: rgba(0,0,0,0.7);
        }

        .chartjs-tooltip-key {
            display: inline-block;
            width: 10px;
            height: 10px;
            margin-right: 10px;
        }
    </style>
</head>
<body>
    <table>
        <tr>
            <td style="width: 23%; vertical-align: top; text-align: left;">
                <div class="auto-style1">
                    <p style="background-color: #555; color: white; font-weight: 100; width: 50px; padding: 0px 0px 0px 5px;">Site A</p>
                </div>
                <div class="auto-style1">
                    <div>
                        <table>
                            <tr>
                                <td class="auto-style2"  style="display:none;">ID</td>
                                <td class="auto-style4"  style="display:none;">
                                    <input id="siteida" class="textInput auto-style3" type="text" readonly="readonly" value="1" /></td>
                                <td class="auto-style5">Name</td>
                                <td>
                                    <input id="sitenamea" class="textInput auto-style6" type="text" readonly="readonly" value="new site a" /></td>
                            </tr>
                        </table>
                    </div>
                    <div>
                        General
                        <hr style="padding: 0px; margin: 0px" />
                    </div>
                    <div>
                        <table>
                            <tr>
                                <td class="auto-style7">Tower Height / Elev (m)</td>
                                <td>
                                    <input id="deviceheighta" class="textInput auto-style8" type="text" readonly="readonly" value="10 / 343" /></td>
                            </tr>
                            <tr>
                                <td class="auto-style7">Bearing / Tilt °</td>
                                <td>
                                    <input id="headinga" class="textInput auto-style8" type="text" readonly="readonly" value="262.21 / 0.16" /></td>
                            </tr>
                        </table>

                    </div>
                    <div>
                        Microwave
                        <hr style="padding: 0px; margin: 0px" />
                    </div>

                    <div>
                        <table>
                            <tr>
                                <td class="auto-style7">Frequency (GHz)</td>
                                <td>
                                    <input id="frequencya" class="textInput auto-style8" type="text" readonly="readonly" value="5" /></td>
                            </tr>
                            <tr>
                                <td class="auto-style7">Channel Width (MHz)</td>
                                <td>
                                    <input id="channelwidtha" class="textInput auto-style8" type="text" readonly="readonly" value="100" /></td>
                            </tr>
                            <tr>
                                <td class="auto-style7">Output Power (dBm)</td>
                                <td>
                                    <input id="outputpowera" class="textInput auto-style8" type="text" readonly="readonly" value="29" /></td>
                            </tr>
                            <tr>
                                <td class="auto-style7">Antenna Gain (dBi)</td>
                                <td>
                                    <input id="antennagaina" class="textInput auto-style8" type="text" readonly="readonly" value="34" /></td>
                            </tr>
                            <tr>
                                <td class="auto-style7">Losses (dB)</td>
                                <td>
                                    <input id="lossesa" class="textInput auto-style8" type="text" readonly="readonly" value="0" /></td>
                            </tr>
                        </table>
                    </div>
                </div>
            </td>
            <td style="width: 54%; vertical-align: top; text-align: left;">
                <div>
                    <table>
                        <tr>
                            <td style="width: 7%; font-size: 12px;display:none;">Link ID</td>
                            <td id="linkid" style="width: 5%; font-size: 12px; color: var(--barcolor);display:none;">1</td>
                            <td style="width: 6%; font-size: 12px;">Name</td>
                            <td id="linkname" style="width: 10%; font-size: 12px; color: var(--barcolor);">new link 1</td>
                            <td style="width: 6%; font-size: 12px;">Type</td>
                            <td id="linktype" style="width: 66%; font-size: 12px; color: var(--barcolor);">internal</td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 45%;">
                                <img src="../../../img/leftlink.png" /></td>
                            <td id="distance" style="width: 10%; font-size: 10px; color: var(--barcolor); text-align: center;">10 Km</td>
                            <td style="width: 45%;">
                                <img src="../../../img/rightlink.png" /></td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 15%;">
                                <table>
                                    <tr>
                                        <td style="font-size: 10px; display: none;">Effc %</td>
                                        <td id="effca" style="font-size: 10px; color: var(--barcolor); display: none;">100</td>
                                    </tr>
                                    <tr>
                                        <td style="font-size: 10px; display: none;">Rate (Mbps)</td>
                                        <td id="ratea" style="font-size: 10px; color: var(--barcolor); display: none;">243</td>
                                    </tr>
                                    <tr>
                                        <td style="font-size: 10px;">Lev (dBm)</td>
                                        <td id="leva" style="font-size: 10px; color: var(--barcolor);">34</td>
                                    </tr>
                                </table>
                            </td>
                            <td id="connectivityInfo" style="width: 70%; font-size: 10px; text-align: center; color: orangered;"></td>
                            <td style="width: 15%;">
                                <table>
                                    <tr>
                                        <td style="font-size: 10px; display: none;">Effc %</td>
                                        <td id="effcb" style="font-size: 10px; color: var(--barcolor); display: none;">100</td>
                                    </tr>
                                    <tr>
                                        <td style="font-size: 10px; display: none;">Rate (Mbps)</td>
                                        <td id="rateb" style="font-size: 10px; color: var(--barcolor); display: none;">243</td>
                                    </tr>
                                    <tr>
                                        <td style="font-size: 10px;">Lev (dBm)</td>
                                        <td id="levb" style="font-size: 10px; color: var(--barcolor);">34</td>
                                    </tr>
                                </table>
                            </td>
                        </tr>
                    </table>
                    <div style="margin-top: 5px;">
                        <canvas id="chart1" style="width: 100%; height: 205px;"></canvas>
                    </div>
                </div>
            </td>
            <td style="width: 23%; vertical-align: top; text-align: left;">
                <div class="auto-style1">
                    <p id="sitebp" style="background-color: #555; color: white; font-weight: 100; width: 50px; padding: 0px 0px 0px 5px;">Site A</p>
                </div>
                <div class="auto-style1">
                    <div>
                        <table>
                            <tr>
                                <td class="auto-style2"  style="display:none;">ID</td>
                                <td class="auto-style4"  style="display:none;">
                                    <input id="siteidb" class="textInput auto-style3" type="text" readonly="readonly" value="2" /></td>
                                <td class="auto-style5">Name</td>
                                <td>
                                    <input id="sitenameb" class="textInput auto-style6" type="text" readonly="readonly" value="new site b" /></td>
                            </tr>
                        </table>
                    </div>
                    <div>
                        General
                        <hr style="padding: 0px; margin: 0px" />
                    </div>
                    <div>
                        <table>
                            <tr>
                                <td class="auto-style7">Tower Height / Elev (m)</td>
                                <td>
                                    <input id="deviceheightb" class="textInput auto-style8" type="text" readonly="readonly" value="15 / 23" /></td>
                            </tr>
                            <tr>
                                <td class="auto-style7">Bearing / Tilt °</td>
                                <td>
                                    <input id="headingb" class="textInput auto-style8" type="text" readonly="readonly" value="10 / 23.16" /></td>
                            </tr>
                        </table>

                    </div>
                    <div>
                        Microwave
                        <hr style="padding: 0px; margin: 0px" />
                    </div>
                    <div>
                        <table>
                            <tr>
                                <td class="auto-style7">Frequency (GHz)</td>
                                <td>
                                    <input id="frequencyb" class="textInput auto-style8" type="text" readonly="readonly" value="5" /></td>
                            </tr>
                            <tr>
                                <td class="auto-style7">Channel Width (MHz)</td>
                                <td>
                                    <input id="channelwidthb" class="textInput auto-style8" type="text" readonly="readonly" value="100" /></td>
                            </tr>
                            <tr>
                                <td class="auto-style7">Output Power (dBm)</td>
                                <td>
                                    <input id="outputpowerb" class="textInput auto-style8" type="text" readonly="readonly" value="29" /></td>
                            </tr>
                            <tr>
                                <td class="auto-style7">Antenna Gain (dBi)</td>
                                <td>
                                    <input id="antennagainb" class="textInput auto-style8" type="text" readonly="readonly" value="34" /></td>
                            </tr>
                            <tr>
                                <td class="auto-style7">Losses (dB)</td>
                                <td>
                                    <input id="lossesb" class="textInput auto-style8" type="text" readonly="readonly" value="0" /></td>
                            </tr>
                        </table>
                    </div>
                </div>
            </td>
        </tr>
    </table>
    <script src="../../../Scripts/cnip/webui/generic.js"></script>
    <script src="../../../Scripts/cnip/webui/dialogs/linkprofile.js"></script>
</body>
</html>