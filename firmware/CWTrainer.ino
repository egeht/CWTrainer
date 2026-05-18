// CWTrainer
//
// by egeht exRA9MLF
// ver. 20260518
#define VER "0.92RC"
// 0.8 - замена обработки дребезга на бибоиотеку\
// 0.81 - реализация ямбик режима, вынов всего в функции
// 0.83 - как-то заработала, но похоже есть ошибки связанные с временем нажатия клавиш
// 0,84 - оставляем один опрос клавиш на цикл, реагируем только на устойчивые состояния, а не на события.
// 0.85 - замедлил опрос до один в 1 миллисекунду, почистил от старья. Работает в режиме ямбик А
// 0,86 - реализация режимов моде А и моде Б
// 0.87 - реализация режимов моде А и моде Б (отлажен)
// 0.88 - добавлен приямой ключ
// 0.89 - обработка нажатия кнопок управления меню
// 0.90 - сохранение настроек в eprom
// 0.91 - настройка программы
// 0.92 - RC

#include <SimpleButton.h>  // подавление дребезга
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);  // адрес, столбцов, строк

bool IfDebug = false;  // true - включить отладку
// bool IfDebug = true;  // true - включить отладку
bool IfDebug1 = false;
// bool IfDebug1 = true;
// Pin configuration

#define P_DOT 4       // D4 Connects to the dot lever of the paddle
#define P_DASH 5      // D5 Connects to the dash lever of the paddle
#define P_AUDIO 3     // D3 Speaker output
#define P_LCD_SDA A4  // LCD 1602 SDA
#define P_LCD_SCL A5  // LCD 1602 SCL
#define P_BTN A0      // Buttons

// microsecs in minute
#define US_PER_MIN 60000000

// 50 dot-long intervals in one "PARIS".
#define DOTS_PER_WORD 50

// логическое состояние ключа
#define DOT 1
#define DASH 2
#define NONE 3

// Режимы работы ямбик-ключа
#define IAMBIC_MODE_A 0  // классический режим A
#define IAMBIC_MODE_B 1  // режим B (с лишним элементом)
#define STRAIGHT_MODE 2  // вертикальный ключ

uint8_t iambicMode = IAMBIC_MODE_A;  // по умолчанию режим A
uint8_t pendingElement = NONE;       // для режима B: отложенный элемент после отпускания

float duration_dash = 3;        // длительность тире (в точках)
float pause_sign = 1;           // длительность паузы между сигналами (точка/тире)
float pause_letter = 3.0;       // длительность паузы между буквами
float pause_word = 7.0;         // длительность паузы между словами
uint16_t tone_frequency = 700;  // частота тона в герцах
uint8_t speed = 15;             // скорость передачи в словах в минуту

// длительности в единицах времени (ms - миллисекуды, us - микросекунды)
uint32_t stateTimerUs, duration_dot_us, duration_dash_us, pause_sign_us, pause_letter_us, pause_word_us;

uint8_t state;        // состояние автомата// состояния конечного автомата
#define state_Idle 1  // ничего не нажато, ждём paddle
#define state_Dot 3   // ключуем точка
#define state_Dash 4  // ключуем тире
#define state_Gap 5   // выдерживаем паузу после элемента

// уровень сигнала на пинах ключа
SimpleButton btnDot;   // true если точка нажата с подавлением дребезга
SimpleButton btnDash;  // true если тире нажата с подавлением дребезга

String morseLine = "";        // буфер точек/тире для показа на экране
String decodedLine = "";      // буфер расшифрованных букв для показа на экране
String morseBuffer = "";      // буфер одной буквы для рассшифровки
uint32_t silenceTimerUs = 0;  // таймер тишины с последнего элемента
bool decodingActive = false;  // флаг "идёт набор буквы"
bool wordSpaceAdded = false;  // флаг что слово закончено и пробел выведен

bool dotMemory = false;         // нажата точка
bool dashMemory = false;        // нажато тире
uint8_t currentElement = NONE;  // что сейчас звучит или только что звучало: DOT/DASH/NONE

bool dotIsPressed = false;  // устойчивое состояние: true - нажата, false - отпущена
bool dashIsPressed = false;
bool isToneOn;  // в режиме прямого ключа сигнал звучит

uint16_t BtnValue = 0;  // Состояние покоя кнопок

// Переменные меню
#define MENU_TIMEOUT 30000  // 30 секунд таймаут
#define MENU_OFF 0          // обычная работа ключа
#define MENU_SELECT 1       // выбор параметра (UP/DOWN перебираем параметры)
#define MENU_EDIT 2         // редактирование выбранного параметра (UP/DOWN меняем значение)
uint8_t menuState = 0;      // 0 - выкл, 1 - выбор, 2 - редактирование
uint8_t menuItem = 0;       // текущий пункт меню (0..6)
uint32_t menuTimer = 0;     // таймер для авто-выхода
bool menuNeedSave = false;  // флаг, что нужно сохранить настройки

// Названия пунктов меню (до 7 пунктов)
const char* menuItems[] = {
  "Speed", "Freq", "Dash", "Sign", "Letter", "Word", "Mode"
};
const uint8_t MENU_ITEMS_COUNT = 7;

// переменные которые нужно нестраивать и запоминать после изменения
// uint8_t iambicMode = IAMBIC_MODE_A;  // по умолчанию режим A
// float duration_dash = 3;        // длительность тире (в точках)
// float pause_sign = 1;           // длительность паузы между сигналами (точка/тире)
// float pause_letter = 3.0;       // длительность паузы между буквами
// float pause_word = 7.0;         // длительность паузы между словами
// uint16_t tone_frequency = 700;  // частота тока в герцах
// uint8_t speed = 15;             // скорость передачи в словах в минуту
// EEPROM адреса для хранения настроек
#define EE_IAMBIC_MODE 0
#define EE_DURATION_DASH 1
#define EE_PAUSE_SIGN 5  // float занимает 4 байта
#define EE_PAUSE_LETTER 9
#define EE_PAUSE_WORD 13
#define EE_TONE_FREQUENCY 17
#define EE_SPEED 19
#define EE_MAGIC_NUMBER 21  // для проверки, что EEPROM инициализирован

#define MAGIC_NUMBER_VAL 0xAB  // маркер, что настройки сохранены

// кнопки управления меню
#define BTN_NO 0
#define BTN_MENU 1
#define BTN_UP 2
#define BTN_DOWN 3
#define BTN_ENTER 4

//*****************************************************************************************
//******************************
// Initializing the Arduino
//******************************
//*****************************************************************************************
void setup() {

  pinMode(P_AUDIO, OUTPUT);
  pinMode(P_BTN, INPUT);

  btnDot.begin(P_DOT);
  btnDash.begin(P_DASH);

  lcd.init();  // инициализация экрана
  lcd.noCursor();
  lcd.noBlink();
  lcd.backlight();  // включить подсветку

  // вывод заставки и сигнала

  lcd.setCursor(2, 0);  // столбец 1 строка 0
  lcd.print("(c) exRA9MLF");

  lcd.setCursor(3, 1);  // столбец 4 строка 1
  lcd.print("ver.");
  lcd.setCursor(9, 1);  // столбец 4 строка 1
  lcd.print(VER);

  // выдаем сигнал готовности VVV
  for (int i = 0; i <= 2; i++) {
    tone(P_AUDIO, tone_frequency, 80);
    delay(160);
    tone(P_AUDIO, tone_frequency, 80);
    delay(160);
    tone(P_AUDIO, tone_frequency, 80);
    delay(160);
    tone(P_AUDIO, tone_frequency, 240);
    delay(320);
  }

  delay(2000);
  lcd.clear();

  // Загружаем настройки из EEPROM
  loadSettingsFromEEPROM();

  updateSpeed();  // устанавливаем текущие значения параметров передачи

  state = state_Idle;  // автомат в начальное состояние
  stateTimerUs = 0;
  decodedLine = "";
  morseLine = "";
  morseBuffer = "";
  dotMemory = false;
  dashMemory = false;
  currentElement = NONE;
  dotIsPressed = false;
  dashIsPressed = false;
  isToneOn = false;
  silenceTimerUs = micros();  // чтобы сразу не началась передача

  if (IfDebug1 || IfDebug) {
    // Initializing serial port for debugging purposes
    Serial.begin(115200);  // 115200
    delay(10);
    Serial.println();
    Serial.println("Start");
    Serial.print("Dot=");
    Serial.print(duration_dot_us);  // длительность точки
    Serial.print(",Dash=");
    Serial.print(duration_dash_us);
    Serial.print(",Sign=");
    Serial.print(pause_sign_us);
    Serial.print(",Letter=");
    Serial.print(pause_letter_us);
    Serial.print(",Word=");
    Serial.print(pause_word_us);
  }
}

//***********************************************************************************
//****************************************
// Main routine
//****************************************
//***********************************************************************************

void loop() {
  uint32_t gapUs;       // текущая длительность паузы
  uint8_t nextElement;  // следующий элемент для передачи


  uint8_t newBtnValue = GetBtnValue();  // Получаем актуальное состояние кнопок с коррекцией дребезга

  if (BtnValue != newBtnValue) {  // Срабатывает только при ИЗМЕНЕНИИ состояния
    BtnValue = newBtnValue;       // запоминаем новое значение

    // Обработка меню (если активно)
    if (menuState != MENU_OFF) {
      handleMenu(BtnValue);
      if (menuState == MENU_OFF) return;  // если вышли из меню — пропускаем передачу
    }
    // Вход в меню по кнопке MENU (только если меню не активно)
    else if (BtnValue == BTN_MENU) {
      menuState = MENU_SELECT;
      menuItem = 0;
      menuTimer = millis();
      noTone(P_AUDIO);
      showMenu();
      return;  // пропускаем цикл передачи
    }
  }

  // Если меню активно — ничего больше не делаем
  if (menuState != MENU_OFF) return;


  readPaddles();  // читаем клавиши CW paddle

  // ОТЛАДКА: выводим состояние каждые 100 мс (чтобы не заспамить)
  static uint32_t lastDebug = 0;

  if (iambicMode == STRAIGHT_MODE) {
    // прямой колюч, просто пищим

    if (dotIsPressed) {
      tone(P_AUDIO, tone_frequency);
      isToneOn = true;
    } else {
      if (isToneOn) {
        noTone(P_AUDIO);
        isToneOn = false;
      }
    }
    return;
  }

  switch (state) {
    case state_Idle:
      // Проверка, выдержана ли пауза после последнего элемента
      if (currentElement != NONE && (micros() - silenceTimerUs < pause_sign_us)) {
        break;
      }

      nextElement = chooseNextElement();

      if (nextElement == DOT) {
        consumeElement(DOT);
        currentElement = DOT;
        stateTimerUs = micros();
        tone(P_AUDIO, tone_frequency);
        addMorse('.');
        state = state_Dot;
      } else if (nextElement == DASH) {
        consumeElement(DASH);
        currentElement = DASH;
        stateTimerUs = micros();
        tone(P_AUDIO, tone_frequency);
        addMorse('-');
        state = state_Dash;
      }
      break;

    case state_Dot:
      if (micros() - stateTimerUs >= duration_dot_us) {
        noTone(P_AUDIO);
        silenceTimerUs = micros();
        state = state_Gap;
      }
      break;

    case state_Dash:
      if (micros() - stateTimerUs >= duration_dash_us) {
        noTone(P_AUDIO);
        silenceTimerUs = micros();
        state = state_Gap;
      }
      break;

    case state_Gap:
      gapUs = micros() - silenceTimerUs;
      // ... декодирование букв/слов (эту часть можно оставить без изменений) ...
      // Конец буквы
      if (gapUs >= pause_letter_us && decodingActive) {
        String decoded = decodeMorse(morseBuffer);
        addLetter(decoded);
        morseBuffer = "";
        decodingActive = false;
      }

      // Конец слова
      if (gapUs >= pause_word_us) {
        if (!wordSpaceAdded && decodedLine.length() > 0) {
          addLetter(" ");
          wordSpaceAdded = true;
        }
      }
      if (gapUs >= pause_sign_us) {
        nextElement = chooseNextElement();
        if (nextElement == DOT) {
          consumeElement(DOT);
          currentElement = DOT;
          stateTimerUs = micros();
          tone(P_AUDIO, tone_frequency);
          addMorse('.');
          state = state_Dot;
        } else if (nextElement == DASH) {
          consumeElement(DASH);
          currentElement = DASH;
          stateTimerUs = micros();
          tone(P_AUDIO, tone_frequency);
          addMorse('-');
          state = state_Dash;
        }
      }
      break;
  }
}


//*********************************
// функции
//*********************************

// расчет тайминга в микросекундах по скорости в wpm и коэффициентов длительности
void updateSpeed() {
  duration_dot_us = US_PER_MIN / (DOTS_PER_WORD * speed);  // длительность точки
  duration_dash_us = duration_dot_us * duration_dash;      // длительность тире
  pause_sign_us = duration_dot_us * pause_sign;            // длительность паузы между знаками
  pause_letter_us = duration_dot_us * pause_letter;        // длительность паузы между буквами
  pause_word_us = duration_dot_us * pause_word;            // длительность паузы между словами
}

// Функция чтения paddle
// обновляет подтверждённые состояния двух лепестков
void readPaddles() {
  // Обновляем стабильное состояние каждой клавиши
  // isHeld() == true  -> кнопка нажата и стабильна
  // isIdle()  == true -> кнопка отпущена и стабильна
  // Эти два метода взаимно исключают друг друга (если не между состояниями)

  static uint32_t lastRead = 0;
  if (micros() - lastRead < 1000) return;  // читаем не чаще 1 раза в 1 мс
  lastRead = micros();

  bool oldDotPressed = dotIsPressed;
  bool oldDashPressed = dashIsPressed;

  dotIsPressed = btnDot.isHeld();
  dashIsPressed = btnDash.isHeld();

  // Если нужна дополнительная защита: если isIdle() истинно, то точно не нажата
  if (btnDot.isIdle()) dotIsPressed = false;
  if (btnDash.isIdle()) dashIsPressed = false;

  // Однократные события нажатия (фронт)
  bool dotJustPressed = btnDot.debounce();
  bool dashJustPressed = btnDash.debounce();

  // События отпускания (фронт вверх)
  bool dotJustReleased = btnDot.onRelease();
  bool dashJustReleased = btnDash.onRelease();

  // Теперь эти события можно использовать для установки флагов памяти
  if (dotJustPressed) dotMemory = true;
  if (dashJustPressed) dashMemory = true;

  // === ЛОГИКА ДЛЯ РЕЖИМА B ===
  if (iambicMode == IAMBIC_MODE_B) {
    static bool bothWerePressed = false;
    static uint32_t bothLostTime = 0;

    // Если обе нажаты - запоминаем
    if (dotIsPressed && dashIsPressed) {
      bothWerePressed = true;
      bothLostTime = 0;
    }

    // Если обе отпущены и до этого были нажаты
    if (bothWerePressed && !dotIsPressed && !dashIsPressed) {
      if (currentElement != NONE) {
        pendingElement = (currentElement == DOT) ? DASH : DOT;
      }
      bothWerePressed = false;
    }

    // Таймаут: если прошло 100 мс, а вторая кнопка так и не нажата - сбрасываем
    if (bothWerePressed && !(dotIsPressed && dashIsPressed)) {
      if (bothLostTime == 0) bothLostTime = millis();
      if (millis() - bothLostTime > 100) {
        bothWerePressed = false;
        bothLostTime = 0;
      }
    }
  }
}

// Функция выбора следующего элемента
// Если обе памяти установлены, следующий элемент — противоположный только что сыгранному.
// Это и даёт нормальное iambic-чередование.
uint8_t chooseNextElement() {
  // Для режима B: отложенный элемент имеет высший приоритет
  if (iambicMode == IAMBIC_MODE_B && pendingElement != NONE) {
    uint8_t result = pendingElement;
    pendingElement = NONE;
    return result;
  }

  uint8_t result = NONE;

  // 1. Удерживается только точка
  if (dotIsPressed && !dashIsPressed) {
    result = DOT;
  }
  // 2. Удерживается только тире
  else if (!dotIsPressed && dashIsPressed) {
    result = DASH;
  }
  // 3. Удерживаются обе (squeeze) -> чередование
  else if (dotIsPressed && dashIsPressed) {
    if (currentElement == DOT) result = DASH;
    else if (currentElement == DASH) result = DOT;
    else result = DOT;
  }
  // 4. Обе отпущены, но что-то осталось в памяти
  else if (dotMemory && dashMemory) {
    if (currentElement == DOT) result = DASH;
    else if (currentElement == DASH) result = DOT;
    else result = DOT;
  } else if (dotMemory) result = DOT;
  else if (dashMemory) result = DASH;

  return result;
}

// Функция сброса использованной памяти
// Если запускаем точку, очищаем только dotMemory; если тире — только dashMemory.
// Вторая память может остаться и дать следующий элемент.
void consumeElement(uint8_t el) {
  if (el == DOT) {
    dotMemory = false;
  } else if (el == DASH) {
    dashMemory = false;
  }

  // Для режима B: если начали отложенный элемент, сбрасываем
  if (iambicMode == IAMBIC_MODE_B && pendingElement == el) {
    pendingElement = NONE;
  }
}

void addMorse(char dotOrDash) {
  //полученный знак выводим на экран и добавляем в буфер для последющей расшифровки.
  decodingActive = true;
  morseLine += dotOrDash;
  morseBuffer += dotOrDash;
  wordSpaceAdded = false;
  // если строка заполнена, сдвигаем влево
  if (morseLine.length() > 16) morseLine = morseLine.substring(1);  // сдвиг
}

void addLetter(String symbol) {
  // добавляем символ в буквенную строку и выводим на экран
  decodedLine += symbol;
  if (decodedLine.length() > 16) decodedLine = decodedLine.substring(decodedLine.length() - 16);
  lcd.setCursor(0, 0);
  lcd.print(decodedLine + "                ");
  morseLine += ' ';
  if (morseLine.length() > 16) morseLine = morseLine.substring(1);  // сдвиг
  lcd.setCursor(0, 1);
  lcd.print(morseLine + "                ");  // новая + пробелы
}

// декодируем набранный символ
String decodeMorse(String morseCode) {
  if (morseCode == ".-") return "A";
  if (morseCode == "-...") return "B";
  if (morseCode == "-.-.") return "C";
  if (morseCode == "-..") return "D";
  if (morseCode == ".") return "E";
  if (morseCode == "..-.") return "F";
  if (morseCode == "--.") return "G";
  if (morseCode == "....") return "H";
  if (morseCode == "..") return "I";
  if (morseCode == ".---") return "J";
  if (morseCode == "-.-") return "K";  // общее приглашение к передаче
  if (morseCode == ".-..") return "L";
  if (morseCode == "--") return "M";
  if (morseCode == "-.") return "N";
  if (morseCode == "---") return "O";
  if (morseCode == ".--.") return "P";
  if (morseCode == "--.-") return "Q";
  if (morseCode == ".-.") return "R";
  if (morseCode == "...") return "S";
  if (morseCode == "-") return "T";
  if (morseCode == "..-") return "U";
  if (morseCode == "...-") return "V";
  if (morseCode == ".--") return "W";
  if (morseCode == "-..-") return "X";
  if (morseCode == "-.--") return "Y";
  if (morseCode == "--..") return "Z";

  if (morseCode == "-----") return "0";
  if (morseCode == ".----") return "1";
  if (morseCode == "..---") return "2";
  if (morseCode == "...--") return "3";
  if (morseCode == "....-") return "4";
  if (morseCode == ".....") return "5";
  if (morseCode == "-....") return "6";
  if (morseCode == "--...") return "7";
  if (morseCode == "---..") return "8";
  if (morseCode == "----.") return "9";

  if (morseCode == ".-.-.-") return ",";
  if (morseCode == "--..--") return ".";
  if (morseCode == "-.-.--") return "!";
  if (morseCode == "..---..") return "?";
  if (morseCode == "-..-.") return "/";

  if (morseCode == ".-.-.") return "AR";
  if (morseCode == "...-.-") return "SK";  // конец работы
  if (morseCode == "-...-") return "=";    // BT -
  if (morseCode == "-.--.") return "KN";
  if (morseCode == "-.-.-") return "KA";
  if (morseCode == "...-...-...-") return "VVV";  // начало передачи

  return "*";
}

uint8_t GetBtnValue() {  // Функция устраняющая дребезг
  static int count;
  static uint8_t oldBtn;  // Переменная для хранения предыдущего значения состояния кнопок
  static uint8_t innerBtn;
  static uint8_t actualBtn;
  uint16_t actualBtnValue = analogRead(P_BTN);  // Получаем актуальное состояние

  // настроить значения здесь под ваши резисторы в блоке кнопок
  if (actualBtnValue <= 128) {
    actualBtn = BTN_NO;
  } else if (actualBtnValue <= 384) {
    actualBtn = BTN_MENU;
  } else if (actualBtnValue <= 640) {
    actualBtn = BTN_UP;
  } else if (actualBtnValue <= 896) {
    actualBtn = BTN_DOWN;
  } else {
    actualBtn = BTN_ENTER;
  }

  if (innerBtn != actualBtn) {  // Пришло значение отличное от предыдущего
    count = 0;                  // Все обнуляем и начинаем считать заново
    innerBtn = actualBtn;       // Запоминаем новое значение
  } else {
    count += 1;  // Увеличиваем счетчик
  }

  if ((count >= 10) && (actualBtn != oldBtn)) {  // Счетчик преодолел барьер, можно иницировать смену состояний
    oldBtn = actualBtn;                          // Присваиваем новое значение
  }
  return oldBtn;
}

//*****************************************************************************************
// Функции работы с EEPROM
//*****************************************************************************************

// Сохранение всех настроек в EEPROM
void saveSettingsToEEPROM() {
  EEPROM.put(EE_IAMBIC_MODE, iambicMode);
  EEPROM.put(EE_DURATION_DASH, duration_dash);
  EEPROM.put(EE_PAUSE_SIGN, pause_sign);
  EEPROM.put(EE_PAUSE_LETTER, pause_letter);
  EEPROM.put(EE_PAUSE_WORD, pause_word);
  EEPROM.put(EE_TONE_FREQUENCY, tone_frequency);
  EEPROM.put(EE_SPEED, speed);
  EEPROM.put(EE_MAGIC_NUMBER, MAGIC_NUMBER_VAL);

  if (IfDebug) {
    Serial.println("Settings saved to EEPROM");
  }
}

// Загрузка настроек из EEPROM
void loadSettingsFromEEPROM() {
  // Проверяем, были ли сохранены настройки ранее
  uint8_t magicNumber;
  EEPROM.get(EE_MAGIC_NUMBER, magicNumber);

  if (magicNumber == MAGIC_NUMBER_VAL) {
    // Настройки есть, загружаем их
    EEPROM.get(EE_IAMBIC_MODE, iambicMode);
    EEPROM.get(EE_DURATION_DASH, duration_dash);
    EEPROM.get(EE_PAUSE_SIGN, pause_sign);
    EEPROM.get(EE_PAUSE_LETTER, pause_letter);
    EEPROM.get(EE_PAUSE_WORD, pause_word);
    EEPROM.get(EE_TONE_FREQUENCY, tone_frequency);
    EEPROM.get(EE_SPEED, speed);

    // Проверка на валидность загруженных значений
    if (iambicMode > STRAIGHT_MODE) iambicMode = IAMBIC_MODE_A;
    if (duration_dash < 2.0 || duration_dash > 4.0) duration_dash = 3.0;
    if (pause_sign < 0.5 || pause_sign > 2.0) pause_sign = 1.0;
    if (pause_letter < 2.0 || pause_letter > 5.0) pause_letter = 3.0;
    if (pause_word < 5.0 || pause_word > 10.0) pause_word = 7.0;
    if (tone_frequency < 200 || tone_frequency > 2000) tone_frequency = 700;
    if (speed < 5 || speed > 50) speed = 15;

    if (IfDebug) {
      Serial.println("Settings loaded from EEPROM");
    }
  } else {
    // Настроек нет, сохраняем текущие значения по умолчанию
    saveSettingsToEEPROM();
  }
}

// Сброс настроек к значениям по умолчанию
void resetSettingsToDefault() {
  iambicMode = IAMBIC_MODE_A;
  duration_dash = 3.0;
  pause_sign = 1.0;
  pause_letter = 3.0;
  pause_word = 7.0;
  tone_frequency = 700;
  speed = 15;

  saveSettingsToEEPROM();
  updateSpeed();
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Работа с меню
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Отображение меню выбора параметра
void showMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Set: ");
  lcd.print(menuItems[menuItem]);

  lcd.setCursor(0, 1);
  printMenuItemValue(menuItem);
}

// Вывод значения параметра на экран
void printMenuItemValue(uint8_t item) {
  switch (item) {
    case 0:  // Speed
      lcd.print("                ");
      lcd.setCursor(0, 1);
      lcd.print(speed);
      lcd.print(" wpm     ");
      break;
    case 1:  // Freq
      lcd.print("                ");
      lcd.setCursor(0, 1);
      lcd.print(tone_frequency);
      lcd.print(" Hz      ");
      break;
    case 2:  // Dash
      lcd.print("                ");
      lcd.setCursor(0, 1);
      lcd.print("x");
      lcd.print(duration_dash, 1);
      lcd.print("       ");
      break;
    case 3:  // Sign pause
      lcd.print("                ");
      lcd.setCursor(0, 1);
      lcd.print("x");
      lcd.print(pause_sign, 1);
      lcd.print("       ");
      break;
    case 4:  // Letter pause
      lcd.print("                ");
      lcd.setCursor(0, 1);
      lcd.print("x");
      lcd.print(pause_letter, 1);
      lcd.print("       ");
      break;
    case 5:  // Word pause
      lcd.print("                ");
      lcd.setCursor(0, 1);
      lcd.print("x");
      lcd.print(pause_word, 1);
      lcd.print("       ");
      break;
    case 6:  // Mode
      lcd.print("                ");
      lcd.setCursor(0, 1);
      if (iambicMode == IAMBIC_MODE_A) lcd.print("Iambic A");
      else if (iambicMode == IAMBIC_MODE_B) lcd.print("Iambic B");
      else lcd.print("Straight");
      break;
  }
}

// Отображение режима редактирования
void showEdit() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(menuItems[menuItem]);
  lcd.print(":");

  lcd.setCursor(0, 1);
  printMenuItemValue(menuItem);
  lcd.setCursor(0, 1);
  lcd.cursor();  // показываем курсор для редактирования
}

// Обработка действий в меню
void handleMenu(uint8_t btn) {
  // Таймаут
  if (millis() - menuTimer > MENU_TIMEOUT) {
    exitMenu();
    return;
  }

  if (btn == BTN_NO) return;  // ничего не нажато

  menuTimer = millis();  // сброс таймера при любой кнопке

  if (menuState == MENU_SELECT) {
    switch (btn) {
      case BTN_UP:
        menuItem = (menuItem == 0) ? MENU_ITEMS_COUNT - 1 : menuItem - 1;
        showMenu();
        break;
      case BTN_DOWN:
        menuItem = (menuItem + 1) % MENU_ITEMS_COUNT;
        showMenu();
        break;
      case BTN_ENTER:
        menuState = MENU_EDIT;
        showEdit();
        break;
      case BTN_MENU:
        exitMenu();
        break;
    }
  } else if (menuState == MENU_EDIT) {
    switch (btn) {
      case BTN_UP:
        changeValue(true);
        showEdit();
        break;
      case BTN_DOWN:
        changeValue(false);
        showEdit();
        break;
      case BTN_ENTER:
        // Сохраняем и возвращаемся в выбор
        menuNeedSave = true;
        menuState = MENU_SELECT;
        lcd.noCursor();
        showMenu();
        break;
      case BTN_MENU:
        // Отмена редактирования, возврат в выбор (без сохранения?)
        // Или сохраняем? Давайте сохранять при возврате
        menuNeedSave = true;
        menuState = MENU_SELECT;
        lcd.noCursor();
        showMenu();
        break;
    }
  }
}

// Изменение значения параметра
void changeValue(bool increment) {
  switch (menuItem) {
    case 0:  // Speed
      if (increment && speed < 50) speed++;
      else if (!increment && speed > 5) speed--;
      updateSpeed();
      break;
    case 1:  // Freq
      if (increment && tone_frequency < 2000) tone_frequency += 10;
      else if (!increment && tone_frequency > 200) tone_frequency -= 10;
      break;
    case 2:  // Dash
      if (increment && duration_dash < 4.0) duration_dash += 0.1;
      else if (!increment && duration_dash > 2.0) duration_dash -= 0.1;
      updateSpeed();
      break;
    case 3:  // Sign pause
      if (increment && pause_sign < 2.0) pause_sign += 0.1;
      else if (!increment && pause_sign > 0.5) pause_sign -= 0.1;
      updateSpeed();
      break;
    case 4:  // Letter pause
      if (increment && pause_letter < 5.0) pause_letter += 0.1;
      else if (!increment && pause_letter > 2.0) pause_letter -= 0.1;
      updateSpeed();
      break;
    case 5:  // Word pause
      if (increment && pause_word < 10.0) pause_word += 0.1;
      else if (!increment && pause_word > 5.0) pause_word -= 0.1;
      updateSpeed();
      break;
    case 6:  // Mode
      if (increment) {
        iambicMode = (iambicMode + 1);
        if (iambicMode > 2) iambicMode = 0;
      } else {
        if (iambicMode == 0) iambicMode = 2;
        else iambicMode = iambicMode - 1;
      }
      break;
  }
}

// Выход из меню
void exitMenu() {
  if (menuNeedSave) {
    saveSettingsToEEPROM();
    menuNeedSave = false;
  }
  menuState = 0;
  lcd.noCursor();
  lcd.clear();
  state = state_Idle;
  stateTimerUs = micros();
  silenceTimerUs = micros();
  // Восстанавливаем отображение, если было
  if (decodedLine.length() > 0 || morseLine.length() > 0) {
    lcd.setCursor(0, 0);
    lcd.print(decodedLine + "                ");
    lcd.setCursor(0, 1);
    lcd.print(morseLine + "                ");
  }
}
