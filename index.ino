/*APS - Estrutura de dados 
Seguidor de linha com leitura de cartões RFID
Autores do projeto: 
  Douglas Ferreira
  Gabriel Ramos
  Jefferson Gomes
  Lucas Oliveira
  Victor Camacho
*/ 

//Importações de bibliotecas
#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal.h>
#include <StackArray.h>

/*DECLARAÇÃO DE VARIAVEIS*/
//Seguidor de linha
#define MotorA_sentido1 2
#define MotorA_sentido2 4
#define MotorB_sentido1 8
#define MotorB_sentido2 14
#define MotorA_PWM 3
#define MotorB_PWM 15

#define veloc0 0
#define veloc1 80
#define veloc2 180
#define veloc3 255

#define Sensor_direita 6
#define Sensor_esquerda 7

bool direita, esquerda;


//RFID

//Pinos Reset e SS módulo MFRC522
#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN);

LiquidCrystal lcd(6, 7, 5, 4, 3, 2);

#define pino_botao_le A2
#define pino_botao_gr A3

MFRC522::MIFARE_Key key;

void setup()
{

  //Seguidor de linha
  Serial.begin(9600);
  pinMode(MotorA_sentido1, OUTPUT);
  pinMode(MotorA_sentido2, OUTPUT);
  pinMode(MotorB_sentido1, OUTPUT);
  pinMode(MotorB_sentido2, OUTPUT);
  pinMode(MotorA_PWM, OUTPUT);
  pinMode(MotorB_PWM, OUTPUT);
  pinMode(Sensor_direita, INPUT);
  pinMode(Sensor_esquerda, INPUT);

  //RFID
  Serial.begin(9600);   //Inicia a serial
  SPI.begin();      //Inicia  SPI bus
  mfrc522.PCD_Init();   //Inicia MFRC522

  //Inicializa o LCD 14x2
  lcd.begin(14, 2);
  loop();

  //Prepara chave - padrao de fabrica = FFFFFFFFFFFFh
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
}

void loop()
{

  //Seguidor de linha
  digitalWrite(MotorA_sentido1, LOW);
  digitalWrite(MotorA_sentido2, HIGH);
  digitalWrite(MotorB_sentido1, HIGH);
  digitalWrite(MotorB_sentido2, LOW);

  direita = digitalRead(Sensor_direita);
  esquerda = digitalRead(Sensor_esquerda);
  Serial.print(direita);
  Serial.print(" || ");
  Serial.println(esquerda);

  //Rodando os motores dependendo das leituras
  //se direita = branco e esquerda = branco: carrinho segue reto
  if (direita != false && esquerda != false) {
    analogWrite(MotorA_PWM, veloc3);
    analogWrite(MotorB_PWM, veloc3);
  }
  //se direita = branco e esquerda = preto: acelera apenas um motor e carrinho virará a direita
  else if (direita != false && esquerda != true) {
    //delay(400);
    analogWrite(MotorA_PWM, veloc3);
    analogWrite(MotorB_PWM, veloc0);
    //delay(400);
  }
  //se a direita = preto e esquerda = branco: acelera apenas um motor e carrinho virará a esquerda
  else if (direita != true && esquerda != false) {
    //delay(400);
    analogWrite(MotorA_PWM, veloc0);
    analogWrite(MotorB_PWM, veloc3);
    //delay(400);
  }
  //se a direita = preto e esquerda = preto, então o carrinho freará
  else if (direita != true && esquerda != true) {
    analogWrite(MotorA_PWM, veloc0);
    analogWrite(MotorB_PWM, veloc0);
  }


  //RFID
  Serial.println("Aproxime o seu cartao do leitor...");
  lcd.clear();
  lcd.print(" Aproxime o seu");
  lcd.setCursor(0, 1);
  lcd.print("cartao do leitor");

  //Aguarda cartao
  while ( ! mfrc522.PICC_IsNewCardPresent())
  {
    delay(100);
  }
  if ( ! mfrc522.PICC_ReadCardSerial())
  {
    return;
  }
  //Mostra UID na serial
  Serial.print("UID da tag : ");
  String conteudo = "";
  byte letra;
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    conteudo.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    conteudo.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();

  //Obtem os dados do setor 0, bloco 1
  byte sector         = 0;
  byte blockAddr      = 1;
  byte trailerBlock   = 7;
  byte status;
  byte buffer[18];
  byte size = sizeof(buffer);

  //Autenticacao usando chave A
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
                                    trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  status = mfrc522.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Read() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
  }

  //armazena o numéro em uma pilha
  StackArray <char> numeros;
  for (byte i = 0; i < 16; i + 2) {
    numeros.push(char(buffer[i] + buffer[i + 1] +  " "));
  }

  //Mostra os dados no Serial Monitor e LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write("Desempilhando:");

  if (numeros.count() == 5) { // Se a quantidade de itens na Pilha for igual a 5
    lcd.setCursor(0, 1);// Coluna 1, linha 2
    int controleColuna = 0;
    for (int i = 0; i < 5; i++) {
      // Mostra os dados e vai desimpelhando
      numeros.setPrinter(Serial);//Mostra o primeiro da pilha no serial
      numeros.setPrinter(lcd);// Mostra o primeiro da pilha no lcd
      numeros.pop();// Retira o que está no topo da pilha
      controleColuna = controleColuna + 3;
      lcd.setCursor(controleColuna, 1);
    }
  }

  Serial.println();

  // Halt PICC
  mfrc522.PICC_HaltA();
  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();
  delay(3000);
}