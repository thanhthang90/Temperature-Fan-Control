#include <Arduino.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <Preferences.h>
#include <Ticker.h>

char ssid[] = "wifi-ssid";
char password[] = "wifi-password";

// Blynk
#define BLYNK_TEMPLATE_ID ""
#define BLYNK_TEMPLATE_NAME "Fan Temperature Control 4 Pin"
#define BLYNK_AUTH_TOKEN ""

#include <BlynkSimpleEsp32.h>

WidgetLED led1(V0);

// GPIO where the DS18B20 is connected to
const int PIN_TEMPERATURE = 4;
OneWire oneWire(PIN_TEMPERATURE);
DallasTemperature sensors(&oneWire);


// Motor
#define PIN_FAN 2

// Fan
bool isFanRunning = false;

const float TEMP_THRESHOLD_MIN = 31;//Nhiệt độ dưới 31 thì quạt chạy mức thấp
const float TEMP_THRESHOLD_MAX = 33;//Nhiệt độ trên 33 thì quạt chạy mức cao
const int PWM_THRESHOLD_MIN = 80;//Tốc độ bình thường 
const int PWM_THRESHOLD_MAX = 150; //Tốc độ cao
float tempMin = TEMP_THRESHOLD_MIN;
float tempMax = TEMP_THRESHOLD_MAX;
int pwmMin = PWM_THRESHOLD_MIN;
int pwmMax = PWM_THRESHOLD_MAX;

bool isOn = true;

//====================== MEDTHOD =========================================
float getTemperature();
void setFan(bool isRunningOn);
void updateFan();
//---------------------- OTHER ------------------------------------------
//  Preferences
Preferences preferences;
char KEY_TEMP_MIN[] = "KEY_TEMP_MIN";
char KEY_TEMP_MAX[] = "KEY_TEMP_MAX";
char KEY_PWM_MIN[] = "KEY_PWM_MIN";
char KEY_PWM_MAX[] = "KEY_PWM_MAX";
char KEY_ON[] = "KEY_ON";
char KEY_AUTO[] = "KEY_AUTO";

BLYNK_CONNECTED() {
  Serial.println("BLYNK_CONNECTED");
  Blynk.virtualWrite(V0, isFanRunning ? 1 : 0);
  Blynk.virtualWrite(V2, tempMin);
  Blynk.virtualWrite(V3, tempMax);
  Blynk.virtualWrite(V4, pwmMin);
  Blynk.virtualWrite(V5, pwmMax);
  Blynk.virtualWrite(V6, isOn);
}

BLYNK_WRITE(V2) {
  // assigning incoming value from pin V1 to a variable
  tempMin = param.asFloat();
  preferences.putFloat(KEY_TEMP_MIN, tempMin);
  Serial.print("V2 value is: ");
  Serial.println(tempMin);
}
BLYNK_WRITE(V3) {
  tempMax = param.asFloat();
  preferences.putFloat(KEY_TEMP_MAX, tempMax);
  Serial.print("V3 value is: ");
  Serial.println(tempMax);
}
BLYNK_WRITE(V4) {
  pwmMin = param.asInt();
  Serial.print("V4 value is: ");
  Serial.println(pwmMin);
  preferences.putInt(KEY_PWM_MIN, pwmMin);
  updateFan();
}
BLYNK_WRITE(V5) {
  pwmMax = param.asInt();
  Serial.print("V5 value is: ");
  Serial.println(pwmMax);
  preferences.putInt(KEY_PWM_MAX, pwmMax);
  updateFan();
}
BLYNK_WRITE(V6) {
  int value = param.asInt();
  Serial.print("V6 value is: ");
  Serial.println(value);
  isOn = (value == 1) ? true : false;
  preferences.putBool(KEY_ON, isOn);
  setFan(isOn);
}

//====================== TICKER ==========================================
unsigned long timerSecond;
int timerSecondDelay = 3 * 1000; // 3 second delay

// Restart sau lại mỗi 3 ngày
const int TIMER_RESTART_DEVICE = 60 * 60 * 24 * 3; // Second
Ticker timerRestartDevice;
void timerRestartDeviceTrigger() { ESP.restart(); }

//====================== SETUP ==========================================
void setup() {
  Serial.begin(115200);


  pinMode(PIN_FAN, OUTPUT);
  analogWrite(PIN_FAN, 0); // 0/80/150

  // Preference setup
  preferences.begin("my-app", false);
  isOn = preferences.getInt(KEY_ON, true);
  tempMin = preferences.getFloat(KEY_TEMP_MIN, TEMP_THRESHOLD_MIN);
  tempMax = preferences.getFloat(KEY_TEMP_MAX, TEMP_THRESHOLD_MAX);
  pwmMin = preferences.getInt(KEY_PWM_MIN, PWM_THRESHOLD_MIN);
  pwmMax = preferences.getInt(KEY_PWM_MAX, PWM_THRESHOLD_MAX);

  Serial.println("Temperature range:  " + String(tempMin) + "/" +
                 String(tempMax));

  // Start the DS18B20 sensor
  sensors.begin();
  if (isOn) {
    setFan(true);
  } else {
    setFan(false);
  }
  Serial.print("Start...");
  timerSecond = millis();
  timerRestartDevice.attach(TIMER_RESTART_DEVICE, timerRestartDeviceTrigger);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);
}

void loop() {
  Blynk.run();

  //Kiểm tra nhiệt độ mỗi 3 giây
  if (millis() > timerSecond + timerSecondDelay) {
    timerSecond = millis();

    updateFan();
  }
}

void updateFan() {
  float temp = getTemperature();
  Serial.print(temp);
  Serial.println("ºC");
  Blynk.virtualWrite(V1, temp);

  if (isOn) {
    if (temp < tempMin) {
      setFan(false);
    } else if (temp >= tempMax) {
      setFan(true);
    }
  } else
    setFan(false);
}

void setFan(bool isRunningOn) {
  Serial.printf("setFan: ", isRunningOn);
  Serial.println(isRunningOn);
  if (isRunningOn) {
    isFanRunning = true;
    analogWrite(PIN_FAN, pwmMax); //  set tốc độ cao
    Serial.println("setFan: ON");
    led1.on();
  } else {
    isFanRunning = false;
    Serial.println("setFan: OFF");
    analogWrite(PIN_FAN, pwmMin); //  set  tốc độ thấp
    led1.off();
  }
}

float getTemperature() {
  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);
  return temp;
}