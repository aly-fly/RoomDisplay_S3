#include <Arduino.h>
#include "ping/ping_sock.h"
#include "display.h"

// https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/protocols/icmp_echo.html

esp_ping_handle_t ping_h;
bool ping_running = false;
uint32_t packets_received;
bool gLogToDisplay = true;

static void on_ping_success(esp_ping_handle_t hdl, void *args)
{
    // optionally, get callback arguments
    // const char* str = (const char*) args;
    // printf("%s\r\n", str); // "foo"
    uint8_t ttl;
    uint16_t seqno;
    uint32_t elapsed_time, recv_len;
    //ip_addr_t addr;
    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TTL, &ttl, sizeof(ttl));
    //esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &addr, sizeof(addr));
    esp_ping_get_profile(hdl, ESP_PING_PROF_SIZE, &recv_len, sizeof(recv_len));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed_time, sizeof(elapsed_time));
//    printf("%d bytes from %s icmp_seq=%d ttl=%d time=%d ms\n",
//           recv_len, inet_ntoa(addr.u_addr.ip4), seqno, ttl, elapsed_time);
    char txt[60];
    sprintf(txt, "Received %d bytes icmp_seq=%d ttl=%d time=%d ms\n", recv_len, seqno, ttl, elapsed_time);
    Serial.print(txt);
    if (gLogToDisplay) DisplayText(txt, CLGREEN);
}

static void on_ping_timeout(esp_ping_handle_t hdl, void *args)
{
    uint16_t seqno;
    //ip_addr_t addr;
    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    //esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &addr, sizeof(addr));
//    printf("From %s icmp_seq=%d timeout\n", inet_ntoa(addr.u_addr.ip4), seqno);
    char txt[60];
    sprintf(txt, "Timeout icmp_seq=%d\n", seqno);
    Serial.print(txt);
    if (gLogToDisplay) DisplayText(txt, CLORANGE);
}

static void on_ping_end(esp_ping_handle_t hdl, void *args)
{
    uint32_t transmitted;
    uint32_t total_time_ms;

    esp_ping_get_profile(hdl, ESP_PING_PROF_REQUEST, &transmitted, sizeof(transmitted));
    esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY, &packets_received, sizeof(packets_received));
    esp_ping_get_profile(hdl, ESP_PING_PROF_DURATION, &total_time_ms, sizeof(total_time_ms));
    char txt[60];
    sprintf(txt, "%d packets transmitted, %d received, time %dms\n", transmitted, packets_received, total_time_ms);
    Serial.print(txt);
    if (gLogToDisplay) DisplayText(txt);
    ping_running = false;
}

void initialize_ping(ip_addr_t target_addr)
{
    esp_ping_config_t ping_config = ESP_PING_DEFAULT_CONFIG();
    ping_config.target_addr = target_addr;          // target IP address
    ping_config.count = 2; // ESP_PING_COUNT_INFINITE;    // ping in infinite mode, esp_ping_stop can stop it

    /* set callback functions */
    esp_ping_callbacks_t cbs;
    cbs.on_ping_success = on_ping_success;
    cbs.on_ping_timeout = on_ping_timeout;
    cbs.on_ping_end = on_ping_end;
//    cbs.cb_args = "foo";  // arguments that feeds to all callback functions, can be NULL
//  cbs.cb_args = eth_event_group; // ??

    esp_ping_new_session(&ping_config, &cbs, &ping_h);
}

void destroy_ping(){
    ping_running = false;
    esp_ping_stop(ping_h);
    esp_ping_delete_session(ping_h);
}


uint32_t ppiinngg (uint32_t IP_to_ping, bool logToDisplay = true) {
    printf("Ping...\n");
    gLogToDisplay = logToDisplay;
    if (gLogToDisplay) DisplayText("Ping start...\n");

    ip_addr_t addr1;
    addr1.type = IPADDR_TYPE_V4;
    addr1.u_addr.ip4.addr = IP_to_ping;

    initialize_ping(addr1);
    packets_received = 0;
    ping_running = true;
    esp_ping_start(ping_h);

    for (int i = 0; i < 200; i++)  // max 20 sec
    {
        if (!ping_running) break;
        delay(100);
        yield();        
    }    

    destroy_ping();
    printf("Ping finished\n");
    if (gLogToDisplay) DisplayText("Ping finished.\n");
    return (packets_received > 0);
}


