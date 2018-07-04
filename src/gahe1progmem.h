#ifndef _GAHE1PROGMEM_H
#define _GAHE1PROGMEM_H

const char Page_WaitAndReload[] PROGMEM = R"=====(
<p>Rebooting...</p>
<p>Please wait in <strong><span id="counter">20</span></strong> second(s).</p>
<p>This page will refresh automatically.</p>
<script type="text/javascript">
function countdown() {
    var i = document.getElementById('counter');
    if (parseInt(i.textContent)<=0) {
        location.href = '/';
    }
    i.textContent = parseInt(i.textContent)-1;
}
setInterval(function(){ countdown(); },1000);
</script>
)=====";

//const char Page_Restart[] PROGMEM = R"=====(
//<meta http-equiv="refresh" content="10; URL=/general.html">
//Please Wait....Configuring and Restarting.
//)=====";

const char Page_Restart[] PROGMEM = R"=====(
<p>Rebooting...</p>
<p>Please wait in <strong><span id="counter">20</span></strong> second(s).</p>
<p>This page will refresh automatically.</p>
<script type="text/javascript">
function countdown() {
    var i = document.getElementById('counter');
    if (parseInt(i.textContent)<=0) {
        location.href = '/';
    }
    i.textContent = parseInt(i.textContent)-1;
}
setInterval(function(){ countdown(); },1000);
</script>
)=====";

const char pgm_configfilenetwork[] PROGMEM = "/confignetwork.json";
const char pgm_configfiletime[] PROGMEM = "/configtime.json";

//network
const char pgm_ssid[] PROGMEM = "ssid";
const char pgm_password[] PROGMEM = "password";
const char pgm_dhcp[] PROGMEM = "dhcp";
const char pgm_static_ip[] PROGMEM = "static_ip";
const char pgm_gateway[] PROGMEM = "gateway";
const char pgm_netmask[] PROGMEM = "netmask";
const char pgm_dns0[] PROGMEM = "dns0";
const char pgm_dns1[] PROGMEM = "dns1";

const char pgm_got_text_message[] PROGMEM = "I got your text message";
const char pgm_got_binary_message[] PROGMEM = "I got your binary message";

const char pgm_mode[] PROGMEM = "mode";
const char pgm_pass[] PROGMEM = "pass";
const char pgm_ip[] PROGMEM = "ip";
const char pgm_sta_ip[] PROGMEM = "sta_ip";
const char pgm_sta_mac[] PROGMEM = "sta_mac";
const char pgm_ap_ip[] PROGMEM = "ap_ip";
const char pgm_ap_mac[] PROGMEM = "ap_mac";
const char pgm_dns[] PROGMEM = "dns";
// const char pgm_last_sync[] PROGMEM = "last_sync";
// const char pgm_time[] PROGMEM = "time";
// const char pgm_date[] PROGMEM = "date";
// const char pgm_uptime[] PROGMEM = "uptime";
// const char pgm_last_boot[] PROGMEM = "last_boot";

const char pgm_WIFI_AP[] PROGMEM = "WIFI_AP";
const char pgm_WIFI_STA[] PROGMEM = "WIFI_STA";
const char pgm_WIFI_AP_STA[] PROGMEM = "WIFI_AP_STA";
const char pgm_WIFI_OFF[] PROGMEM = "WIFI_OFF";
const char pgm_NA[] PROGMEM = "N/A";

//time
const char pgm_ntpserver_0[] PROGMEM = "ntpserver_0";
const char pgm_ntpserver_1[] PROGMEM = "ntpserver_1";
const char pgm_ntpserver_2[] PROGMEM = "ntpserver_2";
const char pgm_syncinterval[] PROGMEM = "syncinterval";
const char pgm_timezone[] PROGMEM = "timezone";
const char pgm_dst[] PROGMEM = "dst";
const char pgm_checked[] PROGMEM = "checked";
const char pgm_blank[] PROGMEM = "";

const char pgm_time[] PROGMEM = "time";
const char pgm_date[] PROGMEM = "date";
const char pgm_lastsync[] PROGMEM = "lastsync";
const char pgm_nextsync[] PROGMEM = "nextsync";
const char pgm_uptime[] PROGMEM = "uptime";
const char pgm_enablentp[] PROGMEM = "enablentp";
const char pgm_enablertc[] PROGMEM = "enablertc";


//const char pgm_devicename[] PROGMEM = "/rumah";
const char pgm_hostname[] PROGMEM = "hostname";

const char pgm_auth[] PROGMEM = "auth";
const char pgm_user[] PROGMEM = "user";

const char pgm_publish_1s_PACKET3[] PROGMEM = "publish_1s_PACKET3";

const char pgm_pub_param[] PROGMEM = "pub_param";
const char pgm_pub_default_basetopic[] PROGMEM = "pub_default_basetopic";
const char pgm_pub_1s_basetopic[] PROGMEM = "pub_1s_basetopic";
const char pgm_pub_10s_basetopic[] PROGMEM = "pub_10s_basetopic";
const char pgm_pub_15s_basetopic[] PROGMEM = "pub_15s_basetopic";
const char pgm_pub_thingspeak_basetopic[] PROGMEM = "pub_thingspeak_basetopic";
const char pgm_pub_emoncms_basetopic[] PROGMEM = "pub_emoncms_basetopic";
const char pgm_powerthreshold_basetopic[] PROGMEM = "pub_powerthreshold_basetopic";

const char pgm_pub_basetopic[] PROGMEM = "pub_basetopic";
const char pgm_pub_prefix[] PROGMEM = "pub_prefix";

const char pgm_voltage[] PROGMEM = "voltage";
const char pgm_ampere[] PROGMEM = "ampere";
const char pgm_watt[] PROGMEM = "watt";
const char pgm_var[] PROGMEM = "var";
const char pgm_frequency[] PROGMEM = "frequency";
const char pgm_pstkwh[] PROGMEM = "pstkwh";
const char pgm_pstkvarh[] PROGMEM = "pstkvarh";
const char pgm_ngtkvarh[] PROGMEM = "ngtkvarh";
const char pgm_powerfactor[] PROGMEM = "powerfactor";
const char pgm_apparentpower[] PROGMEM = "apparentpower";
const char pgm_unk2[] PROGMEM = "unk2";

const char pgm_templaterequest[] PROGMEM = "templaterequest";
const char pgm_method[] PROGMEM = "method";
const char pgm_url[] PROGMEM = "url";
const char pgm_node[] PROGMEM = "node";
const char pgm_apikey[] PROGMEM = "apikey";
const char pgm_host[] PROGMEM = "host";
const char pgm_online[] PROGMEM = "ONLINE";

//system info
const char pgm_filename[] PROGMEM = "filename";
const char pgm_compiledate[] PROGMEM = "compiledate";
const char pgm_compiletime[] PROGMEM = "compiletime";
const char pgm_lastboot[] PROGMEM = "lastboot";
const char pgm_chipid[] PROGMEM = "chipid";
const char pgm_cpufreq[] PROGMEM = "cpufreq";
const char pgm_sketchsize[] PROGMEM = "sketchsize";
const char pgm_freesketchspace[] PROGMEM = "freesketchspace";
const char pgm_flashchipid[] PROGMEM = "flashchipid";
const char pgm_flashchipmode[] PROGMEM = "flashchipmode";
const char pgm_flashchipsize[] PROGMEM = "flashchipsize";
const char pgm_flashchiprealsize[] PROGMEM = "flashchiprealsize";
const char pgm_flashchipspeed[] PROGMEM = "flashchipspeed";
const char pgm_cyclecount[] PROGMEM = "cyclecount";
const char pgm_corever[] PROGMEM = "corever";
const char pgm_sdkver[] PROGMEM = "sdkver";
const char pgm_bootmode[] PROGMEM = "bootmode";
const char pgm_bootversion[] PROGMEM = "bootversion";
const char pgm_resetreason[] PROGMEM = "resetreason";


//url
const char pgm_systeminfofile[] PROGMEM = "/systeminfo.json";


#endif
