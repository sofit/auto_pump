#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

#define PIN_COUNT 6
#define PUMP_PERIOD 60 * 60 * 24 * 3l  // период работы помпы в секундах
#define PUMP_WORK_TIME 5               // время работы в секундах
#define PUMP_PIN 0                     // пин помпы

#define LED_PIN 1
#define LAMP_PIN 1                     // пин лампы
#define LAMP_ON_PERIOD 60 * 60 * 4u    // период работы лампы в секундах
#define LAMP_OFF_PERIOD 60 * 60 * 12u  // период отдыха лампы в секундах

#define PHOTO_RESISTOR_PIN 1                    //Analog 1 for P2
#define PHOTO_RESISTOR_CHECK_PERIOD 5 * 1 * 1u  //период опроса фоторезистора
#define PHOTO_RESISTOR_MIN_VALUE 100            // минимальные значение освещенности, при котором необходимо включить лампу (0-255)

unsigned char lampTryNumber = 0;
unsigned long pumpTimer;
unsigned int lampTimer, lampOffPeriod;
boolean pumpRunning = false, lampOn = false;

#define adc_disable() (ADCSRA &= ~(1 << ADEN))  // disable ADC (before power-off)
#define adc_enable() (ADCSRA |= (1 << ADEN))    // re-enable ADC

void setup() {
  // все пины как входы, экономия энергии
  for (byte i = 0; i < PIN_COUNT; i++) {
    pinMode(i, INPUT);
  }

  lampTimer = 0;
  lampOffPeriod = 0;

  // adc_disable();  // отключить АЦП (экономия энергии)

  wdt_reset();          // инициализация ватчдога
  wdt_enable(WDTO_1S);  // разрешаем ватчдог
  // 15MS, 30MS, 60MS, 120MS, 250MS, 500MS, 1S, 2S, 4S, 8S

  enableInterrupt();
  sei();                                // разрешаем прерывания
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);  // максимальный сон
}

void loop() {
  pumpTimer++;
  lampTimer++;

  if (!pumpRunning) {                // если помпа выключена
    if (pumpTimer >= PUMP_PERIOD) {  // таймер периода
      pumpTimer = 0;                 // сброс таймера
      pumpRunning = true;            // флаг на запуск
      pinMode(PUMP_PIN, OUTPUT);     // пин как выход
      digitalWrite(PUMP_PIN, HIGH);  // врубить
    }
  } else {                              // если помпа включена
    if (pumpTimer >= PUMP_WORK_TIME) {  // таймер времени работы
      pumpTimer = 0;                    // сброс
      pumpRunning = false;              // флаг на выкл
      digitalWrite(PUMP_PIN, LOW);      // вырубить
      pinMode(PUMP_PIN, INPUT);         // пин как вход (экономия энергии)
    }
  }

  if (!lampOn) {
    if (lampTimer > lampOffPeriod && lampTimer >= PHOTO_RESISTOR_CHECK_PERIOD) {
      lampTimer = 0;
      lampOffPeriod = 0;
      if ((1023 - analogRead(PHOTO_RESISTOR_PIN)) < PHOTO_RESISTOR_MIN_VALUE) {
        if (lampTryNumber < 3) {  // фоторезистор должен дать низкое значение трижды
          lampTryNumber++;
        } else {
          lampTryNumber = 0;
          lampOn = true;
          pinMode(LAMP_PIN, OUTPUT);     // пин как выход
          digitalWrite(LAMP_PIN, HIGH);  // врубить
        }
      } else {
        lampTryNumber = 0;
      }
    }
  } else {
    if (lampTimer > LAMP_ON_PERIOD) {
      lampTimer = 0;
      lampOn = false;
      digitalWrite(LAMP_PIN, LOW);  // вырубить
      pinMode(LAMP_PIN, INPUT);     // пин как вход (экономия энергии)
      lampOffPeriod = LAMP_OFF_PERIOD;
    }
  }

  sleep_enable();  // разрешаем сон
  sleep_cpu();     // спать!
}

ISR(WDT_vect) {
  enableInterrupt();
}

void enableInterrupt() {
  WDTCR |= _BV(WDIE);  // разрешаем прерывания по ватчдогу. Иначе будет реcет.
}
