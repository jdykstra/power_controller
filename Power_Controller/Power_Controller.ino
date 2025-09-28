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
 * 
 *  Manual control is also possible via the switches on the power controller.  The left-hand
 *  switch toggles system power, while the right-hand switch locks out refrigerator control.
 */

/*  Build options. */

/*  Hardware definitions.  */
#define PIN_CMD_OUT     3       /* DC level command to refrig controller */
#define PIN_PWR_SW      7       /* Power switch input */
#define PIN_REFRIG_OVERRIDE 8   /* Refrigerator override switch input */
#define PIN_PWR_A       9       /* Power output A */
#define PIN_PWR_B       10      /* Power output B */
#define PIN_PWR_C       11      /* Power output C */
#define PIN_PILOT       LED_BUILTIN   /* Pilot light */

/* Output pin HIGH level turns power on. */
#define ON              HIGH
#define OFF             LOW


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
int prevOverrideState = LOW;          /* Previous state of override switch (LOW = off) */


void setSystemState(int newState)
{
  if (newState == SYS_UP) {
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
    setRefrigState(REFRIG_OFF);
  } else {
    digitalWrite(PIN_PILOT, OFF);

    digitalWrite(PIN_PWR_C, OFF);
    delay(3*1000);
    digitalWrite(PIN_PWR_B, OFF);
    delay(1*1000);
    digitalWrite(PIN_PWR_A, OFF);

    /* Turn on the refrigerator. */
    setRefrigState(REFRIG_ON);
  }

  currentSysState = newState;
}


void setRefrigState(int newState)
{
  /* Check if override switch is active (HIGH = on) */
  if (digitalRead(PIN_REFRIG_OVERRIDE) == HIGH)
    if (newState == REFRIG_OFF)
      return;
  
  /* Set the refrigerator state */
  if (newState == REFRIG_ON) {
    digitalWrite(PIN_CMD_OUT, LOW);
    currentRefrigState = REFRIG_ON;
  } else {
    digitalWrite(PIN_CMD_OUT, HIGH);
    currentRefrigState = REFRIG_OFF;
  }
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
    if (actionValue == 1) { // on
      if (currentSysState == SYS_DOWN) {
        setSystemState(SYS_UP);
        sendOkResponse();
      } else
        sendOkResponse(); // Already on
    } else { // off
      if (currentSysState != SYS_DOWN) {
        setSystemState(SYS_DOWN);
        sendOkResponse();
      } else
        sendOkResponse(); // Already off
    }
  } else if (device.equalsIgnoreCase("refrigerator")) {
    if (actionValue == 1) { // on
      setRefrigState(REFRIG_ON);
      sendOkResponse();
    } else { // off
      setRefrigState(REFRIG_OFF);
      sendOkResponse();
    }
  } else
    Serial.println(F("ERR Invalid device. Use 'system' or 'refrigerator'"));
}


void processStatusCommand(String remainder) {
  // Parse "status device"
  if (remainder.equalsIgnoreCase("system")) {
    if (currentSysState == SYS_UP)
      Serial.println(F("on"));
    else
      Serial.println(F("off"));
  } else if (remainder.equalsIgnoreCase("refrigerator")) {
    if (currentRefrigState == REFRIG_ON)
      Serial.println(F("on"));
    else
      Serial.println(F("off"));
  } else
    Serial.println(F("ERR Invalid device. Use 'system' or 'refrigerator'"));
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
  pinMode(PIN_REFRIG_OVERRIDE, INPUT_PULLUP);
  pinMode(PIN_PWR_A, OUTPUT);
  pinMode(PIN_PWR_B, OUTPUT);
  pinMode(PIN_PWR_C, OUTPUT);
  pinMode(PIN_CMD_OUT, OUTPUT);
}


void loop() {

  /* Process switch commands. We don't bother debouncing the
   *  power switch;  processing it will take a lot longer than
   *  the bounce period.
   */
  if (digitalRead(PIN_PWR_SW) == LOW){
    if (currentSysState == SYS_DOWN) 
      setSystemState(SYS_UP);
    else
      setSystemState(SYS_DOWN);
  }

  /* Process refrigerator override switch */
  int currentOverrideState = digitalRead(PIN_REFRIG_OVERRIDE);
  if (currentOverrideState != prevOverrideState) {
    /* Override switch state changed */
    if (currentOverrideState == HIGH)
      /* Override switch turned on - if refrigerator is off, turn it on */
      if (currentRefrigState == REFRIG_OFF)
        setRefrigState(REFRIG_ON);
    /* When override switch is turned off, do not change refrigerator state */
    prevOverrideState = currentOverrideState;
  }

  processSerialCommands();
}
