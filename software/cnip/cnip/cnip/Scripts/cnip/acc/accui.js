if (puaction === 'login') {
    document.title = 'cnip - Login';
    document.getElementById('headl3').hidden = false;
    document.getElementById('headl3').innerHTML = 'Sign In';
    document.getElementById('pname').disabled = true;
    document.getElementById('pcompany').disabled = true;
    document.getElementById('pemaili').hidden = false;
    document.getElementById('pemail').hidden = false;
    document.getElementById('pemail').disabled = false;
    document.getElementById('pemail').focus();
    document.getElementById('pemail').select();
    document.getElementById('ppassword').disabled = true;
    document.getElementById('ppasscode').disabled = true;
    document.getElementById('btnSubmit').value = 'Login';
    document.getElementById('signuplnk').hidden = false;
    document.getElementById('signupllnk').hidden = false;
    document.getElementById('resetpasswordlnk').hidden = false;
    document.getElementById('turnon2steplnk').hidden = false;
    document.getElementById('turnoff2steplnk').hidden = false;
}
if (puaction === 'loginact') {
    document.title = 'cnip - Login';
    document.getElementById('headl3').hidden = false;
    document.getElementById('headl3').innerHTML = 'Sign In';
    document.getElementById('pname').disabled = true;
    document.getElementById('pcompany').disabled = true;
    document.getElementById('pemail').disabled = true;
    document.getElementById('ppasswordi').hidden = false;
    document.getElementById('ppassword').hidden = false;
    document.getElementById('ppassword').disabled = false;
    document.getElementById('ppassword').focus();
    document.getElementById('ppassword').select();
    g2stepen = p2stepen;
    if (g2stepen === 'true') {
        document.getElementById('ppasscodei').hidden = false;
        document.getElementById('ppasscode').hidden = false;
        document.getElementById('ppasscode').disabled = false;
    }
    else {
        document.getElementById('ppasscode').disabled = true;
    }
    loginattempt = loginattempt;
    document.getElementById('loginl').hidden = false;
    document.getElementById('loginc').hidden = false;
    document.getElementById('loginc').innerHTML = '<br/><br/>Login Attempt : ' + loginattempt;
    document.getElementById('btnSubmit').value = 'Login';
    document.getElementById('resetpasswordlnk').hidden = false;
    document.getElementById('turnoff2steplnk').hidden = false;
}
if (puaction === 'signup') {
    document.title = 'cnip - Sign Up';
    document.getElementById('headl3').hidden = false;
    document.getElementById('headl3').innerHTML = 'Sign Up';
    document.getElementById('pnamei').hidden = false;
    document.getElementById('pname').hidden = false;
    document.getElementById('pname').disabled = false;
    document.getElementById('pname').focus();
    document.getElementById('pname').select();
    document.getElementById('pcompanyi').hidden = false;
    document.getElementById('pcompany').hidden = false;
    document.getElementById('pcompany').disabled = false;
    document.getElementById('pemaili').hidden = false;
    document.getElementById('pemail').hidden = false;
    document.getElementById('pemail').disabled = false;
    document.getElementById('ppasswordi').hidden = false;
    document.getElementById('ppassword').hidden = false;
    document.getElementById('ppassword').disabled = false;
    document.getElementById('ppasscode').disabled = true;
    document.getElementById('btnSubmit').value = 'Submit';
    document.getElementById('loginllnk').hidden = false;
    document.getElementById('loginlnk').hidden = false;
}
if (puaction === 'resetpassword') {
    document.title = 'cnip - Reset Password';
    document.getElementById('headl5').hidden = false;
    document.getElementById('headl5').innerHTML = 'Reset Password';
    document.getElementById('pname').disabled = true;
    document.getElementById('pcompany').disabled = true;
    document.getElementById('pemaili').hidden = false;
    document.getElementById('pemail').hidden = false;
    document.getElementById('pemail').disabled = false;
    document.getElementById('pemail').focus();
    document.getElementById('pemail').select();
    document.getElementById('ppassword').disabled = true;
    document.getElementById('ppasscode').disabled = true;
    document.getElementById('btnSubmit').value = 'Submit';
}
if (puaction === 'resetpasswordact') {
    document.title = 'cnip - Reset Password';
    document.getElementById('headl5').hidden = false;
    document.getElementById('headl5').innerHTML = 'Reset Password';
    document.getElementById('pname').disabled = true;
    document.getElementById('pcompany').disabled = true;
    document.getElementById('pemail').disabled = true;
    document.getElementById('ppasswordi').hidden = false;
    document.getElementById('ppassword').hidden = false;
    document.getElementById('ppassword').disabled = false;
    document.getElementById('ppassword').focus();
    document.getElementById('ppassword').select();
    document.getElementById('ppasscode').disabled = true;
    document.getElementById('btnSubmit').value = 'Submit';
}
if (puaction === 'turnoff2step') {
    document.title = 'cnip - Turn Off 2-Step Verification';
    document.getElementById('headl5').hidden = false;
    document.getElementById('headl5').innerHTML = 'Turn Off 2-Step Verification';
    document.getElementById('pname').disabled = true;
    document.getElementById('pcompany').disabled = true;
    document.getElementById('pemaili').hidden = false;
    document.getElementById('pemail').hidden = false;
    document.getElementById('pemail').disabled = false;
    document.getElementById('pemail').focus();
    document.getElementById('pemail').select();
    document.getElementById('ppasswordi').hidden = false;
    document.getElementById('ppassword').hidden = false;
    document.getElementById('ppassword').disabled = false;
    document.getElementById('ppasscode').disabled = true;
    document.getElementById('btnSubmit').value = 'Submit';
}
if (puaction === 'turnon2step') {
    document.title = 'cnip - Turn On 2-Step Verification';
    document.getElementById('headl5').hidden = false;
    document.getElementById('headl5').innerHTML = 'Turn On 2-Step Verification';
    document.getElementById('pname').disabled = true;
    document.getElementById('pcompany').disabled = true;
    document.getElementById('pemaili').hidden = false;
    document.getElementById('pemail').hidden = false;
    document.getElementById('pemail').disabled = false;
    document.getElementById('pemail').focus();
    document.getElementById('pemail').select();
    document.getElementById('ppasswordi').hidden = false;
    document.getElementById('ppassword').hidden = false;
    document.getElementById('ppassword').disabled = false;
    document.getElementById('ppasscode').disabled = true;
    document.getElementById('btnSubmit').value = 'Submit';
    document.getElementById('g2step2i').hidden = false;
    document.getElementById('g2step2l').hidden = false;
}
if (puaction === 'turnon2stepact') {
    document.title = 'cnip - Turn On 2-Step Verification';
    document.getElementById('headl5').hidden = false;
    document.getElementById('headl5').innerHTML = 'Turn On 2-Step Verification';
    document.getElementById('pname').disabled = true;
    document.getElementById('pcompany').disabled = true;
    document.getElementById('pemail').disabled = true;
    document.getElementById('ppassword').disabled = true;
    document.getElementById('ppasscode').disabled = true;
    document.getElementById('btnSubmit').value = 'Submit';
    document.getElementById('g2stepi').hidden = false;
    document.getElementById('g2stepl').hidden = false;
    document.getElementById('barcodeimgi').hidden = false;
    document.getElementById('barcodeimg').hidden = false;
    document.getElementById('barcodeimg').src = barcode;
    document.getElementById('setupcode').innerHTML = setupcode;
}
function doSubmit() {
    let vpname = document.getElementById('pname').value;
    let vpcompany = document.getElementById('pcompany').value;
    let vpemail = document.getElementById('pemail').value;
    let vppassword = document.getElementById('ppassword').value;
    let vppasscode = document.getElementById('ppasscode').value;
    vpemail = vpemail.toLowerCase();
    if (puaction === 'signup') {
        if (vpemail !== '' && vpname !== '' && vpcompany !== '' && vppassword !== '') {
            window.open('../pages/accsrvrq.aspx?uaction=signup&name=' + vpname + '&company=' + vpcompany + '&email=' + vpemail + '&password=' + vppassword, '_self');
        }
    }
    if (puaction === 'resetpassword') {
        if (vpemail !== '') {
            window.open('../pages/accsrvrq.aspx?uaction=resetpassword&email=' + vpemail, '_self');
        }
    }
    if (puaction === 'turnoff2step') {
        if (vpemail !== '' && vppassword !== '') {
            window.open('../pages/accsrvrq.aspx?uaction=turnoff2step&email=' + vpemail + '&password=' + vppassword, '_self');
        }
    }
    if (puaction === 'login') {
        if (vpemail !== '') {
            window.open('../pages/accui.aspx?uaction=loginact&email=' + vpemail, '_self');
        }
    }
    if (puaction === 'turnon2step') {
        if (vpemail !== '' && vppassword !== '') {
            window.open('../pages/accui.aspx?uaction=turnon2stepact&email=' + vpemail + '&password=' + vppassword, '_self');
        }
    }
    if (puaction === 'loginact') {
        window.open('../pages/accsrv.aspx?uid=' + puid + '&ukey=' + pukey + '&password=' + vppassword + '&passcode=' + vppasscode + '&uaction=login', '_self');
    }
    if (puaction === 'resetpasswordact') {
        if (vppassword !== '') {
            window.open('../pages/accsrv.aspx?uid=' + puid + '&ukey=' + pukey + '&password=' + vppassword + '&uaction=resetpassword', '_self');
        }
    }
    if (puaction === 'turnon2stepact') {
        window.open('../pages/accsrv.aspx?uid=' + puid + '&ukey=' + pukey + '&uaction=turnon2step', '_self');
    }
}