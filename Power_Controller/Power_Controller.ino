/*    power_controller.ino - Audio Power Controller Arduino sketch
 *     
 *     Note - This still contains code supporting the audio system in my office, which is no longer needed.
 *     That includes the office power sequencing code, the speaker switch, and the override switch.
 */

#include <IRLibRecv.h>
#include <IRLibDecodeBase.h>
#include <IRLibSendBase.H>
#include <IRLib_P01_NEC.h>

/*  Build options. */
#define IR_RETRANSMIT_INTERVAL 5*60*1000  /*  Retransmit refrig off code this frequently */

/* 
 *  IR Command codes. 
 */
#define CODE_LIVING_ROOM_POWER   0x8322718E
#define CODE_OFFICE_POWER        0x83228C73
#define CODE_REFRIG_OFF   0x8322639C
#define CODE_REFRIG_ON    0x8322629D


/*  Hardware definitions.  */
#define PIN_IR_IN       2       /* IR receive input */
#define PIN_CMD_OUT     3       /* DC level command to refrig controller */
#define PIN_PWR_SW      7       /* Power switch input */
#define PIN_OVER_SW     8       /* Override switch input */
#define PIN_PWR_A       9       /* Power output A */
#define PIN_PWR_B       10      /* Power output B */
#define PIN_PWR_C       11      /* Power output C */
#define PIN_SPKR_SW     12      /* Speaker switch output--Currently N.C.  */
#define PIN_PILOT       LED_BUILTIN   /* Pilot light */

/* Output pin HIGH level turns power on. */
#define ON              HIGH
#define OFF             LOW

/* Objects for infrared communication. */
IRrecv myReceiver(PIN_IR_IN);
IRdecodeNEC myDecoder;


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
} t_sw;


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
      Serial.println(F("Power system up."));

      /*  
       *   There's some sort of interaction between the controller's  
       *   power supply and relay picking.  Pick each one 
       *   individually to avoid this.
       */
      digitalWrite(PIN_PILOT, ON);
      delay(1*1000);
      digitalWrite(PIN_PWR_A, ON);
      delay(1*1000);
      digitalWrite(PIN_PWR_B, ON);
      delay(1*1000);
      digitalWrite(PIN_PWR_C, ON);

      /* Turn off the refrigerator. */
      digitalWrite(PIN_CMD_OUT, HIGH);
      break;
      
    case SYS_OFFICE_UP:
      digitalWrite(PIN_PILOT, ON);
      digitalWrite(PIN_SPKR_SW, ON);
      digitalWrite(PIN_PWR_A, ON);
      delay(7*1000);
      digitalWrite(PIN_PWR_B, ON);
      break;
      
    default:
      Serial.println(F("Null state transition."));
  }
}


void stateLivingRoomUp(int newSysState)
{
  switch (newSysState){
    case SYS_DOWN:
      Serial.println(F("Power system down."));
      digitalWrite(PIN_PILOT, OFF);
      
      digitalWrite(PIN_PWR_C, OFF);
      delay(1*1000);
      digitalWrite(PIN_PWR_B, OFF);
      delay(1*1000);
      digitalWrite(PIN_PWR_A, OFF);

      /* Turn on the refrigerator. */
      digitalWrite(PIN_CMD_OUT, LOW);
      break;
      
    case SYS_OFFICE_UP:
      digitalWrite(PIN_SPKR_SW, ON);
      digitalWrite(PIN_PWR_C, OFF);

      /* Turn on the refrigerator. */
      digitalWrite(PIN_CMD_OUT, LOW);
      break;
      
    default:
      Serial.println(F("Null state transition."));
  }
}


void stateOfficeUp(int newSysState)
{
  switch (newSysState){
    case SYS_LIVING_ROOM_UP:
      digitalWrite(PIN_PWR_C, ON);
      digitalWrite(PIN_SPKR_SW, OFF);

      /* Turn off the refrigerator. */
      digitalWrite(PIN_CMD_OUT, HIGH);
      break;
      
    case SYS_DOWN:
      digitalWrite(PIN_PILOT, OFF);
      digitalWrite(PIN_PWR_B, OFF);
      delay(5*1000);
      digitalWrite(PIN_PWR_A, OFF);
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
  pinMode(PIN_PWR_A, OUTPUT);
  pinMode(PIN_PWR_B, OUTPUT);
  pinMode(PIN_PWR_C, OUTPUT);
  pinMode(PIN_SPKR_SW, OUTPUT);
  pinMode(PIN_CMD_OUT, OUTPUT);
 
  /* Initialize IRLib2. */
  /*  
   *   The IR repeater lengthens marks, at least partially due to
   *   the slow fall time of the line from the receivers to the 
   *   power controller due to line capacitance.  This compensory 
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

         case CODE_REFRIG_OFF:
           digitalWrite(PIN_CMD_OUT, HIGH);
           break;

         case CODE_REFRIG_ON:
           digitalWrite(PIN_CMD_OUT, LOW);
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
}
