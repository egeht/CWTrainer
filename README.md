# CWTrainer

Arduino nano CW electronic key/trainer

I needed an electronic CW key to learn Morse code (for practicing transmission). I came across an old key project from a radio amateur with the call sign PA3HCM (https://www.pa3hcm.nl/?p=1010). I used it as a basis. In the process, I modified the program so much that nothing remained of the original. :-)

The circuit diagram is almost unchanged (there's not much to change), I just replaced the board with an Arduino nano. To output sound to a speaker or headphones, I added a PAM8403 amplifier and a volume control. Just in case, I connected it to a transceiver, but I don't know why. If desired, you can plug a 3.5mm jack with an LED soldered onto it and see the light signals. I added four buttons to control the key setup menu. I also designed the enclosure.

What it can do:
- Works in iambic modes A and B.
- Works with a classic straight key.
When working with an iambic key, it displays the transmitted dots/dashes (on the bottom line) and the transmitted character (on the top line). This allows you to monitor the transmission.

The following parameters are configurable and stored in non-volatile memory:
- speed in WPM
- dash duration
- pause duration between signals
- pause duration between characters
- pause duration between words
All durations are entered in dot durations.

A printed circuit board (PCB) with a connector for the Arduino and a board with an audio amplifier and a volume control resistor is mounted on the bottom of the case (bottom step). The LCD1602 screen, a PCB with buttons, three connectors, and a speaker are mounted on the top of the case (top step).

I won't give any details on the components and layout, as everyone will have to use what they have on hand for this project.

This isn't a full-fledged simulator; it doesn't give you any tasks. And it doesn't check whether you've typed the right thing. It only shows what you've entered. But that's all I needed. :-)

You can see an example of it here - https://youtube.com/shorts/XV8_UpXhKHY?feature=share

# CWTrainer

Arduino nano CW электронный ключ/тренажер
 
Мне понадобился электронный CW ключ для изучения азбуки Морзе (для тренировки передачи). На глаза попался старый проект ключа от радиолюбителя с позывным PA3HCM (https://www.pa3hcm.nl/?p=1010). Взял его за основу. в процессе доработал программу так, что от оригинала ничего не осталось. :-)

Схема почти не поменялась (там и менять-то особенно не чего), только заменил платку на Arduino nano. Что бы можно было выводить звук на динамик или наушники добавил УНЧ на PAM8403 и регулятор громкости. На всякий случай сделал вывод на трансивер, не знаю зачем. При желании можно в него воткнуть джек 3.5 с припаянным светодиодом и видеть световые сигналы. Добавил четыре кнопки для управления меню настройки ключа. Нарисовал корпус.

Что умеет:
- Работает в ямбических режимах А и Б.
- Работает с классическим прямым ключом.
При работе с ямбическим ключом выводит на экран переданные точки/тире (в нижней строке) и переданный знак (в верхней строке). Таким образом можно контролировать передачу.

Настраивается и сохраняется в энергонезависимой памяти:
- скорость в WPM
- длительность тире
- длительность паузы между сигналами
- длительность паузы между символами
- длительность паузы между словами
Все длительности вводятся в длительностях точки.

На нижней части корпуса (bottom.step) крепится печатная (монтажная) плата с разъемом для Ардуино и платка с УНЧ и резистором регулятора громкости. На верхней части корпуса (top.step) экран LCD1602, печатная платка с кнопками, три разъема и динамик.

Не даю подробности комплектации и компоновки, потому что каждый будет делать исходя из того что у него есть под рукой для этого проекта.

Это не полноценный тренажер, он не дает вам ни каких заданий. И ни как не проверяет то ли вы набрали. Он только показывает что вы передали. Но мне только это и нужно было. :-)

Пример работы можно посмотреть здесь - https://youtube.com/shorts/XV8_UpXhKHY?feature=share
