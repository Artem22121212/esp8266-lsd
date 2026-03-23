#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Настройки для дисплея LCD 1602 с I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Пин для управления контрастностью (V0)
#define CONTRAST_PIN 0  // D3 = GPIO0

// Структура для хранения настроек в EEPROM
struct Settings {
  char ssid[32];
  char password[32];
  int contrast;        // Контрастность (0-255)
  bool configured;
  char lastText[64];
};

Settings settings;
ESP8266WebServer server(80);

// Переменные
String currentText = "IP: ";
int currentContrast = 128;  // Среднее значение
bool apMode = false;
unsigned long lastDisplayUpdate = 0;

// ПРОТОТИПЫ ФУНКЦИЙ
void loadSettings();
void saveSettings();
void initDisplay();
void setContrast(int contrast);
void updateDisplay();
void setupAP();
void setupStation();
void handleRoot();
void handleSetText();
void handleGetText();
void handleSetContrast();
void handleGetSettings();
void handleConnectWiFi();
void handleResetWiFi();
void handleFactoryReset();
void handleRestart();
void handleStatus();

// HTML страница
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial; text-align: center; margin-top: 50px; background: #f0f0f0; }
        .container { max-width: 500px; margin: auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        input, select { width: 90%; padding: 10px; margin: 10px 0; border: 1px solid #ddd; border-radius: 5px; }
        button { background: #4CAF50; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; margin: 5px; }
        button:hover { background: #45a049; }
        button.danger { background: #f44336; }
        button.danger:hover { background: #da190b; }
        .error { color: red; font-size: 12px; margin-top: -5px; margin-bottom: 10px; }
        .success { color: green; }
        .slider-container { margin: 20px 0; }
        .slider { width: 100%; }
        .slider-value { display: inline-block; margin-left: 10px; font-weight: bold; }
        .status { padding: 10px; margin: 10px 0; border-radius: 5px; }
        .status.online { background: #d4edda; color: #155724; }
        .status.ap { background: #fff3cd; color: #856404; }
        .current-text { background: #e9ecef; padding: 15px; border-radius: 5px; margin: 15px 0; font-size: 18px; word-break: break-all; }
        .control-group { border: 1px solid #ddd; border-radius: 5px; padding: 15px; margin: 15px 0; }
        .control-group h3 { margin-top: 0; color: #333; }
        .contrast-demo { display: flex; align-items: center; justify-content: space-between; margin: 10px 0; }
        .contrast-box { width: 50px; height: 30px; background: #333; color: white; display: flex; align-items: center; justify-content: center; font-size: 12px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>ESP8266 LCD 1602 Control</h1>
        <div id="status" class="status"></div>
        
        <div class="current-text">
            <strong>Current Text:</strong><br>
            <span id="currentText">Loading...</span>
        </div>
        
        <div class="control-group">
            <h3>WiFi Settings</h3>
            <form id="wifiForm">
                <input type="text" id="ssid" placeholder="WiFi SSID" required>
                <input type="password" id="password" placeholder="WiFi Password">
                <button type="submit">Connect to WiFi</button>
            </form>
            <div id="wifiMessage"></div>
        </div>
        
        <div class="control-group">
            <h3>Display Text</h3>
            <form id="textForm">
                <input type="text" id="displayText" placeholder="Enter text (English only)" maxlength="32">
                <div id="textError" class="error"></div>
                <button type="submit">Update Display</button>
            </form>
        </div>
        
        <div class="control-group">
            <h3>Display Contrast Control</h3>
            <p>Replaces broken blue potentiometer<br>Connected to V0 pin via D3 (GPIO0)</p>
            <div class="slider-container">
                <input type="range" id="contrast" class="slider" min="0" max="255" value="128">
                <span id="contrastValue" class="slider-value">128</span>
            </div>
            <div class="contrast-demo">
                <span>Low Contrast</span>
                <div class="contrast-box" id="contrastDemo">ABC</div>
                <span>High Contrast</span>
            </div>
            <button onclick="setContrast()">Apply Contrast</button>
            <button onclick="autoCalibrate()">Auto Calibrate</button>
        </div>
        
        <div class="control-group">
            <h3>System</h3>
            <button onclick="restartESP()">Restart ESP</button>
            <button class="danger" onclick="resetWiFi()">Reset WiFi Settings</button>
            <button class="danger" onclick="factoryReset()">Factory Reset</button>
        </div>
    </div>
    
    <script>
        function validateEnglish(text) {
            const englishRegex = /^[a-zA-Z0-9\s!@#$%^&*()_+\-=[\]{};':"\\|,.<>/?]*$/;
            return englishRegex.test(text);
        }
        
        document.getElementById('textForm').addEventListener('submit', function(e) {
            e.preventDefault();
            let text = document.getElementById('displayText').value;
            
            if (!validateEnglish(text)) {
                document.getElementById('textError').innerHTML = 'Error: Please use only English characters!';
                return false;
            }
            
            if (text.length > 32) {
                document.getElementById('textError').innerHTML = 'Error: Maximum 32 characters!';
                return false;
            }
            
            document.getElementById('textError').innerHTML = '';
            
            fetch('/settext', {
                method: 'POST',
                headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                body: 'text=' + encodeURIComponent(text)
            })
            .then(response => response.text())
            .then(data => {
                alert(data);
                updateCurrentText();
            });
        });
        
        document.getElementById('wifiForm').addEventListener('submit', function(e) {
            e.preventDefault();
            let ssid = document.getElementById('ssid').value;
            let password = document.getElementById('password').value;
            
            fetch('/connectwifi', {
                method: 'POST',
                headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                body: 'ssid=' + encodeURIComponent(ssid) + '&password=' + encodeURIComponent(password)
            })
            .then(response => response.text())
            .then(data => {
                document.getElementById('wifiMessage').innerHTML = data;
                if(data.includes('Saved')) {
                    document.getElementById('wifiMessage').style.color = 'green';
                    setTimeout(() => { location.reload(); }, 3000);
                }
            });
        });
        
        function setContrast() {
            let contrast = document.getElementById('contrast').value;
            fetch('/setcontrast', {
                method: 'POST',
                headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                body: 'contrast=' + contrast
            })
            .then(response => response.text())
            .then(data => {
                alert(data);
                updateContrastDemo(contrast);
            });
        }
        
        function autoCalibrate() {
            let contrast = 128;
            document.getElementById('contrast').value = contrast;
            document.getElementById('contrastValue').innerHTML = contrast;
            setContrast();
        }
        
        function updateContrastDemo(value) {
            let box = document.getElementById('contrastDemo');
            let intensity = Math.floor(value / 2.55);
            box.style.backgroundColor = `rgb(${intensity}, ${intensity}, ${intensity})`;
        }
        
        function restartESP() {
            if(confirm('Restart ESP?')) {
                fetch('/restart');
                setTimeout(() => { alert('Restarting...'); }, 100);
            }
        }
        
        function resetWiFi() {
            if(confirm('Reset WiFi settings only?')) {
                fetch('/resetwifi');
                setTimeout(() => { location.reload(); }, 1000);
            }
        }
        
        function factoryReset() {
            if(confirm('FACTORY RESET: This will erase all settings. Continue?')) {
                fetch('/factoryreset');
                setTimeout(() => { alert('Factory reset, restarting...'); }, 100);
            }
        }
        
        function updateCurrentText() {
            fetch('/gettext')
            .then(response => response.text())
            .then(text => {
                document.getElementById('currentText').innerHTML = text;
            });
        }
        
        document.getElementById('contrast').addEventListener('input', function() {
            document.getElementById('contrastValue').innerHTML = this.value;
            updateContrastDemo(this.value);
        });
        
        function updateStatus() {
            fetch('/status')
            .then(response => response.json())
            .then(data => {
                let statusDiv = document.getElementById('status');
                if(data.mode === 'ap') {
                    statusDiv.className = 'status ap';
                    statusDiv.innerHTML = 'Mode: Access Point<br>IP: ' + data.ip + '<br>Connect to WiFi to use network mode';
                } else {
                    statusDiv.className = 'status online';
                    statusDiv.innerHTML = 'Mode: WiFi Station<br>IP: ' + data.ip + '<br>Connected to: ' + data.ssid;
                }
            });
        }
        
        function loadSettings() {
            fetch('/getsettings')
            .then(response => response.json())
            .then(data => {
                document.getElementById('contrast').value = data.contrast;
                document.getElementById('contrastValue').innerHTML = data.contrast;
                updateContrastDemo(data.contrast);
            });
        }
        
        updateStatus();
        updateCurrentText();
        loadSettings();
        setInterval(updateStatus, 5000);
        setInterval(updateCurrentText, 2000);
    </script>
</body>
</html>
)rawliteral";

// РЕАЛИЗАЦИЯ ФУНКЦИЙ
void loadSettings() {
  EEPROM.begin(512);
  EEPROM.get(0, settings);
  
  if (settings.configured != 0xAA) {
    memset(settings.ssid, 0, 32);
    memset(settings.password, 0, 32);
    settings.contrast = 128;
    settings.configured = 0xAA;
    strcpy(settings.lastText, "Hello!");
    saveSettings();
  }
  
  currentContrast = settings.contrast;
  currentText = String(settings.lastText);
}

void saveSettings() {
  EEPROM.put(0, settings);
  EEPROM.commit();
}

void initDisplay() {
  // Настраиваем пин для управления контрастностью
  pinMode(CONTRAST_PIN, OUTPUT);
  
  // Инициализируем I2C для дисплея
  Wire.begin(4, 5); // SDA = D2 (GPIO4), SCL = D1 (GPIO5)
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ESP8266 LCD");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  
  // Устанавливаем контрастность
  setContrast(currentContrast);
  delay(2000);
}

void setContrast(int contrast) {
  // Контрастность управляется через PWM на пине CONTRAST_PIN
  // Чем выше напряжение на V0, тем меньше контраст
  // Поэтому инвертируем значение: 255 = min контраст, 0 = max контраст
  int pwmValue = 255 - constrain(contrast, 0, 255);
  analogWrite(CONTRAST_PIN, pwmValue);
  
  currentContrast = contrast;
  settings.contrast = contrast;
  saveSettings();
  
  Serial.print("Contrast set to: ");
  Serial.print(contrast);
  Serial.print(" (PWM: ");
  Serial.print(pwmValue);
  Serial.println(")");
}

void updateDisplay() {
  lcd.clear();
  lcd.setCursor(0, 0);
  
  String line1 = currentText;
  String line2 = "";
  
  if (currentText.length() > 16) {
    line1 = currentText.substring(0, 16);
    line2 = currentText.substring(16, min(32, (int)currentText.length()));
  }
  
  lcd.print(line1);
  if (line2.length() > 0) {
    lcd.setCursor(0, 1);
    lcd.print(line2);
  }
}

void setupAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("i21602", "i123456i");
  
  IPAddress myIP = WiFi.softAPIP();
  currentText = "AP Mode IP:" + myIP.toString();
  updateDisplay();
  
  Serial.println("AP Started");
  Serial.print("IP: ");
  Serial.println(myIP);
  
  apMode = true;
}

void setupStation() {
  if (strlen(settings.ssid) > 0) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(settings.ssid, settings.password);
    
    int attempts = 0;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Connecting to");
    lcd.setCursor(0, 1);
    lcd.print(settings.ssid);
    
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      currentText = "IP:" + WiFi.localIP().toString();
      updateDisplay();
      apMode = false;
      Serial.println("\nConnected to WiFi");
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());
    } else {
      lcd.clear();
      lcd.print("WiFi Failed!");
      delay(2000);
      setupAP();
    }
  } else {
    setupAP();
  }
}

void handleRoot() {
  server.send(200, "text/html", index_html);
}

void handleSetText() {
  if (server.hasArg("text")) {
    String text = server.arg("text");
    
    bool hasRussian = false;
    for (int i = 0; i < text.length(); i++) {
      char c = text.charAt(i);
      if (c >= 0x80 && c <= 0xFF) {
        hasRussian = true;
        break;
      }
    }
    
    if (hasRussian) {
      server.send(400, "text/plain", "Error: Russian characters not allowed! Use English only.");
      return;
    }
    
    currentText = text;
    strcpy(settings.lastText, text.c_str());
    saveSettings();
    updateDisplay();
    server.send(200, "text/plain", "Text updated successfully!");
  } else {
    server.send(400, "text/plain", "No text provided");
  }
}

void handleGetText() {
  server.send(200, "text/plain", currentText);
}

void handleSetContrast() {
  if (server.hasArg("contrast")) {
    int contrast = server.arg("contrast").toInt();
    contrast = constrain(contrast, 0, 255);
    setContrast(contrast);
    server.send(200, "text/plain", "Contrast set to " + String(contrast));
  } else {
    server.send(400, "text/plain", "No contrast value");
  }
}

void handleGetSettings() {
  String json = "{\"contrast\":" + String(settings.contrast) + "}";
  server.send(200, "application/json", json);
}

void handleConnectWiFi() {
  if (server.hasArg("ssid")) {
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    
    strcpy(settings.ssid, ssid.c_str());
    strcpy(settings.password, password.c_str());
    saveSettings();
    
    server.send(200, "text/plain", "WiFi settings saved! Restarting...");
    delay(1000);
    ESP.restart();
  } else {
    server.send(400, "text/plain", "No SSID provided");
  }
}

void handleResetWiFi() {
  memset(settings.ssid, 0, 32);
  memset(settings.password, 0, 32);
  saveSettings();
  server.send(200, "text/plain", "WiFi settings reset! Restarting...");
  delay(1000);
  ESP.restart();
}

void handleFactoryReset() {
  settings.configured = 0;
  saveSettings();
  server.send(200, "text/plain", "Factory reset! Restarting...");
  delay(1000);
  ESP.restart();
}

void handleRestart() {
  server.send(200, "text/plain", "Restarting...");
  delay(100);
  ESP.restart();
}

void handleStatus() {
  String json = "{\"mode\":\"" + String(apMode ? "ap" : "sta") + "\",";
  json += "\"ip\":\"" + (apMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString()) + "\",";
  json += "\"ssid\":\"" + String(settings.ssid) + "\"}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n\nESP8266 LCD 1602 System Starting...");
  
  initDisplay();
  loadSettings();
  
  currentText = String(settings.lastText);
  
  if (settings.configured == 0xAA && strlen(settings.ssid) > 0) {
    setupStation();
  } else {
    setupAP();
  }
  
  server.on("/", handleRoot);
  server.on("/settext", handleSetText);
  server.on("/gettext", handleGetText);
  server.on("/setcontrast", handleSetContrast);
  server.on("/getsettings", handleGetSettings);
  server.on("/connectwifi", handleConnectWiFi);
  server.on("/resetwifi", handleResetWiFi);
  server.on("/factoryreset", handleFactoryReset);
  server.on("/restart", handleRestart);
  server.on("/status", handleStatus);
  
  server.begin();
  Serial.println("HTTP server started");
  
  updateDisplay();
}

void loop() {
  server.handleClient();
  
  if (WiFi.status() == WL_CONNECTED && !apMode && millis() - lastDisplayUpdate > 5000) {
    if (currentText.indexOf("IP:") == -1) {
      currentText = "IP:" + WiFi.localIP().toString();
      updateDisplay();
    }
    lastDisplayUpdate = millis();
  }
}