#include <Servo.h>

// ── Pin definitions ──────────────────────────────────────────
const int trigPin  = 11;
const int echoPin  = 10;
const int servoPin = 9;
const int redPin   = 3;
const int greenPin = 5;
const int bluePin  = 6;

// ── Constants ────────────────────────────────────────────────
const int OPEN_ANGLE             = 90;
const int CLOSED_ANGLE           = 0;
const int DETECT_RANGE           = 10;
const unsigned long OPEN_DURATION    = 3000;
const unsigned long WARNING_DURATION = 1000;
const int SWEEP_SPEED            = 30;

// ── State machine ────────────────────────────────────────────
enum DoorState { CLOSED, OPENING, OPEN, WARNING, CLOSING };
DoorState doorState = CLOSED;

// ── Timing & tracking ────────────────────────────────────────
unsigned long stateStartTime = 0;
unsigned long lastSweepTime  = 0;
unsigned long lastBlinkTime  = 0;
unsigned long lastSensorTime = 0;
bool blinkOn     = false;
int currentAngle = 0;

Servo myServo;

// ── Helper: set RGB color (common cathode) ─────────────────────
void setColor(int r, int g, int b) {
  analogWrite(redPin,   r);
  analogWrite(greenPin, g);
  analogWrite(bluePin,  b);
}

// ── Helper: get distance ──────────────────────────────────────
float getDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000);
  return duration * 0.034 / 2;
}

void setup() {
  pinMode(trigPin,  OUTPUT);
  pinMode(echoPin,  INPUT);
  pinMode(redPin,   OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin,  OUTPUT);

  myServo.attach(servoPin);
  myServo.write(CLOSED_ANGLE);
  setColor(255, 0, 0);
  Serial.begin(9600);
}

void loop() {
  unsigned long now = millis();

  switch (doorState) {

    // ── CLOSED: solid red ─────────────────────────────────────
    case CLOSED: {
      setColor(255, 0, 0);

      if (now - lastSensorTime >= 100) {
        lastSensorTime = now;
        float distance = getDistance();
        Serial.print("Distance: ");
        Serial.println(distance);

        if (distance > 0 && distance <= DETECT_RANGE) {
          doorState      = OPENING;
          stateStartTime = now;
          lastSweepTime  = now;
          lastBlinkTime  = now;
          blinkOn        = true;
          currentAngle   = CLOSED_ANGLE;
          Serial.println("Object detected! Opening...");
        }
      }
      break;
    }

    // ── OPENING: blink yellow ─────────────────────────────────
    case OPENING: {
      if (now - lastBlinkTime >= 200) {
        lastBlinkTime = now;
        blinkOn = !blinkOn;
        setColor(blinkOn ? 255 : 0, blinkOn ? 255 : 0, 0); // yellow blink
      }

      if (now - lastSweepTime >= SWEEP_SPEED) {
        lastSweepTime = now;
        currentAngle++;
        myServo.write(currentAngle);

        if (currentAngle >= OPEN_ANGLE) {
          doorState      = OPEN;
          stateStartTime = now;
          setColor(0, 255, 0); // immediately show green
          Serial.println("Fully open!");
        }
      }
      break;
    }

    // ── OPEN: solid green ─────────────────────────────────────
    case OPEN: {
      setColor(0, 255, 0);

      if (now - lastSensorTime >= 200) {
        lastSensorTime = now;
        float distance = getDistance();

        if (distance > 0 && distance <= DETECT_RANGE) {
          stateStartTime = now;
          Serial.println("Object still detected, resetting timer...");
        } else {
          Serial.println("No object, counting down...");
        }
      }

      if (now - stateStartTime >= (OPEN_DURATION - WARNING_DURATION)) {
        doorState      = WARNING;
        stateStartTime = now;
        lastBlinkTime  = now;
        Serial.println("Warning: about to close!");
      }
      break;
    }

    // ── WARNING: blink blue ───────────────────────────────────
    case WARNING: {
      if (now - lastBlinkTime >= 300) {
        lastBlinkTime = now;
        blinkOn = !blinkOn;
        setColor(0, 0, blinkOn ? 255 : 0);
      }

      if (now - lastSensorTime >= 200) {
        lastSensorTime = now;
        float distance = getDistance();

        if (distance > 0 && distance <= DETECT_RANGE) {
          doorState      = OPEN;
          stateStartTime = now;
          Serial.println("Object detected again! Staying open...");
        }
      }

      if (now - stateStartTime >= WARNING_DURATION) {
        doorState      = CLOSING;
        stateStartTime = now;
        lastSweepTime  = now;
        lastBlinkTime  = now;
        blinkOn        = true;
        currentAngle   = OPEN_ANGLE;
        Serial.println("Closing...");
      }
      break;
    }

    // ── CLOSING: blink red ────────────────────────────────────
    case CLOSING: {
      if (now - lastBlinkTime >= 200) {
        lastBlinkTime = now;
        blinkOn = !blinkOn;
        setColor(blinkOn ? 255 : 0, 0, 0); // red blink
      }

      if (now - lastSweepTime >= SWEEP_SPEED) {
        lastSweepTime = now;
        currentAngle--;
        myServo.write(currentAngle);

        if (currentAngle <= CLOSED_ANGLE) {
          doorState = CLOSED;
          setColor(255, 0, 0); // solid red
          Serial.println("Closed. Waiting...");
        }
      }
      break;
    }
  }
}
