// =====================
// Agriculture Rover - Arduino Motor & Sensor Control
// =====================

// Motor A (Left)
const int ENA = 11; 
const int IN1 = 10; 
const int IN2 = 9;  

// Motor B (Right)
const int ENB = 6; 
const int IN3 = 8; 
const int IN4 = 7; 

// Tuning Parameters
#define SPEED_BASE 100   // Normal forward speed
#define SPEED_TURN 130   // Turning speed (usually needs to be higher to overcome friction)
#define NUDGE_TIME 300   // Time (ms) to move blindy forward to clear the stop line

// IR Sensors
const int IRSensorLeft  = 2; 
const int IRSensorRight = 3; 

// State Variables
int plantCount = 1;         
bool atPlant   = false;     

void setup() {
  // Motor Pins
  pinMode(ENA, OUTPUT); pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT); pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);

  // Sensor Pins
  pinMode(IRSensorLeft, INPUT);
  pinMode(IRSensorRight, INPUT);

  Serial.begin(9600);
  stopMotors();
  
  // Wait for Pi handshaking before starting
  waitForStartSignal(); 
  Serial.println("System Started.");
}

void loop() {
  // Read Sensors
  bool leftSensor  = digitalRead(IRSensorLeft);
  bool rightSensor = digitalRead(IRSensorRight);

  // LOGIC ASSUMPTION: 
  // LOW  = White Surface (Line not detected)
  // HIGH = Black Line (Line detected)
  
  bool bothHigh = (leftSensor == HIGH && rightSensor == HIGH);

  // 1. Check for Stop Condition (Plant)
  if (bothHigh) {
    stopMotors();

    // Check if this is a NEW stop (prevent looping logic)
    if (!atPlant) {
      atPlant = true; // Mark that we have handled this specific stop line
      handlePlantStop(); 
    }
    // If atPlant is already true, we do nothing and stay stopped 
    // until the "Nudge" in handlePlantStop pushes us off the line.
    
  } else {
    // 2. We are NOT on a stop line (sensors reading White/White or White/Black)
    atPlant = false; // Reset flag so we can stop at the NEXT plant

    // Line Following Logic
    if (leftSensor == LOW && rightSensor == LOW) {
      // Both sensors on white -> Straddling the line -> Move Forward
      moveForward();
    }
    else if (leftSensor == LOW && rightSensor == HIGH) {
      // Right sensor hit line -> Turn Right
      turnRight();
    }
    else if (leftSensor == HIGH && rightSensor == LOW) {
      // Left sensor hit line -> Turn Left
      turnLeft();
    }
    else {
      // Fallback (Should typically be covered by bothHigh, but good safety)
      stopMotors();
    }
  }

  delay(10); // Small loop stabilization
}

// =====================
// Logic Functions
// =====================

void handlePlantStop() {
  Serial.println("STATUS: AT_PLANT");
  stopMotors(); 

  // Clear serial buffer to ensure no old "DONE" messages trigger this prematurely
  while (Serial.available()) { Serial.read(); }

  // Send Plant ID to Pi
  String plantID = "PLANT" + String(plantCount);
  Serial.println(plantID);
  
  bool gotDone = false;

  // BLOCKING LOOP: Wait for Pi processing
  while (!gotDone) {
    if (Serial.available() > 0) {
      String msg = Serial.readStringUntil('\n');
      msg.trim(); 
      
      if (msg == "DONE") {
        gotDone = true;
        Serial.println("STATUS: RESUMING");
      } 
    }
  }

  plantCount++;
  
  // --- THE FIX IS HERE ---
  // We force the robot to move forward blindly for a short time.
  // This pushes the sensors off the black "Stop" line so the main loop
  // doesn't immediately stop the robot again.
  moveForward();
  delay(NUDGE_TIME); 
  // We do NOT stop motors here. We let the loop() take over immediately 
  // to correct the path.
}

void waitForStartSignal() {
  // Blinking logic or just waiting could go here
  bool isReady = false;
  while (!isReady) {
    if (Serial.available() > 0) {
      String msg = Serial.readStringUntil('\n');
      msg.trim();
      if (msg == "READY") {
        isReady = true;
        Serial.println("ARDUINO_READY");
      }
    }
  }
}

// =====================
// Motor Functions
// =====================

void moveForward() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW); analogWrite(ENA, SPEED_BASE); 
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW); analogWrite(ENB, SPEED_BASE); 
}

void turnRight() {
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH); analogWrite(ENA, SPEED_TURN); 
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);  analogWrite(ENB, SPEED_TURN); 
}

void turnLeft() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);  analogWrite(ENA, SPEED_TURN); 
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH); analogWrite(ENB, SPEED_TURN); 
}

void stopMotors() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW); analogWrite(ENA, 0); 
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW); analogWrite(ENB, 0); 
}
