/* ESP8266 connected to an ILI9341 TFT (320x240 display shows weather data received via WiFi connection to Weather Underground Servers and using their 'Forecast' API
 *  Data is decoded using Copyright Benoit Blanchon's (c) 2014-2017 excellent JSON library
 The MIT License (MIT) Copyright (c) 2017 by David Bird.
 Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, but not sub-license and/or 
 to sell copies of the Software or to permit persons to whom the Software is furnished to do so, subject to the following conditions: 
   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
See more at http://dsbird.org.uk
*/
//########################   Weather Display  #############################
// Receives and displays the weather forecast from the Weather Underground
// and then displays using a JSON decoder wx data to display on a web page using a webserver
//################# LIBRARIES ################
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiManager.h>      // https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>      // https://github.com/bblanchon/ArduinoJson
#include <DNSServer.h>
#include <Adafruit_GFX.h>     // v1.1.8 functions correctly
#include <Adafruit_ILI9341.h> // Only v1.02 functions correctly, changes made in 1.03 prevent BMP drawing
#include <FS.h>
//################ DISPLAY CONNECTIONS #######
#define _CS   D4 // D4 goes to TFT CS
#define _DC   D8 // D8 goes to TFT DC
#define _mosi D7 // D7 goes to TFT MOSI
#define _sclk D5 // D5 goes to TFT SCK/CLK
#define _rst     // ESP RST to TFT RESET
#define _miso    // Not connected
//       3.3V    // Goes to TFT LED  
//       5v      // Goes to TFT Vcc
//       Gnd     // Goes to TFT Gnd        

// Use hardware SPI (on ESP D7 and D5 as above)
Adafruit_ILI9341 tft = Adafruit_ILI9341(_CS, _DC);
//################ VARIABLES #################

//------ NETWORK VARIABLES---------
// Use your own API key by signing up for a free developer account at http://www.wunderground.com/weather/api/
String API_key       = "2aaedf86898e4dd7";           // See: http://www.wunderground.com/weather/api/d/docs (change here with your KEY)
String City          = "Melksham";                   // Your home city
String Country       = "UK";                         // Your country   
String Conditions    = "conditions";                 // See: http://www.wunderground.com/weather/api/d/docs?d=data/index&MR=1
char   wxserver[]    = "api.wunderground.com";       // Address for WeatherUnderGround
unsigned long        lastConnectionTime = 0;         // Last time you connected to the server, in milliseconds
const unsigned long  postingInterval = 20*60L*1000L; // Delay between updates, in milliseconds, WU allows 500 requests per-day maximum and no more than 10 per-minute!
String version_num   =  "v1.10";
#define DELAY_SHORT  (1*1000)     // 1-sec
#define DELAY_ERROR  (20*60*1000) // 20 minute delay between updates after an error
bool Metric = true;  // Set to false for Imperial units
bool Hybrid = true;  // Set to false to show 'kph' wind speed instead of 'mph' and rain in 'mm' instead of 'in'

//################ PROGRAM VARIABLES #########
// Weather 
String webpage, city, country, date_time, observationtime, wx_forecast,
       DWDay0, DMon0, DDateDa0, DDateMo0, DDateYr0, Dicon0, DHtemp0, DLtemp0, DHumi0, DPop0, DRain0, DW_spd0, DW_dir0, DW_dir_deg0, 
       DWDay1, DMon1, DDateDa1, DDateMo1, DDateYr1, Dicon1, DHtemp1, DLtemp1, DHumi1, DPop1, DRain1, DW_spd1, DW_dir1, DW_dir_deg1, 
       DWDay2, DMon2, DDateDa2, DDateMo2, DDateYr2, Dicon2, DHtemp2, DLtemp2, DHumi2, DPop2, DRain2, DW_spd2, DW_dir2, DW_dir_deg2, 
       DWDay3, DMon3, DDateDa3, DDateMo3, DDateYr3, Dicon3, DHtemp3, DLtemp3, DHumi3, DPop3, DRain3, DW_spd3, DW_dir3, DW_dir_deg3;
// Astronomy
String  DphaseofMoon, Sunrise, Sunset, Moonrise, Moonset, Moon_image, Moon_light;
bool    Centred = true, notCentred = false;
       
void setup() {
  system_setup();
  display_progress("Getting forecast data",60);
  get_wx_data("forecast");
  display_progress("Getting astronomy data",80);
  get_wx_data("astronomy");
  display_progress("Starting...",100);
  lastConnectionTime = millis();
}

void loop() {
  DisplayHeader();
  DisplayWxDetails("",    Dicon0,DHtemp0,DLtemp0,DHumi0,DW_spd0,DW_dir0,DW_dir_deg0,DPop0,DRain0,80,30);
  DisplayMoonIcon(Moon_image,220,45);
  DisplayWxDetails(DWDay1,Dicon1,DHtemp1,DLtemp1,DHumi1,DW_spd1,DW_dir1,DW_dir_deg1,DPop1,DRain1,30,153);
  DisplayWxDetails(DWDay2,Dicon2,DHtemp2,DLtemp2,DHumi2,DW_spd2,DW_dir2,DW_dir_deg2,DPop2,DRain2,135,153);
  DisplayWxDetails(DWDay3,Dicon3,DHtemp3,DLtemp3,DHumi3,DW_spd3,DW_dir3,DW_dir_deg3,DPop3,DRain3,240,153);
  for (int loop=1; loop <= 5; loop++) scroll_message("Forecast for "+City+" until "+observationtime+"... "+wx_forecast,10,123,ILI9341_ORANGE,1,47);
  if (millis() - lastConnectionTime > postingInterval) { // 20-minutes
    get_wx_data("forecast");
    get_wx_data("astronomy");
    lastConnectionTime = millis();
  }
}

//################ SYSTEM FUNCTIONS ##########
void DisplayHeader(){
  clear_screen();
  int x = 72;
  int y = 0;
  BmpDraw_Wx("/wuIcon.bmp",275,11);
  tft.drawRoundRect(x,y,170,20,5, ILI9341_YELLOW);
  tft.fillRoundRect(x+1,y+1,168,18,4, ILI9341_BLUE);
  display_text(x+7,y+3,DWDay0+" "+DDateDa0+"-"+DMon0+"-"+DDateYr0,ILI9341_WHITE,2,notCentred);
  display_text(280,0,version_num,ILI9341_CYAN,1,notCentred); 
}

void DisplayWxDetails(String WDay, String icon, String Htemp, String Ltemp, String Humi, String Wspd, String WDir, String WDeg, String PoP, String Rain, int x, int y){
  String units;
  if (WDay != "") {
    tft.drawRoundRect(x+12,y-14,25,13,3,ILI9341_YELLOW);
    tft.fillRoundRect(x+13,y-13,23,11,2,ILI9341_BLUE);
    display_text(x+25,y-11,WDay,ILI9341_WHITE,1,Centred);
    display_text(x-16,y+11,Htemp+String(char(247)),ILI9341_ORANGE,1,notCentred);
    display_text(x-16,y+21,Ltemp+String(char(247)),ILI9341_CYAN,1,notCentred);
  }
  else
  {
    display_text(x-35,y+10,Htemp+String(char(247)),ILI9341_ORANGE,3,Centred);
    display_text(x-35,y+55,Ltemp+String(char(247)),ILI9341_CYAN,2,Centred);
  }
  if (Hybrid) units = "mph"; else if (Metric) units = "kph"; else units = "mph";
  DisplayWxIcon(icon,x,y);
  display_text(x+25,y+53,"Humi "+Humi+"%",ILI9341_CYAN,1,Centred);
  display_text(x+25,y+65,WDir+" @ "+Wspd+units,ILI9341_YELLOW,1,Centred);
  DisplayWindDirection(WDir,x,y);
  if (Hybrid) units = "mm"; else if (Metric) units = "mm"; else units = "in";
  display_text(x+25,y+77,PoP+"%"+(Rain=="0"?" of rain":" of "+Rain+units),ILI9341_CYAN,1,Centred);
}

void DisplayMoonIcon(String image, int x, int y) {
  BmpDraw_Wx("/"+image,x,y);
  display_text(x-15,y+10,"Sun",ILI9341_CYAN,1,Centred);
  display_text(x-15,y+22,Sunrise,ILI9341_WHITE,1,Centred);
  display_text(x-15,y+34,Sunset,ILI9341_WHITE,1,Centred);
  display_text(x+65,y+10,"Moon",ILI9341_CYAN,1,Centred);
  display_text(x+65,y+22,Moonrise,ILI9341_WHITE,1,Centred);
  display_text(x+65,y+34,Moonset,ILI9341_WHITE,1,Centred);
  display_text(x+25,y+60,DphaseofMoon+" "+Moon_light+"%",ILI9341_WHITE,1,Centred);
  tft.drawFastHLine(5,135,310,ILI9341_YELLOW);
}

void DisplayWindDirection(String WDir, int x, int y) {
  if (WDir == "N"  || WDir == "North") BmpDraw_Wx("/S.bmp",x+55,y+15);
  if (WDir == "S"  || WDir == "South") BmpDraw_Wx("/N.bmp",x+55,y+15);
  if (WDir == "E"  || WDir == "East")  BmpDraw_Wx("/W.bmp",x+55,y+15);
  if (WDir == "W"  || WDir == "West")  BmpDraw_Wx("/E.bmp",x+55,y+15);
  if (WDir == "NE" || WDir == "NNE" || WDir == "ENE") BmpDraw_Wx("/SW.bmp",x+55,y+15);
  if (WDir == "SE" || WDir == "ESE" || WDir == "SSE") BmpDraw_Wx("/NW.bmp",x+55,y+15);
  if (WDir == "SW" || WDir == "SSW" || WDir == "WSW") BmpDraw_Wx("/NE.bmp",x+55,y+15);
  if (WDir == "NW" || WDir == "NNW" || WDir == "WNW") BmpDraw_Wx("/SE.bmp",x+55,y+15);
}

void scroll_message(String Message,int x,int y,int colour,int text_size,int marquee_chars){
  tft.setTextColor(colour,ILI9341_BLACK);
  tft.setTextSize(text_size);
  Message += "    ";
  bool first_display = true;
  const int width = marquee_chars; // width of the marquee display (in characters)
  for (int offset = 0; offset < Message.length(); offset++) {
    String txt = "";
    for (int i = 0; i <= width; i++) txt += Message.charAt((offset + i) % Message.length());
    tft.setCursor(x, y); 
    tft.print(txt);
    if (first_display) {
      delay(3000);
      first_display = false;
    }
    delay(25);
  }
  tft.setCursor(x, y); 
  for (int i = 0; i < width; i++) tft.print(" ");  
}

void get_wx_data (String request_type) {
  static char RxBuf[8704];
  String request;
  request  = "GET /api/" + API_key + "/"+ request_type + "/q/" + Country + "/" + City + ".json HTTP/1.1\r\n";
  request += F("User-Agent: Weather Webserver v2.0\r\n");
  request += F("Accept: */*\r\n");
  request += "Host: " + String(wxserver) + "\r\n";
  request += F("Connection: close\r\n");
  request += F("\r\n");
  //Serial.println(request);
  //Serial.print(F("Connecting to ")); Serial.println(wxserver);
  // Use WiFiClient class to create TCP connections
  WiFiClient httpclient;
  if (!httpclient.connect(wxserver, 80)) {
    Serial.println(F("connection failed"));
    delay(DELAY_ERROR);
    return;
  }
  Serial.print(request);
  httpclient.print(request); //send the http request to the server
  httpclient.flush();
  // Collect http response headers and content from Weather Underground, discard HTTP headers, leaving JSON formatted information returned in RxBuf.
  int    respLen = 0;
  bool   skip_headers = true;
  String rx_line;
  while (httpclient.connected() || httpclient.available()) {
    if (skip_headers) {
      rx_line = httpclient.readStringUntil('\n'); //Serial.println(rx_line);
      if (rx_line.length() <= 1) { // a blank line denotes end of headers
        skip_headers = false;
      }
    }
    else {
      int bytesIn;
      bytesIn = httpclient.read((uint8_t *)&RxBuf[respLen], sizeof(RxBuf) - respLen);
      //Serial.print(F("bytesIn ")); Serial.println(bytesIn);
      if (bytesIn > 0) {
        respLen += bytesIn;
        if (respLen > (int)sizeof(RxBuf)) respLen = sizeof(RxBuf);
      }
      else if (bytesIn < 0) {
        Serial.print(F("read error "));
        Serial.println(bytesIn);
      }
    }
    delay(1);
  }
  httpclient.stop();

  if (respLen >= (int)sizeof(RxBuf)) {
    Serial.print(F("RxBuf overflow ")); Serial.println(respLen);
    delay(DELAY_ERROR);
    return;
  }
  RxBuf[respLen++] = '\0'; // Terminate the C string
  Serial.print(F("respLen ")); Serial.println(respLen); //Serial.println(RxBuf);
  if (request_type == "forecast"){
    if (showWeather_forecast(RxBuf)) {delay(DELAY_SHORT);} else {delay(DELAY_ERROR);}
  }
  if (request_type == "astronomy"){
    if (showWeather_astronomy(RxBuf)) {delay(DELAY_SHORT);} else {delay(DELAY_ERROR);}
  }
}

bool showWeather_astronomy(char *json) {
  StaticJsonBuffer<1*1024> jsonBuffer;
  char *jsonstart = strchr(json, '{'); // Skip characters until first '{' found
  //Serial.print(F("jsonstart ")); Serial.println(jsonstart);
  if (jsonstart == NULL) {
    Serial.println(F("JSON data missing"));
    return false;
  }
  json = jsonstart;
  // Parse JSON
  JsonObject& root = jsonBuffer.parseObject(json);
  if (!root.success()) {
    Serial.println(F("jsonBuffer.parseObject() failed"));
    return false;
  }
  // Extract weather info from parsed JSON
  JsonObject& current = root["moon_phase"];
  String percentIlluminated = current["percentIlluminated"];
  String phaseofMoon = current["phaseofMoon"];
  int SRhour         = current["sunrise"]["hour"];
  int SRminute       = current["sunrise"]["minute"];
  int SShour         = current["sunset"]["hour"];
  int SSminute       = current["sunset"]["minute"];
  int MRhour         = current["moonrise"]["hour"];
  int MRminute       = current["moonrise"]["minute"];
  int MShour         = current["moonset"]["hour"];
  int MSminute       = current["moonset"]["minute"];
  Sunrise    = (SRhour<10?"0":"")+String(SRhour)+":"+(SRminute<10?"0":"")+String(SRminute);
  Sunset     = (SShour<10?"0":"")+String(SShour)+":"+(SSminute<10?"0":"")+String(SSminute);
  Moonrise   = (MRhour<10?"0":"")+String(MRhour)+":"+(MRminute<10?"0":"")+String(MRminute);
  Moonset    = (MShour<10?"0":"")+String(MShour)+":"+(MSminute<10?"0":"")+String(MSminute);
  Moon_light = percentIlluminated;
  // New Moon example //http://icons.wunderground.com/graphics/moonpictsnew/moon1.gif
  // NOte: Tjhese images are for the NORTHERN hemisphere! e.g. When first quarter the moon is illuminated on the right
  // but in the southern hemisphere it is illuminated on the left...so you need to change the images to the alternative ones on WU Site.
  // Paste the example link above into your browser and change the image name until it matches your needs e.g. change moon1.gif to moon2.gif
  //
  // Convert the GIF images to 24-bit 50x50 pixel BMP versons then upload to the ESP8266
  //
  if (phaseofMoon == "New Moon")        Moon_image = "moon1.bmp"; else
  if (phaseofMoon == "Waxing Crescent") Moon_image = "moon4.bmp"; else
  if (phaseofMoon == "First Quarter")   Moon_image = "moon8.bmp"; else
  if (phaseofMoon == "Waxing Gibbous")  Moon_image = "moon10.bmp"; else
  if (phaseofMoon == "Full")            Moon_image = "moon16.bmp"; else
  if (phaseofMoon == "Waning Gibbous")  Moon_image = "moon20.bmp"; else
  if (phaseofMoon == "Last Quarter")    Moon_image = "moon23.bmp"; else
  if (phaseofMoon == "Waning Crescent") Moon_image = "moon26.bmp";
  DphaseofMoon = phaseofMoon;
  return true;
}

bool showWeather_forecast(char *json) {
  DynamicJsonBuffer jsonBuffer(8704);
  char *jsonstart = strchr(json, '{');
  //Serial.print(F("jsonstart ")); Serial.println(jsonstart);
  if (jsonstart == NULL) {
    Serial.println(F("JSON data missing"));
    return false;
  }
  json = jsonstart;

  // Parse JSON
  JsonObject& root = jsonBuffer.parseObject(json);
  if (!root.success()) {
    Serial.println(F("jsonBuffer.parseObject() failed"));
    return false;
  }
  // Extract weather info from parsed JSON
  JsonObject& wxforecast = root["forecast"]["txt_forecast"];
  if (Metric) {String fcttext0 = wxforecast["forecastday"][0]["fcttext_metric"]; wx_forecast = fcttext0;}
  else        {String fcttext0 = wxforecast["forecastday"][0]["fcttext"];        wx_forecast = fcttext0;}
  JsonObject& forecast = root["forecast"]["simpleforecast"];
  //# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 
  String Temp_mon   = forecast["forecastday"][0]["date"]["monthname"];
  String Mon0       = forecast["forecastday"][0]["date"]["monthname_short"]; DMon0       = Mon0;
  int DateYr0       = forecast["forecastday"][0]["date"]["year"];            DDateYr0    = String(DateYr0).substring(2);
  int DateDa0       = forecast["forecastday"][0]["date"]["day"];             DDateDa0    = DateDa0<10?"0"+String(DateDa0):String(DateDa0);
  String WDay0      = forecast["forecastday"][0]["date"]["weekday_short"];   DWDay0      = WDay0;
  String TempHr     = forecast["forecastday"][0]["date"]["hour"];            observationtime = TempHr+":";
  String TempMin    = forecast["forecastday"][0]["date"]["min"];             observationtime += TempMin+"Hr on ";
  observationtime  += DDateDa0+" "+DMon0+" "+DDateYr0;
  if (Metric) {
    int Htemp0      = forecast["forecastday"][0]["high"]["celsius"];         DHtemp0     = String(Htemp0);
    int Ltemp0      = forecast["forecastday"][0]["low"]["celsius"];          DLtemp0     = String(Ltemp0);
    int rain0       = forecast["forecastday"][0]["qpf_allday"]["mm"];        DRain0      = String(rain0);
    int w_kph0      = forecast["forecastday"][0]["avewind"]["kph"];          DW_spd0     = String(w_kph0);
  }
  else
  {
    int Htemp0      = forecast["forecastday"][0]["high"]["fahrenheit"];      DHtemp0     = String(Htemp0); 
    int Ltemp0      = forecast["forecastday"][0]["low"]["fahrenheit"];       DLtemp0     = String(Ltemp0); 
    int rain0       = forecast["forecastday"][0]["qpf_allday"]["in"];        DRain0      = String(rain0);
    int w_mph0      = forecast["forecastday"][0]["avewind"]["mph"];          DW_spd0     = String(w_mph0);
  }
  String icon_url0  = forecast["forecastday"][0]["icon"];                    Dicon0      = icon_url0;
  String pop0       = forecast["forecastday"][0]["pop"];                     DPop0       = String(pop0);
  if (Hybrid && Metric)
  {
    int w_mph0      = forecast["forecastday"][0]["avewind"]["mph"];          DW_spd0     = String(w_mph0);
  }
  String w_dir0     = forecast["forecastday"][0]["avewind"]["dir"];          DW_dir0     = String(w_dir0);
  String w_dir_deg0 = forecast["forecastday"][0]["avewind"]["degrees"];      DW_dir_deg0 = String(w_dir_deg0);
  int humi0         = forecast["forecastday"][0]["avehumidity"];             DHumi0      = String(humi0);
  //# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 
  String WDay1      = forecast["forecastday"][1]["date"]["weekday_short"];   DWDay1      = WDay1;
  String Mon1       = forecast["forecastday"][1]["date"]["monthname_short"]; DMon1       = Mon1;
  int DateDa1       = forecast["forecastday"][1]["date"]["day"];             DDateDa1    = DateDa1<10?"0"+String(DateDa1):String(DateDa1);
  int DateYr1       = forecast["forecastday"][1]["date"]["year"];            DDateYr1    = String(DateYr1).substring(2);
  if (Metric) {
    int Htemp1      = forecast["forecastday"][1]["high"]["celsius"];         DHtemp1     = String(Htemp1);
    int Ltemp1      = forecast["forecastday"][1]["low"]["celsius"];          DLtemp1     = String(Ltemp1);
    int rain1       = forecast["forecastday"][1]["qpf_allday"]["mm"];        DRain1      = String(rain1);
    int w_kph1      = forecast["forecastday"][1]["avewind"]["kph"];          DW_spd1     = String(w_kph1);
  }
  else
  {
    int Htemp1      = forecast["forecastday"][1]["high"]["fahrenheit"];      DHtemp1     = String(Htemp1); 
    int Ltemp1      = forecast["forecastday"][1]["low"]["fahrenheit"];       DLtemp1     = String(Ltemp1); 
    int rain1       = forecast["forecastday"][1]["qpf_allday"]["in"];        DRain1      = String(rain1);
    int w_mph1      = forecast["forecastday"][1]["avewind"]["mph"];          DW_spd1     = String(w_mph1);
  }
  String icon_url1  = forecast["forecastday"][1]["icon"];                    Dicon1      = icon_url1;
  String pop1       = forecast["forecastday"][1]["pop"];                     DPop1       = String(pop1);
  if (Hybrid && Metric)
  {
    int w_mph1      = forecast["forecastday"][1]["avewind"]["mph"];          DW_spd1     = String(w_mph1);
  }
  String w_dir1     = forecast["forecastday"][1]["avewind"]["dir"];          DW_dir1     = String(w_dir1);
  String w_dir_deg1 = forecast["forecastday"][1]["avewind"]["degrees"];      DW_dir_deg1 = String(w_dir_deg1);
  int humi1         = forecast["forecastday"][1]["avehumidity"];             DHumi1      = String(humi1);
  //# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 
  String WDay2      = forecast["forecastday"][2]["date"]["weekday_short"];   DWDay2      = WDay2;
  String Mon2       = forecast["forecastday"][2]["date"]["monthname_short"]; DMon2       = Mon2;
  int DateDa2       = forecast["forecastday"][2]["date"]["day"];             DDateDa2    = DateDa2<10?"0"+String(DateDa2):String(DateDa2);
  int DateYr2       = forecast["forecastday"][2]["date"]["year"];            DDateYr2    = String(DateYr2).substring(2);
  if (Metric) {
    int Htemp2      = forecast["forecastday"][2]["high"]["celsius"];         DHtemp2     = String(Htemp2);
    int Ltemp2      = forecast["forecastday"][2]["low"]["celsius"];          DLtemp2     = String(Ltemp2);
    int rain2       = forecast["forecastday"][2]["qpf_allday"]["mm"];        DRain2      = String(rain2);
    int w_kph2      = forecast["forecastday"][2]["avewind"]["kph"];          DW_spd2     = String(w_kph2);
  }
  else
  {
    int Htemp2      = forecast["forecastday"][2]["high"]["fahrenheit"];      DHtemp2     = String(Htemp2); 
    int Ltemp2      = forecast["forecastday"][2]["low"]["fahrenheit"];       DLtemp2     = String(Ltemp2); 
    int rain2       = forecast["forecastday"][2]["qpf_allday"]["in"];        DRain2      = String(rain2);
    int w_mph2      = forecast["forecastday"][2]["avewind"]["mph"];          DW_spd2     = String(w_mph2);
  }
  String icon_url2  = forecast["forecastday"][2]["icon"];                    Dicon2      = icon_url2;
  String pop2       = forecast["forecastday"][2]["pop"];                     DPop2       = String(pop2);
  if (Hybrid && Metric)
  {
    int w_mph2        = forecast["forecastday"][2]["avewind"]["mph"];        DW_spd2     = String(w_mph2);
  }
  String w_dir2     = forecast["forecastday"][2]["avewind"]["dir"];          DW_dir2     = String(w_dir2);
  String w_dir_deg2 = forecast["forecastday"][2]["avewind"]["degrees"];      DW_dir_deg2 = String(w_dir_deg2);
  int humi2         = forecast["forecastday"][2]["avehumidity"];             DHumi2      = String(humi2);
  //# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 
  String WDay3      = forecast["forecastday"][3]["date"]["weekday_short"];   DWDay3      = WDay3;
  String Mon3       = forecast["forecastday"][3]["date"]["monthname_short"]; DMon3       = Mon3;
  int DateDa3       = forecast["forecastday"][3]["date"]["day"];             DDateDa3    = DateDa3<10?"0"+String(DateDa3):String(DateDa3);
  int DateYr3       = forecast["forecastday"][3]["date"]["year"];            DDateYr3    = String(DateYr3).substring(2);
  if (Metric) {
    int Htemp3      = forecast["forecastday"][3]["high"]["celsius"];         DHtemp3     = String(Htemp3);
    int Ltemp3      = forecast["forecastday"][3]["low"]["celsius"];          DLtemp3     = String(Ltemp3);
    int rain3       = forecast["forecastday"][3]["qpf_allday"]["mm"];        DRain3      = String(rain3);
    int w_kph3      = forecast["forecastday"][3]["avewind"]["kph"];          DW_spd3     = String(w_kph3);
  }
  else
  {
    int Htemp3      = forecast["forecastday"][3]["high"]["fahrenheit"];      DHtemp3     = String(Htemp3); 
    int Ltemp3      = forecast["forecastday"][3]["low"]["fahrenheit"];       DLtemp3     = String(Ltemp3); 
    int rain3       = forecast["forecastday"][3]["qpf_allday"]["in"];        DRain3      = String(rain3);
    int w_mph3      = forecast["forecastday"][3]["avewind"]["mph"];          DW_spd3     = String(w_mph3);
  }
  String icon_url3  = forecast["forecastday"][3]["icon"];                    Dicon3      = icon_url3;
  String pop3       = forecast["forecastday"][3]["pop"];                     DPop3       = String(pop3);
  if (Hybrid && Metric)
  {
    int w_mph3      = forecast["forecastday"][3]["avewind"]["mph"];          DW_spd3     = String(w_mph3);
  }
  String w_dir3     = forecast["forecastday"][3]["avewind"]["dir"];          DW_dir3     = String(w_dir3);
  String w_dir_deg3 = forecast["forecastday"][3]["avewind"]["degrees"];      DW_dir_deg3 = String(w_dir_deg3);
  int humi3         = forecast["forecastday"][3]["avehumidity"];             DHumi3      = String(humi3);
 
//  JsonObject& current = root["forecast"]["simpleforecast"];
//  int DateDa = current["forecastday"][0]["date"]["day"];     Serial.print(String(DateDa)+"/");
//  int DateMo = current["forecastday"][0]["date"]["month"];   Serial.print((DateMo<10?"0":"")+String(DateMo)+"/");
//  int DateYr = current["forecastday"][0]["date"]["year"];    Serial.print(String(DateYr)+" ");
//  int Htemp  = current["forecastday"][0]["high"]["celsius"]; Serial.print("High temp :"+String(Htemp));
//  int Ltemp  = current["forecastday"][0]["low"]["celsius"];  Serial.println(" Low temp :"+String(Ltemp));
  return true;
}

// This function opens a Windows Bitmap (BMP) file and displays it at the given coordinates.  It's sped up
// by reading many pixels worth of data at a time (rather than pixel by pixel).  Increasing the buffer
// size takes more of the Arduino's precious RAM but makes loading a little faster.  20 pixels seems a
// good balance.
#define BUFFPIXEL 40

void BmpDraw_Wx(String filename, uint16_t x, uint16_t y) {
  File     bmpFile;
  int      bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3*BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
  uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  goodBmp = false;       // Set to true on valid header parse
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int      w, h, row, col;
  uint8_t  r, g, b;
  uint32_t pos = 0, startTime = millis();

  if((x >= tft.width()) || (y >= tft.height())) return;
  SPIFFS.begin();
  if ((bmpFile = SPIFFS.open(filename, "r")) == false) { 
    Serial.print(F("File not found: "));
    Serial.println(filename);
    return; 
  } 

  // Parse BMP header
  if(read16(bmpFile) == 0x4D42) { // BMP signature
    Serial.print(F("File size: ")); Serial.println(read32(bmpFile));
    (void)read32(bmpFile); // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    Serial.print(F("Image Offset: ")); Serial.println(bmpImageoffset, DEC);
    // Read DIB header
    Serial.print(F("Header size: ")); Serial.println(read32(bmpFile));
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if(read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile); // bits per pixel
      Serial.print(F("Bit Depth: ")); Serial.println(bmpDepth);
      if((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed
        goodBmp = true; // Supported BMP format -- proceed!
        Serial.print(F("Image size: "));Serial.print(bmpWidth);Serial.print('x');Serial.println(bmpHeight);

        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * 3 + 3) & ~3;

        // If bmpHeight is negative, image is in top-down order.
        // This is not canon but has been observed in the wild.
        if(bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }
        // Crop area to be loaded
        w = bmpWidth;   
        h = bmpHeight;  
        if((x+w-1) >= tft.width())  w = tft.width()  - x; 
        if((y+h-1) >= tft.height()) h = tft.height() - y;
        // Set TFT address window to clipped image bounds
        tft.setAddrWindow(x, y, x+w-1, y+h-1);  
        for (row=0; row<h; row++) { // For each scanline...
          // Seek to start of scan line.  It might seem labor-intensive to be doing this on every line, but this
          // method covers a lot of gritty details like cropping and scanline padding.  Also, the seek only takes
          // place if the file position actually needs to change (avoids a lot of cluster math in SD library).
          if(flip) // Bitmap is stored bottom-to-top order (normal BMP) 
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else     // Bitmap is stored top-to-bottom
            pos = bmpImageoffset + row * rowSize;
          if(bmpFile.position() != pos) { // Need seek?
            bmpFile.seek(pos, SeekSet);
            buffidx = sizeof(sdbuffer); // Force buffer reload
          }

          for (col=0; col<w; col++) { // For each pixel...
            // Time to read more pixel data?
            if (buffidx >= sizeof(sdbuffer)) { // Indeed
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0; // Set index to beginning
            }

            // Convert pixel from BMP to TFT format, push to display
            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];
            tft.pushColor(tft.color565(r,g,b));
          } // end pixel
        } // end scanline
        //Serial.print(F("Loaded in "));
        //Serial.print(millis() - startTime);
        //Serial.println(" ms");
      } // end goodBmp
    }
  }

  bmpFile.close();
  if(!goodBmp) Serial.println(F("BMP format not recognized."));
}

uint16_t read16(File &f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(File &f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

void DisplayWxIcon(String displayIcon, int x, int y) {
  BmpDraw_Wx("/"+displayIcon+".bmp",x,y);
}

void display_progress (String title, int percent) {
  int title_pos = (320 - title.length()*12)/2; // Centre title
  int x_pos = 35; int y_pos = 105;
  int bar_width = 250; int bar_height = 15;
  tft.fillRect(x_pos-30,y_pos-20,320,16,ILI9341_BLACK); // Clear titles
  display_text(title_pos,y_pos-20,title,ILI9341_GREEN,2,notCentred);
  tft.drawRoundRect(x_pos,y_pos,bar_width+2,bar_height,5,ILI9341_YELLOW); // Draw progress bar outline
  tft.fillRoundRect(x_pos+2,y_pos+1,percent*bar_width/100-2,bar_height-3,4,ILI9341_BLUE); // Draw progress
  delay(1000);
}

void display_text(int x, int y, String message, int txt_colour, int txt_size, bool centred) {
  int txt_scale = 6; // Defaults to size = 1 so Font size is 5x8 !
  if (txt_size == 2) txt_scale = 11; // Font size is 10x16
  if (txt_size == 3) txt_scale = 17;
  if (txt_size == 4) txt_scale = 23;
  if (txt_size == 5) txt_scale = 27;
  if (centred) {
    x = x - message.length()*txt_scale/2;
  }
  tft.setCursor(x, y);
  tft.setTextColor(txt_colour);
  tft.setTextSize(txt_size);
  tft.print(message);
  tft.setTextSize(2); // Back to default text size
}

void clear_screen() {
  tft.fillScreen(ILI9341_BLACK);
}

void system_setup() {
  tft.begin();
  tft.setRotation(3);
  tft.setTextSize(2);
  clear_screen();
  display_progress("Initialising",5);
  Serial.begin(115200);    // initialise serial communications
  WiFiManager wifiManager; // Connect to Wi-Fi
  // New OOB ESP8266 has no Wi-Fi credentials so will connect and not need the next command to be uncommented and compiled in, a used one with incorrect credentials will
  // so restart the ESP8266 and connect your PC to the wireless access point called 'ESP8266_AP' or whatever you call it below in ""
  // wifiManager.resetSettings(); // Command to be included if needed, then connect to http://192.168.4.1/ and follow instructions to make the WiFi connection
  // Set a timeout until configuration is turned off, useful to retry or go to sleep in n-seconds
  //wifiManager.setTimeout(180);
  //fetches ssid and password and tries to connect, if connections succeeds it starts an access point with the name called "ESP8266_AP" and waits in a blocking loop for configuration
  display_progress("Connecting to network",20);
  if(!wifiManager.autoConnect("ESP8266_AP")) {
    Serial.println(F("failed to connect and timeout occurred"));
    delay(6000);
    ESP.reset(); //reset and try again
    delay(180000);
   }
  // At this stage the WiFi manager will have successfully connected to a network, or if not will try again in 180-seconds
  Serial.println(F("WiFi connected..."));
  //----------------------------------------------------------------------
  Serial.println(F("Use this URL to connect: http://")); Serial.println(WiFi.localIP().toString()+"/");// Print the IP address
  display_progress("Setting time",40);
  configTime(0 * 3600, 0, "pool.ntp.org", "time.nist.gov");
}

// #######################################################################
// <img src='https://icons.wxug.com/logos/JPG/wundergroundLogo_4c.jpg' width='45' height='20'/>";
// https://icons.wxug.com/logos/JPG/wundergroundLogo_4c.jpg

/* Response from WeatherUnderground for 'Conditions' query
{
  "response": {
  "version":"0.1",
  "termsofService":"http://www.wunderground.com/weather/api/d/terms.html",
  "features": {
  "forecast": 1
  }
  }
    ,
  "forecast":{
    "txt_forecast": {
    "date":"2:44 PM GMT",
    "forecastday": [
    {
    "period":0,
    "icon":"chancerain",
    "icon_url":"http://icons.wxug.com/i/c/k/chancerain.gif",
    "title":"Saturday",
    "fcttext":"Chance of showers. Lows overnight in the upper 30s.",
    "fcttext_metric":"Showers possible. Low 3C.",
    "pop":"90"
    }
    ,
    {
    "period":1,
    "icon":"nt_chancerain",
    "icon_url":"http://icons.wxug.com/i/c/k/nt_chancerain.gif",
    "title":"Saturday Night",
    "fcttext":"Showers this evening becoming a steady light rain overnight. Low 38F. Winds SW at 10 to 20 mph. Chance of rain 90%.",
    "fcttext_metric":"Showers this evening becoming a steady light rain overnight. Low 3C. Winds SW at 15 to 30 km/h. Chance of rain 90%.",
    "pop":"90"
    }
    ,
    {
    "period":2,
    "icon":"rain",
    "icon_url":"http://icons.wxug.com/i/c/k/rain.gif",
    "title":"Sunday",
    "fcttext":"Windy with rain likely. High 48F. Winds WSW at 20 to 30 mph. Chance of rain 90%. Rainfall near a quarter of an inch. Winds could occasionally gust over 40 mph.",
    "fcttext_metric":"Windy with periods of rain. High 9C. Winds WSW at 30 to 50 km/h. Chance of rain 90%. Rainfall around 6mm. Winds could occasionally gust over 65 km/h.",
    "pop":"90"
    }
    ,
    {
    "period":3,
    "icon":"nt_partlycloudy",
    "icon_url":"http://icons.wxug.com/i/c/k/nt_partlycloudy.gif",
    "title":"Sunday Night",
    "fcttext":"Partly cloudy. A few sprinkles possible. Low 37F. Winds W at 15 to 25 mph.",
    "fcttext_metric":"A few clouds from time to time. A few sprinkles possible. Low 3C. Winds W at 25 to 40 km/h.",
    "pop":"20"
    }
    ,
    {
    "period":4,
    "icon":"mostlycloudy",
    "icon_url":"http://icons.wxug.com/i/c/k/mostlycloudy.gif",
    "title":"Monday",
    "fcttext":"More clouds than sun. High near 50F. Winds WNW at 10 to 20 mph.",
    "fcttext_metric":"More clouds than sun. High around 10C. Winds WNW at 15 to 25 km/h.",
    "pop":"10"
    }
    ,
    {
    "period":5,
    "icon":"nt_partlycloudy",
    "icon_url":"http://icons.wxug.com/i/c/k/nt_partlycloudy.gif",
    "title":"Monday Night",
    "fcttext":"Partly cloudy skies. Low 36F. Winds WNW at 10 to 15 mph.",
    "fcttext_metric":"A few clouds. Low 2C. Winds WNW at 15 to 25 km/h.",
    "pop":"10"
    }
    ,
    {
    "period":6,
    "icon":"chancerain",
    "icon_url":"http://icons.wxug.com/i/c/k/chancerain.gif",
    "title":"Tuesday",
    "fcttext":"Partly cloudy in the morning. Increasing clouds with periods of showers later in the day. High around 50F. Winds SW at 10 to 15 mph. Chance of rain 60%.",
    "fcttext_metric":"Partly cloudy early followed by increasing clouds with showers developing later in the day. High around 10C. Winds SW at 10 to 15 km/h. Chance of rain 60%.",
    "pop":"60"
    }
    ,
    {
    "period":7,
    "icon":"nt_chancerain",
    "icon_url":"http://icons.wxug.com/i/c/k/nt_chancerain.gif",
    "title":"Tuesday Night",
    "fcttext":"Cloudy with light rain early. Low 44F. Winds SW at 10 to 20 mph. Chance of rain 60%.",
    "fcttext_metric":"Light rain early. Then remaining cloudy. Low 7C. Winds SW at 15 to 30 km/h. Chance of rain 60%.",
    "pop":"60"
    }
    ]
    },
    "simpleforecast": {
    "forecastday": [
    {"date":{
  "epoch":"1488654000",
  "pretty":"7:00 PM GMT on March 04, 2017",
  "day":4,
  "month":3,
  "year":2017,
  "yday":62,
  "hour":19,
  "min":"00",
  "sec":0,
  "isdst":"0",
  "monthname":"March",
  "monthname_short":"Mar",
  "weekday_short":"Sat",
  "weekday":"Saturday",
  "ampm":"PM",
  "tz_short":"GMT",
  "tz_long":"Europe/London"
},
    "period":1,
    "high": {
    "fahrenheit":"53",
    "celsius":"11"
    },
    "low": {
    "fahrenheit":"38",
    "celsius":"3"
    },
    "conditions":"Chance of Rain",
    "icon":"chancerain",
    "icon_url":"http://icons.wxug.com/i/c/k/chancerain.gif",
    "skyicon":"",
    "pop":90,
    "qpf_allday": {
    "in": 0.17,
    "mm": 4
    },
    "qpf_day": {
    "in": null,
    "mm": null
    },
    "qpf_night": {
    "in": 0.16,
    "mm": 4
    },
    "snow_allday": {
    "in": 0.0,
    "cm": 0.0
    },
    "snow_day": {
    "in": null,
    "cm": null
    },
    "snow_night": {
    "in": 0.0,
    "cm": 0.0
    },
    "maxwind": {
    "mph": 13,
    "kph": 21,
    "dir": "",
    "degrees": 0
    },
    "avewind": {
    "mph": 1,
    "kph": 2,
    "dir": "South",
    "degrees": 178
    },
    "avehumidity": 85,
    "maxhumidity": 0,
    "minhumidity": 0
    }
    ,
    {"date":{
  "epoch":"1488740400",
  "pretty":"7:00 PM GMT on March 05, 2017",
  "day":5,
  "month":3,
  "year":2017,
  "yday":63,
  "hour":19,
  "min":"00",
  "sec":0,
  "isdst":"0",
  "monthname":"March",
  "monthname_short":"Mar",
  "weekday_short":"Sun",
  "weekday":"Sunday",
  "ampm":"PM",
  "tz_short":"GMT",
  "tz_long":"Europe/London"
},
    "period":2,
    "high": {
    "fahrenheit":"48",
    "celsius":"9"
    },
    "low": {
    "fahrenheit":"37",
    "celsius":"3"
    },
    "conditions":"Rain",
    "icon":"rain",
    "icon_url":"http://icons.wxug.com/i/c/k/rain.gif",
    "skyicon":"",
    "pop":90,
    "qpf_allday": {
    "in": 0.21,
    "mm": 5
    },
    "qpf_day": {
    "in": 0.21,
    "mm": 5
    },
    "qpf_night": {
    "in": 0.00,
    "mm": 0
    },
    "snow_allday": {
    "in": 0.0,
    "cm": 0.0
    },
    "snow_day": {
    "in": 0.0,
    "cm": 0.0
    },
    "snow_night": {
    "in": 0.0,
    "cm": 0.0
    },
    "maxwind": {
    "mph": 30,
    "kph": 48,
    "dir": "WSW",
    "degrees": 254
    },
    "avewind": {
    "mph": 22,
    "kph": 35,
    "dir": "WSW",
    "degrees": 254
    },
    "avehumidity": 81,
    "maxhumidity": 0,
    "minhumidity": 0
    }
    ,
    {"date":{
  "epoch":"1488826800",
  "pretty":"7:00 PM GMT on March 06, 2017",
  "day":6,
  "month":3,
  "year":2017,
  "yday":64,
  "hour":19,
  "min":"00",
  "sec":0,
  "isdst":"0",
  "monthname":"March",
  "monthname_short":"Mar",
  "weekday_short":"Mon",
  "weekday":"Monday",
  "ampm":"PM",
  "tz_short":"GMT",
  "tz_long":"Europe/London"
},
    "period":3,
    "high": {
    "fahrenheit":"50",
    "celsius":"10"
    },
    "low": {
    "fahrenheit":"36",
    "celsius":"2"
    },
    "conditions":"Mostly Cloudy",
    "icon":"mostlycloudy",
    "icon_url":"http://icons.wxug.com/i/c/k/mostlycloudy.gif",
    "skyicon":"",
    "pop":10,
    "qpf_allday": {
    "in": 0.00,
    "mm": 0
    },
    "qpf_day": {
    "in": 0.00,
    "mm": 0
    },
    "qpf_night": {
    "in": 0.00,
    "mm": 0
    },
    "snow_allday": {
    "in": 0.0,
    "cm": 0.0
    },
    "snow_day": {
    "in": 0.0,
    "cm": 0.0
    },
    "snow_night": {
    "in": 0.0,
    "cm": 0.0
    },
    "maxwind": {
    "mph": 20,
    "kph": 32,
    "dir": "WNW",
    "degrees": 300
    },
    "avewind": {
    "mph": 13,
    "kph": 21,
    "dir": "WNW",
    "degrees": 300
    },
    "avehumidity": 74,
    "maxhumidity": 0,
    "minhumidity": 0
    }
    ,
    {"date":{
  "epoch":"1488913200",
  "pretty":"7:00 PM GMT on March 07, 2017",
  "day":7,
  "month":3,
  "year":2017,
  "yday":65,
  "hour":19,
  "min":"00",
  "sec":0,
  "isdst":"0",
  "monthname":"March",
  "monthname_short":"Mar",
  "weekday_short":"Tue",
  "weekday":"Tuesday",
  "ampm":"PM",
  "tz_short":"GMT",
  "tz_long":"Europe/London"
},
    "period":4,
    "high": {
    "fahrenheit":"50",
    "celsius":"10"
    },
    "low": {
    "fahrenheit":"44",
    "celsius":"7"
    },
    "conditions":"Chance of Rain",
    "icon":"chancerain",
    "icon_url":"http://icons.wxug.com/i/c/k/chancerain.gif",
    "skyicon":"",
    "pop":60,
    "qpf_allday": {
    "in": 0.11,
    "mm": 3
    },
    "qpf_day": {
    "in": 0.06,
    "mm": 2
    },
    "qpf_night": {
    "in": 0.05,
    "mm": 1
    },
    "snow_allday": {
    "in": 0.0,
    "cm": 0.0
    },
    "snow_day": {
    "in": 0.0,
    "cm": 0.0
    },
    "snow_night": {
    "in": 0.0,
    "cm": 0.0
    },
    "maxwind": {
    "mph": 15,
    "kph": 24,
    "dir": "SW",
    "degrees": 230
    },
    "avewind": {
    "mph": 10,
    "kph": 16,
    "dir": "SW",
    "degrees": 230
    },
    "avehumidity": 77,
    "maxhumidity": 0,
    "minhumidity": 0
    }
    ]
    }
  }
}
*/
