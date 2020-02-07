// Programa para monitoreo de Sala de Servidores
// Creado por Benjamin Cortez 01/2020


// Librerias
#include <EtherCard.h> // Libreria ENC28J60
#include <DHT.h> // Libreria Sensor DHT11
#include <ThingSpeak.h> // Libreria ThingSpeak

#include <avr/wdt.h>

DHT dht(2,DHT11);

#define APIKEY "2Y7M6JQHLCRAL7RX" // APIKEY ThingSpeak Escritura

// Configuración de Variables
static byte mymac[] = { 0x98,0x25,0x37,0x14,0xe4,0xa6 };
const char website[] PROGMEM = "api.thingspeak.com";
byte Ethernet::buffer[700];
Stash stash;
byte session;

//Contador
int res = 100;


void initialize_ethernet(void){
  for(;;){ // Realizar hasta que sea exitoso
    //Reinicia el modulo ethernet
    //Serial.println("Reiniciando Modulo Ethernet...");
   // digitalWrite(10, LOW);
    //delay(2000);
    //digitalWrite(10, HIGH);
    //delay(2000);

    // Cambiar 'SS' si esque el ENC28J60 esta en otro pin que no sea el 10
    if (ether.begin(sizeof Ethernet::buffer, mymac, SS) == 0){
      Serial.println( "ERROR >>> No se accedió al controlador Ethernet");
      continue;
    }

     Serial.println( "Se accedió al controlador Ethernet");

    if (!ether.dhcpSetup()){
      Serial.println("ERROR >>> Falló el DHCP");
      wdt_enable(WDTO_15MS); // turn on the WatchDog and don't stroke it.
    for(;;) { 
      // do nothing and wait for the eventual...
    } 
    continue;
    }
    
    ether.printIp("IP:  ", ether.myip);
    ether.printIp("GW:  ", ether.gwip);
    ether.printIp("DNS: ", ether.dnsip);

    if (!ether.dnsLookup(website))
      Serial.println("ERROR >>> Ha fallado la resolución DNS");

    ether.printIp("SRV: ", ether.hisip);

  /*  while (ether.clientWaitingGw())
    ether.packetLoop(ether.packetReceive());
*/
    //reset init value
    res = 180;
    break;
  }
}

void setup () {
  Serial.begin(9600);
  Serial.println("\n[Iniciando programa]");
  MCUSR = 0;

  // Inicia el sensor DHT11
  dht.begin();
  Serial.println("[Sensor DHT11 Inicializado]");
  
  //Inicia el modulo Ethernet
  initialize_ethernet();


  
}

void loop () {
  //Si no hay réplica en el paquete enviado, reinicia el modulo ethernet
  if (res > 220){
   // initialize_ethernet();
   wdt_enable(WDTO_15MS); // turn on the WatchDog and don't stroke it.
    for(;;) { 
      // do nothing and wait for the eventual...
    } 
  }

  res = res + 1;

  ether.packetLoop(ether.packetReceive());

    // Captura los datos del DHT11
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    float ind = dht.computeHeatIndex(t,h,false); 
    Serial.println(res);

// 200 es 1 min, 1000 son 5 min
  if (res == 200) {

  
    Serial.println(">>> El sensor se ha actualizado");
    // Prepara el Stash para enviar la información a ThingSpeak
    byte sd = stash.create();
    stash.print("field1=");
    stash.print(t);
    stash.print("&field2=");
    stash.print(h);
    stash.print("&field3=");
    stash.print(ind);
    stash.save();

    //Muestra la información que se enviará
    Serial.println(">>>=========<<<");
    Serial.print("TEMP: ");
    Serial.print(t);
    Serial.print("°C\nHUM: ");
    Serial.print(h);
    Serial.print(" %\nIND: ");
    Serial.print(ind);
    Serial.println(" %C");
    Serial.println(">>>=========<<<");


    // Genera la solicitud POST para enviar la informacion
    Stash::prepare(PSTR("POST /update HTTP/1.1" "\r\n"
      "Host: $F" "\r\n"
      "Connection: close" "\r\n"
      "X-THINGSPEAKAPIKEY: $F" "\r\n"
      "Content-Type: application/x-www-form-urlencoded" "\r\n"
      "Content-Length: $D" "\r\n"
      "\r\n"
      "$H"),
    website, PSTR(APIKEY), stash.size(), sd);

    
    // Envia el paquete
    session = ether.tcpSend();
    // Genera la solicitud POST para enviar la informacion
    Stash::prepare(PSTR("POST /update HTTP/1.1" "\r\n"
      "Host: $F" "\r\n"
      "Connection: close" "\r\n"
      "X-THINGSPEAKAPIKEY: $F" "\r\n"
      "Content-Type: application/x-www-form-urlencoded" "\r\n"
      "Content-Length: $D" "\r\n"
      "\r\n"
      "$H"),
    website, PSTR(APIKEY), stash.size(), sd);

    // Envia el paquete
    session = ether.tcpSend();

 //añadido de: http://jeelabs.net/boards/7/topics/2241
 int freeCount = stash.freeCount();
    if (freeCount <= 3) {   Stash::initMap(56); }
  }

   const char* reply = ether.tcpReply(session);

   if (reply != 0) {
     res = 0;
     Serial.println(F(" >>>Respuesta recibida...."));
     Serial.println(reply);
   }
   delay(300);
}
