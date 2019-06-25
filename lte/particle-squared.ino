#include "Particle.h"
#include "dct.h"

SerialLogHandler logHandler(LOG_LEVEL_ALL);
SYSTEM_MODE(SEMI_AUTOMATIC);

#define SET_APN_WITH_PASSWORD false
#define SET_EXTERNAL_SIM false
#define SET_INTERNAL_SIM false

void setup() {

    Serial.begin();

    #if SET_EXTERNAL_SIM
    Cellular.setActiveSim(EXTERNAL_SIM);
    #elif SET_INTERNAL_SIM
    Cellular.setActiveSim(INTERNAL_SIM);
    #endif

    Cellular.setCredentials("soracom.io","sora","sora");
    // Cellular.clearCredentials();

    // Remove setup flag
    const uint8_t val = 0x01;
    dct_write_app_data(&val, DCT_SETUP_DONE_OFFSET, 1);

    Cellular.on();

    // Sometimes we don't want to set these.
    #if SET_APN_WITH_PASSWORD
    Cellular.command("AT+CGDCONT=1,\"IP\",\"soracom.io\"");
    Cellular.command("AT+UAUTHREQ=1,%d,\"%s\",\"%s\"",2,"sora","sora");
    Cellular.command("AT+CGDCONT=2");
    // Cellular.command("AT+UAUTHREQ=2,%d,\"%s\",\"%s\"",2,"sora","sora");
    Cellular.command("AT+CFUN=15");
    delay(30000);

    while( true ) {

        if( Cellular.command(30000, "AT") != WAIT) {
            break;
        }

    }
    #endif

    // This is just so you know the operation is complete
    pinMode(D7, OUTPUT);
    digitalWrite(D7, HIGH);

    Cellular.command("AT+UAUTHREQ?\r\n");
    Cellular.command("AT+CGDCONT?\r\n");
    Cellular.command("AT+CREG?\r\n");

    Particle.connect();
}

void loop() {

    Cellular.command("AT+UAUTHREQ?\r\n");
    Cellular.command("AT+CGDCONT?\r\n");
    Cellular.command("AT+CREG?\r\n");

    delay(10000);

}