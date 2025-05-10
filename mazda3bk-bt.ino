/*
  Автор Андрей Юрлов
  Дата 04.05.2025 16:22
  https://www.drive2.ru/r/mazda/3/686092782904809284/

  Программа Arduino для управления CSR8645 BT модулем, 
  которая анализирует напряжение на аналоговом входе (детект нажатой кнопки на руле)
  и управляет цифровыми выходами согласно заданным условиям.
  
  Условия:
  - Если напряжение 3.70-3.90В держится  >300мс -> включаем MUTE_PIN  (кнопка MUTE CSR8645 BT module)
  - Если напряжение 3.70-3.90В держится >3000мс -> включаем RESET_PIN (reset CSR8645 BT module)
  - Если напряжение 2.30-2.50В держится  >300мс -> включаем PREV_PIN  (кнопка PREV CSR8645 BT module)
  - Если напряжение 1.50-1.80В держится  >300мс -> включаем NEXT_PIN  (кнопка NEXT CSR8645 BT module)
  - При старте системы на RESET_PIN подаётся импульс 3000мс (reset CSR8645 BT module)

  Замеры (DEBUG 1, J701 ST-SW1 белый с черной полосой):
  - No   = 4.38В
  - Mute = 3.80В
  - Next = 1.67В
  - Prev = 2.40В
  - Vol+ = 0.98В
  - Vol- = 0.41В
  - Mode = 3.13В
*/

#define DEBUG 0

// ========== ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ И КОНСТАНТЫ ==========

// Пороговые значения напряжения (в вольтах)
const float VOLTAGE_LOW_1  = 3.70;  // Нижний порог 1-го диапазона
const float VOLTAGE_HIGH_1 = 3.90;  // Верхний порог 1-го диапазона
const float VOLTAGE_LOW_2  = 2.30;  // Нижний порог 2-го диапазона
const float VOLTAGE_HIGH_2 = 2.50;  // Верхний порог 2-го диапазона
const float VOLTAGE_LOW_3  = 1.50;  // Нижний порог 3-го диапазона
const float VOLTAGE_HIGH_3 = 1.80;  // Верхний порог 3-го диапазона

// Временные параметры (в миллисекундах)
const unsigned long DURATION_THRESHOLD_1 = 300;  // Минимальное время удержания напряжения
const unsigned long DURATION_THRESHOLD_2 = 3000; // Длительное время удержания напряжения MUTE для сброса BT модуля
const unsigned long INITIAL_DELAY        = 3000; // Длительность импульса на D4 при старте

// Назначение пинов Arduino
const int SIGNAL_PIN = A0;  // Аналоговый вход для измерения напряжения
const int MUTE_PIN   = 3;   // Цифровой выход 1 для кнопки MUTE
const int PREV_PIN   = 4;   // Цифровой выход 2 для кнопки PREV
const int NEXT_PIN   = 5;   // Цифровой выход 3 для кнопки NEXT
const int RESET_PIN  = 6;  // Цифровой выход 4 для сброса BT модуля

// Переменные для отслеживания времени удержания напряжения
unsigned long startTime1 = 0;  // Таймер для 1-го диапазона
unsigned long startTime2 = 0;  // Таймер для 2-го диапазона
unsigned long startTime3 = 0;  // Таймер для 3-го диапазона

// Флаги активности условий
bool condition1Active = false;  // Флаг активности 1-го диапазона
bool condition2Active = false;  // Флаг активности 2-го диапазона
bool condition3Active = false;  // Флаг активности 3-го диапазона

// Настройки фильтра
const int numReadings = 10;   // Количество измерений для усреднения
float readings[numReadings];  // Массив для хранения измерений
int readIndex = 0;            // Текущий индекс в массиве
float filteredVoltage = 0;    // Отфильтрованное напряжение

// ========== ФУНКЦИЯ НАСТРОЙКИ ==========
void setup() {
  // Настройка внутреннего напряжение AVCC к выводу AREF
  analogReference(DEFAULT);

  // Настройка цифровых пинов как выходов
  pinMode(MUTE_PIN,  OUTPUT);
  pinMode(PREV_PIN,  OUTPUT);
  pinMode(NEXT_PIN,  OUTPUT);
  pinMode(RESET_PIN, OUTPUT);
  
  // Инициализация массива нулями
  for (int i = 0; i < numReadings; i++) {
    readings[i] = 0;
  }

#if DEBUG
  // Инициализация последовательного порта для отладки
  Serial.begin(9600);

  // Вывод в монитор порта для отладки
  Serial.println("BT module is resetting...");
#endif
  
  // При старте подаём импульс на RESET_PIN длительностью 3000 мс
  digitalWrite(RESET_PIN, HIGH);
  delay(INITIAL_DELAY);
  digitalWrite(RESET_PIN, LOW);

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
  delay(20);
}

// ========== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ==========
void readVoltage()
{
  // Считываем новое значение и добавляем в массив
  readings[readIndex] = (float) (analogRead(SIGNAL_PIN) * 5.0) / 1024;
  readIndex = (readIndex + 1) % numReadings;

  // Вычисляем среднее значение напряжения
  filteredVoltage = 0;
  for (int i = 0; i < numReadings; i++) {
    filteredVoltage += readings[i];
  }
  filteredVoltage /= numReadings;

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
        Serial.println("Starting point for MUTE_PIN");
#endif
      } else if (millis() - startTime1 >= DURATION_THRESHOLD_1) {
        // Условие держится дольше 300 мс - активируем выход
        condition1Active = true;
        digitalWrite(MUTE_PIN, HIGH);
#if DEBUG
        Serial.println("Output MUTE_PIN is high");
#endif
      } else if (millis() - startTime1 >= DURATION_THRESHOLD_2) {
        // Условие держится дольше 3000 мс - активируем выход
        condition1Active = true;
        digitalWrite(RESET_PIN, HIGH);
#if DEBUG
        Serial.println("Output RESET_PIN is high");
#endif
      }
    }
  } else {
    // Напряжение вышло из диапазона
    if (condition1Active) {
      // Если выход был активен - выключаем его
      digitalWrite(MUTE_PIN,  LOW);
      digitalWrite(RESET_PIN, LOW);
#if DEBUG
      Serial.println("Output MUTE_PIN/RESET_PIN is low");
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
      } else if (millis() - startTime2 >= DURATION_THRESHOLD_1) {
        condition2Active = true;
        digitalWrite(PREV_PIN, HIGH);
#if DEBUG
        Serial.println("Output PREV_PIN is high");
#endif
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
      } else if (millis() - startTime3 >= DURATION_THRESHOLD_1) {
        condition3Active = true;
        digitalWrite(NEXT_PIN, HIGH);
#if DEBUG
        Serial.println("Output NEXT_PIN is high");
#endif
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
