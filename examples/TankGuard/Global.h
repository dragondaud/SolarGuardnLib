WiFiClient    wifiClient;
PubSubClient  MQTTclient(wifiClient);

const char* UserAgent = "SolarGuardn/1.0 (Arduino ESP8266)";

String PUB_IP;

// openssl s_client -connect maps.googleapis.com:443 | openssl x509 -fingerprint -noout
const char* gMapsCrt = "‎‎67:7B:99:A4:E5:A7:AE:E4:F0:92:01:EF:F5:58:B8:0B:49:CF:53:D4";
const char* gMapsKey = "AIzaSyChydnQOGtnS-G1BH0ZVNtKpItRfwO23aY"; // https://developers.google.com/maps/documentation/timezone/intro

time_t  TWOAM;
time_t  UPTIME;
long    TIMER;

bool    isBME = false;

BME280I2C::Settings settings(
  BME280::OSR_X4,  // tempOSR
  BME280::OSR_X4,  // humOSR
  BME280::OSR_X1,  // presOSR
  BME280::Mode_Forced,
  BME280::StandbyTime_1000ms,
  BME280::Filter_Off,
  BME280::SpiEnable_False,
  0x76 // I2C address
);
BME280I2C     bme(settings);

