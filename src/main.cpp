#include <Arduino.h>

#include <EEPROM.h>
#include <SPI.h>
#include <MFRC522.h>

constexpr uint8_t RST_PIN = 9;
constexpr uint8_t SS_PIN = 10;

bool successRead; //Состояние сканера
byte storedCard[4]; //Хранит карту из EEPROM
byte readCard[4];   //Хранит считанную карту

uint8_t isLocked;

MFRC522 mfrc522(SS_PIN, RST_PIN);

#define green_led 5
#define red_led 6
#define lock_pin 2

#define RED_LED_ON LOW //инверсия красного светодиода
#define RED_LED_OFF HIGH

#define pre_open_delay 1000 //пауза перед открытием замка
#define open_delay 3000     //сколько задерживается замок в открытом положении
#define led_blink_freq 100   //с какой частотой мигает светодиод

void blinkLed(int led_pin, int count)
{
    for (int i = 0; i < count; i++)
    {
        if (led_pin == red_led)
        {
            digitalWrite(led_pin, RED_LED_ON);
            delay(led_blink_freq);
            digitalWrite(led_pin, RED_LED_OFF);
            delay(led_blink_freq);
        }
        else
        {
            digitalWrite(led_pin, HIGH);
            delay(led_blink_freq);
            digitalWrite(led_pin, LOW);
            delay(led_blink_freq);
        }
    }
}

void openLock()
{
    blinkLed(green_led, 1);
    delay(pre_open_delay);
    digitalWrite(lock_pin, HIGH);
    delay(open_delay);
    digitalWrite(lock_pin, LOW);
    blinkLed(green_led, 2);
}

bool getID()
{
    if (!mfrc522.PICC_IsNewCardPresent())
    {
        return false;
    }
    if (!mfrc522.PICC_ReadCardSerial())
    {
        return false;
    }
    for (uint8_t i = 0; i < 4; i++)
    {
        readCard[i] = mfrc522.uid.uidByte[i];
    }
    mfrc522.PICC_HaltA();
    return true;
}

void successfulOperation()
{
    blinkLed(green_led, 5);
}

void failedOperation()
{
    blinkLed(red_led, 3);
}

bool compareCards(byte a[], byte b[])
{
    for (uint8_t k = 0; k < 4; k++)
    {
        if (a[k] != b[k])
        {
            return false;
        }
    }
    return true;
}

void writeCard(byte card[])
{
    for (uint8_t j = 0; j < 4; j++)
    {
        EEPROM.write(j + 1, card[j]);
    }
}
void readID()
{
    for (uint8_t i = 0; i < 4; i++)
    {
        storedCard[i] = EEPROM.read(i + 1);
    }
}

void setup()
{
    pinMode(green_led, OUTPUT);
    pinMode(red_led, OUTPUT);
    pinMode(lock_pin, OUTPUT);
    SPI.begin();
    mfrc522.PCD_Init();
    isLocked = EEPROM.read(0); //Читаем состояние замка из энергонезависимой памяти
}
void loop()
{
    successRead = getID(); //пытаемся прочитать карту
    if (successRead)//если прочитали успешно
    {
        blinkLed(green_led, 1);
        if (isLocked == 1) //Если замок уже был закрыт
        {
            readID();//достаём из памяти сохранённую карту
            if (compareCards(readCard, storedCard))//сравниваем полученную и сохранённые карты
            {
                openLock();//открываем замок
                
                isLocked = 0;//запоминаем что открыли его
                EEPROM.write(0, isLocked);
                
                successfulOperation();//мигаем лампочками
            }
            else
            {
                failedOperation();//негативно мигаем лампочками
            }
        }
        else //Если замок был открыт
        {
            writeCard(readCard);//записываем полученную карту
            
            isLocked = 1;//запоминаем что закрыли замок
            EEPROM.write(0, isLocked);
            
            openLock();//открываем замок(чтобы достать шток и закрыть)
            successfulOperation();//мигаем лампочками
        }
    }
}