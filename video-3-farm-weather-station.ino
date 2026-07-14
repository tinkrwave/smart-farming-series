// ============================================================
//  tinkrwave — Smart Farm Monitor v1.4 (Final Master)
//  DHT11 + Digital Rain Sensor + Dual-Color OLED + Status LEDs
//
//  PIN MAP (Optimized for your working brass setup):
//  DHT11 DATA  → GPIO4  (+ 10kΩ Pull-up to 5V)
//  Rain DO     → GPIO2  (Digital Rain Detect — LOW when wet)
//  OLED SDA    → GPIO8  (Your original working I2C line)
//  OLED SCL    → GPIO9  (Your original working I2C line)
//  RED LED     → GPIO1  (Alert: Rain blinking / High Temp Alert)
//  GREEN LED   → GPIO5  (Status: Farm Conditions Normal)
// ============================================================

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

// ── OLED Layout Configuration ─────────
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ── DHT22 Sensor Configuration ────────
#define DHT_PIN  4
#define DHT_TYPE DHT11 // if DHT22 change DHT11 to DHT22
DHT dht(DHT_PIN, DHT_TYPE);

// ── LED & Sensor Pin Layout ───────────
#define RAIN_DO   2   // Digital input only
#define LED_RED   0   // Left leaf LED
#define LED_GREEN 5   // Right leaf LED

// ── Farm Management Thresholds ────────
const float TEMP_HIGH = 35.0; // High temp limit for crops
const float TEMP_LOW  = 10.0; // Frost warning limit

// ── Global Variables ──────────────────
float temperature = 0;
float humidity    = 0;
bool  isRaining   = false;
bool  ledBlinkState = false; // Controls non-blocking LED flash cycles

// ══════════════════════════════════════
// SETUP FUNCTION
// ══════════════════════════════════════
void setup() {
  Serial.begin(115200);

  // Pin Direction Configuration
  pinMode(RAIN_DO, INPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  // Power-On Hardware Diagnostic Flash
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, HIGH);
  delay(400);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);

  // Initialize DHT22
  dht.begin();

  // Initialize I2C OLED using your working hardware lines
  Wire.begin(8, 9); // SDA=GPIO8, SCL=GPIO9
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("⚠️ OLED not found! Verify your I2C brass paths.");
    while (true); 
  }

  // Initial Startup UI Screen
  display.clearDisplay();
  display.setTextColor(WHITE);
  
  // YELLOW ZONE (Top 16 Pixels)
  display.setTextSize(1);
  display.setCursor(25, 2);
  display.println("MINI MAKR LAB");
  display.drawLine(0, 15, 127, 15, WHITE);

  // BLUE ZONE (Bottom 48 Pixels)
  display.setCursor(15, 28);
  display.println("SMART FARM SYSTEM");
  display.setCursor(30, 48);
  display.println("Initializing...");
  display.display();
  delay(2000);
}

// ══════════════════════════════════════
// MAIN PROGRAM LOOP
// ══════════════════════════════════════
void loop() {
  readSensors();
  processAlertLogic();
  updateOLED();
  
  // 500ms execution window creates a snappy, responsive LED blink rate 
  // without clogging processor resources.
  delay(500); 
}

// ══════════════════════════════════════
// DATA COLLECTION
// ══════════════════════════════════════
void readSensors() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  // Only update global variables if data reading is valid
  if (!isnan(t) && !isnan(h)) {
    temperature = t;
    humidity    = h;
  }

  // Read Digital input. Most modules output LOW when water bridges the traces.
  isRaining = (digitalRead(RAIN_DO) == LOW); 
}

// ══════════════════════════════════════
// FARM ALERT SYSTEM LOGIC
// ══════════════════════════════════════
void processAlertLogic() {
  // Flip the blinking boolean state helper variable on every loop pass
  ledBlinkState = !ledBlinkState;

  // PRIORITY 1: Rain Event (Blink both LEDs out-of-phase)
  if (isRaining) {
    digitalWrite(LED_RED, ledBlinkState ? HIGH : LOW);
    digitalWrite(LED_GREEN, ledBlinkState ? LOW : HIGH);
  }
  
  // PRIORITY 2: Temperature Limits (When weather is dry)
  else {
    if (temperature >= TEMP_HIGH || temperature <= TEMP_LOW) {
      // Critical Thermal Threshold Breached: Steady Red, Green Killed
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_GREEN, LOW);
    } 
    else {
      // All Variables Normal: Steady Green Comfort Light, Red Killed
      digitalWrite(LED_GREEN, HIGH);
      digitalWrite(LED_RED, LOW);
    }
  }
}

// ══════════════════════════════════════
// EXECUTING DUAL-ZONE DISPLAY GRAPHICS
// ══════════════════════════════════════
void updateOLED() {
  display.clearDisplay();
  display.setTextColor(WHITE);

  // ── YELLOW ZONE LAYOUT (System Status Bar) ──
  display.setTextSize(1);
  display.setCursor(4, 2);
  display.print("FARM MONITOR");
  
  display.setCursor(82, 2);
  if (isRaining) {
    display.print("[RAIN]");
  } else if (temperature >= TEMP_HIGH) {
    display.print("[HEAT]");
  } else if (temperature <= TEMP_LOW) {
    display.print("[COLD]");
  } else {
    display.print("[OK]");
  }
  display.drawLine(0, 14, 127, 14, WHITE);

  // ── BLUE ZONE LAYOUT (Environmental Metrics Data) ──
  // Display Temperature
  display.setCursor(0, 20);
  display.print("Temp: ");
  display.print(temperature, 1);
  display.print(" C");

  // Display Humidity
  display.setCursor(0, 34);
  display.print("Humi: ");
  display.print(humidity, 1);
  display.print(" %");

  // Display Clean Digital Status (No fake frozen percentages)
  display.setCursor(0, 48);
  display.print("Status: ");
  display.print(isRaining ? "RAINING!" : "DRY / NORMAL");

  display.display();
}
