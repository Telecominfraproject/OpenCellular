<%@ Page Language="C#" AutoEventWireup="true" CodeBehind="glavni.aspx.cs" Inherits="cnip.pages.glavni" %>

<!DOCTYPE html>
<html xmlns="http://www.w3.org/1999/xhtml">
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
    <style type="text/css">
      .card {
            border-radius: 0;
            background: none;
            border: none;
            margin-bottom: 0;
            box-shadow: none;
            -webkit-box-shadow: none;
            height:auto;
            margin:auto;
            width:auto;
        }

        .card-body {
            padding: 0;
        }

        .btn {
            border-color: var(--bordercolor);
            color: var(--fontcolor);
            width: 150px;
        }

            .btn:hover {
                color: var(--hovercolor);
                font-weight: 600;
            }

        img {
            vertical-align: middle;
        }

        .splash-container {
            max-width: 1000px;
            position: relative;
            margin: auto;
        }

        h4 {
            font-size: calc(14px + .5vw);
            color: var(--hovercolor);
        }
    </style>
</head>
<body style="height: 100%; overflow: hidden;">
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
    <div class="card">
        <div class="card card-body">
            <div>
                <h4 class="text-center">
                    <b>Community Network Interactive Planner</b>
                    <br />
                    an online radio propagation and network design tool
                    <br />
                    fast, easy, reliable and full of unique insights
                    </h4>
            </div>
            <div style="text-align: center; margin-top: 20px; margin-bottom: 20px;">
                <a id="signup" href="accui.aspx?uaction=signup" class="btn btn-default"><i class="fa fa-user"></i>&nbsp;Sign Up</a>
                &nbsp;                    
                <a id="login" href="accui.aspx?uaction=login" class="btn btn-default"><i class="fa fa-sign-in-alt"></i>&nbsp;Login</a>
            </div>
        </div>
    </div>
    <div class="splash-container">
        <div>
            <img src="../img/glavni.png" style="width: 100%" />
        </div>
    </div>
</body>
</html>