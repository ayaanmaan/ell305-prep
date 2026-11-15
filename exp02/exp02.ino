#include <Keypad.h>
#include <LiquidCrystal.h>
#include <math.h>

LiquidCrystal lcd(10, 11, A2, A3, A4, A5);

const char ROWS = 5;
const char COLS = 4;
const char keys[ROWS][COLS] = {
    {'(', ')', 'C', 'A'},
    {'1', '2', '3', '+'},
    {'4', '5', '6', '-'},
    {'7', '8', '9', '*'},
    {'.', '0', '=', '/'}
};

const char rowPins[ROWS] = {5, 6, 7, 8, 9};
const char colPins[COLS] = {0, 1, 2, 4};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

bool isCalc = true;
bool lastT = HIGH, lastS = HIGH, err = false, dec = false;

enum CalcState {CALC, HIST};
CalcState cState = CALC;
String expr = "";
String hist[20];
int histCount = 0, histIdx = 0, p = 0;

enum DLState {ENTER, GRANTED, LOCKOUT};
DLState dlState = ENTER;
String dlPwd = "";
int fails = 0;
int lTime = 5;
unsigned long lStart = 0;
long lastRemain = -1;

float expression();

float factor() {
    if (err || p >= expr.length()) {
        err = true;
        return 0;
    }
    if (expr[p] == '-') {
        p++;
        return -factor();
    }
    if (expr[p] == 's') { 
        p++; 
        return sqrt(factor()); 
    }
    if (expr[p] == '(') {
        p++; float v = expression();
        if (p < expr.length() && expr[p] == ')') p++; 
        else err = true;
        return v;
    }
    int start = p;
    while (p < expr.length() && (isDigit(expr[p]) || expr[p] == '.')) p++;
    if (start == p) { 
        err = true; 
        return 0; 
    }
    return expr.substring(start, p).toFloat();
}

float term() {
    float v = factor();
    while (p < expr.length() && (expr[p] == '*' || expr[p] == '/')) {
        char op = expr[p++];
        if (op == '*') v *= factor();
        else { 
            float f = factor(); 
            if (f == 0) err = true; 
            else v /= f; 
        }
    }
    return v;
}

float expression() {
    float v = term();
    while (p < expr.length() && (expr[p] == '+' || expr[p] == '-')) {
        char op = expr[p++];
        if (op == '+') v += term();
        else v -= term();
    }
    return v;
}

void showHist() {
    lcd.clear();
    if (histCount == 0) { 
        lcd.print("Empty"); 
        return; 
    }
    lcd.print("[" + String(histIdx + 1) + "/" + String(histCount) + "]");
    lcd.setCursor(0, 1);
    lcd.print(hist[histIdx]);
}

void dlDraw() {
    lcd.clear();
    if (dlState == GRANTED) {
        lcd.print("ACCESS GRANTED");
    } 
    else if (dlState == ENTER) {
        lcd.print("Enter code:");
        lcd.setCursor(0, 1);
        for (unsigned int i = 0; i < dlPwd.length(); i++) lcd.print('*');
    }
}

void setup() {
    lcd.begin(16, 2);
    pinMode(12, INPUT_PULLUP);
    pinMode(3, INPUT_PULLUP);
    lcd.print("CALC MODE");
    delay(500);
    lcd.clear();
}

void loop() {
    bool t = digitalRead(3);
    if (!t && lastT) {
        isCalc = !isCalc;
        lcd.clear();
        if (isCalc) {
            if (dlState == GRANTED) {
                dlState = ENTER;
                dlPwd = "";
            }
            lcd.print("CALC MODE");
            delay(500);
            lcd.clear();
            if (cState == HIST) showHist();
            else lcd.print(expr);
        } 
        else {
            lcd.print("DIGILOCK MODE");
            delay(500);
            if (dlState == LOCKOUT) lastRemain = -1;
            else dlDraw();
        }
    }
    lastT = t;

    if (!isCalc && dlState == LOCKOUT) {
        long remain = lTime - (millis() - lStart) / 1000;
        if (remain <= 0) {
            dlState = ENTER;
            dlPwd = "";
            lTime *= 2;
            dlDraw();
        } else if (remain != lastRemain) {
            lcd.clear();
            lcd.print("Try again after");
            lcd.setCursor(0, 1);
            lcd.print(String(remain) + " seconds");
            lastRemain = remain;
        }
    }

    bool sqbtn = digitalRead(12);
    if (isCalc && !sqbtn && lastS && cState == CALC && expr.length() < 16) {
        expr += "s"; 
        dec = false; 
        lcd.clear(); 
        lcd.print(expr);
    }
    lastS = sqbtn;

    char k = keypad.getKey();
    if (!k) return;

    if (!isCalc) {
        if (dlState == ENTER) {
            if (k == '=') {
                if (dlPwd == "123" || dlPwd == "456" || dlPwd == "789") {
                    dlState = GRANTED;
                    fails = 0;
                    lTime = 5;
                    dlDraw();
                } 
                else {
                    fails++;
                    lcd.clear();
                    if (fails >= 6) {
                        lcd.print("MAX FAILED");
                        lcd.setCursor(0, 1);
                        lcd.print("ATTEMPTS");
                        delay(500);
                        lcd.clear();
                        lcd.print("DATA RESET");
                        delay(500);
                        fails = 0;
                        lTime = 5;
                        dlPwd = "";
                        dlDraw();
                    } 
                    else {
                        lcd.print("ACCESS FAILED");
                        delay(500);
                        if (fails % 3 == 0) {
                            dlState = LOCKOUT;
                            lStart = millis();
                            lastRemain = -1;
                        } 
                        else {
                            lcd.clear();
                            lcd.print("Failed attempt " + String(fails));
                            delay(500);
                            dlPwd = "";
                            dlDraw();
                        }
                    }
                }
            } 
            else if (k == 'C' || k == 'A') {
                dlPwd = "";
                dlDraw();
            } 
            else if (dlPwd.length() < 16) {
                dlPwd += k;
                dlDraw();
            }
        }
    } 
    else {
        if (cState == HIST) {
            if (k == '-') { 
                if (histIdx > 0) histIdx--; 
                showHist(); 
            }
            else if (k == '+') { 
                if (histIdx < histCount - 1) histIdx++; 
                showHist(); 
            }
            else if (k == 'C' || k == 'A') {
                cState = CALC; 
                expr = ""; 
                dec = false;
                if (k == 'A') histCount = 0;
                lcd.clear();
            }
        }
        else if (cState == CALC) {
            if (k == 'C' || k == 'A') {
                expr = ""; 
                dec = false;
                if (k == 'A') histCount = 0;
                lcd.clear();
            } 
            else if (k == '=') {
                if (expr == "") {
                    cState = HIST; 
                    histIdx = max(0, histCount - 1); 
                    showHist();
                } 
                else {
                    p = 0; 
                    err = false;
                    float ans = expression();
                    lcd.setCursor(0, 1);
                    if (err || p != (int)expr.length()) {
                        lcd.print("=INVALID"); 
                        delay(500);
                        expr = ""; 
                        dec = false; 
                        lcd.clear();
                    } 
                    else {
                        String res;

                        if (ans == (long)ans) { 
                            res = String((long)ans); 
                            dec = false; 
                        } 
                        else { 
                            res = String(ans); 
                            dec = true;     
                        }

                        if (histCount < 20) hist[histCount++] = res;
                        expr = res; 
                        lcd.print("=" + res);
                    }
                }
            } 
            else if (expr.length() < 16) {
                if (k == '.' && !dec) { 
                    dec = true; 
                    expr += k; 
                    lcd.clear(); 
                    lcd.print(expr); 
                }
                else {
                    if (k == '+' || k == '-' || k == '*' || k == '/' || k == '(' || k == ')') dec = false;
                    expr += k; 
                    lcd.clear(); 
                    lcd.print(expr);
                }
            }
        }
    }
}
