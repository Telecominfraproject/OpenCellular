document.getElementById('uname').innerHTML = username;
document.getElementById('uemail').innerHTML = email;
/* account functions*/
function Logoff(e) {
    if (e) { e.stopPropagation(); }
    if (actionRunning) { return; }
    Ajax_runccmd("Logoff", "");
    window.open('../pages/glavni.aspx', '_self');
}