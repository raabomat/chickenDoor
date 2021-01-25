/*##############################################################################
 Author:
* JR

 Connections:
 *              -> Adafruit Pro Trinket - Wemos D1 mini pro
 * Mdriv:1A     -> 4
 * Mdriv:1B     -> 5
 * Mdriv:E1     -> 6
 * switchOpen:  -> 14 (A0)                D5 (GPIO 14)
 * switchClose: -> 15 (A1)                D0 (GPIO 16)
 * endSwClosed  -> 12
 * endSwOpen    -> 11
 *
##############################################################################*/

// Define constants and variables 
const int Dir1a = 4;
const int Dir1b = 5;
const int Enable1 = 6;
const int switchOpen = 14;
const int switchClose = 15;
const int portEndSwClosed = 12;
const int portEndSwOpen = 11;
const int sers =13;

#define DOOR_STATE_OPEN 4
#define DOOR_STATE_OPENING 5
#define DOOR_STATE_CLOSED 6
#define DOOR_STATE_CLOSING 7
#define DOOR_STATE_DUNNO 9
#define DOOR_STATE_INIT 10
#define SWITCH_OFF HIGH
#define SWITCH_ON LOW

#define MAX_RUN_TIME 6000 /* max time motor can run in ms */
#define INPUT_PIN_DEBOUNCE_TIME 1000 /* cycles not ms */

int switchLvl = 17;
int switchLvlOpenOld = 17;
int switchLvlCloseOld = 17;

int endSWClosed = 17;
int endSWOpen = 17;
int doorState = 17;

int endSWClosedOld = 17;
int endSWOpenOld = 17;

int debounceClosed = 0;
int debounceOpen = 0;

int debounceGo = 0;
int debounceGoOpen = 0;
int debounceGoClose = 0;

unsigned long ulStartTime = 0;

void setup()
{
  pinMode(Dir1a, OUTPUT);
  pinMode(Dir1b, OUTPUT);
  pinMode(Enable1, OUTPUT);
  pinMode(sers, OUTPUT);

  pinMode(portEndSwClosed, INPUT);
  pinMode(portEndSwOpen, INPUT);

  pinMode(switchOpen, INPUT_PULLUP);
  pinMode(switchClose, INPUT_PULLUP);

  switchLvl = 17;
  switchLvlOpenOld = digitalRead(switchOpen);
  switchLvlCloseOld = digitalRead(switchClose);

  doorState = DOOR_STATE_INIT;
  stopMotor();
}

// main loop
void loop()
{
  if (switchLvl == 17)
  {
    delay(2000);
    switchLvl = 18;
  }
  stateMachine();

  if ((usCheckForOpenIt() == SWITCH_ON) || (usCheckForCloseIt() == SWITCH_ON))
  {
    digitalWrite(sers, HIGH);
  }
  else
  {
   digitalWrite(sers, LOW);
  }
}

void stateMachine(void)
{
  switch (doorState)
  {
    case DOOR_STATE_INIT:
      endSWClosed = digitalRead (portEndSwClosed);
      endSWOpen = digitalRead (portEndSwOpen);
      endSWClosedOld = endSWClosed;
      endSWOpenOld = endSWOpen;

      /* closed */
      if ((endSWClosed == SWITCH_ON) && (endSWOpen == SWITCH_OFF))
      {
        doorState = DOOR_STATE_CLOSED;
      }
      /* open */
      else if ((endSWClosed == SWITCH_OFF) && (endSWOpen == SWITCH_ON))
      {
        doorState = DOOR_STATE_OPEN;
      }
      else
      {
        /* doorstate unknown */
        doorState = DOOR_STATE_DUNNO;
      }
    break;
    case DOOR_STATE_CLOSED:
      /* wait for switch */
      switchLvl = usCheckForOpenIt();
      if (switchLvl == SWITCH_ON)
      {
        switchLvl = SWITCH_OFF;
        openGate();
        doorState = DOOR_STATE_OPENING;
      }
    break;
    case DOOR_STATE_OPENING:
      /* opening until endswOpen high */
      if (usCheckState() == DOOR_STATE_OPEN)
      {
        stopMotor();
        delay(1000);
        doorState = DOOR_STATE_OPEN;
      }
      else
      {
         /* shut motor off and leave state after MAX_RUN_TIME */
         if (usRunningTooLong() == 1)
         {
            stopMotor();
            delay(1000);
            doorState = DOOR_STATE_DUNNO;
         }
      }
    break;
    case DOOR_STATE_OPEN:
      /* wait for switch */
      switchLvl = usCheckForCloseIt();
      if (switchLvl == SWITCH_ON)
      {
        switchLvl = SWITCH_OFF;
        closeGate();
        doorState = DOOR_STATE_CLOSING;
      }
    break;
    case DOOR_STATE_CLOSING:
      /* closing until endswClosed high */
      if (usCheckState() == DOOR_STATE_CLOSED)
      {
        stopMotor();
        delay(1000);
        doorState = DOOR_STATE_CLOSED;
      }
      else
      {
         /* shut motor off and leave state after MAX_RUN_TIME */
         if (usRunningTooLong() == 1)
         {
            stopMotor();
            delay(1000);
            doorState = DOOR_STATE_DUNNO;
         }
      }
    break;
    case DOOR_STATE_DUNNO:
    	/* do nothing */
    	/* set motor turning freely*/
    	analogWrite(Enable1, 0);
    break;
    default:
    break;
 }

}

void openGate (void)
{
  digitalWrite(Dir1a, LOW);
  digitalWrite(Dir1b, HIGH);
  analogWrite(Enable1,200);
  /* get current time for runtime timeout */
  ulStartTime = millis();
}

void closeGate (void)
{
  digitalWrite(Dir1a, HIGH);
  digitalWrite(Dir1b, LOW);
  analogWrite(Enable1,200);
  /* get current time for runtime timeout */
  ulStartTime = millis();
}

void stopMotor (void)
{
  analogWrite(Enable1,255);
  digitalWrite(Dir1a, HIGH);
  digitalWrite(Dir1b, HIGH);
}

int usRunningTooLong (void)
{
   int usRet = 0;
   /* still running; check for maximum runtime`*/
   if (( ulStartTime > millis() ) || ((ulStartTime + MAX_RUN_TIME) < ulStartTime))
   {
      /* overflow or soon to be overflow; get new millis */
      ulStartTime = millis();
   }
   else
   {
      /* if running longer than MAX_RUN_TIME --> switch off */
      if (ulStartTime + MAX_RUN_TIME <= millis() )
      {
         /* runs too long! */
         usRet = 1;
      }
      else
      {
         /* running not too long yet */
         usRet = 0;
      }
   }
   return usRet;
}

int usCheckForOpenIt(void)
{
	int switchOpenCurrent;
	switchOpenCurrent = digitalRead(switchOpen);
	if (switchOpenCurrent != switchLvlOpenOld)
	{
	    debounceGoOpen++;
	    if (debounceGoOpen > INPUT_PIN_DEBOUNCE_TIME)
	    {
	    	debounceGoOpen = 0;
	    	switchLvlOpenOld = switchOpenCurrent;
	    }
	}
	else
	{
		debounceGoOpen = 0;
	}

	return switchLvlOpenOld;
}

int usCheckForCloseIt(void)
{
	int switchCloseCurrent;
	switchCloseCurrent = digitalRead(switchClose);
	if (switchCloseCurrent != switchLvlCloseOld)
	{
		debounceGoClose++;
		if (debounceGoClose > INPUT_PIN_DEBOUNCE_TIME)
		{
			debounceGoClose = 0;
			switchLvlCloseOld = switchCloseCurrent;
		}
	}
	else
	{
		debounceGoClose = 0;
	}
	return switchLvlCloseOld;
}

int usCheckState(void)
{
  endSWClosed = digitalRead (portEndSwClosed);
  endSWOpen = digitalRead (portEndSwOpen);

  if (endSWClosed != endSWClosedOld)
  {
    /* end switch closed changed */
    debounceClosed++;
    if (debounceClosed > INPUT_PIN_DEBOUNCE_TIME)
    {
      debounceClosed = 0;
      endSWClosedOld = endSWClosed;
    }
  }
  else
  {
    debounceClosed = 0;
  }

  if (endSWOpen != endSWOpenOld)
  {
    debounceOpen++;
    if (debounceOpen > INPUT_PIN_DEBOUNCE_TIME)
    {
      debounceOpen = 0;
      endSWOpenOld = endSWOpen;
    }
  }
  else
  {
    debounceOpen = 0;
  }

  /* closed */
  if ((endSWClosedOld == SWITCH_ON) && (endSWOpenOld == SWITCH_OFF))
  {
    return DOOR_STATE_CLOSED;
  }
  /* open */
  else if ((endSWClosedOld == SWITCH_OFF) && (endSWOpenOld == SWITCH_ON))
  {
    return DOOR_STATE_OPEN;
  }
  else
  {
    /* doorstate unknown */
    return DOOR_STATE_DUNNO;
  }
}
