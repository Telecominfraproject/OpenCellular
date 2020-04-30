<%@ Page Language="C#" AutoEventWireup="true" CodeBehind="accui.aspx.cs" Inherits="cnip.pages.accui" %>

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
                <div class="card-header">
                    <h3 id="headl3" hidden="hidden"></h3>
                    <h5 id="headl5" hidden="hidden"></h5>
                </div>
                <div class="card-body">
                    <form id="accui" onsubmit="doSubmit();return false">
                        <div id="g2stepi" hidden="hidden" class="input-group form-group">
                            <a id="g2stepl" hidden="hidden" style="color: var(--fontcolor); font-size: small">Open Google Authenticator app on your device, click add and scan the below barcode to activate 2-Step Verification then click Submit.</a>
                        </div>
                        <div id="barcodeimgi" hidden="hidden" class="input-group form-group" style="display: table-cell; vertical-align: middle; text-align: center; width: 400px; height: 200px;">
                            <img id="barcodeimg" src="" />
                        </div>
                        <div id="pnamei" hidden="hidden" class="input-group form-group">
                            <div class="input-group-prepend">
                                <span class="input-group-text"><i class="fas fa-user"></i></span>
                            </div>
                            <input id="pname" hidden="hidden" name="pname" type="text" class="form-control" placeholder="name" pattern="([^\s][A-z0-9À-ž\s]+[^\s])" required="required" title="name should only contain letters and numbers and must not begin and end with space" />
                        </div>
                        <div id="pcompanyi" hidden="hidden" class="input-group form-group">
                            <div class="input-group-prepend">
                                <span class="input-group-text"><i class="fas fa-building"></i></span>
                            </div>
                            <input id="pcompany" hidden="hidden" name="pcompany" type="text" class="form-control" placeholder="company" pattern="([^\s][A-z0-9À-ž\s]+[^\s])" required="required" title="company should only contain letters and numbers and must not begin and end with space" />
                        </div>
                        <div id="pemaili" hidden="hidden" class="input-group form-group">
                            <div class="input-group-prepend">
                                <span class="input-group-text"><i class="fas fa-envelope"></i></span>
                            </div>
                            <input id="pemail" hidden="hidden" name="pemail" type="email" class="form-control" placeholder="email" required="required" title="please enter a valid email address" />
                        </div>
                        <div id="ppasswordi" hidden="hidden" class="input-group form-group">
                            <div class="input-group-prepend">
                                <span class="input-group-text"><i class="fas fa-key"></i></span>
                            </div>
                            <input id="ppassword" hidden="hidden" name="ppassword" type="password" class="form-control" placeholder="password" pattern="(?=.*\d)(?=.*[a-z])(?=.*[A-Z])\w{8,}" autocapitalize="off" autocorrect="off" autocomplete="off" required="required" title="password must contain at least one number/uppercase/lowercase letter and be at least 8 characters that are letters, numbers or the underscore" />
                        </div>
                        <div id="ppasscodei" hidden="hidden" class="input-group form-group">
                            <div class="input-group-prepend">
                                <span class="input-group-text"><i class="fas fa-code"></i></span>
                            </div>
                            <input id="ppasscode" hidden="hidden" name="ppasscode" type="text" class="form-control" placeholder="passcode" pattern="\d{6}" autocorrect="off" autocomplete="off" required="required" title="6 digit 2-Step Verification code" />
                        </div>
                        <div id="g2step2i" hidden="hidden" class="input-group form-group">
                            <a id="g2step2l" hidden="hidden" style="color: var(--fontcolor); font-size: small">2-Step Verification provides stronger security by requiring a second step of verification when you sign in. In addition to your password, you'll also need a code generated by the Google Authenticator app on your device.
                                <br />
                                <br />
                                Install the Google Authenticator app
                                <br />
                                1. Go to App store or Google Play store
                                <br />
                                2. Search for Google Authenticator
                                <br />
                                3. Download and install the application
                            </a>
                        </div>
                        <a id="loginl" hidden="hidden" style="color: var(--fontcolor); font-size: small">Please note your account will be suspended after three unsuccessful login attempts.</a>
                        <a id="loginc" hidden="hidden" style="color: var(--fontcolor); font-size: small"></a>
                        <div class="form-group">
                            <input id="btnSubmit" name="btnSubmit" type="submit" value="Submit" class="btn float-right login_btn" />
                        </div>
                    </form>
                </div>
                <div class="card-footer">
                    <div class="d-flex justify-content-center links">
                        <a id="loginllnk" hidden="hidden" style="color: var(--fontcolor);">Have an account?</a>
                        <a id="loginlnk" hidden="hidden" href="accui.aspx?uaction=login">Login</a>
                    </div>
                    <div class="d-flex justify-content-center links">
                        <a id="signupllnk" hidden="hidden" style="color: var(--fontcolor);">Don't have an account?</a>
                        <a id="signuplnk" hidden="hidden" href="accui.aspx?uaction=signup">Sign Up</a>
                    </div>
                    <div class="d-flex justify-content-center">
                        <a id="resetpasswordlnk" hidden="hidden" href="accui.aspx?uaction=resetpassword">Forgot your password?</a>
                    </div>
                    <div class="d-flex justify-content-center">
                        <a id="turnon2steplnk" hidden="hidden" href="accui.aspx?uaction=turnon2step">Turn On 2-Step Verification?</a>
                    </div>
                    <div class="d-flex justify-content-center">
                        <a id="turnoff2steplnk" hidden="hidden" href="accui.aspx?uaction=turnoff2step">Turn Off 2-Step Verification?</a>
                    </div>
                </div>
            </div>
        </div>
    </div>
    <script type="text/javascript">
        var puaction = '<%=Getuaction()%>';
        var p2stepen = '<%=Get2stepen()%>';
        var loginattempt = '<%=Getloginattempt()%>';
        var barcode = '<%=Getbarcode()%>';
        var setupcode = '<%=Getsetupcode()%>';
        var puid = '<%=Getuid()%>';
        var pukey = '<%=Getukey()%>';
    </script>
    <script src="../Scripts/cnip/acc/accui.js"></script>
</body>
</html>