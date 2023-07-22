/*
  Version 2.6 modificada por LU6APA y LU4BA - 28/03/2023
*/
#include <Wire.h>
#include <SPI.h>
#include <LoRa.h>
#include <Adafruit_BMP085.h>

Adafruit_BMP085 bmp;

char Version[] = "v2.6 - 28/03/2023";

#define CS    18      // Pin de CS del módulo LoRa
#define RST   14      // Pin de Reset del módulo LoRa
#define IRQ   7      // Pin del IRQ del módulo LoRa
#define LED   25     // Pin del LED onboard

#define SERIAL_BAUDRATE   115200    // Velocidad del Puerto Serie

#define INTERVAL_TIME_TX  1000    // Cantidad de Segundos entre cada TX

// Configuraciones del módulo LoRa. Tener en cuenta que esta configuración debe ser igual en el Rx!
// 433E6 for Asia
// 866E6 for Europe
// 915E6 for North America 
// 915E6 to 928E3 for Argentina
#define LORA_FREQUENCY      915000000  // Frecuencia en Hz a la que se quiere transmitir.
#define LORA_SYNC_WORD      0xF3       // Byte value to use as the sync word, defaults to 0x12
#define LORA_POWER          17         // TX power in dB, defaults to 17. Supported values are 2 to 20 for PA_OUTPUT_PA_BOOST_PIN, and 0 to 14 for PA_OUTPUT_RFO_PIN.
#define LORA_SPREAD_FACTOR  7          // Spreading factor, defaults to 7. Supported values are between 6 and 12 (En Argentina se puede utilizar entre 7 a 10)
#define LORA_SIG_BANDWIDTH  125E3      // Signal bandwidth in Hz, defaults to 125E3. Supported values are 7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3, 41.7E3, 62.5E3, 125E3, 250E3, and 500E3 
#define LORA_CODING_RATE    5          // Denominator of the coding rate, defaults to 5. Supported values are between 5 and 8, these correspond to coding rates of 4/5 and 4/8. The coding rate numerator is fixed at 4.

double baseline;
double T, P, A;
unsigned int pktNumber = 0;
double bitRate;

void setup()
{
  // Set de Serial
  Serial.begin(SERIAL_BAUDRATE);
  
  // Set Led onboard con Output
  pinMode(LED, OUTPUT);

  // Set de pin RST como Output y se pone a High para que pueda operar
  pinMode(RST, OUTPUT);
  digitalWrite(RST, HIGH);  
    
  //Inicializar módulo LoRa
  LoRa.setPins(CS, RST, IRQ);
    
  while (!LoRa.begin(LORA_FREQUENCY)) 
  {
      Serial.println(".");
      delay(500);
  }
  
  LoRa.setTxPower(LORA_POWER);              
  LoRa.setSpreadingFactor(LORA_SPREAD_FACTOR);           
  LoRa.setSignalBandwidth(LORA_SIG_BANDWIDTH);
  LoRa.setCodingRate4(LORA_CODING_RATE);       
  LoRa.setSyncWord(LORA_SYNC_WORD);    

  // Calculo del BitRate = (SF * (BW / 2 ^ SF)) * (4.0 / CR)
  bitRate = (LORA_SPREAD_FACTOR * (LORA_SIG_BANDWIDTH / pow(2, LORA_SPREAD_FACTOR))) * (4.0 / LORA_CODING_RATE);

  Serial.println("LoRa OK!");
  
  // Inicializar el BMP180
  bool bmpIsInit = false;
  
  Serial.println("Iniciando BMP180 ");

  // Si el BMP180 no inicializa, no arranca la placa y el led onboard queda encendido! 
  digitalWrite(LED, HIGH); 
  
  do {
        delay(100);  
        Wire.begin(4, 15);
        if (bmp.begin(BMP085_STANDARD))
        {
              Serial.println("BMP180 inicio OK");
              readTempPressure();
              baseline = P;
              Serial.print("Presion Base: ");
              Serial.print(baseline);
              Serial.println(" hpa");
              bmpIsInit = true;
              digitalWrite(LED, LOW); 
        } 
        else
        {
          Serial.println("No entro :-(");
        }     
        
      } while (!bmpIsInit);
  
}

void loop()
{
  // // Version v2.5 - 27/05/2022
  // // LoRa BitRate: 5468.75 bps
  // // Temperatura:  22.088 °C 
  // // Presion Base: 1015.081 hpa
  // // Presion Absoluta: 1014.979 hpa | 761.299 mmHg | 14.721 psi
  // // Altura:  0.843 m, 
  // // Packet Number: 641
  // // Enviando datos: [Packet Number],[Presion Base],[Presion Absoluta],[Altura],[Temperatura]
  // // 641,1015.081,1014.979,0.843,22.088
  // // =====================================================================
  
  // Leer Presion, Temperatura y calcular Altura
  readTempPressure();

  Serial.print("TX - Version ");
  Serial.println(Version);

  Serial.print("LoRa BitRate: ");
  Serial.print(bitRate);
  Serial.println(" bps");
    
  Serial.print("Temperatura: ");
  if (T >= 0.0) Serial.print(" ");
  Serial.print(T, 1);
  Serial.println(" °C ");
     
  Serial.print("Presion Base: ");
  Serial.print(baseline * 0.01);
  Serial.println(" hpa");
      
  Serial.print("Presion Absoluta: ");
  Serial.print(P * 0.01, 2);
  Serial.print(" hpa | ");
  Serial.print(P * 0.750063755);
  Serial.print(" mmHg | ");
  Serial.print(P * 0.014503774);
  Serial.println(" psi"); 
  
  Serial.print("Altura: ");
  if (A >= 0.0) Serial.print(" ");
  Serial.print(A, 2);
  Serial.println(" m, ");

  pktNumber++;  
  Serial.print("Packet Number: ");
  Serial.println(pktNumber);
  
  Serial.println("Enviando datos: [Packet Number],[Presion Base],[Presion Absoluta],[Altura],[Temperatura]");
  Serial.print(pktNumber);
  Serial.print(",");
  Serial.print(baseline * 0.01);
  Serial.print(",");
  Serial.print(P * 0.01, 2);
  Serial.print(",");
  Serial.print(A, 2);
  Serial.print(",");
  Serial.println(T, 1);  
    
  //Send LoRa packet to receiver
  digitalWrite(LED, HIGH);    
  
  //Serial.println("POR ENTRAR AL LORA");
  LoRa.beginPacket();
  LoRa.print(pktNumber); 
  LoRa.print(","); 
  LoRa.print(baseline * 0.01);
  LoRa.print(",");
  LoRa.print(P * 0.01, 2);
  LoRa.print(",");
  LoRa.print(A, 2);
  LoRa.print(",");
  LoRa.print(T, 1);
  LoRa.endPacket();
  
  digitalWrite(LED, LOW);

  // Volver a 0 para que no haga overflow
  if (pktNumber >= 65500)
  {
    pktNumber = 0;
  }
  
  Serial.println("=====================================================================");

  // Tiempo de espera entre Tx y Tx
  delay(INTERVAL_TIME_TX);
}

void readTempPressure()
{
  // Funcion que lee la temperatura, presion y altura del módulo BMP180.
  T = bmp.readTemperature();
  P = bmp.readPressure();
  A = bmp.readAltitude(); 
} 


