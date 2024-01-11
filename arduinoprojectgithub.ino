




#include "Adafruit_Keypad.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoMqttClient.h>
#include <WiFiS3.h>


LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

const char broker[] = "192.168.144.1";
int        port     = 1883;
const char topic[]  = "mishahoekstra/activatieMelding";
char topic2[]  = "mishahoekstra/is_alarm_Activated";

const long interval = 1000;
unsigned long previousMillis = 0;

int count = 0;

String IsalarmActive;

char ssid[] = "IoTatelierF2144";    // your network SSID (name)
char pass[] = "IoTatelier";    // your network password (use for WPA, or use as key for WEP)

bool IsTriggerd;

const byte ROWS = 4; // rows
const byte COLS = 4; // columns
//define the symbols on the buttons of the keypads
char keys[ROWS][COLS] = {
        {'1','2','3','A'},
        {'4','5','6','B'},
        {'7','8','9','C'},
        {'*','0','#','D'}
};
byte rowPins[ROWS] = {2, 3, 4, 5}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {6, 7, 8, 9}; //connect to the column pinouts of the keypad

//initialize an instance of class NewKeypad
Adafruit_Keypad customKeypad = Adafruit_Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS);


const int TrigPin = 10;
const int EchoPin = 11;
const int Buzzerpin = 12;

const  int passcodeLength = 4;
char correctPasscode[passcodeLength] = {'5', '1', '9', 'C'};

char enteredPasscode[passcodeLength];
int passcodeIndex = 0;



void setup() {
    // initialize serial communication:
    Serial.begin(9600);
    mqttClient.subscribe(topic2);
//Initialize serial and wait for port to open:
    while (!Serial) {
        ; // wait for serial port to connect. Needed for native USB port only
    }

    // attempt to connect to WiFi network:
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
        // failed, retry
        Serial.print(".");
        delay(5000);
    }

    Serial.println("You're connected to the network");
    Serial.println();
    Serial.println(WiFi.localIP());
    Serial.println();
// You can provide a unique client ID, if not set the library uses Arduino-millis()
    // Each client must have a unique client ID
    // mqttClient.setId("clientId");

    // You can provide a username and password for authentication
    // mqttClient.setUsernamePassword("username", "password");

    Serial.print("Attempting to connect to the MQTT broker: ");
    Serial.println(broker);

    if (!mqttClient.connect(broker, port)) {
        Serial.print("MQTT connection failed! Error code = ");
        Serial.println(mqttClient.connectError());

        while (1);
    }

    Serial.println("You're connected to the MQTT broker!");
    Serial.println();

    mqttClient.onMessage(onMqttMessage);

    customKeypad.begin();
    pinMode(TrigPin, OUTPUT);
    pinMode(EchoPin, INPUT);
    pinMode(Buzzerpin, OUTPUT);

    lcd.init();

    lcd.backlight();
    lcd.setCursor(0,0);
    lcd.print("Passcode:");

}

void loop() {

    mqttClient.poll();

    unsigned long currentMillis = millis();

    previousMillis = currentMillis;



    while (IsalarmActive == "on"){






        if (IsTriggerd == true)
        {
            // send message, the Print interface can be used to set the message contents
            mqttClient.beginMessage(topic);
            mqttClient.print("Alarm activated! ");
            mqttClient.endMessage();
        }else
        {
            mqttClient.beginMessage(topic);
            mqttClient.print("Alarm deactivated ");
            mqttClient.endMessage();
        }




        long duration, inches, cm;


        digitalWrite(TrigPin, LOW);
        delayMicroseconds(2);
        digitalWrite(TrigPin, HIGH);
        delayMicroseconds(10);
        digitalWrite(TrigPin, LOW);



        duration = pulseIn(EchoPin, HIGH);

        // convert the time into a distance
        inches = microsecondsToInches(duration);
        cm = microsecondsToCentimeters(duration);

        // Serial.print(inches);
        // Serial.print("in, ");
        // Serial.print(cm);
        // Serial.print("cm");
        // Serial.println();

        delay(100);

        // put your main code here, to run repeatedly:
        customKeypad.tick();

        keypadEvent e = customKeypad.read();
        char Input = (char)e.bit.KEY;
        Serial.println((int)Input);


        delay(10);



        if (e.bit.EVENT == KEY_JUST_PRESSED) {
            Serial.println("Key pressed");
            Serial.println(Input);  // Print the ASCII value of the key
            enteredPasscode[passcodeIndex] = Input;
            passcodeIndex++;

            for (int i = 0; i < passcodeLength; i++)
            {
                lcd.setCursor(i + 10,0);
                lcd.print(enteredPasscode[i]);
            }


            if (passcodeIndex == passcodeLength) {
                if (comparePasscode()) {
                    lcd.setCursor(0,1);
                    lcd.print("Correct passcode");
                    delay(3000);
                    lcd.clear();
                    lcd.setCursor(0,0);
                    lcd.print("Passcode:");
                    IsTriggerd = false;
                    passcodeIndex = 0;

                    //Clear the correct passcode after it is entered
                    for (int i = 0; i < passcodeLength; i++) {
                        enteredPasscode[i] = -1;  // 0 or any other placeholder value
                    }
                } else {
                    // Incorrect passcode, reset the entered passcode
                    lcd.setCursor(0,1);
                    lcd.print("Wrong passcode");
                    delay(3000);
                    IsTriggerd = true;
                    passcodeIndex = 0;  // Reset the entered passcode
                    for (int i = 0; i < passcodeLength; i++) {
                        enteredPasscode[i] = -1;  // 0 or any other placeholder value
                    }
                    lcd.clear();
                    lcd.setCursor(0,0);
                    lcd.print("Passcode:");
                }
            }
        }

// Turn on the buzzer when something is within 15 cm and the correct passcode has not been entered
        if (cm <= 15) {
            IsTriggerd = true;
        }

        if (IsTriggerd == true){
            digitalWrite(Buzzerpin, HIGH);
        } else {
            digitalWrite(Buzzerpin, LOW);
        }



    }
}

long microsecondsToInches(long microseconds) {
    // According to Parallax's datasheet for the PING))), there are 73.746
    // microseconds per inch (i.e. sound travels at 1130 feet per second).
    // This gives the distance travelled by the ping, outbound and return,
    // so we divide by 2 to get the distance of the obstacle.
    // See: https://www.parallax.com/package/ping-ultrasonic-distance-sensor-downloads/
    return microseconds / 74 / 2;
}

long microsecondsToCentimeters(long microseconds) {
    // The speed of sound is 340 m/s or 29 microseconds per centimeter.
    // The ping travels out and back, so to find the distance of the object we
    // take half of the distance travelled.
    return microseconds / 29 / 2;
}

bool comparePasscode() {
    for (int i = 0; i < passcodeLength; i++) {
        if (enteredPasscode[i] != correctPasscode[i]) {
            return false;  // Incorrect passcode
        }
    }
    return true;  // Correct passcode

}

void onMqttMessage(int messageSize) {
    // we received a message, print out the topic and contents
    Serial.println("Received a message with topic '");
    Serial.print(mqttClient.messageTopic());
    Serial.print("', length ");
    Serial.print(messageSize);
    Serial.println(" bytes:");

    // use the Stream interface to print the contents
    IsalarmActive = "";
    while (mqttClient.available()) {
        IsalarmActive += (char)mqttClient.read();
    }

    Serial.println();

    Serial.println();
}

