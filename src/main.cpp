#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

//Screen text char arrays stored as a string table in flash memory.
const char txtStartup1[] PROGMEM = "=-=-= WELCOME =-=-= ";
const char txtStartup2[] PROGMEM = "Syringe Pump v0.0.0";
const char txtStartup3[] PROGMEM = "Kairolite Labs 2022";
const char txtStartup4[] PROGMEM = "=HeatLab FTUI 2022=";
const char txtHome1[] PROGMEM = "==PRESS TO SELECT==";
const char txtHome2[] PROGMEM = "A - Infusion";
const char txtHome3[] PROGMEM = "B - Withdrawal";
const char txtHome4[] PROGMEM = "C - Settings/Help";
const char txtSetting1[] PROGMEM = "======SETTINGS======";
const char txtSetting2[] PROGMEM = "A - Manual Adjust";
const char txtSetting3[] PROGMEM = "B - Help";
const char txtSetting4[] PROGMEM = "C - About/Info";
const char txtAbout1[] PROGMEM = "=ABOUT THIS PROJECT=";
const char txtAbout2[] PROGMEM = "Syringe Pump v0.0.0";
const char txtAbout3[] PROGMEM = "[GitHub @ Kairolite]";
const char txtAbout4[] PROGMEM = "Made at HeatLab FTUI";
const char txtHelp1[] PROGMEM = "========HELP========";
const char txtHelp2[] PROGMEM = "ABC - Select Option";
const char txtHelp3[] PROGMEM = "D   - Return/Exit";
const char txtHelp4[] PROGMEM = "*/# - <<Left/Right>>";
const char txtMAdjust1[] PROGMEM = "=MANUAL ADJUSTMENT=";
const char txtMAdjust2[] PROGMEM = "  Position: 000000";
const char txtMAdjust3[] PROGMEM = "";
const char txtMAdjust4[] PROGMEM = "   <<[*] || [#]>>";
const char txtType1[] PROGMEM = "SELECT SYR. TYPE [-]";
const char txtType2[] PROGMEM = "A - OneMed";
const char txtType3[] PROGMEM = "B - Terumo";
const char txtType4[] PROGMEM = "C - Manual Setting";
const char txtMType1[] PROGMEM = "CUSTOM SYR. SET. [-]";
const char txtMType2[] PROGMEM = "A - Volume: 00000 mL";
const char txtMType3[] PROGMEM = "B - In.Dia: 00.00 mm";
const char txtMType4[] PROGMEM = "C-[OK]    D-[CANCEL]";
const char txtSInstall1[] PROGMEM = "PLEASE LOAD SYRINGE";
const char txtSInstall2[] PROGMEM = "  Position: 000000";
const char txtSInstall3[] PROGMEM = "   <<[*] || [#]>>";
const char txtSInstall4[] PROGMEM = "C-[DONE]  D-[CANCEL]";
const char txtTConfirm1[] PROGMEM = "CONFIRM SYRINGE TYPE";
const char txtTConfirm2[] PROGMEM = "brandName | 00000 mL";
const char txtTConfirm3[] PROGMEM = "Inner Dia.= 00.00 mm";
const char txtTConfirm4[] PROGMEM = "C - [OK]  |";

const char *const string_table[] PROGMEM = {
  txtStartup1, txtStartup2, txtStartup3, txtStartup4,
  txtHome1, txtHome2, txtHome3, txtHome4,
  txtSetting1, txtSetting2, txtSetting3, txtSetting4,
  txtAbout1, txtAbout2, txtAbout3, txtAbout4,
  txtHelp1, txtHelp2, txtHelp3, txtHelp4,
  txtMAdjust1, txtMAdjust2, txtMAdjust3,txtMAdjust4,
  txtType1, txtType2, txtType3, txtType4,
  txtMType1, txtMType2, txtMType3, txtMType4,
  txtSInstall1, txtSInstall2, txtSInstall3, txtSInstall4,
  txtTConfirm1, txtTConfirm2, txtTConfirm3, txtTConfirm4,
};

char buffer[20];

// The Keypad object
#define ROWS (byte)4 //four rows
#define COLS (byte)4 //four columns

char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {7, 6, 5, 4}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {3, 2, 1, 0}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

char tempKey; // Store last key input.

// The I2C LCD object
LiquidCrystal_I2C lcd(0x27,20,4);

// Program Variables
bool infusionValue = true;
char infusionState(){
  return (infusionValue) ? 'I' : 'W';
}

char syringeVolumeSize [5];
char syringeDiameterSize [5];

// Syringe Size Database
// 0        1     2     3     4     5     6     7
// brandID, 1mL,  3mL,  5mL,  10mL, 20mL, 30mL, 50mL;
int syringeDB[3][8] = {
  {1,470,850,1180,1620,2190,2620,2950}, // Placeholder values
  {2,473,900,1304,1579,2018,2336,2945}, // Terumo Plastic
  {3,478,866,1207,1450,1913,2169,2672}  // BD Plastic
};
byte syringeBrand;

bool isLineSelected = false;
byte cursorColumn = 0;
byte cursorRow = 0;

bool stepperState = false;
long stepperPosition = 0;

// Program Screens
enum class Screen:uint8_t{
  Startup,
  Home,
  Setting,
  About,
  Help,
  ManualAdjust,
  TypeSelect,
  ManualType,
  SyringeInstall,
  TypeConfirm,
  FlowrateSetting,
  FlowrateError,
  FlowrateConfirm,
  Active,
  Pause,
  Done,
  SyringeRemoval,
  SyringeReset
};

Screen currentScreen = Screen::Startup;
Screen lastScreen = Screen::Startup;

void gotoScreen(Screen targetScreen){
  lastScreen = currentScreen;
  currentScreen = targetScreen;

  lcd.clear();
  for (int i=0; i<4; i++){
    lcd.setCursor(0,i);
    strcpy_P(buffer, (char *)pgm_read_word(&(string_table[((uint8_t)targetScreen*4)+i])));
    lcd.print(buffer);
  }

  if (targetScreen==Screen::TypeSelect) {
    lcd.setCursor(18,0);
    lcd.print(infusionState());

    for (byte i = 0; i < 5; i++) {
      syringeVolumeSize[i] = '0';
      syringeDiameterSize[i] = '0';
    }
  }
}

void selectLine(byte column, byte row){
  isLineSelected=true;
  cursorRow = row;
  cursorColumn = column;
  lcd.setCursor(0,cursorRow);
  lcd.print(">>>");
  lcd.setCursor(cursorColumn,cursorRow);
  lcd.cursor_on();
  // lcd.blink_on();
}

void deselectLine(){
  isLineSelected=false;
  // lcd.blink_off();
  lcd.cursor_off();
  strcpy_P(buffer, (char *)pgm_read_word(&(string_table[((uint8_t)currentScreen*4)+cursorRow])));
  for (byte i = 0; i < 4; i++) {
    lcd.setCursor(i,cursorRow);
    lcd.print(buffer[i]);
  }
  lcd.home();
  cursorRow = 0;
  cursorColumn = 0;
}

void cursorIncrement(bool direction, byte startRange=0, byte endRange=20){
  if (direction and cursorColumn < endRange) {
    cursorColumn++;
  }
  if (!direction and cursorColumn > startRange){
    cursorColumn--;
  }
  lcd.setCursor(cursorColumn,cursorRow);
}

long stepperIncrement(bool direction, int steps){
  if (!direction and (stepperPosition-steps) >= 0) {
    stepperPosition -= steps;
  }
  else if(direction and (stepperPosition+steps)<=10000){
    stepperPosition += steps;
  }
  return stepperPosition;
}

void printStepPos(byte column, byte row){
  char s[11];
  sprintf(s, "%ld", stepperPosition);
  for (byte i = 0; i < strlen(s)/2; i++) {
    char temp = s[i];
    s[i] = s[strlen(s)-i-1];
    s[strlen(s)-i-1] = temp;
  }

  lcd.rightToLeft();
  lcd.setCursor(column, row);
  lcd.print("000000");
  lcd.setCursor(column, row);
  lcd.print(s);
  lcd.leftToRight();
  lcd.home();
}

void setup() {
  // This is the I2C LCD object initialization.
  lcd.init();
  lcd.clear();
  lcd.backlight();

  gotoScreen(Screen::Startup);
  delay(4000);
  gotoScreen(Screen::Home);
}

void loop() {
  char key = keypad.getKey();// Read the key
  KeyState kState = keypad.getState();

  if (key or kState == HOLD) {
    switch (currentScreen) {
      case Screen::Home:
        switch (key) {
          case 'A':
            infusionValue = true;
            gotoScreen(Screen::TypeSelect);
            break;
          case 'B':
            infusionValue = false;
            gotoScreen(Screen::TypeSelect);
            break;
          case 'C':
            gotoScreen(Screen::Setting);
            break;
          default: break;
        }
        break;

      case Screen::Setting:
        switch (key) {
          case 'A':
            gotoScreen(Screen::ManualAdjust);
            printStepPos(17,1);
            break;
          case 'B':
            gotoScreen(Screen::Help);
            break;
          case 'C':
            gotoScreen(Screen::About);
            break;
          case 'D':
            gotoScreen(Screen::Home);
            break;
          default: break;
        }
        break;

      case Screen::About:
        switch (key) {
          case 'D':
            gotoScreen(Screen::Setting);
            break;
          default: break;
        }
        break;

      case Screen::Help:
        switch (key) {
          case 'D':
            gotoScreen(Screen::Setting);
            break;
          default: break;
        }
        break;

      case Screen::ManualAdjust:
        switch (key) {
          case 'D':
            gotoScreen(Screen::Setting);
            break;
          case '#':
            stepperIncrement(true, 100);
            printStepPos(17,1);
            break;
          case '*':
            stepperIncrement(false, 100);
            printStepPos(17,1);
            break;
          default:
            if (kState == HOLD) {
              switch (tempKey) {
                case '#':
                  stepperIncrement(true, 100);
                  printStepPos(17,1);
                  delay(100);
                  break;
                case '*':
                  stepperIncrement(false, 100);
                  printStepPos(17,1);
                  delay(100);
                  break;
                default: break;
              }
              break;
            }
            else break;
        }
        break;

      case Screen::TypeSelect:
        switch (key) {
          case 'A':
            syringeBrand = 1;
            gotoScreen(Screen::SyringeInstall);
            printStepPos(17,1);
            break;
          case 'B':
            syringeBrand = 2;
            gotoScreen(Screen::SyringeInstall);
            printStepPos(17,1);
            break;
          case 'C':
            syringeBrand = 0;
            gotoScreen(Screen::ManualType);
            lcd.setCursor(18,0);
            lcd.print(infusionState());
            break;
          case 'D':
            gotoScreen(Screen::Home);
            break;
          default: break;
        }
        break;

      case Screen::ManualType:
        switch (key) {
          case 'A':
            (!isLineSelected)? selectLine(12,1) : deselectLine();
            break;
          case 'B':
            (!isLineSelected)? selectLine(12,2) : deselectLine();
            break;
          case 'C':
            deselectLine();
            gotoScreen(Screen::SyringeInstall);
            printStepPos(17,1);
            break;
          case 'D':
            deselectLine();
            gotoScreen(Screen::TypeSelect);
            break;
          case '#':
            cursorIncrement(true,12,16);
            break;
          case '*':
            cursorIncrement(false,12,16);
            break;
          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9':
            if (isLineSelected) {
              if (cursorRow==1) {
                lcd.print(key);
                syringeVolumeSize[cursorColumn-12] = key;
                cursorIncrement(true,12,16);
              }
              // For row 2, skip decimal place
              else if (cursorRow==2 and cursorColumn!=14) {
                lcd.print(key);
                syringeDiameterSize[cursorColumn-12] = key;
                cursorIncrement(true,12,16);

                // Decimal jump feature.
                if (cursorColumn==14) {
                  cursorIncrement(true,12,16);
                }
              }
            }
            break;
          default: break;
        }
        break;

      case Screen::SyringeInstall:
        switch (key) {
          case 'C':
            gotoScreen(Screen::TypeConfirm);

            lcd.setCursor(0,1);
            switch (syringeBrand) {
              case 0:
              lcd.print(F("CustomType"));
              break;
              case 1:
              lcd.print(F("OneMed    "));
              break;
              case 2:
              lcd.print(F("Terumo    "));
              break;
              default:
              lcd.print(F("brandName "));
              break;
            }

            lcd.setCursor(12,1);
            for (byte i = 0; i < sizeof(syringeVolumeSize); i++) {
              lcd.print(syringeVolumeSize[i]);
            }

            lcd.setCursor(12,2);
            for (byte i = 0; i < sizeof(syringeDiameterSize); i++) {
              if (i!=2) {
                lcd.print(syringeDiameterSize[i]);
              }
              else lcd.setCursor(15,2);
            }

            lcd.setCursor(12,3);
            if(infusionValue){
              lcd.print(F("Infusion"));
            }
            if(!infusionValue){
              lcd.print(F("Withdraw"));
            }

            break;

          case 'D':
            gotoScreen(Screen::TypeSelect);
            break;

          case '#':
            stepperIncrement(true, 100);
            printStepPos(17,1);
            break;
          case '*':
            stepperIncrement(false, 100);
            printStepPos(17,1);
            break;
          default:
            if (kState == HOLD) {
              switch (tempKey) {
                case '#':
                  stepperIncrement(true, 100);
                  printStepPos(17,1);
                  delay(100);
                  break;
                case '*':
                  stepperIncrement(false, 100);
                  printStepPos(17,1);
                  delay(100);
                  break;
                default: break;
              }
              break;
            }
            else break;
        }
        break;

      case Screen::TypeConfirm:
        switch (key) {
          case 'D':
            gotoScreen(Screen::TypeSelect);
            break;
          default: break;
        }
        break;

      // case Screen::Home:
      //   switch (key) {
      //     case 'A':
      //       break;
      //     case 'B':
      //       break;
      //     case 'C':
      //       break;
      //     case 'D':
      //       break;
      //     default: break;
      //   }
      //   break;



      default: break;
    }

    // Save last key input.
    if (kState!=HOLD){
        tempKey = key;
      }

  }
}
