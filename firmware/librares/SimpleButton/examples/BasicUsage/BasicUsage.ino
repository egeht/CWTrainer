#include <SimpleButton.h>

SimpleButton myBtn;

void setup() {
  Serial.begin(9600);
  myBtn.begin(2); // Кнопка подключена к пину 2
}

void loop() {
  // Вызываем главный метод в каждом цикле
  if (myBtn.debounce()) {
    Serial.println("Кнопка нажата!");
  }

  if (myBtn.onRelease()) {
    Serial.println("Кнопка отпущена!");
  }
}
