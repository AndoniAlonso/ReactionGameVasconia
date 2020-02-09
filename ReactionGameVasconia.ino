/* 16/05/2019: Andoni Alonso
  Juego de reacción:
  El sistema selecciona un botón aleatoriamente de los 13 disponibles. Mostrará al jugador el botón seleccionado iluminando su correspondiente LED.
  Cuando el jugador pulse el botón correcto, el sistema asigna un punto y seleccionará otro botón, así sucesivamente hasta que se termine el tiempo de juego (45s).


  Para direccionar un total de 13 botones utilizaremos un multiplexor, al igual que para leer los valores de los posible botones. De esta forma únicamente con 4 salidas
  podemos establecer la dirección de salida (LED) o entrada (botón) y otros dos puertos para obtener el valor.
  El resultado se muestra en un display de 7 segmentos con 4 dígitos. Los dos dígitos de la izquierda son los segundos y los de la derecha es la puntuación.
*/
#include <Wire.h> // librería SPI para Arduino
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"
#include <Adafruit_NeoPixel.h>

#define DEBUG 0

#define NUMPIXELS      12 // number of neopixels in strip

const unsigned int gameTypeRing = 2;	//entrada input pin Neopixel ring is attached to
const unsigned int gameTypeBOTON = 3; //entrada del boton de tipo de juego
const unsigned int btnTEST = 5; 	//entrada para ejecutar el test
const unsigned int muxLED = 6;   	//salida  para la señal de LED
const unsigned int muxBOTON = 7; 	//entrada para leer el pulsador de juego
const unsigned int muxIO0 = 8;   	//salidas parar direccionar el multiplexor para seleccionar el botón/led
const unsigned int muxIO1 = 9;
const unsigned int muxIO2 = 10;
const unsigned int muxIO3 = 11;

const char WAIT = 'W';
const char TEST = 'T';
const char PLAY = 'P';

// Diferentes tipos de juego
const unsigned int gameTypeComplete = 0; //panel completo
const unsigned int gameTypeHigh = 1;		//Todos menos los botones superiores
const unsigned int gameTypeWide = 2;		//Todos menos las dos filas superiores
const unsigned int gameTypeSmall = 3;	//Únicamente los cinco botones de abajo a la derecha
int gameTypeBotones[] = {13, 10, 8, 5 }; // para cada tipo de juego establecemos el nº de botones

// listamos los errores que detectamos
const unsigned int ESTADO_NO_CONTEMPLADO = 11;
const unsigned int TURNO_NO_CONTEMPLADO = 22;
const unsigned int TIPO_JUEGO_NO_CONTEMPLADO = 33;
const unsigned int ERROR_NO_CONTROLADO = 99;

// Delay para realizar el refresco del LED de selección del juego
const int delayval = 100; 		// timing delay in milliseconds

const char COMPUTER = 'C';
const char PLAYER = 'P';
char turno; // el turno puede ser del jugador o del sistema (COMPUTER O PLAYER)

Adafruit_7segment matrixTime = Adafruit_7segment();
Adafruit_7segment matrixPoints = Adafruit_7segment();

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, gameTypeRing, NEO_GRB + NEO_KHZ800); //Objeto para anillo neopixel

const unsigned long TIEMPO_JUEGO = 60000; //60 segundos en milisegundos
const unsigned int BOTONES = 13; // Es el número de botones que tiene el juego
const unsigned int BOTON_PRINCIPAL = 4; // De forma arbitraria el botón que se ilumina al principio del juego

unsigned long tiempo_inicio;
long tiempo_restante;
unsigned long tiempo_display = 0;
uint16_t blinkcounter = 0; // para controlar si ha pasado el tiempo necesario entre encendido y apagado de los puntos

unsigned int puntuacion = 0;
  


char estado;
byte ultimo_boton;
byte boton;
int gameType;

void setup() {
  #if DEBUG
    Serial.begin(9600);
    Serial.println("\nReaction Game");
  #endif

  //inicializar el display
  initializeI2C();

  // configuraciones de pines para los multiplexores.
  pinMode(muxIO0, OUTPUT);
  pinMode(muxIO1, OUTPUT);
  pinMode(muxIO2, OUTPUT);
  pinMode(muxIO3, OUTPUT);

  //pines de señal del LED, lectura del botón, botón de TEST,
  //botón de selección del jueto y salida del anillo LED.
  pinMode(muxLED, OUTPUT);
  pinMode(muxBOTON, INPUT_PULLUP);
  pinMode(btnTEST, INPUT_PULLUP);
  pinMode(gameTypeBOTON, INPUT_PULLUP);
  pinMode(gameTypeRing, OUTPUT);

  randomSeed(analogRead(0)); //inicializamos los números aleatorios

  //Si arrancamos el sistema y el pulsador (o interruptor de test) está activado, ejecutamos el modo TEST
  if (digitalRead(btnTEST) == HIGH)
  {
    setWaitMode();  // pasamos al modo WAIT
  }
  else
  {
    estado = TEST;
    #if DEBUG
      Serial.println("Modo TEST");
    #endif
  }

  // Inicializar la librería NeoPixel
  pixels.begin();
  
  gameType = gameTypeComplete;
  mostrarTipoJuego();

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
        mostrar_puntuacion();
        turno = COMPUTER;
        estado = PLAY;
        #if DEBUG
          Serial.println("Modo PLAY");
        #endif
        ultimo_boton = BOTON_PRINCIPAL;
      }
      else{ 
       if (digitalRead(gameTypeBOTON) == LOW) { //mirar si se ha pulsado el botón de selección de juego
         while (digitalRead(gameTypeBOTON) != HIGH) {} // esperar a que suelte el botón
         cambiarTipoJuego();
        }
      }
        
      break;
    case PLAY:
      switch (turno) {//ver a quien le toca jugar
        case (COMPUTER):  // si es a la máquina, tiene que seleccionar un botón diferente del último.
          do
            boton = random(gameTypeBotones[gameType]); //se pide un número aleatorio entre 0 y el número de botones del tipo de juego.
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
            mostrar_puntuacion();
          }
          break;

        default:
          ERROR(TURNO_NO_CONTEMPLADO);
          break;
      }

      if (millis() >= tiempo_display) mostrar_tiempo();

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
      ERROR(ESTADO_NO_CONTEMPLADO);
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
  #if DEBUG
    Serial.println("Boton seleccionado:" + String(boton));
  #endif
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
  #if DEBUG
    Serial.println("Modo WAIT");
  #endif
  setLEDBoton(BOTON_PRINCIPAL, HIGH);

//  matrix.println(TIEMPO_JUEGO / 10);
//  matrix.writeDisplay();
//  delay(10);
}

//mostrar en el display de tiempo el tiempo transcurrido
void mostrar_tiempo()
{
  //mostramos el tiempo
  unsigned long tiempo = tiempo_restante / 10;

  boolean drawDots = false; // Se utiliza para indicar si se muestran o no los puntos en el display de tiempo

  blinkcounter+=50;
  if (blinkcounter < 500) {
    drawDots = false;
  } else if (blinkcounter < 1000) {
    drawDots = true;
  } else {
    blinkcounter = 0;
  }
  
  matrixTime.writeDigitNum(0, (tiempo / 1000), drawDots);
  matrixTime.writeDigitNum(1, (tiempo / 100) % 10, drawDots);
  matrixTime.drawColon(drawDots);
  matrixTime.writeDigitNum(3, (tiempo / 10) % 10, drawDots);
  matrixTime.writeDigitNum(4, tiempo % 10, drawDots);
   
  matrixTime.writeDisplay();
  #if DEBUG
    Serial.println(tiempo);
  #endif
  //delay(10);
  tiempo_display = millis() + 10; //guardamos el tiempo en el que tocaría actualizar (sólo cuando cambie el segundero)
}

//mostrar en el display de puntuación el valor acumulado
void mostrar_puntuacion()
{
  //y la puntuación
  matrixPoints.println(puntuacion);
  
  matrixPoints.writeDisplay();
}


//Se muestran los errores en los displays.
void ERROR(int error_code) {
  
  //según el error de que se trate realizamos diferentes acciones para avisar al usuario.
  switch (error_code) {
  case ESTADO_NO_CONTEMPLADO:
  case TIPO_JUEGO_NO_CONTEMPLADO:
  case TURNO_NO_CONTEMPLADO:
    break;
  default:
      error_code = ERROR_NO_CONTROLADO;
    break;
  }

  // mostramos en el display de tiempo eeee
  matrixTime.print(0xEEEE, HEX);
  matrixTime.writeDisplay();
  // y en el de puntuación el código de error
  matrixPoints.print(error_code, DEC);
  matrixPoints.writeDisplay();
  delay(500);
}

// se ocupa de hacer un test del sistema: activa uno por uno todos los botones de entrada esperando a que se vayan pulsando uno a uno.
void startTest() {
  unsigned short i;
  #if DEBUG
    Serial.println("Modo TEST");
  #endif

  //realizamos un ciclo para iluminar todos los LED del anillo con
  //colores aleatorios.
  for (i = 0; i < NUMPIXELS; i++) {
	  pixels.setPixelColor(i, pixels.Color(random(0, 255), random(0,255), random(0, 255)));
    // This sends the updated pixel color to the hardware.
    pixels.show();

    // Delay for a period of time (in milliseconds).
    delay(delayval);
  }

  //El usuario debe ejecutar la secuencia de botones en orden
  for (i = 0; i < BOTONES; i++) {
    setLEDBoton(i, HIGH);
    while (leer_boton(i) != LOW) { } //esperar hasta que pulse el botón
    puntuacion++;
    mostrar_puntuacion();
    delay(500);

    if (digitalRead(btnTEST) == HIGH) return; //Si el interruptor de TEST está desactivado abortamos el TEST

  }
}

void initializeI2C() {
  const byte s7sAddressTime = 0x70;   // Dirección I2C del módulo de 7 segmentos rojo (el tiempo de juego).
  const byte s7sAddressPoints = 0x71; // Dirección I2C del módulo de 7 segmentos verde (puntuación).
  
  matrixTime.begin(s7sAddressTime);
  matrixPoints.begin(s7sAddressPoints);

  // Imprimimos un número muy grande para que se muestre como un "reseteo" "----"
  matrixTime.print(10000, DEC);
  matrixPoints.print(10000, DEC);
  matrixTime.writeDisplay();
  matrixPoints.writeDisplay();
  delay(500);
}

uint32_t setColorTipoJuego() {
  //Colores del anillo de selección de juego
  int redColor = 0;
  int greenColor = 0;
  int blueColor = 0;

  #if DEBUG
    Serial.println("cambio tipo de juego");
    Serial.println(gameType);
 #endif

  switch (gameType) {
    case gameTypeComplete:  //asignar el color rojo
      redColor = 255;
      greenColor = 0;
      break;

    case gameTypeHigh:      //asignar el color naranja
      redColor = 255;
      greenColor = 128;
      break;

    case gameTypeWide:      //asignar el color amarillo
      redColor = 255;
      greenColor = 255;
      break;

    case gameTypeSmall:     //asignar el color verde
      redColor = 0;
      greenColor = 255;
      break;

    default:
      ERROR(TIPO_JUEGO_NO_CONTEMPLADO);
      break;
  }
  return pixels.Color(redColor, greenColor, blueColor);
}

void mostrarTipoJuego() {
  
  uint32_t pixelColor;
  pixelColor = setColorTipoJuego();
  
  pixels.clear();
  
  for (int i=gameType * 3; i < gameType *3 + 3; i++) {
    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    pixels.setPixelColor(i, pixelColor);

  }

  // This sends the updated pixel color to the hardware.
  pixels.show();

  // Delay for a period of time (in milliseconds).
  delay(delayval);
}

void cambiarTipoJuego(){
  gameType++;
  
  if (gameType > gameTypeSmall) {
    gameType = gameTypeComplete;
  }
  mostrarTipoJuego();
}

//devuelve si el botón seleccionado es un botón válido para el tipo de juego.
//bool esBotonTipoJuego(byte boton){
//  
//  if (boton<gameTypeBotones[gameType])
//    	return true;
//  else return false;
//}
