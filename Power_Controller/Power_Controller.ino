/*    power_controller.ino - Audio Power Controller Arduino sketch */

/*
 *  This module accepts commands via the Arduino serial-over-USB interface:
 *
 *  Control Commands (set):
 *    set system on      - Turn on the audio power system
 *    set system off     - Turn off the audio power system
 *    set refrigerator on    - Turn on the refrigerator
 *    set refrigerator off   - Turn off the refrigerator
 *
 *  Status Commands:
 *    status system      - Returns "on" or "off" (current system state)
 *    status refrigerator - Returns "on" or "off" (current refrigerator state)
 *
 *  Response Format:
 *    OK                 - Command executed successfully
 *    on/off             - Status response
 *    ERR <message>      - Error with description
 * 
 *  Reported efrigerator status may not reflect actual state due to the refrigerator controller's
 *  implementation of compressor protection delays.
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
#define CODE_SYSTEM_POWER   0x8322718E
#define CODE_REFRIG_OFF   0x8322639C
#define CODE_REFRIG_ON    0x8322629D


/*  Hardware definitions.  */
#define PIN_IR_IN       2       /* IR receive input */
#define PIN_CMD_OUT     3       /* DC level command to refrig controller */
#define PIN_PWR_SW      7       /* Power switch input */
#define PIN_PWR_A       9       /* Power output A */
#define PIN_PWR_B       10      /* Power output B */
#define PIN_PWR_C       11      /* Power output C */
#define PIN_PILOT       LED_BUILTIN   /* Pilot light */

/* Output pin HIGH level turns power on. */
#define ON              HIGH
#define OFF             LOW

/* Objects for infrared communication. */
IRrecv myReceiver(PIN_IR_IN);
IRdecodeNEC myDecoder;


/* Switch inputs. */
typedef enum {
  SW_SYS_PWR = 0,                /* System power */
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
  SYS_UP
};

/* Refrigerator states. */
enum {
  REFRIG_ON,
  REFRIG_OFF
};

int currentSysState = SYS_DOWN;
int currentRefrigState = REFRIG_ON;  /* Default state when system is down */

void stateSysDown(int newSysState)
{
  switch (newSysState){
    case SYS_UP:
      Serial.println(F("Power system up."));

      /*
       *   There's some sort of interaction between the controller's
       *   power supply and relay picking.  Pick each one
       *   individually to avoid this.
       */
      digitalWrite(PIN_PILOT, ON);
      delay(1*1000);
      digitalWrite(PIN_PWR_A, ON);
      delay(3*1000);
      digitalWrite(PIN_PWR_B, ON);
      delay(1*1000);
      digitalWrite(PIN_PWR_C, ON);

      /* Turn off the refrigerator. */
      digitalWrite(PIN_CMD_OUT, HIGH);
      currentRefrigState = REFRIG_OFF;
      break;

    default:
      Serial.println(F("Null state transition."));
  }
}


void stateSysUp(int newSysState)
{
  switch (newSysState){
    case SYS_DOWN:
      Serial.println(F("Power system down."));
      digitalWrite(PIN_PILOT, OFF);

      digitalWrite(PIN_PWR_C, OFF);
      delay(3*1000);
      digitalWrite(PIN_PWR_B, OFF);
      delay(1*1000);
      digitalWrite(PIN_PWR_A, OFF);

      /* Turn on the refrigerator. */
      digitalWrite(PIN_CMD_OUT, LOW);
      currentRefrigState = REFRIG_ON;
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

    case SYS_UP:
      stateSysUp(newSysState);
      break;
  }

  currentSysState = newSysState;
}


void cmdSysPower()
{
  switch (currentSysState){
    case SYS_DOWN:
      setSysState(SYS_UP);
      break;

    case SYS_UP:
      setSysState(SYS_DOWN);
      break;
  }
}



void processIRCommands()
{
  /* Process IR commands. */
  if (myReceiver.getResults()) {
    myDecoder.decode();
    Serial.print(F("IR protocol "));
    Serial.print(myDecoder.protocolNum, DEC);
    Serial.print(F(" value "));
    Serial.println(myDecoder.value, HEX);
    if (myDecoder.protocolNum == NEC){
      switch (myDecoder.value){

        case CODE_SYSTEM_POWER:
          cmdSysPower();
          break;

         case CODE_REFRIG_OFF:
           digitalWrite(PIN_CMD_OUT, HIGH);
           currentRefrigState = REFRIG_OFF;
           break;

         case CODE_REFRIG_ON:
           digitalWrite(PIN_CMD_OUT, LOW);
           currentRefrigState = REFRIG_ON;
           break;
      }
    }
    myReceiver.enableIRIn();    //  Restart receiver
  }
}


void sendOkResponse()
{
  Serial.println(F("OK"));
}


void processSerialCommands()
{
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    // Parse command format: "set|status device [action]"
    int spaceIndex = command.indexOf(' ');
    if (spaceIndex == -1) {
      Serial.println(F("ERR Invalid command format. Use: set|status device [action]"));
      return;
    }
    
    String verb = command.substring(0, spaceIndex);
    String remainder = command.substring(spaceIndex + 1);
    
    // Remove any extra whitespace
    verb.trim();
    remainder.trim();
    
    if (verb.equalsIgnoreCase("set")) {
      // Parse "set device action"
      int deviceSpaceIndex = remainder.indexOf(' ');
      if (deviceSpaceIndex == -1) {
        Serial.println(F("ERR Invalid set command format. Use: set device action"));
        return;
      }
      
      String device = remainder.substring(0, deviceSpaceIndex);
      String action = remainder.substring(deviceSpaceIndex + 1);
      device.trim();
      action.trim();
      
      if (device.equalsIgnoreCase("system")) {
        if (action.equalsIgnoreCase("on")) {
          if (currentSysState == SYS_DOWN) {
            cmdSysPower();
            sendOkResponse();
          } else {
            sendOkResponse(); // Already on
          }
        } else if (action.equalsIgnoreCase("off")) {
          if (currentSysState != SYS_DOWN) {
            cmdSysPower();
            sendOkResponse();
          } else {
            sendOkResponse(); // Already off
          }
        } else {
          Serial.println(F("ERR Invalid action. Use 'on' or 'off'"));
        }
      } else if (device.equalsIgnoreCase("refrigerator")) {
        if (action.equalsIgnoreCase("on")) {
          digitalWrite(PIN_CMD_OUT, LOW);
          currentRefrigState = REFRIG_ON;
          sendOkResponse();
        } else if (action.equalsIgnoreCase("off")) {
          digitalWrite(PIN_CMD_OUT, HIGH);
          currentRefrigState = REFRIG_OFF;
          sendOkResponse();
        } else {
          Serial.println(F("ERR Invalid action. Use 'on' or 'off'"));
        }
      } else {
        Serial.println(F("ERR Invalid device. Use 'system' or 'refrigerator'"));
      }
    } else if (verb.equalsIgnoreCase("status")) {
      // Parse "status device"
      if (remainder.equalsIgnoreCase("system")) {
        if (currentSysState == SYS_UP) {
          Serial.println(F("on"));
        } else {
          Serial.println(F("off"));
        }
      } else if (remainder.equalsIgnoreCase("refrigerator")) {
        if (currentRefrigState == REFRIG_ON) {
          Serial.println(F("on"));
        } else {
          Serial.println(F("off"));
        }
      } else {
        Serial.println(F("ERR Invalid device. Use 'system' or 'refrigerator'"));
      }
    } else {
      Serial.println(F("ERR Invalid command. Use 'set' or 'status'"));
    }
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
  pinMode(PIN_PWR_A, OUTPUT);
  pinMode(PIN_PWR_B, OUTPUT);
  pinMode(PIN_PWR_C, OUTPUT);
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

  /* Process switch commands. We don't bother debouncing the
   *  power switch;  processing it will take a lot longer than
   *  the bounce period.
   */
  if (digitalRead(PIN_PWR_SW) == LOW){
    cmdSysPower();
  }

  processIRCommands();
  processSerialCommands();
}
