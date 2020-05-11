// Account activation messages
if (pmsg === 'activate : account already activated' || pmsg === 'activate : account activated successfully') {
    document.getElementById('th1').innerHTML = 'Your account has been activated successfully';
    document.getElementById('sh1').innerHTML = 'You can now login to your account using your email address and password. Please click Login to login to your account.';
    document.getElementById('th2').innerHTML = 'Activate 2-Step Verification?';
    document.getElementById('sh2').innerHTML = '2-step Verification adds another layer of security to prevent unauthorized access to your account. Please click the Turn On 2-Step Verification? link to activate 2-Step Verification for your account.';
    document.getElementById('btn').hidden = false;
    document.getElementById('btn').value = 'Login';
    document.getElementById('btnlnk').href = 'accui.aspx?uaction=login';
    document.getElementById('g2steplnk').hidden = false;
}
if (pmsg === 'activate : error accessing server') {
    document.getElementById('th1').innerHTML = 'Oops. Something went wrong. Please try again later';
    document.getElementById('sh1').innerHTML = 'Please look for the verification email in your inbox and click the link in that email. A confirmation message will appear in your web browser.';
    document.getElementById('th2').innerHTML = '';
    document.getElementById('sh2').innerHTML = 'If you continue facing this issue, please send an email to us at opencellularcnip@gmail.com';
}
if (pmsg === 'activate : link expired') {
    document.getElementById('th1').innerHTML = 'Oops. Link expired. Please try again later';
    document.getElementById('sh1').innerHTML = 'Your account activation link has been expired. If your account is not activated you can reset your password to activate your account. Please click the Forgot your password? link to try resetting the password to activate your account.';
    document.getElementById('th2').innerHTML = '';
    document.getElementById('sh2').innerHTML = 'If you continue facing this issue, please send an email to us at opencellularcnip@gmail.com';
    document.getElementById('forgotlnk').hidden = false;
}
// Reset password messages
if (pmsg === 'resetpassword : password changed successfully') {
    document.getElementById('th1').innerHTML = 'Your password has been changed successfully';
    document.getElementById('sh1').innerHTML = 'You can now login to your account using your email address and password. Please click Login to login to your account.';
    document.getElementById('th2').innerHTML = '';
    document.getElementById('sh2').innerHTML = '';
    document.getElementById('btn').hidden = false;
    document.getElementById('btn').value = 'Login';
    document.getElementById('btnlnk').href = 'accui.aspx?uaction=login';
}
if (pmsg === 'resetpassword : error accessing server') {
    document.getElementById('th1').innerHTML = 'Oops. Something went wrong. Please try again later';
    document.getElementById('sh1').innerHTML = 'Please look for the reset password email in your inbox and click the link in that email. New password request form will appear in your web browser.';
    document.getElementById('th2').innerHTML = '';
    document.getElementById('sh2').innerHTML = 'If you continue facing this issue, please send an email to us at opencellularcnip@gmail.com';
}
if (pmsg === 'resetpassword : link expired') {
    document.getElementById('th1').innerHTML = 'Oops. Link expired. Please try again later';
    document.getElementById('sh1').innerHTML = 'Your password reset link has been expired. Please click the Forgot your password? link to try resetting your password.';
    document.getElementById('th2').innerHTML = '';
    document.getElementById('sh2').innerHTML = 'If you continue facing this issue, please send an email to us at opencellularcnip@gmail.com';
    document.getElementById('forgotlnk').hidden = false;
}
// turn off 2-step verification messages
if (pmsg === 'turnoff2step : 2-Step Verification turned off') {
    document.getElementById('th1').innerHTML = '2-Step Verification deactivated successfully';
    document.getElementById('sh1').innerHTML = 'You can now login to your account using your email address and password. Please click Login to login to your account.';
    document.getElementById('th2').innerHTML = 'Activate 2-Step Verification?';
    document.getElementById('sh2').innerHTML = '2-step Verification adds another layer of security to prevent unauthorized access to your account. Please click the Turn On 2-Step Verification? link to activate 2-Step Verification for your account.';
    document.getElementById('btn').hidden = false;
    document.getElementById('btn').value = 'Login';
    document.getElementById('btnlnk').href = 'accui.aspx?uaction=login';
    document.getElementById('g2steplnk').hidden = false;
}
if (pmsg === 'turnoff2step : error accessing server') {
    document.getElementById('th1').innerHTML = 'Oops. Something went wrong. Please try again later';
    document.getElementById('sh1').innerHTML = 'Please look for the deactivate 2-Step Verification email in your inbox and click the link in that email. A confirmation message will appear in your web browser.';
    document.getElementById('th2').innerHTML = '';
    document.getElementById('sh2').innerHTML = 'If you continue facing this issue, please send an email to us at opencellularcnip@gmail.com';
}
if (pmsg === 'turnoff2step : link expired') {
    document.getElementById('th1').innerHTML = 'Oops. Link expired. Please try again later';
    document.getElementById('sh1').innerHTML = 'Your deactivate 2-Step Verification link has been expired. Please click the Turn Off 2-Step Verification? link to try deactivating 2-Step Verification for your account.';
    document.getElementById('th2').innerHTML = '';
    document.getElementById('sh2').innerHTML = 'If you continue facing this issue, please send an email to us at opencellularcnip@gmail.com';
    document.getElementById('lostlnk').hidden = false;
}
// turn on 2-step verification messages
if (pmsg === 'turnon2stepact : email not registered or invalid password') {
    document.getElementById('th1').innerHTML = 'Email address not registered or Invalid password';
    document.getElementById('sh1').innerHTML = 'Make sure you entered the correct email address and password. Pease click Turn On 2-Step Verification? link to try activating 2-Step Verification for your account.';
    document.getElementById('th2').innerHTML = 'Don\'t have an account or Forgot your password?';
    document.getElementById('sh2').innerHTML = '1. Click Sign Up to create your free account.<br/>2. Click the Forgot your password? link if you don\'t remember your password.';
    document.getElementById('btn').hidden = false;
    document.getElementById('btn').value = 'Sign Up';
    document.getElementById('btnlnk').href = 'accui.aspx?uaction=signup';
    document.getElementById('forgotlnk').hidden = false;
    document.getElementById('g2steplnk').hidden = false;
}
if (pmsg === 'turnon2stepact : 2-Step Verification already setup') {
    document.getElementById('th1').innerHTML = '2-Step Verification is already activated for your account';
    document.getElementById('sh1').innerHTML = 'In addition to your password, you\'ll also need a code generated by the Google Authenticator app on your device. Please click Login to login to your account.';
    document.getElementById('th2').innerHTML = '';
    document.getElementById('sh2').innerHTML = '';
    document.getElementById('btn').hidden = false;
    document.getElementById('btn').value = 'Login';
    document.getElementById('btnlnk').href = 'accui.aspx?uaction=login';
}
if (pmsg === 'turnon2stepact : error accessing server' || pmsg === 'turnon2step : error accessing server') {
    document.getElementById('th1').innerHTML = 'Oops. Something went wrong. Please try again later';
    document.getElementById('sh1').innerHTML = '2-step Verification adds another layer of security to prevent unauthorized access to your account. Please click the Turn On 2-Step Verification? link to activate 2-Step Verification for your account.';
    document.getElementById('th2').innerHTML = '';
    document.getElementById('sh2').innerHTML = '';
    document.getElementById('g2steplnk').hidden = false;
}
if (pmsg === 'turnon2step : 2-Step Verification turned on') {
    document.getElementById('th1').innerHTML = '2-Step Verification has been activated for your account successfully';
    document.getElementById('sh1').innerHTML = 'In addition to your password, you\'ll also need a code generated by the Google Authenticator app on your device. Please click Login to login to your account.';
    document.getElementById('th2').innerHTML = '';
    document.getElementById('sh2').innerHTML = '';
    document.getElementById('btn').hidden = false;
    document.getElementById('btn').value = 'Login';
    document.getElementById('btnlnk').href = 'accui.aspx?uaction=login';
}
if (pmsg === 'turnon2step : link expired') {
    document.getElementById('th1').innerHTML = 'Oops. Link expired. Please try again later';
    document.getElementById('sh1').innerHTML = 'Your activate 2-Step Verification link has been expired. Please click the Turn On 2-Step Verification? link to try activating 2-Step Verification for your account.';
    document.getElementById('th2').innerHTML = '';
    document.getElementById('sh2').innerHTML = 'If you continue facing this issue, please send an email to us at opencellularcnip@gmail.com';
    document.getElementById('btn').hidden = true;
    document.getElementById('g2steplnk').hidden = false;
}
// login messages
if (pmsg === 'loginact : account suspended') {
    document.getElementById('th1').innerHTML = 'Your account has been suspended';
    document.getElementById('sh1').innerHTML = 'Your account may have been suspended due to multiple unsuccessful login attempts. This is an automated security feature and cannot be disabled.';
    document.getElementById('sh2').innerHTML = 'Please click the Forgot your password? link to reset password and un-suspend your account.';
    document.getElementById('forgotlnk').hidden = false;
}
if (pmsg === 'login : login successful') {
    document.getElementById('th1').innerHTML = 'login successful';
}
if (pmsg === 'login : invalid passcode') {
    document.getElementById('th1').innerHTML = 'You have entered an invalid passcode, please try again';
    document.getElementById('sh1').innerHTML = 'Please click Login to try login to your account.';
    document.getElementById('th2').innerHTML = 'Lost your 2-Step Verification device?';
    document.getElementById('sh2').innerHTML = 'Please click the Turn Off 2-Step Verification? link to deactivate 2-Step Verification for your account.';
    document.getElementById('btn').hidden = false;
    document.getElementById('btn').value = 'Login';
    document.getElementById('btnlnk').href = 'accui.aspx?uaction=login';
    document.getElementById('lostlnk').hidden = false;
}
if (pmsg === 'login : invalid password') {
    document.getElementById('th1').innerHTML = 'You have entered an invalid password, please try again';
    document.getElementById('sh1').innerHTML = 'Please click Login to try login to your account.';
    document.getElementById('th2').innerHTML = 'Lost your password?';
    document.getElementById('sh2').innerHTML = 'Please click the Forgot your password? link to reset your password.';
    document.getElementById('btn').hidden = false;
    document.getElementById('btn').value = 'Login';
    document.getElementById('btnlnk').href = 'accui.aspx?uaction=login';
    document.getElementById('forgotlnk').hidden = false;
}
if (pmsg === 'login : link expired') {
    document.getElementById('th1').innerHTML = 'Oops. Link expired. Please try again later';
    document.getElementById('sh1').innerHTML = 'Your login link has been expired. Please click Login to try login to your account.';
    document.getElementById('th2').innerHTML = '';
    document.getElementById('sh2').innerHTML = 'If you continue facing this issue, please send an email to us at opencellularcnip@gmail.com';
    document.getElementById('btn').hidden = false;
    document.getElementById('btn').value = 'Login';
    document.getElementById('btnlnk').href = 'accui.aspx?uaction=login';
}
if (pmsg === 'login : error accessing server' || pmsg === 'loginact : error accessing server') {
    document.getElementById('th1').innerHTML = 'Oops. Something went wrong. Please try again later';
    document.getElementById('sh1').innerHTML = 'Please click Login to try login to your account.';
    document.getElementById('th2').innerHTML = '';
    document.getElementById('sh2').innerHTML = 'If you continue facing this issue, please send an email to us at opencellularcnip@gmail.com';
    document.getElementById('btn').hidden = false;
    document.getElementById('btn').value = 'Login';
    document.getElementById('btnlnk').href = 'accui.aspx?uaction=login';
}
if (pmsg === 'loginact : email not registered') {
    document.getElementById('th1').innerHTML = 'Email address not registered';
    document.getElementById('sh1').innerHTML = 'We didn\'t find any account associated with your email address. Please make sure you entered the correct email address. Please click Login to try login to your account.';
    document.getElementById('btn').hidden = false;
    document.getElementById('btn').value = 'Login';
    document.getElementById('btnlnk').href = 'accui.aspx?uaction=login';
}
if (pmsg === 'loginact : account not active') {
    document.getElementById('th1').innerHTML = 'Your account is not activated';
    document.getElementById('sh1').innerHTML = 'Please look for the verification email in your inbox and click the link in that email. A confirmation message will appear in your web browser.';
    document.getElementById('th2').innerHTML = 'Didn\'t get the email?';
    document.getElementById('sh2').innerHTML = 'Check your spam folder to make sure it didn\'t end up there. You can also add the email address opencellularcnip@gmail.com to your address book and then try sending the email again.' +
        '<br /><br />' +
        'If you\'re still not able to find the email, please send an email to us at opencellularcnip@gmail.com';
}
if (pmsg === 'blank form submitted') {
    document.getElementById('th1').innerHTML = 'Blank form submitted';
}