#include <WiFi.h>
#include <WebServer.h>

//*Bu kod her iki motorun aynı anda çalışmasını sağlar potansiyometre değeri azaldıkça
//Motor 1 yavaşlar Motor 2 hızlanır Motor 1 de etiket biter ise potansiyometre değeri
//Belirlenen eşik değernin atına düşertüğünde motor 1 kendini kapatır motor 2 de kapanıyor fakat
//Kalan etiketlerin Sensörün  geçebilmesi için Motor 2 2 saniye daha çalışır kendini kapatır.
//İsteğe göre potansiyometre değeri web arayüzden ayarlanma revizesi yapılabilri.Arayüzde Sayaç
//Sensör hızı ve Motor Star-Stop Butonları var
//Yapılması gerek ilk yapıldıgı zamanki pinlere göre güncelleştirilmeler yapılmasıdır. */

const char* ssid = "SSID";
const char* password = "Password";

WebServer server(80);

uint8_t mac[6] = {0x93, 0x29, 0x1B, 0xBF, 0x61, 0x5D};

// Motor sürücü pinleri
#define PUL_PIN_1 18 // Motor 1 için start.İlk koddaki pinlere gçre güncellenmeli
#define DIR_PIN_1 19 // Motor 1 için yön.İlk koddaki pinlere gçre güncellenmeli
#define ENA_PIN_1 21 // Musait pın ***İnternette ayırılması özenlikle istenmiş o yüzden eklendim***.İlk koddaki pinlere gçre güncellenmeli

#define PUL_PIN_2 22 // Motor 2 için start.İlk koddaki pinlere gçre güncellenmeli
#define DIR_PIN_2 23 // Motor 2 için yön.İlk koddaki pinlere gçre güncellenmeli
#define ENA_PIN_2 25 // Musait pın ***İnternette ayırılması özenlikle istenmiş o yüzden eklendim***.İlk koddaki pinlere gçre güncellenmeli

const int sensorPin = 32; //***********Değişecek sensor pin girişi 19 Esp32 denemek için 32 numaralı pinde.************/
const int potPin = A0; // Potansiyometre pin.İlk koddaki pinlere gçre güncellenmeli

int counter = 0;
bool gndConnected = false;
bool motor1Running = false;
bool motor2Running = false;

unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50; // Başlangıç değeri, potansiyometreye göre güncellenecek
unsigned long motorRunTime = 1000; // Başlangıç değeri, potansiyometreye göre güncellenecek

unsigned long motor1StartTime = 0;
unsigned long motor2StartTime = 0;

// <fonc>
void handleRoot();
void handleCounter();
void handleReset();
void handleSettings();
void handleMotorStart();
void handleMotorStop();

void handleRoot() {
  String html = "<html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; text-align: center; }";
  html += "h1 { color: #333; }";
  html += "#counter { font-size: 288px; margin: 20px 0; color: #f39c12; }";
  html += "button { padding: 10px 20px; font-size: 20px; background-color: #3498db; color: white; border: none; cursor: pointer; }";
  html += "button:hover { background-color: #2980b9; }";
  html += "</style>";
  html += "<script>";
  html += "function resetCounter() {";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.open('POST', '/reset', true);";
  html += "  xhr.onload = function() {";
  html += "    if (xhr.status == 200) {";
  html += "      updateCounter();";
  html += "    }";
  html += "  };";
  html += "  xhr.send();";
  html += "}";
  html += "function updateCounter() {";
  html += "  var xhrUpdate = new XMLHttpRequest();";
  html += "  xhrUpdate.onreadystatechange = function() {";
  html += "    if (xhrUpdate.readyState == 4 && xhrUpdate.status == 200) {";
  html += "      document.getElementById('counter').innerText = xhrUpdate.responseText;";
  html += "    }";
  html += "  };";
  html += "  xhrUpdate.open('GET', '/counter', true);";
  html += "  xhrUpdate.send();";
  html += "}";
  html += "function applySettings() {";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.open('POST', '/settings', true);";
  html += "  xhr.onload = function() {";
  html += "    if (xhr.status == 200) {";
  html += "      alert('Ayarlar uygulandı. Yeniden başlatmak için sayfayı yenileyin.');";
  html += "    }";
  html += "  };";
  html += "  xhr.send();";
  html += "}";
  html += "function startMotor() {";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.open('POST', '/motor/start', true);";
  html += "  xhr.send();";
  html += "}";
  html += "function stopMotor() {";
  html += "  var xhr = new XMLHttpRequest();";
  html += "  xhr.open('POST', '/motor/stop', true);";
  html += "  xhr.send();";
  html += "}";
  html += "setInterval(updateCounter, 50);"; // Her 50 msde sayaç güncellenir
  html += "</script>";
  html += "</head><body>";
  html += "<h1>Sayım Kontrol</h1>";
  html += "<h2 id='counter'>" + String(counter) + "</h2>";
  html += "<button onclick='resetCounter()'>Sıfırla</button>";

  // Sensör algılama hızı ayarı
  html += "<h2>Sensör Algılama Hızı Ayarla</h2>";
  html += "<label for='debounceDelay'>Debounce Gecikme (ms):</label>";
  html += "<input type='number' id='debounceDelay' value='" + String(debounceDelay) + "' min='1' max='1000'>";
  html += "<button onclick='applySettings()'>Ayarları Uygula</button>";

  // Motor kontrol butonları
  html += "<h2>Motor Kontrol</h2>";
  html += "<button onclick='startMotor()'>Motoru Başlat</button>";
  html += "<button onclick='stopMotor()'>Motoru Durdur</button>";

  // Bağlantı bilgileri
  html += "<h2>Bağlantı Bilgileri</h2>";
  html += "<p>SSID: " + String(WiFi.SSID()) + "</p>";
  html += "<p>IP Adresi: " + WiFi.localIP().toString() + "</p>";

  html += "</body></html>";

  server.send(200, "text/html", html);
}

void handleCounter() {
  server.send(200, "text/plain", String(counter));
}

void handleReset() {
  counter = 0;
  server.send(200, "text/plain", "Sayaç sıfırlandı ve varsayılan ayarlar uygulandı");
  // Resetten sonra ana sayfaya yönlendirme
  server.sendHeader("Location", "/", true);
  server.send(303);
}

void handleSettings() {
  String response = "{";
  response += "\"debounceDelay\": " + String(debounceDelay);
  response += "}";
  server.send(200, "application/json", response);
}

void handleMotorStart() {
  motor1Running = true;
  motor2Running = true;
  motor1StartTime = millis();
  motor2StartTime = millis();
  server.send(200, "text/plain", "Motorlar başlatıldı");
}

void handleMotorStop() {
  motor1Running = false;
  motor2Running = false;
  server.send(200, "text/plain", "Motorlar durduruldu");
}

void setup() {
  Serial.begin(115200);  // Seri haberleşme hızı
  delay(1000);

  WiFi.begin(ssid, password);
  Serial.print("Bağlanılıyor");
  
  // wifi bağlantısı
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.print("Bağlandı: ");
  Serial.println(WiFi.localIP());

  esp_base_mac_addr_set(mac);

  pinMode(sensorPin, INPUT_PULLUP);
  pinMode(potPin, INPUT);
  pinMode(PUL_PIN_1, OUTPUT);
  pinMode(DIR_PIN_1, OUTPUT);
  pinMode(ENA_PIN_1, OUTPUT);
  pinMode(PUL_PIN_2, OUTPUT);
  pinMode(DIR_PIN_2, OUTPUT);
  pinMode(ENA_PIN_2, OUTPUT);

  // Sayaç sıfırlama
  counter = 0;

  // Web sunucu
  server.on("/", HTTP_GET, handleRoot);
  server.on("/reset", HTTP_POST, handleReset);
  server.on("/counter", HTTP_GET, handleCounter);
  server.on("/settings", HTTP_GET, handleSettings);
  server.on("/motor/start", HTTP_POST, handleMotorStart);
  server.on("/motor/stop", HTTP_POST, handleMotorStop);

  server.begin();
  Serial.println("HTTP sunucu başlatıldı");
}

void loop() {
  server.handleClient();

  int sensorValue = digitalRead(sensorPin);
  int potValue = analogRead(potPin); // Potansiyometre değerini oku
  debounceDelay = map(potValue, 0, 1023, 1, 1000); // Potansiyometre değerine göre debounce gecikmesini ayarla
  motorRunTime = map(potValue, 0, 1023, 1000, 10000); // Motor çalışma süresi 1 ile 10 saniye arasında

  unsigned long currentTime = millis();

  if (sensorValue == LOW && !gndConnected && (currentTime - lastDebounceTime) > debounceDelay) {
    counter++;
    gndConnected = true;
    lastDebounceTime = currentTime;
  } else if (sensorValue == HIGH) {
    gndConnected = false;
  }

  // Motor 1'i kontrol et
  if (motor1Running) {
    digitalWrite(ENA_PIN_1, LOW); // Sürücü çalışır
    digitalWrite(DIR_PIN_1, LOW); // LOW atarak yön sola ayarlanır.
    digitalWrite(PUL_PIN_1, HIGH);
    delayMicroseconds(500); // Girdi bekleme s.
    digitalWrite(PUL_PIN_1, LOW);
    delayMicroseconds(500); // Girdi bekleme s.

    // Motor 1 çalışma süresi dolduğunda durdur
    if (millis() - motor1StartTime >= motorRunTime) {
      motor1Running = false;
      motor2StartTime = millis(); // Motor 2'nin ek süreyle çalışmaya devam etmesi için başlangıç zamanını ayarla
    }
  } else {
    digitalWrite(ENA_PIN_1, HIGH); // Driver kapatılıyor
  }

  // Motor 2'yi kontrol et
  if (motor2Running) {
    if (millis() - motor2StartTime >= motorRunTime + 2000) { // Motor 2, Motor 1 durduktan sonra 2 saniye daha çalışır
      motor2Running = false;
    } else {
      digitalWrite(ENA_PIN_2, LOW); // Sürücü çalışır
      digitalWrite(DIR_PIN_2, LOW); //  LOW atarak yön sola ayarlanır.
      digitalWrite(PUL_PIN_2, HIGH);
      delayMicroseconds(500); // Girdi bekleme s.
      digitalWrite(PUL_PIN_2, LOW);
      delayMicroseconds(500); // Girdi bekleme s.
    }
  } else {
    digitalWrite(ENA_PIN_2, HIGH); // Driver kapatılıyor
  }
}
