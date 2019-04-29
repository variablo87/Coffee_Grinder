
#include "Coffee_Grinder.h"
#include "version.h"

#include <WebSocketsServer.h>
WebSocketsServer webSocket(81);               // Create a Websocket server
extern Coffee_Grinder grinder;

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) {
	switch (type) { 
		case WStype_DISCONNECTED:
		  // Reset the control for sending samples of ADC to idle to allow for web server to respond.
		  Serial.printf("[%u] Disconnected!\n", num);
		  yield();
		  break;

		case WStype_CONNECTED: {   // Braces required http://stackoverflow.com/questions/5685471/error-jump-to-case-label
		  IPAddress ip = webSocket.remoteIP(num);
		  Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
		  yield();
		  grinder.socketNumber = num;
		  break;
		  }

		case WStype_TEXT:{
			
			char * pch;
			int i;
			char * cmd;
			
			pch = strtok((char*)payload,",\"][");
			cmd = pch;
			String stringOne = cmd;
			if(pch != NULL){
			  pch = strtok(NULL,",\"][");
			  if(stringOne.equals("TARA")||stringOne.equals("GET_DEFAULT")){
					Serial.printf("\ncmd: %s", cmd);
					grinder.tare(); //set offset
					//webSocket.sendTXT(grinder.socketNumber, "wpMeter,LOG," + GIT_ID+"</br>"+BUILD_DATE + ",1");
					webSocket.sendTXT(grinder.socketNumber, "wpMeter,LOG," + String(GIT_ID) + ",1");
			  }
			  else if(stringOne.equals("SET")){
					Serial.printf("\ncmd: %s [", cmd);
					i=0;
					while (pch != NULL){
						if(i<4){ 
						grinder.mem.webSetting[i++]=atoi(pch);
						}
						Serial.printf("%s,", pch);
						pch = strtok(NULL,",\"][");
					}
					Serial.printf("]");
			  }
			  else if(stringOne.equals("CALIB")){
					Serial.printf("\ncmd: %s [", cmd);
					i=5;
					while (pch != NULL){
						if(i<6){ 
						grinder.mem.webSetting[i++]=atoi(pch);
						}
						Serial.printf("%s,", pch);
						pch = strtok(NULL,",\"][");
					}
					Serial.printf("]");
					if(grinder.mem.grinder.calibration_weight == 0)
					{
						grinder.resetScale();
					}
					else{
						grinder.setScaleFactor();
					}
			  }
			}
			
			//send anwser
			String returnString = "wpMeter,UI";
			for (int i=0;i<5;i++){ returnString = returnString + "," + String(grinder.mem.webSetting[i]); }
			returnString = returnString + '1';
			webSocket.sendTXT(grinder.socketNumber, returnString);
			//webSocket.sendTXT(socketNumber, "wpMeter,UI," + String(mem.webSetting[0]) + ","+String(mem.webSetting[1])+ ","+String(mem.webSetting[2])+ ","+String(mem.webSetting[3])+ ","+String(mem.webSetting[4])+ ",1");
			yield();
		  
		  break;
		}
		case WStype_ERROR:
		  Serial.printf("Error [%u] , %s\n", num, payload);
		  yield();
  }
}

Coffee_Grinder::Coffee_Grinder(int relaisPin,int outNegated,int sckPin, int doutPin)
{
  pinMode(relaisPin, OUTPUT);
  m_relaisPin = relaisPin;
  neg = outNegated;
  m_sckPin = sckPin;
  m_doutPin = doutPin;
	m_state = WAIT;
}
	
void Coffee_Grinder::setup(){
	stop();
	
	//default parameter
	mem.grinder.autoMode=0;
	mem.grinder.manuMode=0;
	mem.grinder.setpoint_weight=17;
	mem.grinder.updateTime = 500;
	
	//read scale_factor from memory
	loadConfig();
	Serial.print("scale: ");
	Serial.println(scale_factor);

	//setup scale
	scale.begin(m_doutPin, m_sckPin);
	scale.set_scale(scale_factor);
	scale.tare(); //Reset the scale to 0

	if (scale.wait_ready_timeout(1000)) {
  	Serial.println("HX711 is connected");
		mem.grinder.scale_is_connected = 1;
	} else {
		Serial.println("HX711 not found.");
		mem.grinder.scale_is_connected = 0;
	}
	
	//setup webSocket
	webSocket.begin();                                // start webSocket server
	webSocket.onEvent(webSocketEvent);                // callback function
}
void Coffee_Grinder::loop(){
	//read scale
	if (scale.wait_ready_timeout(1000)) {
  	//read scale
		weight = scale.get_units(1);
		weight = (floor((weight*10)+0.5))/10;

		//if scale connected change than message over webSocket
		if(mem.grinder.scale_is_connected==0){
			webSocket.sendTXT(socketNumber, "wpMeter,UI," + String(mem.webSetting[0]) + ","+String(mem.webSetting[1])+ ","+String(mem.webSetting[2])+ ","+String(mem.webSetting[3])+ ","+String(mem.webSetting[4])+ ",1");
			mem.grinder.scale_is_connected = 1;
		}	
	} else {
		//if scale connected change than message over webSocket
		if(mem.grinder.scale_is_connected==1){
			webSocket.sendTXT(socketNumber, "wpMeter,UI," + String(mem.webSetting[0]) + ","+String(mem.webSetting[1])+ ","+String(mem.webSetting[2])+ ","+String(mem.webSetting[3])+ ","+String(mem.webSetting[4])+ ",1");
			mem.grinder.scale_is_connected = 0;
		}
	}

	//send every X seconds a value to Websocket
	currentTime = millis();
    if((currentTime - loopTime) > mem.grinder.updateTime) {                            
        String temp_str;
        temp_str = String(weight);
        webSocket.sendTXT(socketNumber, "wpMeter,Arduino," + temp_str + ",1");
        loopTime = currentTime;// typical runtime this IF{} == 300uS - 776uS measured
    }
	
	stateMachine();
	
	//process webSocket
	webSocket.loop();
	
}

void Coffee_Grinder::setScaleFactor(){
	scale_factor = (float) ((scale.read_average()-scale_offset) / mem.grinder.calibration_weight);
	Serial.print("\nscale: ");
	Serial.println(scale_factor);
	//calibration_factor=scale_factor;
	scale.set_offset(scale_offset);
	scale.set_scale(scale_factor);
	logbook("offset",scale_offset);
	logbook("factor",scale_factor);
	saveConfig();
}

void Coffee_Grinder::resetScale(){
	Serial.printf("\nreset scale");
	scale.set_scale();
	scale.set_offset(0);
	scale_offset= scale.read_average();
	Serial.print("\noffset: ");
	Serial.println(scale_offset);
}

void Coffee_Grinder::tare(){
  scale.tare();
	logbook("factor",scale_factor);
}

void Coffee_Grinder::start(){
	digitalWrite(m_relaisPin, 1^neg);
}

void Coffee_Grinder::stop(){
	digitalWrite(m_relaisPin, 0^neg);
}

void Coffee_Grinder::loadConfig() {
  // Loads configuration from EEPROM into RAM
  EEPROM.begin(4095);
  EEPROM.get( 0, scale_factor );
  EEPROM.end();
}

void Coffee_Grinder::saveConfig() {
  // Save configuration from RAM into EEPROM
  EEPROM.begin(4095);
  EEPROM.put( 0, scale_factor );
  delay(200);
  EEPROM.commit();                      // Only needed for ESP8266 to get data written
  EEPROM.end();                         // Free RAM copy of structure
}

void Coffee_Grinder::logbook(String statement,float value ){
	String temp_str = String(value);
	webSocket.sendTXT(socketNumber, "wpMeter,LOG," + statement+": "+temp_str + ",1");
}

void Coffee_Grinder::stateMachine() {

	switch(m_state){
    case WAIT:{
			if(mem.grinder.autoMode==1){
				start();
				m_state = FILL;
				mem.grinder.autoMode = 0;
				logbook("FILL ON",mem.grinder.setpoint_weight);
			}
			if(mem.grinder.manuMode==1){
				start();
				m_state = MANUAL;
				logbook("MANUAL ON",0);
			}
			break;
		}
    case FILL:{
			if(weight >= mem.grinder.setpoint_weight){
				stop();
				m_state = WAIT;
				logbook("FILL OFF",weight);
			}
			break;
		}
    case MANUAL:{
			if(mem.grinder.manuMode==0){
				stop();
				m_state = WAIT;
				logbook("MANUAL OFF",0);
			}
			break;
		}
    default: {
			
			break;
		}
	}
}
