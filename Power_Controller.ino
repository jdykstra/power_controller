/*    power_controller.ino - Audio Power Controller Arduino sketch
 *     
 */

#include <IRLibRecv.h>
#include <IRLibDecodeBase.h>
#include <IRLibSendBase.H>
#include <IRLib_P01_NEC.h>

/*  Build options. */
#define IR_RETRANSMIT_INTERVAL 5*60*1000  /*  Retransmit refrig off code this frequently */

/* 
 *  IR Command codes.  Some of these are shared with the
 *  power controller, and ideally would be in a 
 *  shared header file.
 */
#define CODE_LIVING_ROOM_POWER   0x8322718E
#define CODE_OFFICE_POWER        0x83228C73
#define CODE_REFRIG_OFF   0x8322639C
#define CODE_REFRIG_ON    0x8322629D


/*  Hardware definitions.  */
#define PIN_IR_IN       2       /* IR receive input */
#define PIN_IR_OUT      3       /* IR transmit output */
#define PIN_PWR_SW      7       /* Power switch input */
#define PIN_OVER_SW     8       /* Override switch input */
#define PIN_SRC_PWR     9       /* Source power output */
#define PIN_HIGH_PWR    10      /* High amp power output */
#define PIN_LOW_PWR     11      /* Low amp power output */
#define PIN_SPKR_SW     12      /* Speaker switch output--Currently N.C.  */
#define PIN_PILOT       LED_BUILTIN   /* Pilot light */

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


/* Last timestamp an IR code was retransmitted.  */
unsigned long last_ir_retransmit = millis();

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
      digitalWrite(PIN_PILOT, ON);
      digitalWrite(PIN_SRC_PWR, ON);
      delay(7*1000);
      digitalWrite(PIN_LOW_PWR, ON);
      delay(1*1000);
      digitalWrite(PIN_HIGH_PWR, ON);

      /* Turn off the refrigerator. */
      sendIRCommand(CODE_REFRIG_OFF);  
      break;
      
    case SYS_OFFICE_UP:
      digitalWrite(PIN_PILOT, ON);
      digitalWrite(PIN_SPKR_SW, ON);
      digitalWrite(PIN_SRC_PWR, ON);
      delay(7*1000);
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
      digitalWrite(PIN_PILOT, OFF);
      digitalWrite(PIN_HIGH_PWR, OFF);
      digitalWrite(PIN_LOW_PWR, OFF);
      delay(5*1000);
      digitalWrite(PIN_SRC_PWR, OFF);

      /* Turn on the refrigerator. */
      sendIRCommand(CODE_REFRIG_ON);  
      break;
      
    case SYS_OFFICE_UP:
      digitalWrite(PIN_SPKR_SW, ON);
      digitalWrite(PIN_LOW_PWR, OFF);

      /* Turn on the refrigerator. */
      sendIRCommand(CODE_REFRIG_ON);  
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
      sendIRCommand(CODE_REFRIG_OFF);  
      break;
      
    case SYS_DOWN:
      digitalWrite(PIN_PILOT, OFF);
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
   */
  pinMode(PIN_PWR_SW, INPUT_PULLUP);
  pinMode(PIN_OVER_SW, INPUT_PULLUP);
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
      switch (myDecoder.value){
        case CODE_LIVING_ROOM_POWER:
          cmdLivingRoomPower();
          break;
         case CODE_OFFICE_POWER:
           cmdOfficePower();
           break;
      }
    }
    myReceiver.enableIRIn();    //  Restart receiver
  }

  /* 
   *  Process switch commands. We don't bother debouncing the
   *  power switch;  processing it will take a lot longer than
   *  the bounce period.
   */
  if (digitalRead(PIN_PWR_SW) == LOW){
    cmdLivingRoomPower();
  }

  /*
   * If the current system state isn't SYS_LIVING_ROOM_UP, periodically
   * resend the IR command to turn the refigerator on.  This covers the
   * case where something blocks the command when it is first sent on 
   * power-down.
   */
   if ((currentSysState != SYS_LIVING_ROOM_UP) && 
              (millis() - last_ir_retransmit > IR_RETRANSMIT_INTERVAL)){
     sendIRCommand(CODE_REFRIG_ON);
     last_ir_retransmit = millis();
   }

}

