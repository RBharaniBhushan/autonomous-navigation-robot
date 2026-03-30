/*
 * PROJECT: Continuous Flow Phytopathology System (Hydraulic Fix)
 *
 * LOGIC FEATURES:
 * 1. Robust SMS parsing using indexOf()
 * 2. Soft shutdown to avoid hydraulic pressure shock
 * 3. Continuous water flow throughout treatment cycle
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

/* -------------------- HARDWARE PIN MAPPING -------------------- */

// GSM Module
#define GSM_RX_PIN 2
#define GSM_TX_PIN 3

// Relay Module (ACTIVE LOW)
#define RELAY_WATER_MAIN 4     // Main Water Pump
#define RELAY_VALVE_P1   5     // Plant 1 Valve
#define RELAY_VALVE_P2   6     // Plant 2 Valve
#define RELAY_VALVE_P3   7     // Plant 3 Valve
#define RELAY_MED_A      8     // Anthracnose Pump
#define RELAY_MED_B      9     // Cactus Virus X Pump
#define RELAY_MED_C      10    // Stem Canker Pump

/* -------------------- CONFIGURATION -------------------- */

const int MIN_ACCURACY = 75;
const int LCD_ADDRESS  = 0x27;

// Timings (milliseconds)
const unsigned long TIME_PREWET   = 3000;
const unsigned long TIME_DOSE     = 4000;
const unsigned long TIME_FLUSH    = 5000;
const unsigned long TIME_SPINDOWN = 2000;

/* -------------------- OBJECTS -------------------- */

SoftwareSerial gsm(GSM_RX_PIN, GSM_TX_PIN);
LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2);

/* -------------------- GLOBAL VARIABLES -------------------- */

String gsmBuffer = "";
bool systemBusy = false;

/* -------------------- SETUP -------------------- */

void setup() {
  Serial.begin(9600);
  gsm.begin(9600);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("SYSTEM BOOTING");

  // Initialize all relays to OFF (ACTIVE LOW)
  for (int pin = 4; pin <= 10; pin++) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
  }

  performSelfTest();

  gsm.println("AT+CMGF=1");          // SMS text mode
  delay(200);
  gsm.println("AT+CNMI=1,2,0,0,0");  // Direct SMS to serial

  updateDisplay("STATUS: ONLINE", "WAITING FOR SMS");
}

/* -------------------- LOOP -------------------- */

void loop() {
  if (systemBusy) return;

  while (gsm.available()) {
    char c = gsm.read();

    if (c == '\n') {
      if (gsmBuffer.length() > 5) {
        processIncomingLine(gsmBuffer);
      }
      gsmBuffer = "";
    }
    else if (c != '\r') {
      gsmBuffer += c;
    }
  }
}

/* -------------------- SMS PROCESSING -------------------- */

void processIncomingLine(String line) {
  line.trim();

  if (line.indexOf("PLANT") == -1 || line.indexOf(":") == -1) return;

  Serial.println("CMD: " + line);

  int colonIndex = line.indexOf(':');
  int openParen  = line.indexOf('(');
  int pctIndex   = line.indexOf('%');

  if (colonIndex == -1 || openParen == -1 || pctIndex == -1) return;

  // --- PARSING ---
  String plantID = line.substring(0, colonIndex);
  plantID.toUpperCase();

  String disease = line.substring(colonIndex + 1, openParen);
  disease.trim();
  disease.toLowerCase();

  int accuracy = line.substring(openParen + 1, pctIndex).toInt();

  // --- SAFETY CHECKS ---
  if (accuracy < MIN_ACCURACY) {
    updateDisplay("LOW ACCURACY", String(accuracy) + "% ABORT");
    delay(3000);
    updateDisplay("STATUS: ONLINE", "WAITING...");
    return;
  }

  if (disease == "healthy") {
    updateDisplay("PLANT HEALTHY", "NO ACTION");
    delay(3000);
    updateDisplay("STATUS: ONLINE", "WAITING...");
    return;
  }

  // --- VALVE SELECTION (ROBUST FIX) ---
  int targetValve = -1;
  int targetPump  = -1;

  if      (plantID.indexOf("1") >= 0) targetValve = RELAY_VALVE_P1;
  else if (plantID.indexOf("2") >= 0) targetValve = RELAY_VALVE_P2;
  else if (plantID.indexOf("3") >= 0) targetValve = RELAY_VALVE_P3;
  else {
    updateDisplay("ERROR", "INVALID PLANT");
    delay(2000);
    return;
  }

  // --- MEDICINE SELECTION ---
  if      (disease == "anthracnose")    targetPump = RELAY_MED_A;
  else if (disease == "cactusvirusx")   targetPump = RELAY_MED_B;
  else if (disease == "stemcanker")     targetPump = RELAY_MED_C;
  else {
    updateDisplay("ERROR", "BAD DISEASE");
    delay(2000);
    return;
  }

  // --- EXECUTION ---
  systemBusy = true;
  runContinuousTreatment(plantID, disease, targetValve, targetPump);
  systemBusy = false;
}

/* -------------------- TREATMENT LOGIC -------------------- */

void runContinuousTreatment(String pID, String dName, int valvePin, int medPin) {

  // Start water & open valve
  updateDisplay("STARTING WATER", pID);
  digitalWrite(valvePin, LOW);
  digitalWrite(RELAY_WATER_MAIN, LOW);

  delay(TIME_PREWET);

  // Inject medicine
  updateDisplay("INJECTING", dName);
  digitalWrite(medPin, LOW);
  delay(TIME_DOSE);
  digitalWrite(medPin, HIGH);

  // Flush
  updateDisplay("FLUSHING LINES", "WATER FLOW");
  delay(TIME_FLUSH);

  // Soft shutdown
  updateDisplay("STOPPING PUMP", "VALVE OPEN");
  digitalWrite(RELAY_WATER_MAIN, HIGH);
  delay(TIME_SPINDOWN);

  digitalWrite(valvePin, HIGH);

  updateDisplay("CYCLE COMPLETE", "SYSTEM IDLE");
  delay(2000);
  updateDisplay("STATUS: ONLINE", "WAITING...");
}

/* -------------------- UTILITIES -------------------- */

void performSelfTest() {
  lcd.setCursor(0, 1);
  lcd.print("Self-Test...");

  for (int pin = 4; pin <= 10; pin++) {
    digitalWrite(pin, LOW);  delay(100);
    digitalWrite(pin, HIGH); delay(50);
  }

  lcd.clear();
}

void updateDisplay(String line1, String line2) {
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print(line1.substring(0, 16));

  lcd.setCursor(0, 1);
  lcd.print(line2.substring(0, 16));
}
