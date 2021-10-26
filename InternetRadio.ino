#include <WiFi.h>
#include <AudioFileSource.h>
#include <AudioFileSourceBuffer.h>
#include <AudioFileSourceICYStream.h>
#include <AudioGeneratorTalkie.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>
#include <AudioOutputI2SNoDAC.h>
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h>
#include <spiram-fast.h>

#include "frame.h"

#include "background.h"
#include "Orbitron_Medium_20.h"

TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h
#define TFT_GREY 0x5AEB // New colour

const int pwmFreq = 5000;
const int pwmResolution = 8;
const int pwmLedChannelTFT = 0;

const char *SSID = "xxxxxx";
const char *PASSWORD = "xxxxxx";
const int bufferSize = 16 * 1024; // buffer in byte, default 16 * 1024 = 16kb
char * arrayURL[8] = {
  "http://jenny.torontocast.com:8134/stream",
  
  "http://188.165.212.154:8478/stream",
  "https://igor.torontocast.com:1025/;.mp3",
  "http://streamer.radio.co/s06b196587/listen",
   
  
  "http://media-ice.musicradio.com:80/ClassicFMMP3",
  "http://naxos.cdnstream.com:80/1255_128",
  "http://149.56.195.94:8015/steam",
  "http://ice2.somafm.com/christmas-128-mp3"
};

String arrayStation[8] = {
  "Mega Shuffle",
 
  "WayUp Radio",
  "Asia Dream",
  "KPop Radio",
 
  
  "Classic FM",
  "Lite Favorites",
  "MAXXED Out",
  "SomaFM Xmas"
};

const int LED = 10;   // GPIO LED 
const int BTNA = 0;  // GPIO Play and Pause
const int BTNB = 35;
const int BTNC = 12;
const int BTND = 17;
// GPIO Switch Channel / Volume

AudioGeneratorTalkie *talkie;          
AudioGeneratorMP3 *mp3;
AudioFileSourceICYStream *file;
AudioFileSourceBuffer *buff;
AudioOutputI2S *out;

const int numCh = sizeof(arrayURL)/sizeof(char *);
bool TestMode = false;
uint32_t LastTime = 0;
int playflag = 0;
int ledflag = 0;
//int btnaflag = 0;
//int btnbflag = 0;
float fgain = 4.0;
int sflag = 0;
char *URL = arrayURL[sflag]; 
String station = arrayStation[sflag];

int backlight[5] = {10,30,60,120,220};
byte b=2;
int press1=0;
int press2=0;
bool inv=0;
       
void setup() {
  tft.init();
  tft.setRotation(0);
  tft.setSwapBytes(true);
   tft.setFreeFont(&Orbitron_Medium_20);
   tft.fillScreen(TFT_BLACK);
   tft.pushImage(0, 0,  135, 240, background);

  ledcSetup(pwmLedChannelTFT, pwmFreq, pwmResolution);
  ledcAttachPin(TFT_BL, pwmLedChannelTFT);
  ledcWrite(pwmLedChannelTFT, backlight[b]);
  
  
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  digitalWrite(LED , HIGH);
  pinMode(BTNA, INPUT);
  pinMode(BTNB, INPUT);
  pinMode(BTNC, INPUT_PULLUP);
  pinMode(BTND, INPUT_PULLUP);
 



 

           tft.setCursor(14, 20);
           tft.println("Radio");

 tft.drawLine(0,28,135,28,TFT_GREY);
 delay(500);
 delay(1000);
 initwifi();

 for(int i=0;i<b+1;i++)
   tft.fillRect(108+(i*4),18,2,6,TFT_GREEN);

  tft.drawString("Ready   ",78,44,2 );
  tft.drawString(String(fgain),78,66,2 );
  tft.drawString(String(arrayStation[sflag]),12,108,2);
          tft.setTextFont(1);
          tft.setCursor(8, 211, 1);
          tft.println(WiFi.localIP());

  Serial.printf("STATUS(System) Ready \n\n");
  out = new AudioOutputI2S(0, 1); // Output to builtInDAC
  out->SetOutputModeMono(true);
  out->SetGain(fgain*0.05);
  }

float n=0;

void loop() {
  
  if (playflag == 1) {
  tft.pushImage(50, 126,  animation_width, animation_height, frame[int(n)]);
  n=n+0.05  ;
  if(int(n)==frames)
  n=0; }
  else
  {tft.pushImage(50, 126,  animation_width, animation_height, frame[frames-1]);}
  
  
  static int lastms = 0;
  if (playflag == 0) {
    if (digitalRead(BTNA) ==  LOW) {
      StartPlaying();
      tft.drawString("Playing!   ",78,44,2);
      playflag = 1;
      
    }

    if (digitalRead(BTNB) ==  LOW) {
      sflag = (sflag + 1) % numCh;
      URL = arrayURL[sflag];
      station = arrayStation[sflag];           
      
      tft.setTextSize(1);
    
    
      tft.drawString(String(station),12,108,2);
      delay(300);
    }
  }

  if (playflag == 1) {
    if (mp3->isRunning()) {
      if (millis() - lastms > 1000) {
        lastms = millis();
        Serial.printf("STATUS(Streaming) %d ms...\n", lastms);
        
        ledflag = ledflag + 1;
        if (ledflag > 1) {
          ledflag = 0;
          digitalWrite(LED , HIGH);
          
        } else {
          digitalWrite(LED , LOW);
        }

      }
      if (!mp3->loop()) mp3->stop();
    } else {
      Serial.printf("MP3 done\n");
      playflag = 0;
      
      digitalWrite(LED , HIGH);
     
    }
    if (digitalRead(BTNA) ==  LOW) {
      StopPlaying();
      playflag = 0;
      tft.drawString("Stoped!   ",78,44,2);
      digitalWrite(LED , HIGH);
         
      delay(200);
    }
    if (digitalRead(BTNB) ==  LOW) {
      fgain = fgain + 1.0;
      if (fgain > 10.0) {
        fgain = 1.0;
      }
      out->SetGain(fgain*0.05);
      tft.drawString(String(fgain),78,66,2 );
      Serial.printf("STATUS(Gain) %f \n", fgain*0.05);
      delay(200);
    }
  }

 if(digitalRead(BTNC)==0){
   if(press2==0)
   {press2=1;
   tft.fillRect(108,18,25,6,TFT_BLACK);
 
   b++;
   if(b>4)
   b=0;

   for(int i=0;i<b+1;i++)
   tft.fillRect(108+(i*4),18,2,6,TFT_GREEN);
   ledcWrite(pwmLedChannelTFT, backlight[b]);}
   }else press2=0;

   if(digitalRead(BTND)==0){
   if(press1==0)
   {press1=1;
   inv=!inv;
   tft.invertDisplay(inv);}
   }else press1=0;
  
}

void StartPlaying() {
  file = new AudioFileSourceICYStream(URL);
  file->RegisterMetadataCB(MDCallback, (void*)"ICY");
  buff = new AudioFileSourceBuffer(file, bufferSize);
  buff->RegisterStatusCB(StatusCallback, (void*)"buffer");
  out = new AudioOutputI2S(0, 1); // Output to builtInDAC
  out->SetOutputModeMono(true);
  out->SetGain(fgain*0.05);
  mp3 = new AudioGeneratorMP3();
  mp3->RegisterStatusCB(StatusCallback, (void*)"mp3");
  mp3->begin(buff, out);
  Serial.printf("STATUS(URL) %s \n", URL);
  Serial.flush();
}

void StopPlaying() {
  if (mp3) {
    mp3->stop();
    delete mp3;
    mp3 = NULL;
  }
  if (buff) {
    buff->close();
    delete buff;
    buff = NULL;
  }
  if (file) {
    file->close();
    delete file;
    file = NULL;
  }
  Serial.printf("STATUS(Stopped)\n");
  Serial.flush();
}

void initwifi() {
  WiFi.disconnect();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("STATUS(Connecting to WiFi) ");
    delay(1000);
    i = i + 1;
    if (i > 10) {
      ESP.restart();
    }
  }
  Serial.println("OK");
}

void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string) {
  const char *ptr = reinterpret_cast<const char *>(cbData);
  (void) isUnicode; // Punt this ball for now
  // Note that the type and string may be in PROGMEM, so copy them to RAM for printf
  char s1[32], s2[64];
  strncpy_P(s1, type, sizeof(s1));
  s1[sizeof(s1) - 1] = 0;
  strncpy_P(s2, string, sizeof(s2));
  s2[sizeof(s2) - 1] = 0;
  Serial.printf("METADATA(%s) '%s' = '%s'\n", ptr, s1, s2);

               
                                                                         
  Serial.flush();
}

void StatusCallback(void *cbData, int code, const char *string) {
  const char *ptr = reinterpret_cast<const char *>(cbData);
  // Note that the string may be in PROGMEM, so copy it to RAM for printf
  char s1[64];
  strncpy_P(s1, string, sizeof(s1));
  s1[sizeof(s1) - 1] = 0;
  Serial.printf("STATUS(%s) '%d' = '%s'\n", ptr, code, s1);
  Serial.flush();
}
