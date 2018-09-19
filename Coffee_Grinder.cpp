
#include "Coffee_Grinder.h"

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
			  if(stringOne.equals("TARA")){
				Serial.printf("\ncmd: %s", cmd);
				grinder.tare(); //set offset
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
					grinder.setScaleFaktor();
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
}
	
void Coffee_Grinder::setup(){
	stop();
	
	//default parameter
	mem.grinder.autoMode=0;
	mem.grinder.manuMode=0;
	mem.grinder.setpoint_weight=17;
	mem.grinder.updateTime = 500;
	
	//read scale_faktor from memory
	EEPROM.get( 0, scale_faktor );
	EEPROM.commit();
	EEPROM.end();
  
	//setup scale
	scale.begin(m_doutPin, m_sckPin);
	scale.set_scale(scale_faktor);
	scale.tare(); //Reset the scale to 0
	if(scale.is_connected){
		Serial.println("HX711 is connected");
	}
	else{
		Serial.println("HX711 is not connected");
	}
	mem.grinder.scale_is_connected = scale.is_connected;
	
	//setup webSocket
	webSocket.begin();                                // start webSocket server
	webSocket.onEvent(webSocketEvent);                // callback function
}
void Coffee_Grinder::loop(){
	//read scale
	weight = scale.get_units(1);
	
	//if scale connected change than message over webSocket
	if(mem.grinder.scale_is_connected!=scale.is_connected){
    webSocket.sendTXT(socketNumber, "wpMeter,UI," + String(mem.webSetting[0]) + ","+String(mem.webSetting[1])+ ","+String(mem.webSetting[2])+ ","+String(mem.webSetting[3])+ ","+String(mem.webSetting[4])+ ",1");
	  mem.grinder.scale_is_connected=scale.is_connected;
  }

	//send every X seconds a value to Websocket
	currentTime = millis();
    if((currentTime - loopTime) > mem.grinder.updateTime) {                            
        String temp_str;
        temp_str = String(weight);;
        webSocket.sendTXT(socketNumber, "wpMeter,Arduino," + temp_str + ",1");
        loopTime = currentTime;// typical runtime this IF{} == 300uS - 776uS measured
    }
	
	//todo: process state machine
	if(((weight < mem.grinder.setpoint_weight)&&(mem.grinder.autoMode==1)&& mem.grinder.scale_is_connected) || (mem.grinder.manuMode==1))
	{
		start();
	}
	else{
	if(mem.grinder.autoMode==1){
		mem.grinder.autoMode = 0;
	}
		stop();
	}
	
	//process webSocket
	webSocket.loop();
	
}

void Coffee_Grinder::setScaleFaktor(){
	scale_faktor = (float) ((scale.read_average()-scale_offset) / mem.grinder.calibration_weight);
	Serial.print("\nscale: ");
	Serial.println(scale_faktor);
	//calibration_factor=scale_faktor;
	scale.set_offset(scale_offset);
	scale.set_scale(scale_faktor);
	
	//save scale_faktor
	EEPROM.begin(512);
	EEPROM.put(0, scale_faktor);
	EEPROM.commit();
	EEPROM.end();
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
}

void Coffee_Grinder::start(){
	digitalWrite(m_relaisPin, 1^neg);
}

void Coffee_Grinder::stop(){
	digitalWrite(m_relaisPin, 0^neg);
}
