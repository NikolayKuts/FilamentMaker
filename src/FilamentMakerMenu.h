#ifndef FilamentMakerMenu_h
#define FilamentMakerMenu_h

#include <GyverOLED.h>

#define MENU_ITEM_QUANTITY 4
#define TEMP_POSITION 0
#define HEATING_POSITION 1
#define SPEED_POSITION 2
#define POWER_POSITION 3

GyverOLED<SSD1306_128x64, OLED_BUFFER> oled;

class FilamentMakerMenu
{

public:
    void showData(
        int averageTemperature,
        int setHeatingTemperature,
        bool isHeated,
        bool isHeatingAllowed,
        uint8_t speedLevel,
        bool isWorkAllowed
    ) {
        oled.clear();
        oled.home();
        printTemperature(averageTemperature, setHeatingTemperature);
        printHeating(isWorkAllowed, isHeated, isHeatingAllowed);
        printSpeedLevel(speedLevel);
        printPointer(pointer);
        printPower(isWorkAllowed);
        oled.update();
    }

    void initialize() {
        oled.init();
        oled.setContrast(1);
    }

    void decrementPointer() { pointer = constrain(pointer - 1, 0, MENU_ITEM_QUANTITY - 1); }

    void incrementPointer() { pointer = constrain(pointer + 1, 0, MENU_ITEM_QUANTITY - 1); }

    uint8_t getPointerPosition() { return pointer; }

    void switchSettingMode() { settingMode = !settingMode; };

    bool isSettingModeTurnedOn() { return settingMode; }

private:
    volatile uint8_t pointer = 0;
    bool settingMode = false;
    bool blinkMode = false;

    void printPointer(uint8_t pointer) {
        static bool switcher = false;
        switch (pointer) {
        case TEMP_POSITION: oled.setCursor(0, 0); break;
        case HEATING_POSITION: oled.setCursor(0, 3); break;
        case SPEED_POSITION: oled.setCursor(0, 5); break;
        case POWER_POSITION: oled.setCursor(0, 7); break;
        }
        if (settingMode) {
            if (switcher) { oled.print(">"); }
            else { oled.print(""); }
            
            switcher = !switcher;
        }
        else {
            oled.print(">");
        }
    }

    void printPower(bool isWorkAllowed) {
        oled.setCursor(9, 7);
        oled.print("power");

        char result[4] = "";
        setOffOrOnByBoolean(isWorkAllowed, &result);
        oled.setCursor(55, 7);

        if (settingMode && pointer == POWER_POSITION) { blinkText(result); }
        else { oled.print(result); }
    }

    void printTemperature(int current, int set) {
        uint8_t row = 0;
        oled.setCursor(8, row);
        oled.print("t C");
        oled.circle(28, row + 1, 1, 1);
        oled.setCursor(54, row);
        oled.print("current");

        if (current < 10) { oled.setCursor(108, row); }
        else if (current < 100) { oled.setCursor(110, row); }
        else { oled.setCursor(102, row); }

        oled.print(current);

        oled.setCursorXY(54, 12);
        oled.print("set");

        oled.setCursorXY(103, 12);

        if (settingMode && pointer == TEMP_POSITION) {
            char text[4];
            itoa(set, text, DEC);
            blinkText(text);
        }
        else {
            oled.print(set);
        }
        oled.circle(122, row + 1, 1, 1);
        oled.circle(122, row + 13, 1, 1);
    };

    void printHeating(bool isWrorkAllowed, bool isHeated, bool isHeatingAllowed) {
        uint8_t row = 3;
        oled.setCursor(9, row);
        oled.print("heating");
        printHeatingIcon(isWrorkAllowed, isHeated, isHeatingAllowed, row);

        oled.setCursor(103, row);
        char result[4] = "";
        setOffOrOnByBoolean(isHeatingAllowed, &result);

        if (settingMode && pointer == HEATING_POSITION) { blinkText(result); }
        else { oled.print(result); }
    }

    void printHeatingIcon(bool isWorkAllowed, bool isHeated, bool isHeatingAllowed, uint8_t row) {
        static uint8_t counter = 0;
        uint8_t startIcon = 60;
        uint8_t iconRow = row * 8 + 5;
        if (isHeatingAllowed && isWorkAllowed) {
            if (isHeated) {
                if (counter == 0) {
                    oled.setCursorXY(startIcon, iconRow);
                    counter++;
                }
                else if (counter == 1) {
                    oled.setCursorXY(startIcon, iconRow - 1);
                    counter++;
                }
                else if (counter == 2) {
                    oled.setCursorXY(startIcon, iconRow - 2);
                    counter++;
                }
                else if (counter == 3) {
                    oled.setCursorXY(startIcon, iconRow - 3);
                    counter++;
                }
                else if (counter == 4) {
                    oled.setCursorXY(startIcon, iconRow - 4);
                    counter = 0;
                }
                oled.print("^^");
            }
            else {
                oled.setCursor(startIcon, row);
                oled.print("__");
            }
        }
        else {
            oled.setCursor(startIcon, row);
            oled.print("~X~");
        }
    }

    void printSpeedLevel(int level) {
        uint8_t indicatorStartPosition = 56;
        int indicatorEndPosition = map(level, 0, 100, indicatorStartPosition, 92);
        uint8_t row = 5;
        uint8_t indicatorTopPosition = row * 8 + 2;
        uint8_t indicatorBottomPosition = indicatorTopPosition + 2;
        oled.setCursor(9, row);
        oled.print("speed  [      ]   %");

        if (level < 10) { oled.setCursor(109, row); }
        else if (level < 100) { oled.setCursor(103, row); }
        else { oled.setCursor(98, row); }

        if (settingMode && pointer == SPEED_POSITION) {
            char text[4];
            itoa(level, text, DEC);
            blinkText(text);

        }
        else {
            oled.print(level);
        }

        if (level != 0) {
            oled.rect(indicatorStartPosition, indicatorTopPosition, indicatorEndPosition, indicatorBottomPosition);
        }
        else {
            oled.rect(indicatorStartPosition, indicatorTopPosition, indicatorEndPosition + 1, indicatorBottomPosition, 0);
        }
    }

    void blinkText(char text[]) {
        if (blinkMode) { oled.print(text); }
        else { oled.print(""); }

        blinkMode = !blinkMode;
    }


    void setOffOrOnByBoolean(bool value, char(*response)[4]) {
        if (value) {
            (*response)[0] = 'o';
            (*response)[1] = 'n';
        }
        else {
            (*response)[0] = 'o';
            (*response)[1] = 'f';
            (*response)[2] = 'f';
        }
    }
};



#endif
