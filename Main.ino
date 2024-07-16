#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <FS.h>
#include <EEPROM.h>

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
    
#define CLK_PIN   D5
#define DATA_PIN  D7
#define CS_PIN    D0

AsyncWebServer server(80);
const char* ssid = "RobotikID";
const char* password = "123456789";
String param_pesan[] = {
  "pesan_pertama",
  "pesan_kedua",
  "pesan_ketiga"
};
String param_efek_in[] = {
  "efek_in_pertama",
  "efek_in_kedua",
  "efek_in_ketiga"
};
String param_efek_out[] = {
  "efek_out_pertama",
  "efek_out_kedua",
  "efek_out_ketiga"
};

MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
String pesan[3], tmp_pesan;
uint8_t cnt;
textEffect_t efek_awal[3], efek_akhir[3];
textEffect_t database_efek[] = 
{       
  PA_SCROLL_UP,
  PA_SCROLL_DOWN,
  PA_SCROLL_LEFT,
  PA_SCROLL_RIGHT,
  PA_OPENING_CURSOR,
  PA_CLOSING_CURSOR, 
};
File file;
String nama_file[] = {
  "/text_pertama.txt",
  "/text_kedua.txt",
  "/text_ketiga.txt",
};

void simpan_pesan(String nama_file, String isi_file){
  file = SPIFFS.open(nama_file, "w");
  if(file.println(isi_file)){
    Serial.println("Berhasil Menyimpan Pesan");
  }
  file.close();
}

String baca_pesan(String nama_file){
  String isi_file;
  file = SPIFFS.open(nama_file, "r");
  if(!file){
    file.close();
    file = SPIFFS.open(nama_file, "w");
    Serial.println("File Tidak Ditemukan");
    isi_file = "Hai RobotikID";
    if(file.println(isi_file)){
      Serial.println("Berhasil Membuat File");
    }
    file.close();
  }
  else{
    Serial.println("File Ditemukan");
    while (file.position()<file.size()){
      isi_file = file.readStringUntil('\n');
      isi_file.trim();
      Serial.println(isi_file);
    }
    file.close();
  }
  return isi_file;
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(10);
  if(!SPIFFS.begin()){
      Serial.println("Gagal Memulai SPIFFS");
      return;
  }
  pesan[0] = baca_pesan(nama_file[0]);
  pesan[1] = baca_pesan(nama_file[1]);
  pesan[2] = baca_pesan(nama_file[2]);
  efek_awal[0] = database_efek[EEPROM.read(0)];
  efek_awal[1] = database_efek[EEPROM.read(1)];
  efek_awal[2] = database_efek[EEPROM.read(2)];
  efek_akhir[0] = database_efek[EEPROM.read(3)];
  efek_akhir[1] = database_efek[EEPROM.read(4)];
  efek_akhir[2] = database_efek[EEPROM.read(5)];

  WiFi.softAP(ssid, password);
  IPAddress ip_esp = WiFi.softAPIP();
  Serial.print("IP address: ");
  Serial.println(ip_esp);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS,  "/config.html", "text/html");
  });
  server.serveStatic("/", SPIFFS, "/");
  server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request) {
    for(int a=0; a<3; a++){
      if (request->hasParam(param_pesan[a].c_str())) {
        tmp_pesan = request->getParam(param_pesan[a].c_str())->value();
        pesan[a] = tmp_pesan;
        simpan_pesan(nama_file[a], tmp_pesan);
        Serial.print("Pesan: ");
        Serial.println(tmp_pesan);
      }

      if (request->hasParam(param_efek_in[a].c_str())) {
        tmp_pesan = request->getParam(param_efek_in[a].c_str())->value();
        efek_awal[a] = database_efek[tmp_pesan.toInt()];
        EEPROM.write(a, tmp_pesan.toInt());
        EEPROM.commit();
      }

      if (request->hasParam(param_efek_out[a].c_str())) {
        tmp_pesan = request->getParam(param_efek_out[a].c_str())->value();
        efek_akhir[a] = database_efek[tmp_pesan.toInt()];
        EEPROM.write(a+3, tmp_pesan.toInt());
        EEPROM.commit();
      }
    }
    request->send(SPIFFS,  "/config.html", "text/html");
  });
  
  server.begin();
  
  P.begin();
  P.displayClear();
  P.displayScroll(pesan[cnt].c_str(), PA_CENTER, efek_awal[cnt], 100);
  cnt++;
}

void loop() {
  if(P.displayAnimate()){
    P.setTextBuffer(pesan[cnt].c_str());
    P.setTextEffect(efek_awal[cnt], efek_akhir[cnt]);

    cnt++;
    if(cnt == 3){
      cnt = 0;
    }
    P.displayReset();    
  }
}
