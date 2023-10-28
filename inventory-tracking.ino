#include <map>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>

#include <WiFi.h>
#include <Arduino.h>
#include <Firebase_ESP_Client.h>
#include <PN5180.h>
#include <PN5180ISO15693.h>
#include <HTTPClient.h>

const char* firebaseFunctionsAPI = "http://api.thingspeak.com/update";
WiFiClient client;
HTTPClient http;

// Define Pinouts Usage
#define DEBUG_ENABLE_PIN 39
#define WIFI_DEBUG_PIN 36
#define STATUS_LED_R_PIN 14
#define STATUS_LED_G_PIN 12
#define STATUS_LED_B_PIN 13

#define NFC_OPS_LED_R_PIN 27
#define NFC_OPS_LED_G_PIN 26
#define NFC_OPS_LED_B_PIN 25

#define PIN_PN5180_1_NSS  5
#define PIN_PN5180_1_BUSY 16
#define PIN_PN5180_1_RST  17

// Define Timings
#define HERTZ_INTERVAL 16
#define STATUS_LED_DELAY_MS 64
#define NFC_READ_INTERVAL 320

uint64_t frame = 0; // Frame counter, each frametime is ~16ms; 
// Which approximately produced frequency of 60Hz.
long lastMillis = 0;

uint64_t wifiConnectingLedDelay = 0;
uint64_t wifiConnectingPrintDotDelay = 0;

uint64_t lastNfcReadFrame = 0;

std::map<char*, char*> configs = {
    {"WIFI_SSID", "iPhone 13 mini (J)"},
    {"WIFI_PASSWORD", "0806849422a"},
    {"FIREBASE_API_KEY", "AIzaSyCeZCx25tlXjcM72MahebvmNQS7uWJE0Fg"},
    {"FIREBASE_RTDB_URL", "https://jimmyis-iot-proto-default-rtdb.asia-southeast1.firebasedatabase.app/"},
    {"REGISTERED_INVENTORY_ID", "0"}
};

std::map<char*, bool> rfidTagsStatusMap = {
    {"00", false}
};

std::map<uint8_t, uint8_t> statusLedPinOrderMap = {
    {STATUS_LED_R_PIN, 0},
    {STATUS_LED_G_PIN, 1},
    {STATUS_LED_B_PIN, 2}
};

std::map<uint8_t, uint8_t> statusLedOrderToPinMap = {
    {0, STATUS_LED_R_PIN},
    {1, STATUS_LED_G_PIN},
    {2, STATUS_LED_B_PIN}
};

std::map<uint8_t, uint8_t> nfcStatusLedPinOrderMap = {
    {NFC_OPS_LED_R_PIN, 0},
    {NFC_OPS_LED_G_PIN, 1},
    {NFC_OPS_LED_B_PIN, 2}
};

std::map<uint8_t, uint8_t> nfcStatusLedOrderToPinMap = {
    {0, NFC_OPS_LED_R_PIN},
    {1, NFC_OPS_LED_G_PIN},
    {2, NFC_OPS_LED_B_PIN}
};


enum StatusLedMode { 
    STAY_ON = 1,
    BLINK_10 = 10,
};

// led delay in milliseconds (divide 1024 ms by 32 bits as 4 bytes string will be utilized)
const uint16_t WIFI_CONNECTION_INDICATOR_DELAY = 32; 

// WiFi credentials
// const char* ssid = "MyFiber2G";
// const char* pwd = "0806849422a";
uint32_t ip = 0;
bool isDebugMode = false;
bool isWifiDebugDisplay = false;
bool statusLedMap[] = {
    false, // Red
    false, // Green
    false, // Blue
};

bool nfcStatusLedMap[] = {
    false, // Red
    false, // Green
    false, // Blue
};

// Debug Display Memory Table
std::array<bool, 1> debugDisplayMemoryTable = {
    false // Display IP
};
// debugDisplayMemoryTable.fill(false);

//Define Firebase Data object, Auth object and config object.
FirebaseData firebaseDataObject;
FirebaseAuth firebaseAuth;
FirebaseConfig firebaseConfig;

unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;

PN5180ISO15693 nfc15693(PIN_PN5180_1_NSS, PIN_PN5180_1_BUSY, PIN_PN5180_1_RST);

uint8_t tagReadUid[10];
uint8_t tagUuid[8];
std::string rfidTagUuidString;

void setup() {
    setupBoard();
    updateFrame();
    init();
    wifiConnect();
    setupFirebase();
    nfc15693.begin();
}

void loop() {
    updateFrame();

    checkDebugEnable(&isDebugMode);

    checkWifiConnection();

    if (frame - lastNfcReadFrame > NFC_READ_INTERVAL) {
        nfcStatusLedControl(1);
        lastNfcReadFrame = frame;
        
        nfc15693.reset();
        nfc15693.setupRF();

        ISO15693ErrorCode rc = nfc15693.getInventory(tagReadUid);

        if (rc == ISO15693_EC_OK) {
            // showRfidReadLedBlink();
            // printTagUid(uid);
            nfcStatusLedControl(0);
            Serial.print(F("ISO-15693 Tag found, UID: "));
        
            for (int i = 0; i < 8; i++) {
                Serial.print(tagReadUid[7-i] < 0x10 ? " 0" : " ");
                Serial.print(tagReadUid[7-i], HEX); // LSB is first
                tagUuid[i] = tagReadUid[7-i];
            }

            Serial.println();

            rfidTagUuidString = uint8_to_hex_string(tagUuid, 8);
            std::transform(rfidTagUuidString.begin(), rfidTagUuidString.end(), rfidTagUuidString.begin(), ::toupper);
            Serial.println(rfidTagUuidString.c_str());

            char* rfidTagUuid = const_cast<char*>(rfidTagUuidString.c_str());
            
            if (!rfidTagsStatusMap[rfidTagUuid]) {
              readFirebaseRtdbInventoryData(rfidTagUuid);

              // rfidTagsStatusMap[rfidTagUuid] = !rfidTagsStatusMap[rfidTagUuid];
            }

            if (
                Firebase.ready()
                // && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)
            ) {
                // sendDataPrevMillis = millis();

                std::string target = configs["REGISTERED_INVENTORY_ID"];
                std::string path = "inventory/" + target + "/" + rfidTagUuid;

                if (Firebase.RTDB.setBool(&firebaseDataObject, path, !rfidTagsStatusMap[rfidTagUuid])) {
                    rfidTagsStatusMap[rfidTagUuid] = !rfidTagsStatusMap[rfidTagUuid];

                    Serial.println("PASSED");
                    Serial.println("PATH: " + firebaseDataObject.dataPath());
                    Serial.println("TYPE: " + firebaseDataObject.dataType());
                } else {
                    Serial.println("FAILED");
                    Serial.println("REASON: " + firebaseDataObject.errorReason());
                }
                // count++;
                
                // // Write an Float number on the database path test/float
                // if (Firebase.RTDB.setFloat(&firebaseDataObject, "test/float", 0.01 + random(0,100))){
                //     Serial.println("PASSED");
                //     Serial.println("PATH: " + firebaseDataObject.dataPath());
                //     Serial.println("TYPE: " + firebaseDataObject.dataType());
                // } else {
                //     Serial.println("FAILED");
                //     Serial.println("REASON:  " + firebaseDataObject.errorReason());
                // }
            }
            // showRfidReadLedBlink();
        } else {
            // nfcStatusLedControl(2);
            // printErrorSerial(rc);
        }
        
        nfcStatusLedControl(1);

        Serial.println();
        Serial.println(F("----------------------------------"));
    }

    // delay(1000);
    // if (isDebugMode) {
    //     if (!debugDisplayMemoryTable[0] && digitalRead(WIFI_DEBUG_PIN) == HIGH) {
    //         Serial.print("IP: ");
    //         Serial.println(ip);
    //         debugDisplayMemoryTable[0] = true;
    //     }
    // }
  
    /* Firebase RTDB example code block (Worked) 
    if (
        Firebase.ready()
        // && signupOK 
        && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)
    ) {
        sendDataPrevMillis = millis();
        // Write an Int number on the database path test/int
        if (Firebase.RTDB.setInt(&firebaseDataObject, "test/int", count)){
            Serial.println("PASSED");
            Serial.println("PATH: " + firebaseDataObject.dataPath());
            Serial.println("TYPE: " + firebaseDataObject.dataType());
        } else {
            Serial.println("FAILED");
            Serial.println("REASON: " + firebaseDataObject.errorReason());
        }
        count++;
        
        // Write an Float number on the database path test/float
        if (Firebase.RTDB.setFloat(&firebaseDataObject, "test/float", 0.01 + random(0,100))){
            Serial.println("PASSED");
            Serial.println("PATH: " + firebaseDataObject.dataPath());
            Serial.println("TYPE: " + firebaseDataObject.dataType());
        } else {
            Serial.println("FAILED");
            Serial.println("REASON: " + firebaseDataObject.errorReason());
        }
    }
    */

}

void updateFrame() {
    // millis() Delta is approx. 22-23ms
    if (millis() >= (lastMillis + HERTZ_INTERVAL)) {
        lastMillis = millis();
        frame++;

        if (isDebugMode) {
            Serial.print("Frame no: ");
            Serial.println(frame);
        }
    }
}

void setupBoard() {
    Serial.begin(115200);

    pinMode(DEBUG_ENABLE_PIN, INPUT);
    pinMode(WIFI_DEBUG_PIN, INPUT);
    pinMode(STATUS_LED_R_PIN, OUTPUT);
    pinMode(STATUS_LED_G_PIN, OUTPUT);
    pinMode(STATUS_LED_B_PIN, OUTPUT);
    pinMode(NFC_OPS_LED_R_PIN, OUTPUT);
    pinMode(NFC_OPS_LED_G_PIN, OUTPUT);
    pinMode(NFC_OPS_LED_B_PIN, OUTPUT);
}

void init() {
    StatusLedMode mode = BLINK_10;

    setLedControl(STATUS_LED_B_PIN, mode);
    setNfcStatusLedControl(NFC_OPS_LED_G_PIN, mode);
    delay(100);
    setLedControl(STATUS_LED_R_PIN, mode);
    setNfcStatusLedControl(NFC_OPS_LED_B_PIN, mode);
    delay(100);
    setLedControl(STATUS_LED_G_PIN, mode);
    setNfcStatusLedControl(NFC_OPS_LED_R_PIN, mode);
    delay(100);
    setLedControl(STATUS_LED_B_PIN, mode);
    setNfcStatusLedControl(NFC_OPS_LED_G_PIN, mode);
    delay(100);
    setLedControl(STATUS_LED_R_PIN, mode);
    setNfcStatusLedControl(NFC_OPS_LED_B_PIN, mode);
    delay(100);
    setLedControl(STATUS_LED_G_PIN, mode);
    setNfcStatusLedControl(NFC_OPS_LED_G_PIN, mode);
    delay(100);

    Serial.println("Initializing");
}

void checkDebugEnable(bool *isDebugMode) {
    // This logic works like a simple D-FlipFlop circuit.
    bool isDebugPinOn = digitalRead(DEBUG_ENABLE_PIN) == HIGH;

    // if (!*isDebugMode && digitalRead(DEBUG_ENABLE_PIN) == HIGH) {
    if (!*isDebugMode && isDebugPinOn) {
        *isDebugMode = true;
        Serial.println("Debug mode enabled");
    } else if (*isDebugMode && !isDebugPinOn) {
        *isDebugMode = false;
        debugDisplayMemoryTable.fill(false);
        Serial.println("Debug mode disabled");
    }
}

void wifiConnect() {
    bool isConnect = false;

    Serial.print("Connecting");
    
    WiFi.begin(configs["WIFI_SSID"], configs["WIFI_PASSWORD"]);

    while(!isConnect) {
        if (WiFi.status() == WL_CONNECTED) {
            isConnect = true;
            ip = WiFi.localIP();
        }

        displayWifiStatus(isConnect);
    }

}

void checkWifiConnection() {
  if (WiFi.status() == WL_CONNECTED) {
    
  } else {
    wifiConnect();
    statusLedControl(0);
  }
}

void displayWifiStatus(bool isConnect) {
    if (!isConnect) {
        if(millis() - wifiConnectingPrintDotDelay > (STATUS_LED_DELAY_MS * 8)) {
            wifiConnectingPrintDotDelay = millis();
            Serial.print(".");
        }
        
        if(millis() - wifiConnectingLedDelay > STATUS_LED_DELAY_MS) {
            wifiConnectingLedDelay = millis();
            statusLedControl(0);
        }
        
        // // delay(WIFI_CONNECTION_INDICATOR_DELAY);
        // if((clockCycleNo - wifiConnectingPrintDotDelay) > 4) {
        // }
        
    } else {
        statusLedControl(1);
        Serial.println();
        Serial.println("A WiFi network connected");
    }
}

// void taskRunner(uint16_t lastFrame, uint16_t interval) {
//    if (frame - lastFrame > interval) {
//         lastFrame = frame;
//         // ...Do something asynchronously
//    }
// }

void statusLedControl(uint8_t statusNo) {
    StatusLedMode mode;

    switch (statusNo) {
        case 0:
            mode = BLINK_10;
            setLedControl(STATUS_LED_B_PIN, mode);
            break;
        case 1:
            mode = STAY_ON;
            setLedControl(STATUS_LED_G_PIN, mode);
            break;
    }
}

void nfcStatusLedControl(uint8_t statusNo) {
    StatusLedMode mode;

    switch (statusNo) {
        case 0:
            mode = BLINK_10;
            setNfcStatusLedControl(NFC_OPS_LED_B_PIN, mode);
            break;
        case 1:
            mode = STAY_ON;
            setNfcStatusLedControl(NFC_OPS_LED_G_PIN, mode);
            break;
        case 2:
            mode = BLINK_10;
            setNfcStatusLedControl(NFC_OPS_LED_R_PIN, mode);
            break;
    }
}

void setLedControl(uint8_t activePin, StatusLedMode mode) {
    uint8_t activePinOrder = statusLedPinOrderMap[activePin];
    bool* activeLedStatus = &statusLedMap[activePinOrder];

    for (uint8_t i = 0; i < 3; i++) {
        if (i == activePinOrder) {
            switch (mode) {
                case STAY_ON:
                    digitalWrite(activePin, HIGH);
                    *activeLedStatus = 1;
                    break;
                case BLINK_10:
                    digitalWrite(activePin, !*activeLedStatus);
                    *activeLedStatus = !*activeLedStatus;
                    break;
            }
        } else { // Reset all the rest to off
            digitalWrite(statusLedOrderToPinMap[i], 0);
            statusLedMap[i] = false;
        }

    }
}

void setNfcStatusLedControl(uint8_t activePin, StatusLedMode mode) {
    uint8_t activePinOrder = nfcStatusLedPinOrderMap[activePin];
    bool* activeLedStatus = &nfcStatusLedMap[activePinOrder];

    for (uint8_t i = 0; i < 3; i++) {
        if (i == activePinOrder) {
            switch (mode) {
                case STAY_ON:
                    digitalWrite(activePin, HIGH);
                    *activeLedStatus = 1;
                    break;
                case BLINK_10:
                    digitalWrite(activePin, !*activeLedStatus);
                    *activeLedStatus = !*activeLedStatus;
                    break;
            }
        } else { // Reset all the rest to off
            digitalWrite(nfcStatusLedOrderToPinMap[i], 0);
            nfcStatusLedMap[i] = false;
        }

    }
}

void setupFirebase() {
    /* Required for Firebase, Assign the API key and RTDB URL */
    firebaseConfig.api_key = configs["FIREBASE_API_KEY"];
    firebaseConfig.database_url = configs["FIREBASE_RTDB_URL"];

    /* Sign up, anonymously */
    if (Firebase.signUp(&firebaseConfig, &firebaseAuth, "", "")){
        Serial.println("ok");
        signupOK = true;
    } else {
        Serial.printf("%s\n", firebaseConfig.signer.signupError.message.c_str());
    }

     /* Assign the callback function for the long running token generation task */
    // firebaseConfig.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
    
    Firebase.begin(&firebaseConfig, &firebaseAuth);
    // Firebase.reconnectWiFi(true);
}

void readFirebaseRtdbInventoryData(char* rfidTagUuid) {
    Serial.println("readFirebaseRtdbInventoryData()");

    std::string inventoryId = configs["REGISTERED_INVENTORY_ID"];
    std::string path = "inventory/" + inventoryId + "/" + rfidTagUuid;
    
    if (Firebase.RTDB.getBool(&firebaseDataObject, path)) {

        if (firebaseDataObject.dataTypeEnum() == firebase_rtdb_data_type_boolean) {
          bool rfidTagStatus = firebaseDataObject.to<bool>();

          rfidTagsStatusMap[rfidTagUuid] = rfidTagStatus;

          Serial.print("RFID Tag ID: ");
          Serial.print(rfidTagUuid);
          Serial.print(" , Status => ");
          Serial.println(rfidTagStatus);
        }

    } else {
        Serial.println("Error in readFirebaseRtdbInventoryData()");
        Serial.println(firebaseDataObject.errorReason());
    }
}

std::string uint8_to_hex_string(const uint8_t *v, const size_t s) {
    std::stringstream ss;

    ss << std::hex << std::setfill('0');

    for (int i = 0; i < s; i++) {
      ss << std::hex << std::setw(2) << static_cast<int>(v[i]);
    }

    return ss.str();
}