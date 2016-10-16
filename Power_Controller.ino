/*    power_controller.ino - Audio Power Controller Arduino sketch
 *     
 */

#include <IRLibRecv.h>
#include <IRLibDecodeBase.h>
#include <IRLibSendBase.H>
#include <IRLib_P01_NEC.h>


/*  Hardware definitions.  */
#define PIN_IR_IN       2       /* IR receive input */
#define PIN_IR_OUT      3       /* IR transmit output */
#define PIN_PWR_SW      7       /* Power switch input */
#define PIN_OVER_SW     8       /* Override switch input */
#define PIN_SRC_PWR     9       /* Source power output */
#define PIN_HIGH_PWR    10      /* High amp power output */
#define PIN_LOW_PWR     11      /* Low amp power output */
#define PIN_SPKR_SW     12      /* Speaker switch output */

/* Output pin HIGH level turns power on. */
#define ON              HIGH
#define OFF             LOW

/* Objects for infrared communication. */
IRrecv myReceiver(PIN_IR_IN);
IRdecodeNEC myDecoder;
IRsendNEC mySender;

/*  Number of times to repeat a sent IR code. */
#define IR_REPEAT_COUNT 5

/* Switch inputs. */
typedef enum {
  SW_LR_PWR = 0,                /* Living room power */
  SW_OVERRIDE,                  /* Override */
  N_SW                          /* Number of switches */
} sws;

/* Track switch states. */
typedef struct {
  int       pin;                /* Associated pin number */
  int       current;            /* Current state (HIGH or LOW) */
  int       debounce;           /* ms. remaining in debounce period */
} t_sw;


/* 
 *  Send an IR command.
 */
void sendIRCommand(uint32_t value)
{
  int i;

  /* 
   *  Send an IR command a number of times to maximize the chances
   *  it will be received.
   */
  for (i=0; i < IR_REPEAT_COUNT; i++)
    mySender.send(value);

  /* 
   *  According to the IRLib2 documentation, it's necessary to 
   *  restart the receiver after sending something, because 
   *  the timers (or interrupts?) may have been reconfigured.
   */
   myReceiver.enableIRIn();    //  Restart receiver
}


/* System states. */
enum {
  SYS_DOWN,
  SYS_LIVING_ROOM_UP,
  SYS_OFFICE_UP
};

int currentSysState = SYS_DOWN;

void stateSysDown(int newSysState)
{
  switch (newSysState){
    case SYS_LIVING_ROOM_UP:
      digitalWrite(PIN_SRC_PWR, ON);
      delay(1*1000);
      digitalWrite(PIN_LOW_PWR, ON);
      delay(1*1000);
      digitalWrite(PIN_HIGH_PWR, ON);

      /* Turn off the refrigerator. */
      /* ??  Dummy code for now. */
      sendIRCommand(0x58A741BE);  
      break;
      
    case SYS_OFFICE_UP:
      digitalWrite(PIN_SPKR_SW, ON);
      digitalWrite(PIN_SRC_PWR, ON);
      delay(1*1000);
      digitalWrite(PIN_HIGH_PWR, ON);
      break;
      
    default:
      Serial.println(F("Null state transition."));
  }
}


void stateLivingRoomUp(int newSysState)
{
  switch (newSysState){
    case SYS_DOWN:
      digitalWrite(PIN_HIGH_PWR, OFF);
      digitalWrite(PIN_LOW_PWR, OFF);
      delay(5*1000);
      digitalWrite(PIN_SRC_PWR, OFF);

      /* Turn on the refrigerator. */
      /* ??  Dummy code for now. */
      sendIRCommand(0x58A741BE);  
      break;
      
    case SYS_OFFICE_UP:
      digitalWrite(PIN_SPKR_SW, ON);
      digitalWrite(PIN_LOW_PWR, OFF);

      /* Turn on the refrigerator. */
      /* ??  Dummy code for now. */
      sendIRCommand(0x58A741BE);  
      break;
      
    default:
      Serial.println(F("Null state transition."));
  }
}


void stateOfficeUp(int newSysState)
{
  switch (newSysState){
    case SYS_LIVING_ROOM_UP:
      digitalWrite(PIN_LOW_PWR, ON);
      digitalWrite(PIN_SPKR_SW, OFF);

      /* Turn off the refrigerator. */
      /* ??  Dummy code for now. */
      sendIRCommand(0x58A741BE);  
      break;
      
    case SYS_DOWN:
      digitalWrite(PIN_HIGH_PWR, OFF);
      delay(5*1000);
      digitalWrite(PIN_SRC_PWR, OFF);
      digitalWrite(PIN_SPKR_SW, OFF);
      break;
      
    default:
      Serial.println(F("Null state transition."));
  }
}


/* Move to the specified system state. */
void setSysState(int newSysState)
{
  switch (currentSysState){
    case SYS_DOWN:
      stateSysDown(newSysState);
      break;

    case SYS_LIVING_ROOM_UP:
      stateLivingRoomUp(newSysState);
      break;

    case SYS_OFFICE_UP:
      stateOfficeUp(newSysState);
      break;
  }

  currentSysState = newSysState;
}


void cmdLivingRoomPower()
{
  switch (currentSysState){
    case SYS_DOWN:
      setSysState(SYS_LIVING_ROOM_UP);
      break;

    case SYS_LIVING_ROOM_UP:
      setSysState(SYS_DOWN);
      break;

    case SYS_OFFICE_UP:
      setSysState(SYS_LIVING_ROOM_UP);
      break;
  }
}


void cmdOfficePower()
{
  switch (currentSysState){
    case SYS_DOWN:
      setSysState(SYS_OFFICE_UP);
      break;

    case SYS_LIVING_ROOM_UP:
      setSysState(SYS_OFFICE_UP);
      break;

    case SYS_OFFICE_UP:
      setSysState(SYS_DOWN);
      break;
  }
}


void setup()
{
  /*  Configure serial port for debugging. */
  Serial.begin(9600);
  delay(2000);while(!Serial);     //delay for Leonardo

  /* 
   *  Configure input and output pins, except those managed
   *  by IRLib2.
   *  ??  Verify the outputs come up low.
   */
  pinMode(PIN_PWR_SW, INPUT);
  pinMode(PIN_OVER_SW, INPUT);
  pinMode(PIN_SRC_PWR, OUTPUT);
  pinMode(PIN_HIGH_PWR, OUTPUT);
  pinMode(PIN_LOW_PWR, OUTPUT);
  pinMode(PIN_SPKR_SW, OUTPUT);
 
  /* Initialize IRLib2. */
  /*  
   *   The IR repeater lengthens marks, at least partially due to
   *   the slow fall time of the line from the receivers to the 
   *   central unit due to line capacitance.  This compensory 
   *   value was determined empirically.
   */
  myReceiver.markExcess = 4*myReceiver.markExcess;
  
  IRLib_NoOutput();
  myReceiver.enableIRIn();
  
  Serial.println(F("Initialization complete."));
}


void loop() {

  /* Process IR commands. */
  if (myReceiver.getResults()) {
    myDecoder.decode();
    Serial.print(F("IR protocol "));
    Serial.print(myDecoder.protocolNum, DEC);
    Serial.print(F(" value "));
    Serial.println(myDecoder.value, HEX);
    if (myDecoder.protocolNum == NEC){
      digitalWrite(LED_BUILTIN, HIGH);
      switch (myDecoder.value){
        case 0x58A701FE:
          cmdLivingRoomPower();
          break;
         case 0x58A711EE:
           cmdOfficePower();
           break;
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    myReceiver.enableIRIn();    //  Restart receiver
  }

  /* Process switch commands. */
  
}

