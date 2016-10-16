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

/* Control outputs */
typedef enum {
  CTL_SRC = 0,                  /* Source power */
  CTL_HIGH_AMP,                 /* High amp power */
  CTL_LOW_AMP,                  /* Low amp power */
  CTL_SPKR_SW,                  /* Speaker switch */
  N_CTL                         /* Number of controls */
} ctls;

/* Control output states. */
typedef enum {
  DOWN = 0,
  UP = 1,
  N_STATE                       /* Number of states */
} ctl_state;

/* Track control output states. */
typedef struct {
  int         pin;              /* Associated pin number */
  ctl_state   current;          /* Current state (up or down) */
  int         delay[N_STATE];   /* ms. to delay after entering new state */
  char        *name;            /* Name (for debugging) */
} t_ctlOutput;

t_ctlOutput ctlOutputs[N_CTL] = {
    {PIN_SRC_PWR, DOWN, {1*1000, 2*1000}, "Source"},
    {PIN_HIGH_PWR, DOWN, {5*1000, 2*1000}, "High amp"},
    {PIN_LOW_PWR, DOWN, {1*1000, 2*1000}, "Low amp"},
    {PIN_SPKR_SW, DOWN, {100, 100}, "Speaker switch"},
};


/* Set a control output to the specified value. */
void setCtl(ctls ctl, ctl_state newState)
{
  t_ctlOutput *ctlStruct = &ctlOutputs[ctl];
  Serial.print(ctlStruct->name);

  if (newState == ctlStruct->current){
    Serial.print(F(" still at "));
    Serial.println(newState, DEC);
    return;
  }

  /* 
   *  The state values are chosen so that they can be
   *  passed directly to digitalWrite().
   */
  digitalWrite(ctlStruct->pin, newState);
  ctlStruct->current = newState;

  Serial.print(F(" set to "));
  Serial.print(newState, DEC);
  Serial.print(F("; delaying "));
  Serial.println(ctlStruct->delay[newState], DEC);
  delay(ctlStruct->delay[newState]);
}


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

/* Commands. */
enum {
  CMD_LIVING_ROOM_POWER,
  CMD_OFFICE_POWER,
};


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
      setCtl(CTL_SPKR_SW, DOWN);
      setCtl(CTL_SRC, UP);
      setCtl(CTL_LOW_AMP, UP);
      setCtl(CTL_HIGH_AMP, UP);

      /* Turn off the refrigerator. */
      /* ??  Dummy code for now. */
      sendIRCommand(0x58A741BE);  
      break;
      
    case SYS_OFFICE_UP:
      setCtl(CTL_SPKR_SW, UP);
      setCtl(CTL_SRC, UP);
      setCtl(CTL_HIGH_AMP, UP);
      break;
      
    default:
      Serial.println(F("Null state transition."));
  }
}


void stateLivingRoomUp(int newSysState)
{
  switch (newSysState){
    case SYS_DOWN:
      setCtl(CTL_HIGH_AMP, DOWN);
      setCtl(CTL_LOW_AMP, DOWN);
      setCtl(CTL_SRC, DOWN);
      setCtl(CTL_SPKR_SW, DOWN);

      /* Turn on the refrigerator. */
      /* ??  Dummy code for now. */
      sendIRCommand(0x58A741BE);  
      break;
      
    case SYS_OFFICE_UP:
      setCtl(CTL_LOW_AMP, DOWN);
      setCtl(CTL_SPKR_SW, UP);

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
      setCtl(CTL_LOW_AMP, UP);
      setCtl(CTL_SPKR_SW, DOWN);

      /* Turn off the refrigerator. */
      /* ??  Dummy code for now. */
      sendIRCommand(0x58A741BE);  
      break;
      
    case SYS_DOWN:
      setCtl(CTL_HIGH_AMP, DOWN);
      setCtl(CTL_LOW_AMP, DOWN);
      setCtl(CTL_SRC, DOWN);
      setCtl(CTL_SPKR_SW, DOWN);
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

