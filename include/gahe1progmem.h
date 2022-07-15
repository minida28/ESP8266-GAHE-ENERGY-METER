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
<p>Please wait in <strong><span id="counter">15</span></strong> second(s).</p>
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

// const char pgm_configfilenetwork[] PROGMEM = "/confignetwork.json";
// const char pgm_configfiletime[] PROGMEM = "/configtime.json";

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
// const char pgm_ntpserver_0[] PROGMEM = "ntpserver_0";
// const char pgm_ntpserver_1[] PROGMEM = "ntpserver_1";
// const char pgm_ntpserver_2[] PROGMEM = "ntpserver_2";
// const char pgm_syncinterval[] PROGMEM = "syncinterval";
// const char pgm_timezone[] PROGMEM = "timezone";
// const char pgm_dst[] PROGMEM = "dst";
const char pgm_checked[] PROGMEM = "checked";
const char pgm_blank[] PROGMEM = "";

// const char pgm_time[] PROGMEM = "time";
// const char pgm_date[] PROGMEM = "date";
// const char pgm_lastsync[] PROGMEM = "lastsync";
// const char pgm_nextsync[] PROGMEM = "nextsync";
// const char pgm_uptime[] PROGMEM = "uptime";
// const char pgm_enablentp[] PROGMEM = "enablentp";
// const char pgm_enablertc[] PROGMEM = "enablertc";

//time
const char pgm_time[] PROGMEM = "time";
const char pgm_date[] PROGMEM = "date";
// const char pgm_lastboot[] PROGMEM = "lastboot";
const char pgm_lastsync[] PROGMEM = "lastsync";
const char pgm_lastsyncby[] PROGMEM = "lastsyncby";
const char pgm_lastsyncbyntp[] PROGMEM = "lastsyncbyntp";
const char pgm_lastsyncbyrtc[] PROGMEM = "lastsyncbyrtc";
const char pgm_nextsync[] PROGMEM = "nextsync";
const char pgm_uptime[] PROGMEM = "uptime";
const char pgm_enablentp[] PROGMEM = "enablentp";
const char pgm_enablertc[] PROGMEM = "enablertc";
const char pgm_rtcstatus[] PROGMEM = "rtcstatus";

const char pgm_province[] PROGMEM = "province";
const char pgm_regency[] PROGMEM = "regency";
const char pgm_district[] PROGMEM = "district";
const char pgm_timezone[] PROGMEM = "timezone";
const char pgm_timezonestring[] PROGMEM = "timezonestring";
const char pgm_latitude[] PROGMEM = "latitude";
const char pgm_longitude[] PROGMEM = "longitude";
const char pgm_dst[] PROGMEM = "dst";
const char pgm_ntpserver_0[] PROGMEM = "ntpserver_0";
const char pgm_ntpserver_1[] PROGMEM = "ntpserver_1";
const char pgm_ntpserver_2[] PROGMEM = "ntpserver_2";
const char pgm_syncinterval[] PROGMEM = "syncinterval";
const char pgm_never[] PROGMEM = "NEVER";

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
// const char pgm_url[] PROGMEM = "url";
const char pgm_node[] PROGMEM = "node";
const char pgm_apikey[] PROGMEM = "apikey";
const char pgm_host[] PROGMEM = "host";
const char pgm_online[] PROGMEM = "ONLINE";

// const char pgm_descriptionxmlfile[] PROGMEM = "/description.xml";

const char pgm_RTC[] PROGMEM = "RTC";
const char pgm_NTP[] PROGMEM = "NTP";
const char pgm_None[] PROGMEM = "None";

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

const char pgm_url[] PROGMEM = "url";
const char pgm_param[] PROGMEM = "param";
const char pgm_value[] PROGMEM = "value";
const char pgm_text[] PROGMEM = "text";
const char pgm_type[] PROGMEM = "type";
const char pgm_descriptionxmlfile[] PROGMEM = "/description.xml";
const char pgm_systeminfofile[] PROGMEM = "/systeminfo.json";
const char pgm_statuspagesystem[] PROGMEM = "/status.html";
const char pgm_saveconfig[] PROGMEM = "saveconfig";
const char pgm_configpagelocation[] PROGMEM = "/configlocation.html";
const char pgm_configfilelocation[] PROGMEM = "/configlocation.json";
const char pgm_configpagenetwork[] PROGMEM = "/confignetwork.html";
const char pgm_configfilenetwork[] PROGMEM = "/confignetwork.json";
const char pgm_statuspagenetwork[] PROGMEM = "/statusnetwork.html";
const char pgm_configpagetime[] PROGMEM = "/configtime.html";
const char pgm_configfiletime[] PROGMEM = "/configtime.json";
const char pgm_statuspagetime[] PROGMEM = "/statustime.html";
const char pgm_configpageledmatrix[] PROGMEM = "/configledmatrix.html";
const char pgm_configfileledmatrix[] PROGMEM = "/configledmatrix.json";
const char pgm_configpagesholat[] PROGMEM = "/configsholat.html";
const char pgm_configfilesholat[] PROGMEM = "/configsholat.json";
const char pgm_schedulepagesholat[] PROGMEM = "/sholat.html";
const char pgm_configpagemqtt[] PROGMEM = "/configmqtt.html";
const char pgm_configfilemqtt[] PROGMEM = "/configmqtt.json";
const char pgm_statuspagemqtt[] PROGMEM = "/statusmqtt.html";
const char pgm_configpagemqttpubsub[] PROGMEM = "configPubSub";
const char pgm_configfilemqttpubsub[] PROGMEM = "/configmqttpubsub.json";
const char pgm_settimepage[] PROGMEM = "/setrtctime.html";

//url
// const char pgm_systeminfofile[] PROGMEM = "/systeminfo.json";

//system
const char pgm_heapstart[] PROGMEM = "heapstart";
const char pgm_heapend[] PROGMEM = "heapend";
const char pgm_heap[] PROGMEM = "heap";
const char pgm_freeheap[] PROGMEM = "freeheap";

//SPIIFS
const char pgm_totalbytes[] PROGMEM = "totalbytes";
const char pgm_usedbytes[] PROGMEM = "usedbytes";
const char pgm_blocksize[] PROGMEM = "blocksize";
const char pgm_pagesize[] PROGMEM = "pagesize";
const char pgm_maxopenfiles[] PROGMEM = "maxopenfiles";
const char pgm_maxpathlength[] PROGMEM = "maxpathlength";

// additional
const char pgm_status[] PROGMEM = "status";
const char pgm_channel[] PROGMEM = "channel";
const char pgm_encryption[] PROGMEM = "encryption";
const char pgm_isconnected[] PROGMEM = "isconnected";
const char pgm_autoconnect[] PROGMEM = "autoconnect";
const char pgm_persistent[] PROGMEM = "persistent";
const char pgm_bssid[] PROGMEM = "bssid";
const char pgm_rssi[] PROGMEM = "rssi";
const char pgm_phymode[] PROGMEM = "phymode";

const char WL_IDLE_STATUS_Str[] PROGMEM = "Idle";
const char WL_NO_SSID_AVAIL_Str[] PROGMEM = "SSID not available";
const char WL_SCAN_COMPLETED_Str[] PROGMEM = "Scan Completed";
const char WL_CONNECTED_Str[] PROGMEM = "Connected";
const char WL_CONNECT_FAILED_Str[] PROGMEM = "Connection Failed";
const char WL_CONNECTION_LOST_Str[] PROGMEM = "Connection Lost";
const char WL_DISCONNECTED_Str[] PROGMEM = "Disconnected";
const char WL_NO_SHIELD_Str[] PROGMEM = "WL_NO_SHIELD";

const char pgm_thingspeak_template[] PROGMEM = "%s %sfield1=%s&field2=%s&field3=%s&field4=%s&field5=%s&field6=%s&field7=%s&field8=%s&status=%s HTTP/1.1\r\nHost: %s\r\nX-THINGSPEAKAPIKEY: %s\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\n";
// const char pgm_thingspeak_template[] PROGMEM = "%s %sfield1=%s HTTP/1.1\r\nHost: %s\r\nX-THINGSPEAKAPIKEY: %s\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\n";

const char *const wifistatus_P[] PROGMEM =
    {
        WL_IDLE_STATUS_Str,
        WL_NO_SSID_AVAIL_Str,
        WL_SCAN_COMPLETED_Str,
        WL_CONNECTED_Str,
        WL_CONNECT_FAILED_Str,
        WL_CONNECTION_LOST_Str,
        WL_DISCONNECTED_Str,
        WL_NO_SHIELD_Str};

const char OFF_Str[] PROGMEM = "OFF";
const char STA_Str[] PROGMEM = "STA";
const char AP_Str[] PROGMEM = "AP";
const char STA_AP_Str[] PROGMEM = "AP + STA";
const char MODE_MAX_Str[] PROGMEM = "MAX_MODE";

const char *const wifimode_P[] PROGMEM =
    {
        OFF_Str,
        STA_Str,
        AP_Str,
        STA_AP_Str,
        MODE_MAX_Str};

const char RTC_TIME_VALID_Str[] PROGMEM = "RTC_TIME_VALID";
const char RTC_LOST_CONFIDENT_Str[] PROGMEM = "RTC_LOST_CONFIDENT";
const char CLOCK_NOT_RUNNING_Str[] PROGMEM = "CLOCK_NOT_RUNNING";

const char *const rtcstatus_P[] PROGMEM =
    {
        RTC_TIME_VALID_Str,
        RTC_LOST_CONFIDENT_Str,
        CLOCK_NOT_RUNNING_Str};

#endif
