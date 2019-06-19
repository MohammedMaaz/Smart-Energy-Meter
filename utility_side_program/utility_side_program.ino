#include "functions.h"

void setup()
{
  _init();
  Serial.begin(9600);
  lcd.begin(16, 2);
}

void loop()
{
  //check if card is inserted card is inserted
  if(sl44x2.cardInserted())
  {
    if(inserted == false)
    {
      _init();
      inserted=true;
      if (sl44x2.cardReady())
      {
        //card inserted and recognized
        recognised=true;
        screen='m';
        insertMessage();
      }
    }
    if(recognised)
      //Main Part Here...
      mainProgram();
    else
      badCardMessage();
  }
  else
  {
    if(removed == false)
    {
      removed=true;
      inserted =false;
      sl44x2.cardRemoved(); 
    }
    greeting();
  }
}
