//  ?? Make this more specific once we know which protocols we need.
#include <IRLibAll.h>

/*    power_controller.ino - Audio Power Controller Arduino sketch
 *     
 */
 
int RECV_PIN = 3;

IRrecv myReceiver(RECV_PIN);

IRdecode myDecoder;

void setup()
{
  Serial.begin(9600);
  delay(2000);while(!Serial);//delay for Leonardo
  myReceiver.enableIRIn(); // Start the receiver
  Serial.println(F("Initialization complete."));
}

void loop() {
  if (myReceiver.getResults()) {
    myDecoder.decode();
    myDecoder.dumpResults(false);
    myReceiver.enableIRIn();    //  Restart receiver
  }
}

