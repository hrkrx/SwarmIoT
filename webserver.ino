#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ESP32Servo.h>
const char *ssid = "angryNerds";
const char *password = "00365301373446700800";

// Status led
int led = 13;
// customs defines
// Published values for SG90 servos; adjust if needed
int minUs = 500;
int maxUs = 2400;
int servo1Pin = 32;
int servoINPUT = 35;
int ledR = 0;
int ledG = 0;
int ledB = 0;
int ledW = 0;
int canState = 0;
int onOff = 0;
int fein = 0;
int pulver = 0;
int groob = 0;
bool onState = false;
String currentState = "stop";

Servo servo1;
WebServer server(80);

char *deviceName = "UNOLD";

int sensorSize = 1;
String sensors[] = {"sensorServo"};

int functionSize = 8;
String functions[] = {"functionFein", "functionGrob", "functionPulver", "functionStop", "functionOff", "functionOn", "handleSensorHolderState", "test"};

void setup()
{
    Serial.begin(115600);
    initWebServer();
    Serial.println("setup complete");
}

void setupPins()
{
    pinMode(33, OUTPUT); // converter
    pinMode(12, OUTPUT); // converter
    pinMode(14, OUTPUT); // converter
    pinMode(27, OUTPUT); // converter
    pinMode(26, OUTPUT); // converter
    pinMode(35, OUTPUT); // converter i/o
    pinMode(32, OUTPUT); // servo
    pinMode(servoINPUT, INPUT);  // servo in

    setMotor(155);
}
void loop()
{
    server.handleClient();
    //int i = analogRead(servoINPUT);
    //int pos = 0;
    //pos = map(i, 244, 2326, 0, 180);
    //Serial.println(String(pos) + " | " + String(i));
    //delay(1000);
    // if (Serial.available() > 0)
    // {
    //     int pos = Serial.parseInt();
    //     if (pos > 0)
    //     {
    //         setMotor(pos);
    //     }
    // }
    
    
}

void handleRoot()
{
    digitalWrite(led, 1);
    server.send(200, "text/plain", "");
    digitalWrite(led, 0);
}

void handleNotFound()
{
    digitalWrite(led, 1);
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i = 0; i < server.args(); i++)
    {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
    digitalWrite(led, 0);
}

void initWebServer()
{

    //Init WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.println("");

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    if (MDNS.begin(deviceName /* .local */))
    {
        Serial.println("MDNS responder started");
    }

    server.onNotFound(handleNotFound);

    server.on("/", handleRoot);
    server.on("/config", handleConfig);
    server.on("/sensorServo", handleSensorServo);
    server.on("/functionFein", handleFunctionFein);
    server.on("/functionGrob", handleFunctionGrob);
    server.on("/functionPulver", handleFunctionPulver);
    server.on("/functionStop", handleFunctionStop);
    server.on("/functionOff", handleFunctionOff);
    server.on("/functionOn", handleFunctionOff);
    server.on("/test", testJsonMethods);
    server.begin();
}

// JSON utility methods

String createJsonArrayAttribute(String name, String *data, int arraySize)
{
    Serial.println("createJsonArrayAttribute begin");
    String result = "\"" + name + "\":[ ";
    Serial.println("result prepared");
    if (sizeof(data) > 1)
    {
        for (size_t i = 0; i < arraySize; i++)
        {
            Serial.print("for ");
            Serial.println(i);
            if (i + 1 < sizeof(data))
            {
                result += "\"" + data[i] + "\", ";
            }
            else
            {
                Serial.println("last element");
                result += "\"" + data[i] + "\" ";
            }
        }
        result += "]";
    }
    Serial.println("createJsonArrayAttribute end");
    return result;
}

String createJsonAttribute(String name, String data)
{
    Serial.println("createJsonAttribute begin");

    String result = "\"" + name + "\":\"" + data + "\"";
    Serial.println("createJsonAttribute end");
    return result;
}

// Methods for serving data

void handleConfig()
{
    // send JSON configuration
    Serial.println("handleConfig");
    String deviceName = createJsonAttribute("DeviceName", deviceName);
    String sensorArray = createJsonArrayAttribute("Sensors", sensors, sensorSize);
    String functionArray = createJsonArrayAttribute("Function", functions, functionSize);
    String jsonResponse = "{" + deviceName + ", " + sensorArray + ", " + functionArray + "}";

    server.send(200, "text/plain", jsonResponse);
}

String stateOfServo()
{
    int i = analogRead(servoINPUT);
    int pos = 0;
    pos = map(i, 244, 2326, 0, 180);
    String data = "";
    if (pos > 0 && pos < 55)
    {
        data = "Fein";
        setPinStateFein();
    }
    if (pos > 56 && pos < 90)
    {
        data = "Grob";
        setPinStateGrob();
    }
    if (pos > 91 && pos < 132)
    {
        setPinStatePulver();
        data = "Pulver";
    }
    if (pos > 133)
    {
        setPinStateStop();
        data = "Stop";
    }
    return data;
}

boolean getKoffeeHolderState()
{
    return digitalRead(14);
}

void handleSensorHolderState()
{
    // send JSON information about Sensor
    Serial.println("handleCoffeeHolderState");
    bool data = getKoffeeHolderState();
    currentState = data;
    String jsonData = createJsonAttribute("data", String(data));
    String jsonType = createJsonAttribute("type", "boolen"); // change datatype depending on data!
    String jsonResponse = "{" + jsonData + ", " + jsonType + "}";

    server.send(200, "text/plain", jsonResponse);
}

bool setOff()
{
    if (onState == true)
    {
        onState = false;
        digitalWrite(12, LOW);
    }
}

bool setOn()
{
    if (onState == false && getKoffeeHolderState() == true)
    {
        stateOfServo();
        onState = true;
        digitalWrite(12, LOW);
    }
}

bool setPinStateFein()
{

    digitalWrite(fein, LOW);
    digitalWrite(pulver, HIGH);
    digitalWrite(grob, HIGH);

    setMotor(40);
    return true;
}

bool setPinStateGrob()
{

    digitalWrite(fein, HIGH);
    digitalWrite(pulver, HIGH);
    digitalWrite(grob, LOW);

    setMotor(70);
    return true;
}HIGH

bool setPinStatePulver()
{

    digitalWrite(fein, HIGH);
    digitalWrite(pulver, LOW);
    digitalWrite(grob, HIGH);

    setMotor(110);
    return true;
}

bool setPinStateStop()
{

    digitalWrite(fein, HIGH);
    digitalWrite(pulver, HIGH);
    digitalWrite(grob, HIGH);

    setMotor(155);
    return true;
}

void handleSensorServo()
{
    // send JSON information about Sensor
    Serial.println("handleServo");
    String data = stateOfServo();
    currentState = data;
    String jsonData = createJsonAttribute("data", data);
    String jsonType = createJsonAttribute("type", "String"); // change datatype depending on data!
    String jsonResponse = "{" + jsonData + ", " + jsonType + "}";

    server.send(200, "text/plain", jsonResponse);
}

void handleFunctionFein()
{
    // receive JSON information about function result (success or failure)
    Serial.println("handleFunctionX");

    bool result = setPinStateFein(); //TODO: use actual function here

    String jsonData;

    if (result)
    {
        jsonData = createJsonAttribute("result", "success");
    }
    else
    {
        jsonData = createJsonAttribute("result", "failure");
    }

    String jsonResponse = "{" + jsonData + "}";

    server.send(200, "text/plain", jsonResponse);
}

void handleFunctionGrob()
{
    // receive JSON information about function result (success or failure)
    Serial.println("handleFunctionX");

    bool result = setPinStateGrob(); //TODO: use actual function here

    String jsonData;

    if (result)
    {
        jsonData = createJsonAttribute("result", "success");
    }
    else
    {
        jsonData = createJsonAttribute("result", "failure");
    }

    String jsonResponse = "{" + jsonData + "}";

    server.send(200, "text/plain", jsonResponse);
}

void handleFunctionPulver()
{
    // receive JSON information about function result (success or failure)
    Serial.println("handleFunctionX");

    bool result = setPinStatePulver(); //TODO: use actual function here

    String jsonData;

    if (result)
    {
        jsonData = createJsonAttribute("result", "success");
    }
    else
    {
        jsonData = createJsonAttribute("result", "failure");
    }

    String jsonResponse = "{" + jsonData + "}";

    server.send(200, "text/plain", jsonResponse);
}

void handleFunctionStop()
{
    // receive JSON information about function result (success or failure)
    Serial.println("handleFunctionX");

    bool result = setPinStateStop(); //TODO: use actual function here

    String jsonData;

    if (result)
    {
        jsonData = createJsonAttribute("result", "success");
    }
    else
    {
        jsonData = createJsonAttribute("result", "failure");
    }

    String jsonResponse = "{" + jsonData + "}";

    server.send(200, "text/plain", jsonResponse);
}

void handleFunctionOff()
{
    // receive JSON information about function result (success or failure)
    Serial.println("handleFunctionX");

    bool result = setOff(); //TODO: use actual function here

    String jsonData;

    if (result)
    {
        jsonData = createJsonAttribute("result", "success");
    }
    else
    {
        jsonData = createJsonAttribute("result", "failure");
    }

    String jsonResponse = "{" + jsonData + "}";

    server.send(200, "text/plain", jsonResponse);
}

void handleFunctionOn()
{
    // receive JSON information about function result (success or failure)
    Serial.println("handleFunctionX");

    bool result = setOn(); //TODO: use actual function here

    String jsonData;

    if (result)
    {
        jsonData = createJsonAttribute("result", "success");
    }
    else
    {
        jsonData = createJsonAttribute("result", "failure");
    }

    String jsonResponse = "{" + jsonData + "}";

    server.send(200, "text/plain", jsonResponse);
}

void testJsonMethods()
{
    String jsonData = createJsonAttribute("result", "success");
    Serial.println(jsonData);
    // String json[] = { jsonData };
    // String jsonResponse = createJson(json);
    // Serial.println("json response:");
    // Serial.println(jsonResponse);
    server.send(200, "text/plain", jsonData);
}

void setMotor(int finish)
{
    Serial.println("setMotor begin");
    servo1.attach(servo1Pin, minUs, maxUs);
    Serial.println(finish);
    int i = analogRead(servoINPUT);
    int pos = 0;
    pos = map(i, 244, 2326, 0, 180);
    Serial.println(pos);

    if (pos < finish)
    {
        for (; pos <= finish; pos += 1)
        { // sweep from 0 degrees to 180 degrees
            // in steps of 1 degree
            servo1.write(pos);
            delay(100); // waits 20ms for the servo to reach the position
            //Serial.println(pos);
        }
    }
    else
    {
        Serial.println("here");
        for (; pos >= finish; pos -= 1)
        { // sweep from 0 degrees to 180 degrees
            servo1.write(pos);
            delay(20); // waits 20ms for the servo to reach the position
            //Serial.println(pos);
        }
        delay(20); // waits 20ms for the servo to reach the position
        //Serial.println(pos);
    }
    
    i = analogRead(servoINPUT);
    pos = 0;
    pos = map(i, 244, 2326, 0, 180);

    Serial.println(pos);

    // if (pos + 5 < finish || pos - 5 > finish)
    // {
    //     setMotor(finish);
    // }
    
    servo1.detach();
    Serial.println("setMotor end");
}

void handleFunctionColorLED()
{
    bool r, g, b;
    // set r g b according to get parameters
    r = server.arg("r") == 1;
    g = server.arg("g") == 1;
    b = server.arg("b") == 1;

    setRGBLed(r, g, b);

    String jsonData = createJsonAttribute("result", "success");
    String jsonResponse = "{" + jsonData + "}";

    server.send(200, "text/plain", jsonResponse);
}

void handleFunctionSetMotor()
{
    int newPos = 0;

    newPos = server.arg("pos").toInt();

    setMotor(newPos);
    
    String jsonData = createJsonAttribute("result", "success");
    String jsonResponse = "{" + jsonData + "}";

    server.send(200, "text/plain", jsonResponse);
}

void setRGBLed(bool r, bool g, bool b)
{
    if (r)
    {
        digitalWrite(ledR, LOW);
    }
    else
    {
        digitalWrite(ledR, HIGH);
    }

    if (g)
    {
        digitalWrite(ledG, LOW);
    }
    else
    {
        digitalWrite(ledG, HIGH);
    }

    if (b)
    {
        digitalWrite(ledB, LOW);
    }
    else
    {
        digitalWrite(ledB, HIGH);
    }   
}

