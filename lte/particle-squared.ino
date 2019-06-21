#include "Particle.h"
#include "dct.h"

SerialLogHandler logHandler(LOG_LEVEL_ALL);
SYSTEM_MODE(SEMI_AUTOMATIC);

void setup() {

    Serial.begin();

    // Cellular.setActiveSim(INTERNAL_SIM);
    Cellular.setActiveSim(EXTERNAL_SIM);
    Cellular.clearCredentials();
    // Cellular.setCredentials("soracom.io");

    const uint8_t val = 0x01;
    dct_write_app_data(&val, DCT_SETUP_DONE_OFFSET, 1);

    Cellular.on();

    Cellular.command("AT+CGDCONT=1,\"IP\",\"soracom.io\"");
    Cellular.command("AT+UAUTHREQ=1,%d,\"%s\",\"%s\"",2,"sora","sora");
    Cellular.command("AT+CFUN=15");

    while( true ) {
        int out = Cellular.command("AT");

        if( out != WAIT) {
            break;
        }
    }

    // This is just so you know the operation is complete
    pinMode(D7, OUTPUT);
    digitalWrite(D7, HIGH);

    Cellular.command("AT+UAUTHREQ?\r\n");
    Cellular.command("AT+CGDCONT?\r\n");
    Cellular.command("AT+CREG?\r\n");

    Serial.println("connect");

    Particle.connect();
}

void loop() {
    Serial.println("loop");

    Cellular.command("AT+CGDCONT?\r\n");
    Cellular.command("AT+CREG?\r\n");

    delay(5000);

}