#include <Keypad.h>
#include <LiquidCrystal.h>
#include "SL44x2.h"

///////////////////Smart Card Variables and initializations/////////////////////////
#define SC_C2_RST              7
#define SC_C1_VCC              12
#define SC_C7_IO               10
#define SC_C2_CLK              11
#define SC_SWITCH_CARD_PRESENT 8
#define SC_SWITCH_CARD_PRESENT_INVERT true
#define SC_PIN 0xFFFFFF
SL44x2 sl44x2(SC_C7_IO, SC_C2_RST, SC_C1_VCC, SC_SWITCH_CARD_PRESENT, SC_C2_CLK, SC_SWITCH_CARD_PRESENT_INVERT);

///////////////////LCD Variables and initializations/////////////////////////
const int rs = 13, en = 6, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
/*
 * ***************** LCD Conntections *****************
 VSS -> GND
 VDD -> 5V
 V0 -> Pot Center Pin
 RS -> 13
 RW -> GND
 EN -> 6
 D4 -> 5
 D5 -> 4
 D6 -> 3
 D7 -> 2
 A -> 5V through 220 Ohm Resistor
 K -> GND
 */

////////////////////Keypad Variables and initializations////////////////////////
const byte ROWS = 4; 
const byte COLS = 4; 
char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {29, 28, 27, 26}; //yellow=29 (in sequence)
byte colPins[COLS] = {25, 24, 23, 22}; 
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

/////////////////////////Programs Global Variables and initializations/////////////////////////////
short digitsLength; //number of digits reserved for storing units (not more than 10 digits)
char screen; String unitsInput; double price; short password_max_length; String userPass; String hardCodedPassword; short minRechargeUnits; bool secondScreen;
bool inserted, removed, recognised;

void _init(){
  digitsLength = 10; //customVar
  screen = 'm';
  unitsInput = "";
  price = 0;
  password_max_length=12; //customVar
  hardCodedPassword = "123456"; //customVar
  minRechargeUnits = 20; //customVar
  secondScreen = false;
  userPass = "";
  inserted = false;
  removed = false;
  recognised = false;
}

uint32_t powerOfTen(short power)
{
  uint32_t res=1;
  for(short i=0; i<power; ++i)
    res = res*10;
  return res;
}

String writeUnits(String units){
  uint8_t hex[digitsLength];
  const int len = units.length();
  for(int i=0; i<digitsLength; ++i)
  {
    if(i>=digitsLength-len) //if reached at units string starting index
    {
      uint8_t digit = units[i-(digitsLength-len)]-'0';
      if(digit < 0 || digit > 9) //if cuurent digit not a valid number
        return "Invalid Units!";
      else //if valid gitis
        hex[i] = digit; //convert this char digit into hex digit
    }
    else //if units start index not reached yet
      hex[i] = 0; //hexDigit will be 0
  }
  //once hex array is ready, update main memory and return success or failure status
  if(sl44x2.authenticate(SC_PIN))
  {
    bool res = sl44x2.updateMainMemory(0x61, hex, digitsLength);
    if(res)
      return "Card Recharged!";
    else
      return "Recharge Failed!";
  }
  else
    return "Access Denied!";
}

uint32_t readUnits(){
  uint8_t  data[SL44X2_DATA_SIZE]; //card data holder
  uint32_t res=0; //result holder
  sl44x2.readMainMemory(0, data, SL44X2_DATA_SIZE);

  //traverse from position 61 till position 61+digitsLength to retreive the value of units
  for(uint8_t i=0x61; i<(0x61 + digitsLength); i=i+0x01)
  {
    if(data[i]>=0x0 && data[i]<=0x9) //check if character is a valid digit/number
      res = res + data[i] * powerOfTen(0x60+digitsLength-i);
    else //if not a digit/number
      return -1; //return with error value
  }
  return res;
}

double calculateBill(double units){
  double price;
  if(units <= 50)
    price = units*2.0;
  else
  {
    if(units<=100)
      price = units*5.79;
    else
    {
      price = 100*5.79;
      units=units-100;
      if(units<=100)
        price = price + units*8.11;
      else
      {
        price = price + 100*8.11;
        units=units-100;
        if(units<=100)
          price = price + units*10.20;
        else
        {
          price = price + 100*10.20;
          units=units-100;
          if(units<=400)
            price = price + units*17.60;
          else
          {
            price = price + 400*17.60;
            units=units-400;
            price = price + units*20.70;
          }
        }
      }
    }
  }
  if(price < 75) //minimum charges of 75 Rs must apply
    price = 75;
  return price;
}

void getUnitsFromUser(){
  lcd.setCursor(0, 0);
  lcd.print("Enter Units:     ");
  lcd.setCursor(0, 1);
  for(short i=0; i<16; ++i)
    if(i<unitsInput.length())
      lcd.print(unitsInput[i]);
    else
      lcd.print(' ');
  char ip = customKeypad.getKey();
  if(ip)
  {
    const short num = ip-'0';
    if(num>=0 && num<=9 && unitsInput.length()<digitsLength) //if a valid input
      unitsInput.concat(ip);
    else if(ip=='*') //backspace
      unitsInput.remove(unitsInput.length()-1); //remove last char
    else if(ip=='D') //back
      screen='m'; //move to mainMenu
    else if(ip=='#') //enter
    {
      double units = unitsInput.toDouble();
      if(units < minRechargeUnits)
      {
        lcd.clear();
        lcd.print("Units too Low");
        delay(2000);
      }
      else{
        price = calculateBill(units); //store price in global variable
        {
          lcd.clear();
          screen = 'c'; //navigate to confirmation screen
        } 
      }
    }
  }
}

void confirmSubmission(){
  lcd.setCursor(0, 0);
  lcd.print("Rs: ");
  lcd.print(round(price));
  lcd.setCursor(0, 1);
  lcd.print("Confirm?");
  char ch = customKeypad.getKey();
  if(ch=='#') //confirm
    screen='p'; //move to enterPassword screen
  else if(ch=='D') //back
    screen='g'; //go back
}

void enterPassword(){
  lcd.setCursor(0, 0);
  lcd.print("Enter Password: ");
  lcd.setCursor(0, 1);
  for(short i=0; i<16; ++i)
    if(i<userPass.length())
      lcd.print('*');
    else
      lcd.print(' ');
  char key = customKeypad.getKey();
  if(key=='*') //backspace
    userPass.remove(userPass.length()-1);
  else if(key=='#') //enter
  {
    if(hardCodedPassword==userPass) //if passwords match
      screen = 'r';
    else
    {
      lcd.clear();
      lcd.print("Wrong Password");
      delay(2000);
      screen='m';
    }
  }
  else if(key && userPass.length()<password_max_length)
    userPass.concat(key);
}

void rechargeUnits(){
  lcd.clear();
  uint32_t prevUnits = readUnits();
  uint32_t currUnits = unitsInput.toDouble();
  uint32_t totalUnits = prevUnits + currUnits;
  String units = String(totalUnits);
  if(units.length()>digitsLength)
    lcd.print("Invalid Units");
  else
    lcd.print(writeUnits(units));
  delay(2000);
  screen = 'm';
}

void viewBalance(){
  lcd.setCursor(0, 0);
  lcd.print("Units Balance:  ");
  lcd.setCursor(0, 1);
  uint32_t res = readUnits();
  if(res == -1)
    lcd.print("Error Occured!");
  else
    lcd.print(res);
  char cont = customKeypad.getKey();
  if(cont=='D') //back
    screen='m'; //back to mainMenu
}

void resetUnits(){
  String _status = writeUnits("0");
  lcd.clear();
  if(_status == "Card Recharged!")
    lcd.print("Reset Done!");
  else
    lcd.print("Error Occured!");
  delay(2000);
}

void mainMenu(){
  //reset global containers
  unitsInput = "";
  userPass = "";
  price = 0;
  ////////////////////////
  if(secondScreen == false)
  {
    lcd.setCursor(0, 0);
    lcd.print("1. Recharge     ");
    lcd.setCursor(0, 1);
    lcd.print("2. View Balance  "); 
  }
  else
  {
    lcd.setCursor(0, 0);
    lcd.print("2. View Balance  ");
    lcd.setCursor(0, 1);
    lcd.print("3. Reset Card    ");
  }
  char choiceVal = customKeypad.getKey();
  if(choiceVal == '1')
  {
    lcd.clear();
    screen='g';
  }
  else if(choiceVal == '2')
  {
    lcd.clear();
    screen='v';
  }
  else if(choiceVal == '3')
    resetUnits();
  else if(choiceVal == 'B')
    secondScreen = true;
  else if(choiceVal == 'A')
    secondScreen = false;
}

void greeting(){
  lcd.setCursor(0, 0);
  lcd.print("Recharge System ");
  lcd.setCursor(0, 1);
  lcd.print("Insert Card...  ");
}

void insertMessage(){
  //display message on screen for 2 seconds
  lcd.clear();
  lcd.print("Card Inserted!  ");
  delay(2000);
}

void badCardMessage(){
  lcd.setCursor(0, 0);
  lcd.print("Bad Card!       ");
  lcd.setCursor(0, 1);
  lcd.print("Please Remove...");
}

void mainProgram()
{
  switch(screen)
  {
    case 'm':
      mainMenu();
      break;
    case 'g':
      getUnitsFromUser();
      break;
    case 'c':
      confirmSubmission();
      break;
    case 'p':
      enterPassword();
      break;
    case 'r':
      rechargeUnits();
      break;
    case 'v':
      viewBalance();
      break;
  }
}
