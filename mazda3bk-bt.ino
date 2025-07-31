/*
  Автор Андрей Юрлов
  Дата 04.05.2025 16:22
  https://www.drive2.ru/r/mazda/3/686092782904809284/

  Программа Arduino для управления CSR8645 BT модулем, 
  которая анализирует напряжение на аналоговом входе (детект нажатой кнопки на руле)
  и управляет цифровыми выходами согласно заданным условиям.
  
  Условия:
  - Если напряжение 3.85-4.05В держится >3000мс -> включаем POWER_EN_PIN (reset CSR8645 BT module)
  - Если напряжение 2.52-2.72В держится  >300мс -> включаем PREV_PIN  (кнопка PREV CSR8645 BT module)
  - Если напряжение 1.78-1.98В держится  >300мс -> включаем NEXT_PIN  (кнопка NEXT CSR8645 BT module)
  - При старте системы на POWER_EN_PIN подаётся импульс 3000мс (reset CSR8645 BT module)

  Замеры (DEBUG 1, J701 ST-SW1 белый с черной полосой):
  - No   = 4.38В
  - Mute = 3.95В //Check
  - Next = 1.88В //Check
  - Prev = 2.62В //Check
  - Vol+ = 0.98В
  - Vol- = 0.41В
  - Mode = 3.13В
*/

#define DEBUG 0
#define NUM_READINGS 5 // Количество измерений для усреднения

// ========== ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ И КОНСТАНТЫ ==========

// Пороговые значения напряжения (в вольтах)
const float VOLTAGE_LOW_1  = 3.85;  // Нижний порог 1-го диапазона
const float VOLTAGE_HIGH_1 = 4.05;  // Верхний порог 1-го диапазона
const float VOLTAGE_LOW_2  = 2.52;  // Нижний порог 2-го диапазона
const float VOLTAGE_HIGH_2 = 2.72;  // Верхний порог 2-го диапазона
const float VOLTAGE_LOW_3  = 1.78;  // Нижний порог 3-го диапазона
const float VOLTAGE_HIGH_3 = 1.98;  // Верхний порог 3-го диапазона

// Временные параметры (в миллисекундах)
const unsigned long DURATION_THRESHOLD_1 = 3000; // Длительное время удержания напряжения MUTE для сброса BT модуля
const unsigned long DURATION_THRESHOLD_2 = 300;  // Минимальное время удержания напряжения
const unsigned long INITIAL_DELAY        = 5000; // Длительность импульса на POWER_EN_PIN при старте

// Назначение пинов Arduino
const int BUTTONS_PIN    = A5;  // Аналоговый вход для измерения напряжения
const int START_STOP_PIN = 5;   // Цифровой выход 1 для кнопки MUTE
const int PREV_PIN       = 4;   // Цифровой выход 2 для кнопки PREV
const int NEXT_PIN       = 3;   // Цифровой выход 3 для кнопки NEXT
const int POWER_EN_PIN   = 6;   // Цифровой выход 4 для сброса BT модуля

// Переменные для отслеживания времени удержания напряжения
unsigned long startTime1 = 0;  // Таймер для 1-го диапазона
unsigned long startTime2 = 0;  // Таймер для 2-го диапазона
unsigned long startTime3 = 0;  // Таймер для 3-го диапазона

// Флаги активности условий
bool condition1Active = false;  // Флаг активности 1-го диапазона
bool condition2Active = false;  // Флаг активности 2-го диапазона
bool condition3Active = false;  // Флаг активности 3-го диапазона

// Настройки фильтра
float readings[NUM_READINGS]; // Массив для хранения измерений
int readIndex = 0;            // Текущий индекс в массиве
float filteredVoltage = 0;    // Отфильтрованное напряжение

// ========== ФУНКЦИЯ НАСТРОЙКИ ==========
void setup() {
  // Настройка внутреннего напряжение AVCC к выводу AREF
  analogReference(DEFAULT);

  // Настройка цифровых пинов как выходов
  pinMode(START_STOP_PIN,  OUTPUT);
  pinMode(PREV_PIN,  OUTPUT);
  pinMode(NEXT_PIN,  OUTPUT);
  pinMode(POWER_EN_PIN, OUTPUT);
  
  // Инициализация массива нулями
  for (int i = 0; i < NUM_READINGS; i++) {
    readings[i] = 0;
  }

#if DEBUG
  // Инициализация последовательного порта для отладки
  Serial.begin(9600);

  // Вывод в монитор порта для отладки
  Serial.println("BT module is resetting...");
#endif
  
  // При старте подаём импульс на POWER_EN_PIN длительностью INITIAL_DELAY
  digitalWrite(POWER_EN_PIN, LOW);
  delay(INITIAL_DELAY);
  digitalWrite(POWER_EN_PIN, HIGH);

#if DEBUG
  // Вывод в монитор порта для отладки
  Serial.println("BT module is booting...");
#endif
}

// ========== ОСНОВНОЙ ЦИКЛ ПРОГРАММЫ ==========
void loop() {
  // Чтение напряжения с аналогового входа и преобразование в вольты
  readVoltage();
  
  // Проверка всех условий по диапазонам напряжения
  checkCondition1();  // Проверка MUTE
  checkCondition2();  // Проверка PREV
  checkCondition3();  // Проверка NEXT
  
  // Небольшая задержка для стабильности системы
  delay(10);
}

// ========== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ==========
void readVoltage()
{
  // Считываем новое значение и добавляем в массив
  readings[readIndex] = (float) (analogRead(BUTTONS_PIN) * 5.0) / 1024;
  readIndex = (readIndex + 1) % NUM_READINGS;

  // Вычисляем среднее значение напряжения
  filteredVoltage = 0;
  for (int i = 0; i < NUM_READINGS; i++) {
    filteredVoltage += readings[i];
  }
  filteredVoltage /= NUM_READINGS;

#if DEBUG
  // Вывод в монитор порта для отладки
  Serial.print("Filtered: ");
  Serial.println(filteredVoltage, 3);
#endif
}

// Проверка условия для диапазона MUTE
void checkCondition1() {
  if (filteredVoltage >= VOLTAGE_LOW_1 && filteredVoltage <= VOLTAGE_HIGH_1) {
    // Напряжение в нужном диапазоне
    if (!condition1Active) {
      // Если условие ещё не активно
      if (startTime1 == 0) {
        // Первое попадание в диапазон - запоминаем время
        startTime1 = millis();
#if DEBUG
        Serial.println("Starting point for POWER_EN_PIN");
#endif
      } else if (millis() - startTime1 >= DURATION_THRESHOLD_1) {
        // Условие держится дольше 3000 мс - активируем выход
        condition1Active = true;
        digitalWrite(POWER_EN_PIN, LOW);
#if DEBUG
        Serial.println("Output POWER_EN_PIN is low");
#endif
        delay(DURATION_THRESHOLD_2);
      }
    }
  } else {
    // Напряжение вышло из диапазона
    if (condition1Active) {
      // Если выход был активен - выключаем его
      digitalWrite(POWER_EN_PIN, HIGH);
#if DEBUG
      Serial.println("Output POWER_EN_PIN is high");
#endif
    }
    // Сбрасываем флаги и таймер
    condition1Active = false;
    startTime1 = 0;
  }
}

// Проверка условия для диапазона PREV
void checkCondition2() {
  if (filteredVoltage >= VOLTAGE_LOW_2 && filteredVoltage <= VOLTAGE_HIGH_2) {
    if (!condition2Active) {
      if (startTime2 == 0) {
        startTime2 = millis();
#if DEBUG
        Serial.println("Starting point for PREV_PIN");
#endif
      } else if (millis() - startTime2 >= DURATION_THRESHOLD_2) {
        condition2Active = true;
        digitalWrite(PREV_PIN, HIGH);
#if DEBUG
        Serial.println("Output PREV_PIN is high");
#endif
        delay(DURATION_THRESHOLD_2);
      }
    }
  } else {
    if (condition2Active) {
      digitalWrite(PREV_PIN, LOW);
#if DEBUG
      Serial.println("Output PREV_PIN is low");
#endif
    }
    condition2Active = false;
    startTime2 = 0;
  }
}

// Проверка условия для диапазона NEXT
void checkCondition3() {
  if (filteredVoltage >= VOLTAGE_LOW_3 && filteredVoltage <= VOLTAGE_HIGH_3) {
    if (!condition3Active) {
      if (startTime3 == 0) {
        startTime3 = millis();
#if DEBUG
        Serial.println("Starting point for NEXT_PIN");
#endif
      } else if (millis() - startTime3 >= DURATION_THRESHOLD_2) {
        condition3Active = true;
        digitalWrite(NEXT_PIN, HIGH);
#if DEBUG
        Serial.println("Output NEXT_PIN is high");
#endif
        delay(DURATION_THRESHOLD_2);
      }
    }
  } else {
    if (condition3Active) {
      digitalWrite(NEXT_PIN, LOW);
#if DEBUG
      Serial.println("Output NEXT_PIN is low");
#endif
    }
    condition3Active = false;
    startTime3 = 0;
  }
}
