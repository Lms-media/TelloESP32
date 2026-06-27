#include <TelloESP32.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <LiquidCrystal_I2C.h>

using namespace TelloControl;

TelloESP32 tello;
Adafruit_MPU6050 mpu;
LiquidCrystal_I2C lcd(0x27, 16, 2);

const int BUTTON_PIN = 4;
const int POT_PIN = 32;

bool isPressed = true;

int targetHeight = 50;
int actualHeight = 50;
unsigned long lastPotCheckTime = 0;
const float TILT_THRESHOLD = 6.0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(POT_PIN, INPUT);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Connect MPU6050");
  lcd.setCursor(0, 1);
  lcd.print("Connect Tello");

  lcd.setCursor(0, 0);
  Serial.println("Connect to MPU6050...");
  if (!mpu.begin()) {
    Serial.println("Error connetion to MPU6050.");
    lcd.print("Error MPU6050  ");
  }
  Serial.println("MPU6050 connected!");
  lcd.print("MPU6050 ready!  ");
  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  lcd.setCursor(0, 1);
  Serial.println("Connect to Tello...");
  TelloStatus status = tello.connect("TELLO-5F327A", "");
  
  if (status == TelloStatus::OK) {
    Serial.println("Tello connected!");
    lcd.print("Tello ready!    ");
    actualHeight = tello.get_height();
  } else {
    Serial.print("Error connection. Code: ");
    Serial.println((int)status);
    lcd.print("Error tello: " + String((int)status));
  }
}

void loop() {
  bool currentButtonState = digitalRead(BUTTON_PIN);

  if (currentButtonState && !isPressed) {
    isPressed = true;
    Serial.println("Button pressed!");
    // delay(300);
    if (!tello.isFlying()) {
      Serial.println("Tello takeoff!");
      tello.takeoff();
    } else {
      Serial.println("Tello landing...");
      tello.land();
    }
  } else if (!currentButtonState) {
    isPressed = false;
  }

  if (tello.isFlying()) { 
    int potValue = analogRead(POT_PIN);
    
    targetHeight = ((map(potValue, 0, 4095, 20, 150) + 5) / 10) * 10;
    actualHeight = tello.get_height();
    int heightDifference = targetHeight - actualHeight;
    int up_down = 0;
      
    if (abs(heightDifference) > 10) {
      Serial.println("Sending: up/down to" + String(heightDifference));
      up_down = 20 * (heightDifference / abs(heightDifference));
      Serial.println(up_down);
    }

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    float tiltX = a.acceleration.x;
    float tiltY = a.acceleration.y;

    if (tiltX > TILT_THRESHOLD || tiltX < -TILT_THRESHOLD) {
      Serial.println("forward/back: " + String(-tiltX * 5));
    }

    if (tiltY > TILT_THRESHOLD || tiltY < -TILT_THRESHOLD) {
      Serial.println("left/right to: " + String(tiltY * 5));
    }

    tello.send_rc_control(tiltY * 5, -tiltX * 5, up_down, 0);
  }

  static unsigned long lastTelemetryTime = 0;
  if (millis() - lastTelemetryTime > 1000) {
    int battery = tello.query_battery();
    int temp = tello.query_temp();
    int currentHeight = tello.query_height() * 10;
    
    Serial.print("Battery: "); Serial.print(battery); Serial.print("% | ");
    Serial.print("Height: "); Serial.print(currentHeight); Serial.print(" sm | ");
    Serial.print("Analog height: "); Serial.print(targetHeight); Serial.println(" sm");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("B: " + String(battery) + "% T:" + String(temp) + "C");
    lcd.setCursor(0, 1);
    lcd.print("H: " + String(currentHeight) + " TH:" + String(targetHeight));
    
    lastTelemetryTime = millis();
  }
}