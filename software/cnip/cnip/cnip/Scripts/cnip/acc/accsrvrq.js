if (pmsg === 'turnoff2step : account not active') {
    document.getElementById('th1').innerHTML = 'Your account is not activated';
    document.getElementById('sh1').innerHTML = 'Please look for the verification email in your inbox and click the link in that email. A confirmation message will appear in your web browser.';
    document.getElementById('th2').innerHTML = 'Didn\'t get the email?';
    document.getElementById('sh2').innerHTML = 'Check your spam folder to make sure it didn\'t end up there. You can also add the email address opencellularcnip@gmail.com to your address book and then try sending the email again.' +
        '<br /><br />' +
        'If you\'re still not able to find the email, please send an email to us at opencellularcnip@gmail.com';
}
if (pmsg === 'turnoff2step : 2-Step Verification not setup') {
    document.getElementById('th1').innerHTML = '2-Step Verification is not activated for your account';
    document.getElementById('sh1').innerHTML = '';
    document.getElementById('th2').innerHTML = 'Activate 2-Step Verification?';
    document.getElementById('sh2').innerHTML = '2-step Verification adds another layer of security to prevent unauthorized access to your account. Please click the Turn On 2-Step Verification? link to activate 2-Step Verification for your account.';
    document.getElementById('g2steplnk').hidden = false;
}
if (pmsg === 'turnoff2step : turn off 2-Step Verification link sent') {
    document.getElementById('th1').innerHTML = 'A link has been sent to your email address to deactivate 2-Step Verification for your account';
    document.getElementById('sh1').innerHTML = 'Please look for the deactivate 2-Step Verification email in your inbox and click the link in that email. A confirmation message will appear in your web browser.';
    document.getElementById('th2').innerHTML = 'Didn\'t get the email?';
    document.getElementById('sh2').innerHTML = 'Check your spam folder to make sure it didn\'t end up there. You can also add the email address opencellularcnip@gmail.com to your address book and then try sending the email again.' +
        '<br /><br />' +
        'If you\'re still not receiving the email, please send an email to us at opencellularcnip@gmail.com';
}
if (pmsg === 'turnoff2step : error accessing server') {
    document.getElementById('th1').innerHTML = 'Oops. Something went wrong. Please try again later';
    document.getElementById('sh1').innerHTML = 'Please click the Turn Off 2-Step Verification? link to try deactivating 2-Step Verification for your account.';
    document.getElementById('th2').innerHTML = '';
    document.getElementById('sh2').innerHTML = 'If you continue facing this issue, please send an email to us at opencellularcnip@gmail.com';
    document.getElementById('lostlnk').hidden = false;
}
if (pmsg === 'turnoff2step : failed to send email') {
    document.getElementById('th1').innerHTML = 'Oops. Something went wrong. Please try again later';
    document.getElementById('sh1').innerHTML = 'We are unable to send deactivate 2-Step Verification link to your email address at this time. Please click the Turn Off 2-Step Verification? link to try deactivating 2-Step Verification for your account.';
    document.getElementById('th2').innerHTML = '';
    document.getElementById('sh2').innerHTML = 'If you continue facing this issue, please send an email to us at opencellularcnip@gmail.com';
    document.getElementById('lostlnk').hidden = false;
}
if (pmsg === 'turnoff2step : email not registered or invalid password') {
    document.getElementById('th1').innerHTML = 'Email address not registered or Invalid password';
    document.getElementById('sh1').innerHTML = 'Make sure you entered the correct email address and password. Pease click Turn Off 2-Step Verification? link to try deactivating 2-Step Verification for your account.';
    document.getElementById('th2').innerHTML = 'Don\'t have an account or Forgot your password?';
    document.getElementById('sh2').innerHTML = '1. Click Sign Up to create your free account.<br/>2. Click the Forgot your password? link if you don\'t remember your password.';
    document.getElementById('btn').hidden = false;
    document.getElementById('btn').value = 'Sign Up';
    document.getElementById('btnlnk').href = 'accui.aspx?uaction=signup';
    document.getElementById('forgotlnk').hidden = false;
    document.getElementById('lostlnk').hidden = false;
}
if (pmsg === 'resetpassword : email not registered') {
    document.getElementById('th1').innerHTML = 'Email address not registered';
    document.getElementById('sh1').innerHTML = 'We didn\'t find any account associated with your email address. Please make sure you entered the correct email address. Please click the Forgot your password? link to try resetting your password.';
    document.getElementById('th2').innerHTML = 'Don\'t have an account?';
    document.getElementById('sh2').innerHTML = 'Please click Sign Up to create your free account.';
    document.getElementById('btn').hidden = false;
    document.getElementById('btn').value = 'Sign Up';
    document.getElementById('btnlnk').href = 'accui.aspx?uaction=signup';
    document.getElementById('forgotlnk').hidden = false;
}
if (pmsg === 'resetpassword : link sent') {
    document.getElementById('th1').innerHTML = 'A reset password link has been sent to your email address';
    document.getElementById('sh1').innerHTML = 'Please look for the reset password email in your inbox and click the link in that email. New password request form will appear in your web browser.';
    document.getElementById('th2').innerHTML = 'Didn\'t get the email?';
    document.getElementById('sh2').innerHTML = 'Check your spam folder to make sure it didn\'t end up there. You can also add the email address opencellularcnip@gmail.com to your address book and then try sending the email again.' +
        '<br /><br />' +
        'If you\'re still not receiving the email, please send an email to us at opencellularcnip@gmail.com';
}
if (pmsg === 'resetpassword : failed to send email') {
    document.getElementById('th1').innerHTML = 'Oops. Something went wrong. Please try again later';
    document.getElementById('sh1').innerHTML = 'We are unable to send reset password link to your email address at this time. Please click the Forgot your password? link to try resetting your password.';
    document.getElementById('th2').innerHTML = '';
    document.getElementById('sh2').innerHTML = 'If you continue facing this issue, please send an email to us at opencellularcnip@gmail.com';
    document.getElementById('forgotlnk').hidden = false;
}
if (pmsg === 'resetpassword : error accessing server') {
    document.getElementById('th1').innerHTML = 'Oops. Something went wrong. Please try again later';
    document.getElementById('sh1').innerHTML = 'Please click the Forgot your password? link to try resetting your password.';
    document.getElementById('th2').innerHTML = '';
    document.getElementById('sh2').innerHTML = 'If you continue facing this issue, please send an email to us at opencellularcnip@gmail.com';
    document.getElementById('forgotlnk').hidden = false;
}
if (pmsg === 'signup : email address already registered') {
    document.getElementById('th1').innerHTML = 'Email address already in use';
    document.getElementById('sh1').innerHTML = 'There\'s already an account associated with your email address, if you don\'t own that account, please send an email to us at opencellularcnip@gmail.com.';
    document.getElementById('th2').innerHTML = 'To access your account:';
    document.getElementById('sh2').innerHTML = '1. Click Login.<br/>2. Click the Forgot your password? link if you don\'t remember your password.';
    document.getElementById('btn').hidden = false;
    document.getElementById('btn').value = 'Login';
    document.getElementById('btnlnk').href = 'accui.aspx?uaction=login';
    document.getElementById('forgotlnk').hidden = false;
}
if (pmsg === 'signup : user created successfully') {
    document.getElementById('th1').innerHTML = 'A verfication link has been sent to your email address';
    document.getElementById('sh1').innerHTML = 'Please look for the verification email in your inbox and click the link in that email. A confirmation message will appear in your web browser.';
    document.getElementById('th2').innerHTML = 'Didn\'t get the email?';
    document.getElementById('sh2').innerHTML = 'Check your spam folder to make sure it didn\'t end up there. You can also add the email address opencellularcnip@gmail.com to your address book and then try sending the email again.' +
        '<br /><br />' +
        'If you\'re still not receiving the email, please send an email to us at opencellularcnip@gmail.com';
}
if (pmsg === 'signup : failed to send email') {
    document.getElementById('th1').innerHTML = 'Oops. Something went wrong. Please try again later';
    document.getElementById('sh1').innerHTML = 'We are unable to send verification link to your email address. Please make sure you typed the correct email address. Please click Sign Up to try creating your free account.';
    document.getElementById('th2').innerHTML = '';
    document.getElementById('sh2').innerHTML = 'If you continue facing this issue, please send an email to us at opencellularcnip@gmail.com';
    document.getElementById('btn').hidden = false;
    document.getElementById('btn').value = 'Sign Up';
    document.getElementById('btnlnk').href = 'accui.aspx?uaction=signup';
}
if (pmsg === 'signup : error accessing server') {
    document.getElementById('th1').innerHTML = 'Oops. Something went wrong. Please try again later';
    document.getElementById('sh1').innerHTML = 'We are unable to successfully complete your registration at this time. Please click Sign Up to try creating your free account.';
    document.getElementById('th2').innerHTML = '';
    document.getElementById('sh2').innerHTML = 'If you continue facing this issue, please send an email to us at opencellularcnip@gmail.com';
    document.getElementById('btn').hidden = false;
    document.getElementById('btn').value = 'Sign Up';
    document.getElementById('btnlnk').href = 'accui.aspx?uaction=signup';
}
if (pmsg === 'blank form submitted') {
    document.getElementById('th1').innerHTML = 'Blank form submitted';
}