#include <ESP8266WiFi.h>
//#include <ESP8266mDNS.h>
#include <SDFS.h>
#include <ESPWebDAV.h>



//-------------- Configuration ---------------

#define HOSTNAME      "espwebdav"

#define STASSID       "mywifi3"
#define STAPSK        "pasw123456789"

//#define SERVER_PORT   80
#define SPI_BLOCKOUT_PERIOD 2000UL

#define SD_CS         4
#define CS_SENSE      5

// LED is connected to GPIO2 on this board
#define LED_PIN       2

// Control pin for 3 switches on CS, MOSI, CLK
#define SW_PIN        15

// For an inverting switch logical 1 means the bus is used by ESP.
// So the switch blocks the printer-SD communication
#define SW_INVERT     true

//-------------- Configuration end ---------------



// Uncomment to enable printing info into serial
#define DBG_MODE

// HSPI EPS-12E
#define MISO      12
#define MOSI      13
#define SCLK      14

#define LED_ON        digitalWrite(LED_PIN, LOW)
#define LED_OFF       digitalWrite(LED_PIN, HIGH)

#ifdef DBG_MODE
  #define DBG_INIT          Serial.begin(115200)
  #define DBG_PRINT(...)    Serial.print(__VA_ARGS__)
  #define DBG_PRINTLN(...)  Serial.println(__VA_ARGS__)
  #define DBG_PRINTF(...)   Serial.printf(__VA_ARGS__)
#endif

#if SW_INVERT
  #define TAKE_BUS      HIGH
  #define RELEASE_BUS   LOW
#else
  #define TAKE_BUS      LOW
  #define RELEASE_BUS   HIGH
#endif

FS& gfs = SDFS;

WiFiServer tcp(80);

ESPWebDAV dav;

//String statusMessage;
//bool initFailed = false;

volatile long spiBlockoutTime = 0;
bool weHaveBus = false;

// ------------------------
void takeBusControl()
{
  weHaveBus = true;
  // Block printer-SD communication
  digitalWrite(SW_PIN, TAKE_BUS);
  
  // --------------------
  LED_ON;
  pinMode(MISO, SPECIAL); 
  pinMode(MOSI, SPECIAL); 
  pinMode(SCLK, SPECIAL); 
  pinMode(SD_CS, OUTPUT);
  
}

// ------------------------
void relenquishBusControl()
{
  pinMode(MISO, INPUT); 
  pinMode(MOSI, INPUT); 
  pinMode(SCLK, INPUT); 
  pinMode(SD_CS, INPUT);
  LED_OFF;
  
  // --------------------
  // Allow printer-SD communication
  digitalWrite(SW_PIN, RELEASE_BUS);
  weHaveBus = false;
}

// ------------------------
void blink()
{
  LED_ON; 
  delay(100); 
  LED_OFF; 
  delay(400);
}

// ------------------------
//void errorBlink()
//{
//  for(int i = 0; i < 100; i++)  {
//    LED_ON; 
//    delay(50); 
//    LED_OFF; 
//    delay(50);
//  }
//}

ICACHE_RAM_ATTR void masterDetected()
{
  if(!weHaveBus)
    spiBlockoutTime = millis() + SPI_BLOCKOUT_PERIOD;
}

// ------------------------
void setup()
{
    // ----- DBG -------
    #ifdef DBG_MODE
      DBG_INIT;
      DBG_PRINTLN("Debug printing enabled");
    #endif
    
    // ----- GPIO -------
    // Define an output to control switches
    pinMode(SW_PIN, OUTPUT);
    relenquishBusControl();
    
    // Detect when other master uses SPI bus
    pinMode(CS_SENSE, INPUT);
    attachInterrupt(digitalPinToInterrupt(CS_SENSE),
      masterDetected, FALLING);
    pinMode(LED_PIN, OUTPUT);
    blink();
    
    // wait for other master to assert SPI bus first
    delay(SPI_BLOCKOUT_PERIOD);
  
    // ------ WIFI -------
    WiFi.persistent(false);
    WiFi.hostname(HOSTNAME);
    WiFi.mode(WIFI_STA);
    WiFi.begin(STASSID, STAPSK);
    #ifdef DBG_MODE
      DBG_PRINTLN("Connecting to " STASSID " ...");
    #endif
    
    // Wait for connection
    while (WiFi.status() != WL_CONNECTED)
    {
        blink();
        #ifdef DBG_MODE
          DBG_PRINT(".");
        #endif
    }

    #ifdef DBG_MODE
      DBG_PRINTLN("");
      DBG_PRINT("Connected to "); DBG_PRINTLN(STASSID);
      DBG_PRINT("IP address: ");  DBG_PRINTLN(WiFi.localIP());
      DBG_PRINT("RSSI: ");        DBG_PRINTLN(WiFi.RSSI());
      DBG_PRINT("Mode: ");        DBG_PRINTLN(WiFi.getPhyMode());
    #endif

//    MDNS.begin(HOSTNAME);

    // ----- SD Card and Server -------
    // Check to see if other master is using the SPI bus
    while(millis() < spiBlockoutTime)
      blink();
    
    takeBusControl();
    
//    // start the SD DAV server
//    if(!dav.init(SD_CS, SPI_FULL_SPEED, SERVER_PORT))   {
//      statusMessage = "Failed to initialize SD Card";
//      DBG_PRINT("ERROR: "); DBG_PRINTLN(statusMessage);
//      // indicate error on LED
//      errorBlink();
//      initFailed = true;
//    }
//    else
//      blink();

    #ifdef DBG_MODE
      if(!gfs.begin())
        DBG_PRINTLN("GFS init failed.");
    #else
      gfs.begin();
    #endif
    
    tcp.begin();
    dav.begin(&tcp, &gfs);
//    if(initFailed)
//      Serial.println("Init failed");
    // Show progress in Serial
    #ifdef DBG_MODE
      dav.setTransferStatusCallback([](const char* name, int percent, bool receive)
      {
          DBG_PRINTF("%s: '%s': %d%%\n", receive ? "recv" : "send", name, percent);
      });
    #endif
    
    relenquishBusControl();

    #ifdef DBG_MODE
      DBG_PRINTLN("WebDAV server started");
    #endif
}

//void help()
//{
//    Serial.printf("interactive: F/ormat D/ir C/reateFile\n");
//
//    uint32_t freeHeap;
//    uint16_t maxBlock;
//    uint8_t fragmentation;
//    ESP.getHeapStats(&freeHeap, &maxBlock, &fragmentation);
//    Serial.printf("Heap stats: free heap: %u - max block: %u - fragmentation: %u%%\n",
//                  freeHeap, maxBlock, fragmentation);
//}

// ------------------------
void loop()
{
//    MDNS.update();
//    dav.handleClient();
  
    if(millis() < spiBlockoutTime){
      blink(); return;}

    // do it only if there is a need to read FS
//    if(dav.isClientWaiting()) {
//      if(initFailed)
//        return dav.rejectClient(statusMessage);
      
      // has other master been using the bus in last few seconds
//      if(millis() < spiBlockoutTime)
//        return;
//        return dav.rejectClient("Marlin is reading from SD card");
      
      // a client is waiting and FS is ready and other SPI master is not using the bus
      takeBusControl();
      dav.handleClient();
      relenquishBusControl();
//    }

//    int c = Serial.read();
//    if (c > 0)
//    {
//        if (c == 'F')
//        {
//            Serial.println("formatting...");
//            if (gfs.format())
//                Serial.println("Success");
//            else
//                Serial.println("Failure");
//        }
//        else if (c == 'D')
//        {
//            Serial.printf(">>>>>>>> dir /\n");
//            dav.dir("/", &Serial);
//            Serial.printf("<<<<<<<< dir\n");
//        }
//        else if (c == 'C')
//        {
//            auto f = gfs.open("readme.md", "w");
//            f.printf("hello\n");
//            f.close();
//        }
//        else
//            help();
//    }

    // ------------------------
}
