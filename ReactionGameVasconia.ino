/* 16/05/2019: Andoni Alonso
  Juego de reacción:
  El sistema selecciona un botón aleatoriamente de los 12 disponibles. Mostrará al jugador el botón seleccionado iluminando su correspondiente LED.
  Cuando el jugador pulse el botón correcto, el sistema asigna un punto y seleccionará otro botón, así sucesivamente hasta que se termine el tiempo de juego (45s).


  Para direccionar un total de 12 botones utilizaremos un multiplexor, al igual que para leer los valores de los posible botones. De esta forma únicamente con 4 salidas
  podemos establecer la dirección de salida (LED) o entrada (botón) y otros dos puertos para obtener el valor.
  El resultado se muestra en un display de 7 segmentos con 4 dígitos. Los dos dígitos de la izquierda son los segundos y los de la derecha es la puntuación.
*/
#include <Wire.h> // librería SPI para Arduino
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

const int btnTEST = 5; //entrada utilizada para ejecutar el test
const int muxLED = 6;   //salida utilizada para la señal de LED
const int muxBOTON = 7; //entrada utilizada para leer el botón
const int muxIO0 = 8;   //salidas parar direccionar el multiplexor para seleccionar el botón/led
const int muxIO1 = 9;
const int muxIO2 = 10;
const int muxIO3 = 11;

const char WAIT = 'W';
const char TEST = 'T';
const char PLAY = 'P';

const char COMPUTER = 'C';
const char PLAYER = 'P';
char turno; // el turno puede ser del jugador o del sistema (COMPUTER O PLAYER)

const byte s7sAddress = 0x70; // Dirección I2C del módulo de 7 segmentos.
Adafruit_7segment matrix = Adafruit_7segment();

const unsigned long TIEMPO_JUEGO = 45000; //45 segundos en milisegundos
const unsigned int BOTONES = 10; // Es el número de botones que tiene el juego
const unsigned int BOTON_PRINCIPAL = 7; // De forma arbitraria el botón que se ilumina al principio del juego

unsigned long tiempo_inicio;
unsigned long tiempo_restante;
unsigned long tiempo_display = 0;

unsigned int puntuacion = 0;

char estado;
byte ultimo_boton;
byte boton;

void setup() {
  Serial.begin(9600);
  Serial.println("\nReaction Game");

  //inicializar el display
  initializeI2C();

  // configuraciones de pines para los multiplexores.
  pinMode(muxIO0, OUTPUT);
  pinMode(muxIO1, OUTPUT);
  pinMode(muxIO2, OUTPUT);
  pinMode(muxIO3, OUTPUT);

  //pines de señal del LED, lectura del boton y boton de TEST.
  pinMode(muxLED, OUTPUT);
  pinMode(muxBOTON, INPUT_PULLUP);
  pinMode(btnTEST, INPUT_PULLUP);

  randomSeed(analogRead(0)); //inicializamos los números aleatorios

  //Si arrancamos el sistema y el pulsador (o interruptor de test) está activado, ejecutamos el modo TEST
  if (digitalRead(btnTEST) == HIGH)
  {
    setWaitMode();  // pasamos al modo WAIT
  }
  else
  {
    estado = TEST;
    Serial.println("Modo TEST");
  }


}

void loop() {

  switch (estado) {
    case WAIT:
      if (leer_boton(BOTON_PRINCIPAL) == LOW)
      { //Comenzamos el juego.
        //-ponemos el temporizador al tiempo de juego
        //-ponemos la puntuación a cero
        tiempo_restante = TIEMPO_JUEGO;
        tiempo_inicio = millis(); //nos quedamos con el momento en que empieza el juego
        puntuacion = 0;
        turno = COMPUTER;
        estado = PLAY;
        Serial.println("Modo PLAY");
        ultimo_boton = BOTON_PRINCIPAL;
      }
      break;
    case PLAY:
      switch (turno) {//ver a quien le toca jugar
        case (COMPUTER):  // si es a la máquina, tiene que seleccionar un botón diferente del último.
          do
            boton = random(BOTONES);
          while (boton == ultimo_boton);  //aqui ya tenemos un boton diferente al anterior.
          setLEDBoton(boton, HIGH); //iluminamos el botón que se ha seleccionado
          ultimo_boton = boton;

          turno = PLAYER;
          break;

        case (PLAYER):
          if (leer_boton(boton) == LOW)
          {
            puntuacion++; //incrementamos el número de aciertos
            setLEDBoton(boton, LOW); //apagamos el botón acertado
            turno = COMPUTER;
            mostrar_tiempoymarcador();
          }
          break;

        default:
          ERROR;
          break;
      }

      if (millis() >= tiempo_display) mostrar_tiempoymarcador();

      tiempo_restante = tiempo_inicio + TIEMPO_JUEGO - millis();
      if (tiempo_restante <= 0) {
        delay(5000); // al haber acabado el tiempo esperamos 5 segundos para que el jugador pare y mire la puntuación antes de empezar una nueva partida
        setWaitMode();
      }
      break;
    case TEST:
      //Ejecutamos el modo TEST va ejecutando todos los botones en orden, sin tiempo
      startTest();
      setWaitMode();
      break;
    default:
      ERROR;
      break;
  }

}

int SetMuxChannel(byte channel)
{
  digitalWrite(muxIO0, bitRead(channel, 0));
  digitalWrite(muxIO1, bitRead(channel, 1));
  digitalWrite(muxIO2, bitRead(channel, 2));
  digitalWrite(muxIO3, bitRead(channel, 3));
}

void setLEDBoton(byte boton, int value)
{
  SetMuxChannel(boton); //seleccionamos la dirección a informar
  digitalWrite(muxLED, value); //escribimos el valor
  Serial.println("Boton seleccionado:" + String(boton));
}

int leer_boton(byte boton)
{
  SetMuxChannel(boton); //seleccionamos la dirección a leer
  return digitalRead(muxBOTON);

}

//Poner el juego en modo WAIT
void setWaitMode()
{
  estado = WAIT;
  Serial.println("Modo WAIT");
  setLEDBoton(BOTON_PRINCIPAL, HIGH);

//  matrix.println(TIEMPO_JUEGO / 10);
//  matrix.writeDisplay();
//  delay(10);
}

//TODO
void mostrar_tiempoymarcador()
{
  unsigned long tiempo_y_marcador = tiempo_restante / 1000;
  tiempo_y_marcador = (tiempo_y_marcador * 100) + puntuacion;
  matrix.println(tiempo_y_marcador);
  matrix.writeDisplay();
  delay(10);

  tiempo_display = millis() + 1000; //guardamos el tiempo en el que tocaría actualizar (sólo cuando cambie el segundero)
}


//TODO
void ERROR() {

}

// se ocupa de hacer un test del sistema: activa uno por uno todos los botones de entrada esperando a que se vayan pulsando uno a uno.
void startTest() {
  unsigned short i;

  for (i = 0; i < BOTONES; i++) {
    setLEDBoton(i, HIGH);
    while (leer_boton(i) != LOW) { } //esperar hasta que pulse el botón
    puntuacion++;
    mostrar_tiempoymarcador();
    delay(500);

    if (digitalRead(btnTEST) == HIGH) return; //Si el interruptor de TEST está desactivado abortamos el TEST

  }
}

void initializeI2C() {
  matrix.begin(s7sAddress);

  // Imprimimos un número muy grande para que se muestre como un "reseteo" "----"
  matrix.print(10000, DEC);
  matrix.writeDisplay();
  delay(500);
}

