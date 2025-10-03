/*    power_controller.ino - Audio Power Controller Arduino sketch */

/*
 *  This module accepts commands via the Arduino serial-over-USB interface:
 *
 *  Control Commands (set):
 *    set system on      - Turn on the audio power system
 *    set system PIN_OFF     - Turn PIN_OFF the audio power system
 *    set override on    - Turn on the refrigerator override
 *    set override PIN_OFF   - Turn PIN_OFF the refrigerator override
 *
 *  Status Commands:
 *    status system      - Returns "on" or "PIN_OFF" (current system state)
 *    status override    - Returns "on" or "PIN_OFF" (current refrigerator override state)
 *
 *  Response Format:
 *    OK                 - Command executed successfully
 *    on/PIN_OFF             - Status response
 *    ERR <message>      - Error with description
 * 
 *  Reported refrigerator status may not reflect actual state due to the refrigerator controller's
 *  implementation of compressor protection delays.
 * 
 *  Manual control is also possible via the switches on the power controller.  The left-hand
 *  switch toggles system power, while the right-hand switch activates refrigerator override.
 */

/*  Build options. */

/*  Hardware definitions.  */
#define PIN_CMD_OUT     3       /* DC level command to refrig controller */
#define PIN_PWR_SW      7       /* Power switch input */
#define PIN_OVERRIDE_SW 8     /* Refrigerator override switch input */
#define PIN_PWR_A       9       /* Power output A */
#define PIN_PWR_B       10      /* Power output B */
#define PIN_PWR_C       11      /* Power output C */
#define PIN_PILOT       LED_BUILTIN   /* Pilot light */

/* Output pin HIGH level turns it on. */
#define PIN_ON              HIGH
#define PIN_OFF             LOW


/* States things can be in. */
enum {
  STATE_ON,
  STATE_OFF
};


int currentSysState = STATE_OFF;          /* Current system power state */
int currentRefrigState = STATE_OFF;       /* Current refrigerator state */
int currentSoftOverrideState = STATE_OFF;   /* Current software override state */


void ensureSystemState(int newState)
{  
  if (newState == currentSysState)
    return;

  if (newState == STATE_ON) {
    /*
     *   There's some sort of interaction between the controller's
     *   power supply and relay picking.  Pick each one 
     *   individually to avoid this.
     */
    digitalWrite(PIN_PILOT, PIN_ON);
    digitalWrite(PIN_PWR_A, PIN_ON);
    delay(3*1000);
    digitalWrite(PIN_PWR_B, PIN_ON);
    delay(1*1000);
    digitalWrite(PIN_PWR_C, PIN_ON);

    /* Turn off the refrigerator. */
    ensureRefrigState(STATE_OFF);

  } else {

    digitalWrite(PIN_PILOT, PIN_OFF);
    digitalWrite(PIN_PWR_C, PIN_OFF);
    delay(3*1000);
    digitalWrite(PIN_PWR_B, PIN_OFF);
    delay(1*1000);
    digitalWrite(PIN_PWR_A, PIN_OFF);

    /* Turn on the refrigerator. */
    ensureRefrigState(STATE_ON);
  }

  currentSysState = newState;
}


void ensureRefrigState(int newState)
{
  if (newState == currentRefrigState)
    return;

  /* Check if override switch is active (HIGH = on) */
  if (digitalRead(PIN_OVERRIDE_SW) == HIGH)
    if (newState == STATE_OFF)
      return;
  
  /* Set the refrigerator state */
  if (newState == STATE_ON) 
    digitalWrite(PIN_CMD_OUT, PIN_ON);
  else 
    digitalWrite(PIN_CMD_OUT, PIN_OFF);

  currentRefrigState = newState;
}


void sendOkResponse() {
  Serial.println(F("OK"));
}


int parseOnOffAction(String action) {
  if (action.equalsIgnoreCase("on"))
    return 1;
  else if (action.equalsIgnoreCase("off"))
    return 0;
  else
    return -1; // Invalid
}


void processSetCommand(String remainder) {
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
  
  int actionValue = parseOnOffAction(action);
  if (actionValue == -1) {
    Serial.println(F("ERR Invalid action. Use 'on' or 'off'"));
    return;
  }
  
  if (device.equalsIgnoreCase("system")) {
    if (actionValue == 1)  // on
        ensureSystemState(STATE_ON); 
      sendOkResponse(); // Already on
    } else { // PIN_OFF
        ensureSystemState(STATE_OFF);
        sendOkResponse();
      } else
        sendOkResponse(); // Already PIN_OFF
    }
  } else if (device.equalsIgnoreCase("override")) {
    if (actionValue == 1) { // on
      ensureRefrigState(SW_OVERRIDE_on);
      sendOkResponse();
    } else { // PIN_OFF
      ensureRefrigState(REFRIG_PIN_OFF);
      sendOkResponse();
    }
  } else
    Serial.println(F("ERR Invalid device. Use 'system' or 'override'"));
}


void processStatusCommand(String remainder) {
  // Parse "status device"
  if (remainder.equalsIgnoreCase("system")) {
    if (currentSysState == STATE_ON)
      Serial.println(F("on"));
    else
      Serial.println(F("off"));
  } else if (remainder.equalsIgnoreCase("override")) {
    if (currentSoftOverrideState == SW_OVERRIDE_on)
      Serial.println(F("on"));
    else
      Serial.println(F("off"));
  } else
    Serial.println(F("ERR Invalid device. Use 'system' or 'override'"));
}


void processSerialCommands() {
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
    
    if (verb.equalsIgnoreCase("set"))
      processSetCommand(remainder);
    else if (verb.equalsIgnoreCase("status"))
      processStatusCommand(remainder);
    else
      Serial.println(F("ERR Invalid command. Use 'set' or 'status'"));
  }
}


void setup()
{
  /*  Configure serial port for debugging. */
  Serial.begin(9600);
  delay(2000);while(!Serial);     //delay for Leonardo

  /*
   *  Configure input and output pins.
   */
  pinMode(PIN_PWR_SW, INPUT_PULLUP);
  pinMode(PIN_OVERRIDE_SW, INPUT_PULLUP);
  pinMode(PIN_PWR_A, OUTPUT);
  pinMode(PIN_PWR_B, OUTPUT);
  pinMode(PIN_PWR_C, OUTPUT);
  pinMode(PIN_CMD_OUT, OUTPUT);
}


void loop() {

  /*  
   *  Process switch commands. The power switch is momentary;
   *  we don't bother debouncing it because processing it will take a lot longer than
   *  the bounce period.
   */
  if (digitalRead(PIN_PWR_SW) == LOW){
    if (currentSysState == STATE_OFF) 
      ensureSystemState(STATE_ON);
    else
      ensureSystemState(STATE_OFF);
  }

  /* 
   *  Process refrigerator override hardware switch, which is double-throw,
   *  along with software override. 
   */
  int override = digitalRead(PIN_OVERRIDE_SW);
  if ((override == HIGH) || (currentSoftOverrideState == STATE_ON)) {
    ensureRefrigState(STATE_ON);
  } 

  processSerialCommands();
}
