////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*********** Zuweisung der Variablen **********************************************************************************************************
int  IRLED      = 3;                       // IR-LED an IO 3
int  lamprelais = 2;                       // Relais für die Lampe an IO 2
bool geschaltet = false;                   // ist das Relais gerade an- oder ausgeschaltet?
int  Pause      = 500;                     // die IR-Signale werden manchmal hintereinander gesendet. Damit sie sich nicht verhaspeln, ist eine 
                                           // eine kleine Pause zwischendurch notwendig.

//*********** Vorbereitung der WiFi-Verbindung *************************************************************************************************
#include <ESP8266WiFi.h>
const char* ssid     = "HandyFB";          // Name des AP, wie er in der Liste der verfügbaren WLAN´s erscheint
const char* password = "XXXXXXXXXX";       // hier ein eigenes Password einsetzen
unsigned long urlReqcount;

//*********** Vorbereitung der IR-Bibliothek ***************************************************************************************************
//*********** https://github.com/crankyoldgit/IRremoteESP8266 **********************************************************************************
#include <IRremoteESP8266.h>
#include <IRsend.h>
IRsend irsend(IRLED);                      // IR-LED am IO3 des ESP01 (ESP8266)

//*********** Variablenzuweisung der entschlüsselten IR-Signale ********************************************************************************
uint32_t Panasonicaddress = 0x4004;        // ******* Panasonic address ****
uint64_t PanasonicBreak = 0x40040D00606D;  // ******* Panasonic Pause   **** 
uint64_t PanasonicPlay  = 0x40040D00505D;  // ******* Panasonic Play    ****
uint64_t PanasonicOnOff = 0x40040D00BCB1;  // ******* Panasonic on/off  ****
uint64_t PanasonicStop  = 0x40040D00000D;  // ******* Panasonic stop    ****
uint64_t PanasonicNF    = 0x40040D80169B;  // ******* Panasonic NF      ****
uint32_t JBLaddress     = 0x7080;          // ******* JBL address *********
uint64_t JBLquieter     = 0x10E13EC;       // ******* JBL leiser **********
uint64_t JBLlouder      = 0x10EE31C;       // ******* JBL lauter **********
uint64_t JBLonoff       = 0x212EFF00;      // ******* JBL on/off **********
uint32_t Epsonaddress   = 0x5583;          // ******* EPSON address *******
uint64_t EpsonOnOff     = 0xC1AA09F6;      // ******* EPSON Beamer on/off *
WiFiServer server(80);                     // Verwendung des Servers über Port 80

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  urlReqcount=0;                           // setze die Variablen der URL auf 0 zurück
  //*********** Zuweiszung der GPIO´s ************************************************************************************************************
  pinMode(IRLED, OUTPUT);                  // der IO IRLED sei ein output-Pin
  digitalWrite(IRLED, 0);                  // setze den aktuellen Wert auf 0 zurück
  pinMode(lamprelais, OUTPUT);             // der IO lamprelais sei ein output-Pin
  digitalWrite(lamprelais, 0);             // setze den aktuellen Wert auf 0 zurück
    
  Serial.begin(9600);                      // start serial
  delay(1);                                // 1 µSecunde um die Anweisung in Ruhe zu Ende zu bringen
    
  WiFi.mode(WIFI_AP);                      // versetze den ESP01 in den AP-mode (accesspoint)
  WiFi.softAP(ssid, password);             // übernehme die in der Preambel definierten WLAN-credentials
  server.begin();                          // starte den server

  irsend.begin();                          // bereite den Prozess für das Versenden der IR-Befehle vor
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() { 
  WiFiClient client = server.available();    // Prüfe, ob ein client verbunden ist! Das wäre also z.B. unser Handy
    if (!client) {
    return;
  }

  Serial.println("new client");              // Warte, bis sich ein Client verbindet
  unsigned long ultimeout = millis()+250;
  while(!client.available() && (millis()<ultimeout) ){
    delay(1);
  }

  if(millis()>ultimeout){ 
    Serial.println("client connection time-out!");
    return;                                  // wurde nichts empfangen, dann beginne von vorn und warte einfach weiter
  }

 //*********** wenn die Verbindung mit einem Client hergestellt wurde, wird nun das verarbeitet, was dieser gesendet hat ************************
  String sRequest = client.readStringUntil('\r'); // wenn etwas vom Client empfangen wurde, sei dies ein String mit dem Namen "sRequest"
  client.flush();                            // warte, bis der Client fertig ist mit der Übermittlung und lösche dann den Puffer für den nächsten Empfang

  if(sRequest==""){                          // ist sRequest leer, sende eine Warnung an den serial-Monitor
    Serial.println("leere Antwort!");
    client.stop();                           // weil ja eigentlich doch nichts angekommen ist, beende die Verbindung mit dem client
    return;
  }
  
  //*********** enthält "sRequest" etwas, untersuche den String näher und bilde die Variable "sCmd" für die weitere Verarbeitung ***************
  String sPath="",sParam="", sCmd="";
  String sGetstart="GET ";
  int iStart,iEndSpace,iEndQuest;
  iStart = sRequest.indexOf(sGetstart);
  if (iStart>=0){
    iStart+=+sGetstart.length();
    iEndSpace = sRequest.indexOf(" ",iStart);
    iEndQuest = sRequest.indexOf("?",iStart);
      
    if(iEndSpace>0) {
      if(iEndQuest>0){
        sPath  = sRequest.substring(iStart,iEndQuest);
        sParam = sRequest.substring(iEndQuest,iEndSpace);
      }
      else{
        sPath  = sRequest.substring(iStart,iEndSpace);
      }
    }
  }
  
  //*********** fürs debugging: Zeig mal, was für "sCmd" rausgekommen ist ************************************************************************
  if(sParam.length()>0){
    int iEqu=sParam.indexOf("=");
    if(iEqu>=0){
      sCmd = sParam.substring(iEqu+1,sParam.length());
      Serial.println(sCmd);
    }
  }

 //*********** HTML-Seite des Servers gestalten ************************************************************************************************
  String sResponse,sHeader;

  if(sPath!="/"){
    sResponse="<html><head><title>404 Not Found</title></head><body><h1>Not Found</h1><p>The requested URL was not found on this server.</p></body></html>";
    sHeader  = "HTTP/1.1 404 Not found\r\n";
    sHeader += "Content-Length: ";
    sHeader += sResponse.length();
    sHeader += "\r\n";
    sHeader += "Content-Type: text/html\r\n";
    sHeader += "Connection: close\r\n";
    sHeader += "\r\n";
  }
  else{
    urlReqcount++;
    sResponse  = "<html><head><title>HandyFB</title></head><body>";  
    //             font color: Schriftfarbe    body bgcolor: Bildschirmhintergrund Vgl. hierzu z.B.: https://www.w3schools.com/colors/colors_hex.asp
    sResponse += "<font color=\"#000000\">    <body bgcolor=\"#FFD700\">";

    //*********** CSS-style der Buttons ***********************************************************************************************************
    sResponse += "<style>html {font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}";
    sResponse += ".button1 {background-color: #195B6A; border: none; color: #FFD700; border-radius: 10px; cursor: pointer;"; 
    sResponse += "font-size: 25px; font-weight: bold; padding: 6px; margin: 5px 5px; width: 320px;}";
    sResponse += ".button2 {background-color: #800080; border: none; color: #FFD700; border-radius: 10px; cursor: pointer;"; 
    sResponse += "font-size: 25px; font-weight: bold; padding: 6px; margin: 5px 5px; width: 320px;}";
    sResponse += ".button3 {background-color: #A52A2A; border: none; color: #FFD700; border-radius: 10px; cursor: pointer;"; 
    sResponse += "font-size: 25px; font-weight: bold; padding: 6px; margin: 5px 5px; width: 320px;}";
    sResponse += ".button4 {background-color: #000000; border: none; color: #FFD700; border-radius: 10px; cursor: pointer;"; 
    sResponse += "font-size: 25px; font-weight: bold; padding: 6px; margin: 5px 5px; width: 320px;}";
    sResponse += ".button5 {background-color: ##FFFFE0; border: none; color: #000000; border-radius: 10px; cursor: pointer;"; 
    sResponse += "font-size: 25px; font-weight: bold; padding: 6px; margin: 5px 5px; width: 320px;}";
    sResponse += "</style></head>";
    
    sResponse += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=yes\">";
    
    //*********** Inhalt und Funktionen der Buttons ***********************************************************************************************
    sResponse += "<br /><a>&nbsp;</a>";
    sResponse += "<br /><a href=\"?pin=FUNCTION1\"><button class=\"button1\">Soundbar  on/off</button></a>&nbsp";
    sResponse += "<br /><a href=\"?pin=FUNCTION2\"><button class=\"button1\">Soundbar  lauter</button></a>&nbsp";
    sResponse += "<br /><a href=\"?pin=FUNCTION3\"><button class=\"button1\">Soundbar  leiser</button></a>&nbsp";
    
    sResponse += "<br /><a>&nbsp;</a>";
    sResponse += "<br /><a href=\"?pin=FUNCTION4\"><button class=\"button2\">BR Player on/off</button></a>&nbsp";
    sResponse += "<br /><a href=\"?pin=FUNCTION5\"><button class=\"button2\">BR Player Play</button></a>&nbsp";
    sResponse += "<br /><a href=\"?pin=FUNCTION6\"><button class=\"button2\">BR Player Pause</button></a>&nbsp";
    sResponse += "<br /><a href=\"?pin=FUNCTION10\"><button class=\"button2\">BR Player Stop</button></a>&nbsp";
    sResponse += "<br /><a href=\"?pin=FUNCTION11\"><button class=\"button2\">BR Player Netflix</button></a>&nbsp";
        
    sResponse += "<br /><a>&nbsp;</a>";
    sResponse += "<br /><a href=\"?pin=FUNCTION7\"><button class=\"button3\">Beamer on/off</button></a>&nbsp";

    sResponse += "<br /><a>&nbsp;</a>";
    sResponse += "<br /><a href=\"?pin=FUNCTION8\"><button class=\"button4\">Alle Ger&aumlte on/off</button></a>&nbsp";

    sResponse += "<br /><a>&nbsp;</a>";
    sResponse += "<br /><a href=\"?pin=FUNCTION9\"><button class=\"button5\">Light on/off</button></a>&nbsp";  
    
    //*********** Was soll passieren *********************************************************************************************************
    if (sCmd.length()>0){   
      //    hier aufpassen, der zur Erkennung des Kommandos notwendige Syntax kann von Gerät zu Gerät abweichen. Einfach ausprobieren, bis das Gerät anspricht.
      //    JBL:        irsend.sendNEC      (Comand, Length of comand);
      //    Panasonic:  irsend.sendPanasonic(Panasonicaddress, Comand);
      //    Epson:      irsend.sendEpson    (Comand);
      //    das hängt mit dem Aufbau der Bibliotheken hinter den irsend-Comands zusamen. Da im Zweifelsfall mal reinlesen.

      if(sCmd.indexOf("FUNCTION1")>=0)            //Soundbar on/off
        {
        irsend.sendNEC(JBLonoff, 32);
        delay(Pause);
        }
      else if(sCmd.indexOf("FUNCTION2")>=0)       //Soundbar lauter
        {
        irsend.sendNEC(JBLlouder, 32);
        delay(Pause);
        }
      else if(sCmd.indexOf("FUNCTION3")>=0)       //Soundbar leiser
        {
        irsend.sendNEC(JBLquieter,32);
        delay(Pause);
        }
      else if(sCmd.indexOf("FUNCTION4")>=0)       //BluerayPlayer on/off
        {
        irsend.sendPanasonic(Panasonicaddress, PanasonicOnOff);
        delay(Pause);
        }
      else if(sCmd.indexOf("FUNCTION5")>=0)       //BluerayPlayer  Play
        {
        irsend.sendPanasonic(Panasonicaddress, PanasonicPlay);
        delay(Pause);
        }
      else if(sCmd.indexOf("FUNCTION6")>=0)       //BluerayPlayer  Pause
        {
        irsend.sendPanasonic(Panasonicaddress, PanasonicBreak);
        delay(Pause);  
        }
      else if(sCmd.indexOf("FUNCTION10")>=0)      //BluerayPlayer  Stop
        {
        irsend.sendPanasonic(Panasonicaddress, PanasonicStop);
        delay(Pause);  
        }
      else if(sCmd.indexOf("FUNCTION11")>=0)      //BluerayPlayer  Netflix
        {
        irsend.sendPanasonic(Panasonicaddress, PanasonicNF);
        delay(Pause);  
        } 
      else if(sCmd.indexOf("FUNCTION7")>=0)       //Beamer on/off
        {
        irsend.sendEpson(EpsonOnOff);
        delay(Pause);  
        }
      else if(sCmd.indexOf("FUNCTION8")>=0)       //Beamer, Soundbar und Bluerayplayer alle zusammen an/aus
        {
        irsend.sendNEC(JBLonoff, 32);
        delay(Pause);
        irsend.sendPanasonic(Panasonicaddress, PanasonicOnOff);
        delay(Pause);
        irsend.sendEpson(EpsonOnOff);
        delay(Pause);        
        }
      else if(sCmd.indexOf("FUNCTION9")>=0)       //Licht an/aus
        {
        if (geschaltet == false) {      // welchen Wert hat die Variable "geschaltet" gerade?
        geschaltet = true;              // sollte es false sein, setze "geschaltet" auf true
        }
        else {
        geschaltet = false;             // sollte es true  sein, setze "geschaltet" auf false
        }
        }
        if (geschaltet == true) {       // ist "geschaltet" auf true, setze Relais HIGH
        digitalWrite(lamprelais, HIGH); 
        delay(Pause);      
        }
        if (geschaltet == false) {      // ist "geschaltet" auf false, setze Relais LOW
        digitalWrite(lamprelais, LOW);
        delay(Pause);
        } 
    }

    sHeader  = "HTTP/1.1 200 OK\r\n";
    sHeader += "Content-Length: ";
    sHeader += sResponse.length();
    sHeader += "\r\n";
    sHeader += "Content-Type: text/html\r\n";
    sHeader += "Connection: close\r\n";
    sHeader += "\r\n";
  }
      
  //*********** antworte dem Client das alles empfangen wurde*************************************************************************************
  client.print(sHeader);
  client.print(sResponse);
    
  //*********** beende die Verbindung zum client *************************************************************************************************
  client.stop();
  Serial.println("Client disonnected");


}
