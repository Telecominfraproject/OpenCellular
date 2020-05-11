<%@ Page Language="C#" AutoEventWireup="true" CodeBehind="noteoptions.aspx.cs" Inherits="cnip.pages.noteoptions" %>

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
    <hr />
    <div id="General" class="pageTabContent">
        <table>
            <tr>
                <td style="width: 43%;">Note                   
                    <hr />
                    <table style="display:none;">
                        <tr>
                            <td style="width: 40%;">
                                <span>Id</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="noteid" type="text" readonly="readonly" disabled="disabled" class="textInput" />
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Name</span>
                            </td>
                            <td style="width: 60%;">
                                <input id="notename" type="text" class="textInput" />
                            </td>
                        </tr>
                    </table>
                    <table>
                        <tr>
                            <td style="width: 40%;">
                                <span>Type</span>
                            </td>
                            <td style="width: 60%;">
                                <div id="notetype"></div>
                            </td>
                        </tr>
                    </table>
                </td>
                <td style="width: 2%;"></td>
                <td style="width: 55%;">Description
                    <hr />
                    <table>
                        <tr>
                            <td style="width: 100%;">
                                <textarea id="description" wrap="soft" class="textInput" style="height: 77px;"> </textarea>
                            </td>
                        </tr>
                    </table>
                </td>
            </tr>
        </table>
    </div>
    <button class="pageBottomButton" style="left: 10px; color: red;" title="Delete" onclick="deleteNote(event);"><i class="fas fa-trash-alt"></i></button>
    <button class="pageBottomButton" style="right: 10px;" title="Next" onclick="nextNote(event);"><i class="fas fa-angle-double-right"></i></button>
    <button class="pageBottomButton" style="right: 35px;" title="Previous" onclick="previousNote(event);"><i class="fas fa-angle-double-left"></i></button>
    <button class="pageBottomButton" style="right: 60px; color: rgb(3,169,244);" title="Locate" onclick="zoomToNote(event);"><i class="fas fa-map-marker-alt"></i></button>
    <script src="../../../Scripts/cnip/webui/generic.js"></script>
    <script src="../../../Scripts/cnip/webui/dialogs/noteoptions.js"></script>
</body>
</html>
