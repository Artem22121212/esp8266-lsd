# ESP8266 Smart LCD

A flexible portable device based on ESP8266 that allows quick display of any text on a 1602 LCD screen using a button and web interface.

**Русский** | [English](#english)

---

### Возможности

- Вывод текста на LCD 1602
- Гибкая система действий кнопки (одиночное, двойное, тройное нажатие, длинное удержание)
- Полноценный веб-интерфейс для управления
- Сохранение настроек в EEPROM
- Режим точки доступа (AP) и подключение к Wi-Fi
- Регулировка контраста

### Схема подключения

- LCD 1602 (I2C) → стандартное подключение (SDA=D2, SCL=D1)
- Контраст (Vo) → GPIO0 (D3) через резистор
- Кнопка → GPIO2 (D4) + GND

### Как использовать

1. Загрузите скетч на ESP8266
2. Подключитесь к Wi-Fi сети `SmartLCD` (пароль `admin12345`)
3. Откройте в браузере `192.168.4.1`
4. Войдите под паролем `admin123`
5. Настраивайте текст и действия кнопки

---

### English

A flexible portable device based on ESP8266 that allows quick display of any text on a 1602 LCD screen using a button and web interface.

### Features

- Text output on 1602 LCD
- Flexible button action system (single, double, triple click, long press)
- Full web interface for control
- Settings saved in EEPROM
- Access Point + Station mode
- Contrast adjustment

### Hardware

- LCD 1602 with I2C
- Button on GPIO2 (D4)
- Contrast control on GPIO0 (D3)

### How to use

1. Upload the sketch to ESP8266
2. Connect to Wi-Fi `SmartLCD` (password `admin12345`)
3. Open `192.168.4.1` in browser
4. Login with password `admin123`
5. Configure text and button actions
