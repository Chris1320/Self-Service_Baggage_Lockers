/*
 * Baliuag University Library Locker - An RFID-based Baggage System.
 *
 * ========== MODULES USED ==========
 *
 * - 1x LCD1602 Module
 * - 1x RC522 RFID Module
 * - 1x Membrane Switch Module
 * - 1x SG90 Servo Motor
 *
 * ========== THIRD-PARTY LIBRARIES INCLUDED ==========
 *
 * - Keypad v3.1.1 (https://playground.arduino.cc/Code/Keypad)
 * - LiquidCrystal_I2C v1.1.2
 * (https://github.com/marcoschwartz/LiquidCrystal_I2C)
 * - MFRC522 v1.4.11 (https://github.com/miguelbalboa/rfid)
 * - Servo v1.2.1 (https://www.arduino.cc/en/Reference/Servo)
 *
 * ========== PIN LAYOUT ==========
 *
 * | Pin Number/Code | Description              |
 * | --------------- | ------------------------ |
 * |                 |                          |
 * | A0              | SG90 Servo Motor         |
 * | 1               | Membrane switch column 4 |
 * | 2               | Membrane switch column 3 |
 * | 3               | Membrane switch column 2 |
 * | 4               | Membrane switch column 1 |
 * | 5               | RFID RST                 |
 * | 6               | Membrane switch row 4    |
 * | 7               | Membrane switch row 3    |
 * | 8               | Membrane switch row 2    |
 * | 9               | Membrane switch row 1    |
 * | 10              | RFID SDA/SS              |
 * | 11              | RFID MOSI                |
 * | 12              | RFID MISO                |
 * | 13              | RFID SCK                 |
 * | SDA             | LCD1602 SDA              |
 * | SCL             | LCD1602 I2C SCL          |
 */

#include <Servo.h>
#include <Keypad.h>
#include <MFRC522.h>
#include <SPI.h> // used together with MFRC522.h
#include <LiquidCrystal_I2C.h>

#define BAUD_RATE 9600 // 115200bps is too fast

// Servo configuration
#define PIN_SERVO A0
#define SERVO_UNLOCK 170 // degrees to unlock the door
#define SERVO_LOCK 45    // degrees to lock the door
Servo servo;

// Keypad configuration
#define KEYPAD_ROWS 4
#define KEYPAD_COLS 4
#define KEYPAD_BACKSPACE_CHAR keypad_keys[3][0] // use this key as backspace button
#define KEYPAD_ENTER_CHAR keypad_keys[3][2]     // use this key as enter button
// keypad pinout: R1,R2,R3,R4,C1,C2,C3,C4
byte PIN_KEYPAD_ROWS[KEYPAD_ROWS] = {9, 8, 7, 6};
byte PIN_KEYPAD_COLS[KEYPAD_COLS] = {4, 3, 2, 1};
char keypad_keys[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'1', '2', '3', 'A'}, // map keypad buttons to matrix.
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};
Keypad keypad = Keypad(makeKeymap(keypad_keys), PIN_KEYPAD_ROWS,
                       PIN_KEYPAD_COLS, KEYPAD_ROWS, KEYPAD_COLS);

// LCD I2C configuration
LiquidCrystal_I2C lcd(0x27, 16, 2);

// RFID configuration
#define PIN_RFID_SS 10
#define PIN_RFID_RST 5
MFRC522 rfid(PIN_RFID_SS, PIN_RFID_RST);

// locker configuration
bool skip_detect = false; // flag used to disallow duplicate RFID scans
byte locker_status = 0;   // 0 = vacant; 1 = occupied
String owner_uid = "";
String owner_pin = "";

// Set lines 1 and 2 of the LCD to the contents of <line1>
// and <line2>, overwriting its current contents.
void setLCDContent(String line1, String line2)
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(line2);
}

// left-pad the string using the specified character.
String leftPadString(String str_to_pad, String padchar, int length)
{
    String padded_string = "";
    int pad_length = length - str_to_pad.length();
    // Add padding to the left
    for (int i = 0; i < pad_length; i++)
        padded_string += padchar;
    padded_string += str_to_pad;
    return padded_string;
}

// Read the UID of the RFID card.
String readRFIDUID()
{
    String uid = "";
    for (int i = 0; i < rfid.uid.size; i++)
    {
        uid += String(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
        uid += String(rfid.uid.uidByte[i], HEX);
    }

    return uid;
}

// Ask the new owner their desired PIN.
void getNewOwner(String uid)
{
    Serial.println("Locker status: Vacant");
    byte in_cols[] = {2, 3, 4, 5}; // for LCD
    char date_cols_placeholder[] = {'x', 'x', 'x', 'x'};

    Serial.println("Asking the user a new pin.");
    setLCDContent("Enter new PIN", "> xxxx");
    byte input_len = 4;
    byte pointer = 0;
    char input[input_len + 1] = {}; // +1 to account for null byte
    while (true)
    {
        char keypress = keypad.getKey();
        if (keypress == NULL)
            continue; // wait for user input.

        // backspace
        if (keypress == KEYPAD_BACKSPACE_CHAR)
        {
            if (pointer != 0)
            {
                input[--pointer] = 0;
                lcd.setCursor(in_cols[pointer], 1);
                lcd.print(date_cols_placeholder[pointer]);
            }
            continue;
        }

        if (keypress == KEYPAD_ENTER_CHAR)
        {
            if (pointer != input_len)
                continue; // do nothing if unchanged

            owner_uid = uid;
            owner_pin = String(input);
            locker_status = 1;
            Serial.println("New owner UID: " + owner_uid);
            Serial.println("New owner PIN: " + String(input)); // NOTE: This is not secure in production use.
            setLCDContent("Press # to lock", "Status: Unlocked");
            Serial.println("Unlocking door.");
            servo.write(SERVO_UNLOCK);
            while (true)
            {
                keypress = keypad.getKey();
                if (keypress == keypad_keys[3][2])
                {
                    Serial.println("Locking door.");
                    break;
                }
            }
            servo.write(SERVO_LOCK);
            setLCDContent("This locker is", "occupied");
            Serial.println("My people need me.");
            return;
        }
        input[pointer] = keypress;
        if (pointer == input_len)
            continue;
        if (pointer < input_len - 1)
        {
            lcd.setCursor(in_cols[pointer + 1], 1);
            lcd.print('_');
        }
        lcd.setCursor(in_cols[pointer], 1);
        lcd.print(input[pointer++]);

        Serial.println(String(pointer) + " | " + String(input));
    }
}

// Ask the owner to enter their PIN to unlock the locker.
// This will also remove the owner's UID and PIN from the system,
// allowing the locker to be used by another person.
void FarewellOwner(String uid)
{
    Serial.println("Locker status: Occupied");
    byte in_cols[] = {2, 3, 4, 5}; // for LCD
    char date_cols_placeholder[] = {'x', 'x', 'x', 'x'};

    Serial.println(uid);
    Serial.println(owner_uid);
    if (uid != owner_uid)
    {
        Serial.println("The card owner is not the owner of this locker. Ignoring.");
        setLCDContent("You are not", "the owner!");
        delay(5000);
        setLCDContent("This locker is", "occupied");
        return;
    }

    Serial.println("The card owner is the owner.");
    Serial.println("Asking the user's pin.");
    setLCDContent("Enter PIN", "> xxxx");
    byte input_len = 4;
    byte pointer = 0;
    char input[input_len + 1] = {}; // +1 to account for null byte
    while (true)
    {
        char keypress = keypad.getKey();
        if (keypress == NULL)
            continue; // wait for user input.

        // backspace
        if (keypress == KEYPAD_BACKSPACE_CHAR)
        {
            if (pointer != 0)
            {
                input[--pointer] = 0;
                lcd.setCursor(in_cols[pointer], 1);
                lcd.print(date_cols_placeholder[pointer]);
            }
            continue;
        }

        if (keypress == KEYPAD_ENTER_CHAR)
        {
            if (pointer != input_len)
                continue; // do nothing if unchanged

            if (owner_pin != String(input))
            {
                setLCDContent("Invalid PIN!", "");
                delay(5000);
                setLCDContent("This locker is", "occupied");
                return;
            }

            // clear the owner's UID and PIN
            owner_uid = "";
            owner_pin = "";
            locker_status = 0; // set locker status to vacant
            setLCDContent("Press # to lock", "Status: Unlocked");
            Serial.println("Unlocking door.");
            servo.write(SERVO_UNLOCK);
            while (true)
            {
                keypress = keypad.getKey();
                if (keypress == keypad_keys[3][2])
                {
                    Serial.println("Locking door.");
                    break;
                }
            }
            servo.write(SERVO_LOCK);
            setLCDContent("This locker is", "vacant");
            Serial.println("My people need me.");
            return;
        }
        input[pointer] = keypress;
        if (pointer == input_len)
            continue;
        if (pointer < input_len - 1)
        {
            lcd.setCursor(in_cols[pointer + 1], 1);
            lcd.print('_');
        }
        lcd.setCursor(in_cols[pointer], 1);
        lcd.print(input[pointer++]);

        Serial.println(String(pointer) + " | " + String(input));
    }
}

void setup()
{
    Serial.begin(BAUD_RATE);

    Serial.println("Initializing servo motor...");
    servo.attach(PIN_SERVO);
    servo.write(SERVO_LOCK); // lock the door by default

    Serial.println("Initializing RFID sensor...");
    SPI.begin();
    rfid.PCD_Init();

    Serial.println("Initializing LCD module...");
    lcd.init();
    lcd.backlight();

    Serial.println("Ready.");
    setLCDContent("Scan card below", "Status: Vacant");
}

void loop()
{
    if (rfid.PICC_IsNewCardPresent())
    {
        Serial.println("A new card has been detected.");
        if (skip_detect)
        {
            Serial.println("skip_detect is true. Skipping...");
            skip_detect = false;
            return;
        }

        skip_detect = true;
        if (rfid.PICC_ReadCardSerial())
        {
            Serial.println("UID has been read.");
            MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
            Serial.println("RFID/NFC Tag Type: " + String(rfid.PICC_GetTypeName(piccType)));

            // read the UID
            String uid = readRFIDUID();
            Serial.println("UID: " + uid);

            rfid.PICC_HaltA();      // halt PICC
            rfid.PCD_StopCrypto1(); // stop encryption on PCD

            switch (locker_status)
            {
            case 0:
                getNewOwner(uid);
                break;

            case 1:
                FarewellOwner(uid);
                break;

            default:  // NOTE: This should never happen. If it does, it's just a skill issue.
                Serial.println("Locker status: Unknown");
                break;
            }
        }
    }
}
