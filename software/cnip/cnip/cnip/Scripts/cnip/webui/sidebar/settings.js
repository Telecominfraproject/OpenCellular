$('#coverage').collapse({
    toggle: true
});
$('#radioOptions').collapse({
    toggle: true
});
// initialize controls
initSelectOpt(document.getElementById('opt_antennamodel'), '100%', 'LT OD9-5 890-950 MHz\tLT OD9-6 865-945 MHz\tLT OD9-6 860-960 MHz\tLT OD9-8 865-945 MHz\tLT OD9-8 860-960 MHz\tLT OD9-11 860-960 MHz\tLT OD9-11D1 860-960 MHz', 'LT OD9-5 890-950 MHz');
initSingleSlider(document.getElementById('opt_cellradius'), '', '0.5', '3', '0.1', '1');
initSelectOpt(document.getElementById('opt_gsmband'), '100%', 'GSM900 PGSM', 'GSM900 PGSM');
initSelectOpt(document.getElementById('opt_gsmbandwidth'), '100%', '0.2', '0.2');
initSelectOpt(document.getElementById('opt_lteband'), '100%', 'B08_FDD_900 E-GSM', 'B08_FDD_900 E-GSM');
initSelectOpt(document.getElementById('opt_ltebandwidth'), '100%', '1.4\t3\t5\t10', '10');
initSingleSlider(document.getElementById('opt_frequency'), '', '1', '15', '0.1', '5');
initSelectOpt(document.getElementById('opt_channelwidth'), '100%', '10\t20\t30\t40\t50\t60\t80\t100', '20');
initSingleSlider(document.getElementById('opt_outputpower'), '', '27', '47', '0.5', '47');
initSingleSlider(document.getElementById('opt_antennagain'), '', '19', '51', '0.1', '22.5');
initSingleSlider(document.getElementById('opt_losses'), '', '0', '51', '0.1', '0');
initSingleSlider(document.getElementById('opt_fresnelclearance'), '', '60', '100', '5', '60');
initSelectOpt(document.getElementById('opt_measurementtype'), '100%', 'Path Loss (dB)\tReceived Power (dBm)\tField Strength (dBµV/m)\tSNR (dB)\tVoice Quality Score\tData Rate Score', 'Path Loss (dB)');
initSelectOpt(document.getElementById('opt_terrainresolution'), '100%', '90m\t30m', '90m');
initSelectOpt(document.getElementById('opt_thematic'), '100%', 'default', 'default');
initSingleSlider(document.getElementById('opt_radius'), '', '0.5', '5', '0.1', '1');
initSelectOpt(document.getElementById('opt_propagationmodel'), '100%', 'ITM / Longley-Rice (< 20GHz)\tITWOM 3.0 (< 20GHz)', 'ITM / Longley-Rice (< 20GHz)');
initSingleSlider(document.getElementById('opt_reliabilitys'), '', '0', '99', '1', '1');
initSingleSlider(document.getElementById('opt_reliabilityt'), '', '0', '99', '1', '1');
initSelectOpt(document.getElementById('opt_terrainconductivity'), '100%', 'Salt water\tGood ground\tFresh water\tMarshy land\tFarm land\tForest\tAverage ground\tMountain / Sand\tCity\tPoor ground', 'Average ground');
initSelectOpt(document.getElementById('opt_radioclimate'), '100%', 'Equatorial (Congo)\tContinental Subtropical (Sudan)\tMaritime Subtropical (W. Africa)\tDesert (Sahara)\tContinental Temperate\tMaritime Temperate (Land)\tMaritime Temperate (Sea)', 'Continental Temperate');
initSingleSlider(document.getElementById('opt_receiverheight'), '', '0.1', '5', '0.1', '1.5');
initSingleSlider(document.getElementById('opt_receivergain'), '', '0', '5', '0.01', '2.14');
initSingleSlider(document.getElementById('opt_receiversensitivity'), '', '-140', '-40', '1', '-90');
document.getElementById('opt_lteband').addEventListener('change', setLTEBandwidths);
loadSettings();
/* settings functions*/
function setLTEBandwidths(e) {
    if (e) { e.stopPropagation(); }
    let ltebandwidth;
    switch (document.getElementById('opt_lteband').value) {
        case 'B01_FDD_2100 IMT': ltebandwidth = '5\t10\t15\t20'; break;
        case 'B02_FDD_1900 PCS': ltebandwidth = '1.4\t3\t5\t10\t15\t20'; break;
        case 'B03_FDD_1800 (+) DCS': ltebandwidth = '1.4\t3\t5\t10\t15\t20'; break;
        case 'B04_FDD_1700 AWS-1': ltebandwidth = '1.4\t3\t5\t10\t15\t20'; break;
        case 'B05_FDD_850 Cellular': ltebandwidth = '1.4\t3\t5\t10'; break;
        case 'B06_FDD_850 UMTS only': ltebandwidth = '5\t10'; break;
        case 'B07_FDD_2600 E-IMT': ltebandwidth = '5\t10\t15\t20'; break;
        case 'B08_FDD_900 E-GSM': ltebandwidth = '1.4\t3\t5\t10'; break;
        case 'B09_FDD_1800': ltebandwidth = '5\t10\t15\t20'; break;
        case 'B10_FDD_1700 AWS-1+': ltebandwidth = '5\t10\t15\t20'; break;
        case 'B11_FDD_1500 Lower PDC': ltebandwidth = '5\t10'; break;
        case 'B12_FDD_700 a Lower SMH': ltebandwidth = '1.4\t3\t5\t10'; break;
        case 'B13_FDD_700 c Upper SMH': ltebandwidth = '5\t10'; break;
        case 'B14_FDD_700 PS Upper SMH': ltebandwidth = '5\t10'; break;
        case 'B17_FDD_700 b Lower SMH': ltebandwidth = '5\t10'; break;
        case 'B18_FDD_800 Lower (Japan)': ltebandwidth = '5\t10\t15'; break;
        case 'B19_FDD_800 Upper (Japan)': ltebandwidth = '5\t10\t15'; break;
        case 'B20_FDD_800 Digital Div... (EU)': ltebandwidth = '5\t10\t15\t20'; break;
        case 'B21_FDD_1500 Upper PDC': ltebandwidth = '5\t10\t15'; break;
        case 'B22_FDD_3500': ltebandwidth = '5\t10\t15\t20'; break;
        case 'B23_FDD_2000 S-band': ltebandwidth = '1.4\t3\t5\t10\t15\t20'; break;
        case 'B24_FDD_1600 Upper L-band (US)': ltebandwidth = '5\t10'; break;
        case 'B25_FDD_1900 (+) E-PCS': ltebandwidth = '1.4\t3\t5\t10\t15\t20'; break;
        case 'B26_FDD_850 (+) E-Cellular': ltebandwidth = '1.4\t3\t5\t10\t15'; break;
        case 'B27_FDD_800 SMR': ltebandwidth = '1.4\t3\t5\t10'; break;
        case 'B28_FDD_700 APT': ltebandwidth = '3\t5\t10\t15\t20'; break;
        case 'B29_SDL_700 d Lower SMH': ltebandwidth = '3\t5\t10'; break;
        case 'B30_FDD_2300 WCS': ltebandwidth = '5\t10'; break;
        case 'B31_FDD_450 NMT': ltebandwidth = '1.4\t3\t5'; break;
        case 'B32_SDL_1500 L-band (EU)': ltebandwidth = '5\t10\t15\t20'; break;
        case 'B33_TDD_1900': ltebandwidth = '5\t10\t15\t20'; break;
        case 'B34_TDD_2000 IMT': ltebandwidth = '5\t10\t15'; break;
        case 'B35_TDD_1900 PCS Lower': ltebandwidth = '1.4\t3\t5\t10\t15\t20'; break;
        case 'B36_TDD_1900 PCS Upper': ltebandwidth = '1.4\t3\t5\t10\t15\t20'; break;
        case 'B37_TDD_1900 PCS Center gap': ltebandwidth = '5\t10\t15\t20'; break;
        case 'B38_TDD_2600 E-IMT': ltebandwidth = '5\t10\t15\t20'; break;
        case 'B39_TDD_1900 (+) DCS-IMT Gap': ltebandwidth = '5\t10\t15\t20'; break;
        case 'B40_TDD_2300 S-Band': ltebandwidth = '5\t10\t15\t20'; break;
        case 'B41_TDD_2600 (+) BRS': ltebandwidth = '5\t10\t15\t20'; break;
        case 'B42_TDD_3500 CBRS (EU/Japan)': ltebandwidth = '5\t10\t15\t20'; break;
        case 'B43_TDD_3700 C-Band': ltebandwidth = '5\t10\t15\t20'; break;
        case 'B44_TDD_700 APT': ltebandwidth = '3\t5\t10\t15\t20'; break;
        case 'B45_TDD_1500': ltebandwidth = '5\t10\t15\t20'; break;
        case 'B46_TDD_5200 Unlicensed U-NII': ltebandwidth = '10\t20'; break;
        case 'B47_TDD_5900 V2X U-NII4': ltebandwidth = '10\t20'; break;
        case 'B48_TDD_3600 CBRS (US)': ltebandwidth = '5\t10\t15\t20'; break;
        case 'B49_TDD_3600 (r) C-Band': ltebandwidth = '10\t20'; break;
        case 'B50_TDD_1500 (+) L-Band (EU)': ltebandwidth = '3\t5\t10\t15\t20'; break;
        case 'B51_TDD_1500 (-) E-(L-Band) (EU)': ltebandwidth = '3\t5'; break;
        case 'B52_TDD_3300 C-Band': ltebandwidth = '5\t10\t15\t20'; break;
        case 'B53_TDD_2500 S-Band': ltebandwidth = '1.4\t3\t5\t10'; break;
        case 'B65_FDD_2100 (+) E-IMT': ltebandwidth = '1.4\t3\t5\t10\t15\t20'; break;
        case 'B66_FDD_1700 AWS-3 E-AWS': ltebandwidth = '1.4\t3\t5\t10\t15\t20'; break;
        case 'B67_SDL_700 (EU)': ltebandwidth = '5\t10\t15\t20'; break;
        case 'B68_FDD_700 (ME)': ltebandwidth = '5\t10\t15'; break;
        case 'B69_SDL_2500 DL E-IMT': ltebandwidth = '5\t10\t15\t20'; break;
        case 'B70_FDD_1700 AWS-4 Supp...': ltebandwidth = '5\t10\t15\t20'; break;
        case 'B71_FDD_600 Digital Dividend': ltebandwidth = '5\t10\t15\t20'; break;
        case 'B72_FDD_450 PMR/PAMR (EU)': ltebandwidth = '1.4\t3\t5'; break;
        case 'B73_FDD_450 APAC PMT (APT)': ltebandwidth = '1.4\t3\t5'; break;
        case 'B74_FDD_1500 Lower L‑Band (US)': ltebandwidth = '1.4\t3\t5\t10\t15\t20'; break;
        case 'B75_SDL_1500 (+) DL L-Band (EU)': ltebandwidth = '5\t10\t15\t20'; break;
        case 'B76_SDL_1500 (-) DL E-(L-B) (EU)': ltebandwidth = '5\t10\t15\t20'; break;
        case 'B85_FDD_700 a+ E-Lower SMH': ltebandwidth = '5\t10'; break;
        case 'B87_FDD_410 PMR (APT)': ltebandwidth = '1.4\t3\t5'; break;
        case 'B88_FDD_410 (+) PMR (EU)': ltebandwidth = '1.4\t3\t5'; break;
        default: ltebandwidth = '1.4\t3\t5\t10\t15\t20'; break;
    }
    document.getElementById('opt_ltebandwidth').reloadOpt(ltebandwidth);
}
function loadSettings() {
    let settings = Ajax_runccmd("GetSettings", "").split('\t');
    document.getElementById('opt_measurementtype').setOpt(settings[1]);
    document.getElementById('opt_terrainresolution').setOpt(settings[2]);
    document.getElementById('opt_thematic').setOpt(settings[3]);
    document.getElementById('opt_radius').setValue(settings[4]);
    document.getElementById('opt_propagationmodel').setOpt(settings[5]);
    document.getElementById('opt_reliabilitys').setValue(settings[6]);
    document.getElementById('opt_reliabilityt').setValue(settings[7]);
    document.getElementById('opt_terrainconductivity').setOpt(settings[8]);
    document.getElementById('opt_radioclimate').setOpt(settings[9]);
    document.getElementById('opt_receiverheight').setValue(settings[10]);
    document.getElementById('opt_receivergain').setValue(settings[11]);
    document.getElementById('opt_receiversensitivity').setValue(settings[12]);
    document.getElementById('opt_antennamodel').setOpt(settings[13]);
    document.getElementById('opt_cellradius').setValue(settings[14]);
    document.getElementById('opt_gsmband').setOpt(settings[15]);
    document.getElementById('opt_gsmbandwidth').setOpt(settings[16]);
    document.getElementById('opt_lteband').setOpt(settings[17]);
    document.getElementById('opt_ltebandwidth').setOpt(settings[18]);
    document.getElementById('opt_frequency').setValue(settings[19]);
    document.getElementById('opt_channelwidth').setOpt(settings[20]);
    document.getElementById('opt_outputpower').setValue(settings[21]);
    document.getElementById('opt_antennagain').setValue(settings[22]);
    document.getElementById('opt_losses').setValue(settings[23]);
    document.getElementById('opt_fresnelclearance').setValue(settings[24]);
}
function saveSettings(e) {
    if (e) { e.stopPropagation(); }
    if (getActionRunning()) { return; }
    showProgress('Saving Settings');
    setTimeout(function () {
        _saveSettings();
        closeProgress();
    }, 500);
}

function _saveSettings() {
    let pl_measurementtype = document.getElementById('opt_measurementtype').value;
    let pl_terrainresolution = document.getElementById('opt_terrainresolution').value;
    let pl_thematic = document.getElementById('opt_thematic').value;
    let pl_radius = document.getElementById('opt_radius').value;
    let pl_propagationmodel = document.getElementById('opt_propagationmodel').value;
    let pl_reliabilitys = document.getElementById('opt_reliabilitys').value;
    let pl_reliabilityt = document.getElementById('opt_reliabilityt').value;
    let pl_terrainconductivity = document.getElementById('opt_terrainconductivity').value;
    let pl_radioclimate = document.getElementById('opt_radioclimate').value;
    let pl_receiverheight = document.getElementById('opt_receiverheight').value;
    let pl_receivergain = document.getElementById('opt_receivergain').value;
    let pl_receiversensitivity = document.getElementById('opt_receiversensitivity').value;
    let rp_antennamodel = document.getElementById('opt_antennamodel').value;
    let rp_cellradius = document.getElementById('opt_cellradius').value;
    let rp_gsmband = document.getElementById('opt_gsmband').value;
    let rp_gsmbandwidth = document.getElementById('opt_gsmbandwidth').value;
    let rp_lteband = document.getElementById('opt_lteband').value;
    let rp_ltebandwidth = document.getElementById('opt_ltebandwidth').value;
    let mw_frequency = document.getElementById('opt_frequency').value;
    let mw_channelwidth = document.getElementById('opt_channelwidth').value;
    let mw_outputpower = document.getElementById('opt_outputpower').value;
    let mw_antennagain = document.getElementById('opt_antennagain').value;
    let mw_losses = document.getElementById('opt_losses').value;
    let mw_fresnelclearance = document.getElementById('opt_fresnelclearance').value;
    Ajax_runccmd("SetSettings", "{'options':'" +
        pl_measurementtype + "\t" +
        pl_terrainresolution + "\t" +
        pl_thematic + "\t" +
        pl_radius + "\t" +
        pl_propagationmodel + "\t" +
        pl_reliabilitys + "\t" +
        pl_reliabilityt + "\t" +
        pl_terrainconductivity + "\t" +
        pl_radioclimate + "\t" +
        pl_receiverheight + "\t" +
        pl_receivergain + "\t" +
        pl_receiversensitivity + "\t" +
        rp_antennamodel + "\t" +
        rp_cellradius + "\t" +
        rp_gsmband + "\t" +
        rp_gsmbandwidth + "\t" +
        rp_lteband + "\t" +
        rp_ltebandwidth + "\t" +
        mw_frequency + "\t" +
        mw_channelwidth + "\t" +
        mw_outputpower + "\t" +
        mw_antennagain + "\t" +
        mw_losses + "\t" +
        mw_fresnelclearance + "'}");
}