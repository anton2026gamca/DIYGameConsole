#include <Gamepad.h>

int rightXcenter = 500;
int rightYcenter = 500;
int leftXcenter = 500;
int leftYcenter = 500;
double multiplierRX = 0.254; //127 / 500
double multiplierRY = 0.254;
double multiplierLX = 0.254;
double multiplierLY = 0.254;

Gamepad gp;

const int btnCount = 14;
const int btnPins[btnCount] = {9, 7, 6, 8, 4, 5, 3, 2, 10, 16, 0, 14, 1, 15};
const int btnNums[btnCount] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};

void setup() {

  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  
  for (int i = 0; i < btnCount; i++)
  {
    pinMode(btnPins[i], INPUT_PULLUP);
  }
  
  calibrate();
}

void loop() {
  int lx, ly, rx, ry;
  lx = analogRead(A1);
  ly = analogRead(A0);
  rx = analogRead(A3);
  ry = analogRead(A2);
  //we need to convert a 0-1000 to -127 - 127
  lx = floor((lx - leftXcenter) * multiplierLX);
  ly = floor((ly - leftYcenter) * multiplierLY);
  rx = floor((rx - rightXcenter) * multiplierRX);
  ry = floor((ry - rightYcenter) * multiplierRY);
  if(lx > 127) lx = 127;
  if(ly > 127) ly = 127;
  if(rx > 127) rx = 127;
  if(ry > 127) ry = 127;
  gp.setLeftXaxis(lx);
  gp.setRightXaxis(rx);
  gp.setLeftYaxis(ly);
  gp.setRightYaxis(ry);

  for (int i = 0; i < btnCount; i++)
  {
    bool isPressed = digitalRead(btnPins[i]) == LOW;
    if (isPressed)
      gp.setButtonState(btnNums[i], true);
    else
      gp.setButtonState(btnNums[i], false);
  }

  delay(20);
}

void calibrate()
{
  int lx, ly, rx, ry;
  int i = 0;
  while(i < 13)
  {
    lx = analogRead(A3);
    ly = analogRead(A2);
    rx = analogRead(A1);
    ry = analogRead(A0);
    bool validLX = lx > (leftXcenter - 100) && lx < (leftXcenter + 100);
    bool validLY = ly > (leftYcenter - 100) && ly < (leftYcenter + 100);
    bool validRX = rx > (rightXcenter - 100) && rx < (rightXcenter + 100);
    bool validRY = ry > (rightYcenter - 100) && ry < (rightYcenter + 100);
    if(validLX && validLY && validRX && validRY)
    {
      i++;
      //nothing to do here!
    }
    else i = 0;
    delay(20);
  }
  leftXcenter = lx;
  leftYcenter = ly;
  rightXcenter = rx;
  rightYcenter = ry;
  multiplierLX = (double)127 / (double)lx;
  multiplierLY = (double)127 / (double)ly;
  multiplierRX = (double)127 / (double)rx;
  multiplierRY = (double)127 / (double)ry;
}
