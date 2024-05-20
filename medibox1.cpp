#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>
#include <WiFi.h>
#include "time.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
#define BUZZER 5
#define LED_1 15
#define PB_cancel 34
#define PB_ok 32   // middle
#define PB_down 35 // left
#define PB_up 33   // right
#define DHTPIN 12

const long gmtOffset_sec = 19800;
const int daylightOffset_sec = 0;
const char *ssid = "SLT_FIBRE";
const char *password = "Thusith12345";
const char *ntpServer = "pool.ntp.org";

// SCL pin = 22
// SDA pin = 21

// declare objects
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHTesp dhtSensor;

// global variables
int days = 0;
int hours = 0;
int minutes = 0;
int seconds = 0;
unsigned long time_now = 0;
unsigned long time_last = 0;

bool alarm_enabled = true;
int n_alarms = 3;
int alarm_hours[] = {0, 0, 1};
int alarm_minutes[] = {1, 3, 1};
bool alarm_triggered[] = {false, false, false};

int n_notes = 8;
int C = 262;
int D = 294;
int E = 330;
int F = 349;
int G = 392;
int A = 440;
int B = 494;
int C_H = 523;
int notes[] = {C, D, E, F, G, A, B, C_H};

int current_mode = 0;
int max_modes = 5;
String modes[] = {"1 - Set time", "2 - Set Alarm 1", "3 - Set alarm 2", "4 - Set alarm 3", "5 - Disable alarms"};

// declare functions
void print_line(String text, int coloum, int row, int text_size);
void print_time_now();
void update_time();
void update_time_with_check_alarms();
void ring_alarm();
void go_to_menu();
int wait_for_button_press();
void run_mode(int mode);
void set_time();
void set_alarm(int alarm);
void check_temp();

void setup()
{
    Serial.begin(115200);
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
    {
        Serial.println(F("Screen allocation failed"));

        for (;;)
            ;
    }

    pinMode(BUZZER, OUTPUT);
    pinMode(LED_1, OUTPUT);
    pinMode(PB_cancel, INPUT);
    pinMode(PB_down, INPUT);
    pinMode(PB_ok, INPUT);
    pinMode(PB_up, INPUT);

    dhtSensor.setup(DHTPIN, DHTesp ::DHT11);

    display.clearDisplay();
    print_line("Connecting to wifi", 0, 0, 1);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        // Serial.println(".");
    }
    display.clearDisplay();
    print_line("Connected succesfully", 0, 0, 1);

    // init and get the time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    update_time();

    // disconnect WiFi as it's no longer needed
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    display.display();
    delay(2000);
    display.clearDisplay();
    print_line("Welcome to Medibox", 15, 30, 2);
    delay(2000);
    display.clearDisplay();
}

void loop()
{
    update_time_with_check_alarms();
    if (digitalRead(PB_ok) == LOW)
    {
        delay(200);
        go_to_menu();
    }

    check_temp();
}

void print_line(String text, int coloum, int row, int text_size)
{
    // display.clearDisplay();
    display.setTextSize(text_size);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(coloum, row);
    display.println(text);
    display.display();
}

void print_time_now()
{
    print_line(String(days), 0, 0, 2);
    print_line(":", 20, 0, 2);
    print_line(String(hours), 30, 0, 2);
    print_line(":", 50, 0, 2);
    print_line(String(minutes), 60, 0, 2);
    print_line(":", 80, 0, 2);
    print_line(String(seconds), 100, 0, 2);
}

void update_time()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        display.clearDisplay();
        print_line("Failed to obtain time", 0, 0, 1);
        return;
    }
    // Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");

    struct tm timeinfo;
    getLocalTime(&timeinfo);

    char timehour[3];
    strftime(timehour, 3, "%H", &timeinfo);
    hours = atoi(timehour);

    char timeminute[3];
    strftime(timeminute, 3, "%M", &timeinfo);
    minutes = atoi(timeminute);

    char timesecond[3];
    strftime(timesecond, 3, "%S", &timeinfo);
    seconds = atoi(timesecond);

    char timeday[3];
    strftime(timeday, 3, "%d", &timeinfo);
    days = atoi(timeday);
}

void ring_alarm()
{
    display.clearDisplay();
    print_line("MEDICINE TIME!", 0, 15, 2);
    digitalWrite(LED_1, HIGH);

    bool break_happened = false;

    while (break_happened == false && digitalRead(PB_cancel) == HIGH)
    {
        // ring the buzzer
        for (int i = 0; i < n_notes; i++)
        {
            if (digitalRead(PB_cancel) == LOW)
            {
                break_happened = true;
                delay(200);
                break;
            }
            tone(BUZZER, notes[i]);
            delay(500);
            noTone(BUZZER);
            delay(2);
        }
    }

    digitalWrite(LED_1, LOW);
    display.clearDisplay();
}
// 35 32 33 button pins
void update_time_with_check_alarms()
{
    update_time();
    print_time_now();
    display.clearDisplay();

    if (alarm_enabled == true)
    {
        for (int i = 0; i < n_alarms; i++)
        {
            if (alarm_triggered[i] == false && alarm_hours[i] == hours && alarm_minutes[i] == minutes)
            {
                ring_alarm();
                alarm_triggered[i] = true;
            }
        }
    }
}

int wait_for_button_press()
{
    while (true)
    {
        if (digitalRead(PB_down) == LOW)
        {
            delay(200);
            return PB_down;
        }
        else if (digitalRead(PB_ok) == LOW)
        {
            delay(200);
            return PB_ok;
        }
        else if (digitalRead(PB_up) == LOW)
        {
            delay(200);
            return PB_up;
        }
        else if (digitalRead(PB_cancel) == LOW)
        {
            delay(200);
            return PB_cancel;
        }

        update_time();
    }
}

void go_to_menu()
{
    while (digitalRead(PB_cancel) == HIGH)
    {
        display.clearDisplay();
        print_line(modes[current_mode], 0, 0, 2);

        int pressed = wait_for_button_press();
        if (pressed == PB_up)
        {
            delay(200);
            current_mode += 1;
            current_mode = current_mode % max_modes;
        }

        else if (pressed == PB_down)
        {
            delay(200);
            current_mode -= 1;
            if (current_mode < 0)
            {
                current_mode = max_modes - 1;
            }
        }

        else if (pressed == PB_ok)
        {
            delay(200);
            run_mode(current_mode);
        }

        else if (pressed == PB_cancel)
        {
            delay(200);
            break;
        }
    }
}

void set_time()
{
    int temp_hour = hours;
    while (true)
    {
        display.clearDisplay();
        print_line("Enter hour = " + String(temp_hour), 0, 0, 2);

        int pressed = wait_for_button_press();
        if (pressed == PB_up)
        {
            delay(200);
            temp_hour += 1;
            temp_hour = temp_hour % 24;
        }

        else if (pressed == PB_down)
        {
            delay(200);
            temp_hour -= 1;
            if (temp_hour < 0)
            {
                temp_hour = 23;
            }
        }

        else if (pressed == PB_ok)
        {
            delay(200);
            hours = temp_hour;
            break;
        }

        else if (pressed == PB_cancel)
        {
            delay(200);
            break;
        }
    }

    int temp_minute = minutes;
    while (true)
    {
        display.clearDisplay();
        print_line("Enter minute = " + String(temp_minute), 0, 0, 2);

        int pressed = wait_for_button_press();
        if (pressed == PB_up)
        {
            delay(200);
            temp_minute += 1;
            temp_minute = temp_minute % 60;
        }

        else if (pressed == PB_down)
        {
            delay(200);
            temp_minute -= 1;
            if (temp_minute < 0)
            {
                temp_minute = 59;
            }
        }

        else if (pressed == PB_ok)
        {
            delay(200);
            minutes = temp_minute;
            break;
        }

        else if (pressed == PB_cancel)
        {
            delay(200);
            break;
        }
    }
    display.clearDisplay();
    print_line("Time is set", 0, 0, 2);
    delay(1000);
}

void set_alarm(int alarm)
{
    int temp_hour = alarm_hours[alarm];
    while (true)
    {
        display.clearDisplay();
        print_line("Enter hour = " + String(temp_hour), 0, 0, 2);

        int pressed = wait_for_button_press();
        if (pressed == PB_up)
        {
            delay(200);
            temp_hour += 1;
            temp_hour = temp_hour % 24;
        }

        else if (pressed == PB_down)
        {
            delay(200);
            temp_hour -= 1;
            if (temp_hour < 0)
            {
                temp_hour = 23;
            }
        }

        else if (pressed == PB_ok)
        {
            delay(200);
            alarm_hours[alarm] = temp_hour;
            break;
        }

        else if (pressed == PB_cancel)
        {
            delay(200);
            break;
        }
    }

    int temp_minute = alarm_minutes[alarm];
    while (true)
    {
        display.clearDisplay();
        print_line("Enter minute = " + String(temp_minute), 0, 0, 2);

        int pressed = wait_for_button_press();
        if (pressed == PB_up)
        {
            delay(200);
            temp_minute += 1;
            temp_minute = temp_minute % 60;
        }

        else if (pressed == PB_down)
        {
            delay(200);
            temp_minute -= 1;
            if (temp_minute < 0)
            {
                temp_minute = 59;
            }
        }

        else if (pressed == PB_ok)
        {
            delay(200);
            alarm_minutes[alarm] = temp_minute;
            break;
        }

        else if (pressed == PB_cancel)
        {
            delay(200);
            break;
        }
    }
    display.clearDisplay();
    print_line("Alarm is set", 0, 0, 2);
    delay(1000);
}

void run_mode(int mode)
{
    if (mode == 0)
    {
        set_time();
    }

    else if (mode == 1 || mode == 2 || mode == 3)
    {
        set_alarm(mode - 1);
    }

    else if (mode == 4)
    {
        alarm_enabled = false;
    }
}

void check_temp()
{
    TempAndHumidity data = dhtSensor.getTempAndHumidity();
    if (data.temperature > 35)
    {
        display.clearDisplay();
        print_line("HIGH TEMPERATURE", 0, 40, 1);
    }

    else if (data.temperature < 25)
    {
        display.clearDisplay();
        print_line("LOW TEMPERATURE", 0, 40, 1);
    }

    if (data.humidity > 40)
    {
        display.clearDisplay();
        print_line("HIGH HUMIDITY", 0, 50, 1);
    }

    else if (data.humidity < 20)
    {
        display.clearDisplay();
        print_line("LOW HUMIDITY", 0, 50, 1);
    }
}