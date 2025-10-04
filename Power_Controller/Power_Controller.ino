/*    power_controller.ino - Audio Power Controller Arduino sketch */

/*
 *  This module accepts commands via the Arduino serial-over-USB interface:
 *
 *  Control Commands (set):
 *    set system on      - Turn on the audio power system
 *    set system PIN_OFF     - Turn off the audio power system
 *    set override on    - Turn on the refrigerator override
 *    set override PIN_OFF   - Turn off the refrigerator override
 *
 *  Status Commands:
 *    status system      - Returns "on" or "off" (current system state)
 *    status override    - Returns "on" or "off" (current refrigerator override state)
 *
 *  Response Format:
 *    OK                 - Command executed successfully
 *    on/off             - Status response
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
#define PIN_CMD_OUT     3       /* DC level command to refrig controller.   PIN_ON turns it off. */
#define PIN_PWR_SW      7       /* Power switch input */
#define PIN_OVERRIDE_SW 8       /* Refrigerator override switch input */
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
int currentSoftOverrideState = STATE_OFF;   /* Current software override state */


void setSystemState(int newState)
{  
  currentSysState = newState;
  updateOutputPins();
}


void setSoftwareOverideState(int newState)
{
  currentSoftOverrideState = newState;
}



void updateOutputPins(void)
{  
  int pinValue = (currentSysState == STATE_ON) ? PIN_ON : PIN_OFF;

  /*
    *   There's some sort of interaction between the controller's
    *   power supply and relay picking.  Pick each one 
    *   individually to avoid this.
    */
  digitalWrite(PIN_PILOT, pinValue);
  digitalWrite(PIN_PWR_A, pinValue);
  delay(3*1000);
  digitalWrite(PIN_PWR_B, pinValue);
  delay(1*1000);
  digitalWrite(PIN_PWR_C, pinValue);

  /*  
   * Update the refrigerator power setting depending upon system state, and the
   *  software and hardware overrides.
   */
  int overrideActive = (digitalRead(PIN_OVERRIDE_SW) == HIGH) || (currentSoftOverrideState == STATE_ON))
  if (overrideActive)
    pinValue = PIN_OFF
  else
    pinValue = (currentSysState == STATE_ON) ? PIN_ON : PIN_OFF;
  digitalWrite(PIN_CMD_OUT, pinValue);
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
        setSystemState(STATE_ON); 
    else // PIN_OFF
        setSystemState(STATE_OFF);
    sendOkResponse();
    return;
  }
  
  if (device.equalsIgnoreCase("override")) {
    if (actionValue == 1)  // on
      ensureRefrigState(STATE_ON);
    else
      ensureRefrigState(STATE_OFF);
    sendOkResponse();
    return;
  }

  Serial.println(F("ERR Invalid device. Use 'system' or 'override'"));
}


void processStatusCommand(String remainder) {
  
  // Parse "status device"
  if (remainder.equalsIgnoreCase("system")) {
    if (currentSysState == STATE_ON)
      Serial.println(F("on"));
    else
      Serial.println(F("off"));
    return;
  }
  
  if (remainder.equalsIgnoreCase("override")) {
    if (currentSoftOverrideState == STATE_ON)
      Serial.println(F("on"));
    else
      Serial.println(F("off"));
    return;
  } 
    
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
  int hardwareOverrideState = LOW;

  /*  
   *  Process switch commands. The power switch is momentary;
   *  we don't bother debouncing it because processing it will take a lot longer than
   *  the bounce period.
   */
  if (digitalRead(PIN_PWR_SW) == LOW){
    if (currentSysState == STATE_OFF) 
      setSystemState(STATE_ON);
    else
      setSystemState(STATE_OFF);
  }

  /* 
   *  Process refrigerator override hardware switch, which is double-throw. 
   */
  if (digitalRead(PIN_OVERRIDE_SW) != hardwareOverrideState){
    hardwareOverrideState = digitalRead(PIN_OVERRIDE_SW) == PIN_ON;
    updateOutputPins();
  }

  /*  Process commands from the music server. */
  processSerialCommands();
}
