<%@ Page Language="C#" AutoEventWireup="true" CodeBehind="accsrv.aspx.cs" Inherits="cnip.pages.accsrv" %>

<!DOCTYPE html>
<html xmlns="http://www.w3.org/1999/xhtml" style="overflow: hidden;">
<head runat="server">
    <meta charset="utf-8" />
    <meta http-equiv="X-UA-Compatible" content="IE=edge" />
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <!-- title -->
    <title>CNIP</title>
    <link rel="manifest" href="../manifest.json"/>
    <link rel="shortcut icon" href="https://drive.google.com/uc?id=1CE4IwHGAZakAtwNuTJkIWy1K2rE0pdm2"/>
    <!-- jQuery -->
    <script src="../Scripts/jquery.min.js"></script>
    <!-- bootstrap -->
    <link href="../Scripts/bootstrap.4.1.1/bootstrap.min.css" rel="stylesheet" />
    <script src="../Scripts/bootstrap.4.1.1/bootstrap.min.js"></script>
    <!-- modernizr -->
    <script src="../Scripts/modernizr-2.8.3.js"></script>
    <!-- font awesome -->
    <link href="../Content/all.css" rel="stylesheet" />
    <!-- cnip styles -->
    <link href="../Content/cnip/acc.css" rel="stylesheet" />
    <style>
        h3 {
        font-size:1em;
        }
        h4 {
        font-size:1em;
        }
        h5 {
        font-size:1em;
        }
        h6 {
        font-size:1em;
        }
    </style>
</head>
<body>
    <nav class="navbar navbar-default">
        <div class="container-fluid">
            <div class="navbar-header">
                <a href="glavni.aspx" class="navbar-brand">
                    <img alt="" style="width: 113px; height: 30px;" src="../img/cnip.png" />
                </a>
            </div>
            <div class="nav navbar-nav navbar-right">
                <a href="userguide.aspx" id="userGuide" class="btn btn-primary" target="_blank"><i class="fas fa-book"></i>&nbsp;User Guide</a>
            </div>
        </div>
    </nav>
    <div class="container" style="background-image: url(../img/glavni.png); background-repeat: no-repeat; background-size: contain; max-width: 1000px; position: relative; margin: auto; width: 100%; margin-top: 163.5px;">
        <div class="d-flex justify-content-center h-100">
            <div class="card">
                <div class="card-body">
                    <h3 id="th1" style="color:var(--barcolor);"></h3>
                    <h4 id="sh1"></h4>                    
                    <h5 id="th2" style="color:var(--barcolor);"></h5>
                    <h6 id="sh2"></h6>
                </div>
                <div class="card-footer">
                    <a id="btnlnk" href="#">
                        <input id="btn" hidden="hidden" type="button" value="" class="btn float-right login_btn" />
                    </a>
                    <div class="d-flex justify-content-center">
                        <a id="forgotlnk" hidden="hidden" href="accui.aspx?uaction=resetpassword">Forgot your password?</a>
                    </div>
                    <div class="d-flex justify-content-center">
                        <a id="lostlnk" hidden="hidden" href="accui.aspx?uaction=turnoff2step">Turn Off 2-Step Verification?</a>
                    </div>
                    <div class="d-flex justify-content-center">
                        <a id="g2steplnk" hidden="hidden" href="accui.aspx?uaction=turnon2step">Turn On 2-Step Verification?</a>
                    </div>
                </div>
            </div>
        </div>
    </div>
    <script type="text/javascript">
        var pmsg = '<%=Getmsg()%>';
    </script>
    <script src="../Scripts/cnip/acc/accsrv.js"></script>
</body>
</html>