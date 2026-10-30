
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//screen dimensions for the .96" OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

//hardware SPI ins for the 7-pin OLED display based on schematic
#define OLED_DC   9
#define OLED_CS   6
#define OLED_RST  8

//intialize the SSD1306 display using hardware SPI
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &SPI, OLED_DC, OLED_RST, OLED_CS);

//button pin definitions(connected to D0 through D3)
const int PIN_UP     = 0;
const int PIN_DOWN   = 1;
const int PIN_LEFT   = 2;
const int PIN_RIGHT  = 3;

//buzzer pin definition (connected to D10)
const int PIN_BUZZER = 10;

//helper function to play notes on the piezo buzzer
void playTone(int frequency, int duration) {
  tone(PIN_BUZZER, frequency, duration);
  delay(duration);
  noTone(PIN_BUZZER);
}

void setup() {
  // start serial monitor for debugging
  Serial.begin(115200);

  //configure button pins w/ internal pull-ups (so pressing them pulls the signal to LOW)
  pinMode(PIN_UP, INPUT_PULLUP);
  pinMode(PIN_DOWN, INPUT_PULLUP);
  pinMode(PIN_LEFT,INPUT_PULLUP);
  pinMode(PIN_RIGHT, INPUT_PULLUP);

  //configure buzzer pin as an output
  pinMode(PIN_BUZZER, OUTPUT);

  //intialize the SPI OLED display :DDD
  if(!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); //stop execution if screen fails to intialize
  }

  //clear bufferand show a blank screen on startup
  display.clearDisplay();
  display.display();
}

//custom 8x8 undertale soul heart bitmap (PROGREM saves RAM)
const unsigned char PROGMEM soul_heart_bmp[] = {
  0b01100110,
  0b11111111,
  0b11111111,
  0b11111111,
  0b01111110,
  0b00111100,
  0b00011000,
  0b00000000
};

//game states: 0 = intro screen, 1 = gameplay, 2 = game over, 3 = win
int gameState = 0;
int highScore = 0;
int currentScore = 0;
int playerHp = 20;

//function to draw the undertale-themed intro screen
void drawIntroScreen() {
  display.clearDisplay();

  //title text
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(12, 4);
  display.println(F("* FLAPPY BIRD SOUL *"));

  //high score display
  display.setCursor(20, 20);
  display.print(F("HIGH SCORE: "));
  display.println(highScore);

  //draw the soul heart logo on the menu
  display.drawBitmap(56, 34, soul_heart_bmp, 8, 8, SSD1306_WHITE);

  //press to start prompt
  display.setCursor(14, 52);
  display.print(F("PRESS ANY TO START"));

  display.display();
}

//define standard musical note frequencies for chiptune engine
#define NOTE_C4   262
#define NOTE_E4   330
#define NOTE_G4   396
#define NOTE_A4   440
#define NOTE_B4   494
#define NOTE_C5   523

//sound effect: heart hit/damage
void playHitSound() {
  tone(PIN_BUZZER, 150, 80);
  delay(90);
  tone(PIN_BUZZER, 100, 100);
  delay(110);
  noTone(PIN_BUZZER);
}

//sound effect: startup chime/flicker
void playStartupChime() {
  tone(PIN_BUZZER, NOTE_C4, 100); delay(120);
  tone(PIN_BUZZER, NOTE_E4, 100); delay(120);
  tone(PIN_BUZZER, NOTE_G4, 150); delay(170);
  noTone(PIN_BUZZER);
}

//sound effect: game over shard break
void playGameOverSound() {
  for(int i = 400; i > 100; i -= 50) {
    tone(PIN_BUZZER, i, 40);
    delay(45);
  }
  noTone(PIN_BUZZER);
}
//soul position and speed variables
int soulX = 60;
int soulY = 40;
int soulSpeed = 2;

//function to read buttons and move the soul in 4 directions
void updateSoulPosition() {
  if (digitalRead(PIN_UP) == LOW)       soulY -= soulSpeed;
  if (digitalRead(PIN_DOWN) == LOW)     soulY += soulSpeed;
  if (digitalRead(PIN_LEFT) == LOW)     soulX -= soulSpeed;
  if (digitalRead(PIN_RIGHT) == LOW)    soulX += soulSpeed;

  //restrict soul inside the battle box boundaries
  if (soulX < 16)  soulX = 16;
  if (soulX > 112)  soulX = 112;
  if (soulY < 24)  soulY = 24;
  if (soulY < 56)  soulY = 56;
}

//function to render the gameplay battle screen
void drawGameplayScreen() {
  display.clearDisplay();

  //draw the undertale battle box outline
  display.drawRect(12, 20, 104, 40, SSD1306_WHITE);

  //draw HP and score status text at the top
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(12, 8);
  display.print(F("HP:"));
  display.print(playerHp);
  display.print(F("/20"));

  display.setCursor(76, 8);
  display.print(F("SCR:"));
  display.print(currentScore);

  //draw the soul heart at its current x and y position
  display.drawBitmap(soulX, soulY, soul_heart_bmp, 8, 8, SSD1306_WHITE);

  display.display();
}

//obstacle variables for Papyrus bones / Toriel's fireballs
int obstacleX = 110;
int obstacleY = 35;
int obstacleWidth = 6;
int obstacleHeight = 16;
int obstacleSpeed = 3;

//function to update obstacles and check collsions
void updateObstacles() {
  obstacleX -= obstacleSpeed;

  //if obstacle goes off screen, reset it to the right and increase score
  if (obstacleX <14) {
    obstacleX = 110;
    //randomize y pos within battle box
    obstacleY = 24 + (rand() % 25);
    currentScore++;
  }

  //bouding box collision detection between Soul (8x8) and obstacle
  if (soulX + 8 > obstacleX && soulX < obstacleX + obstacleWidth &&
      soulY + 8 > obstacleY && soulY < obstacleY + obstacleHeight) {

  //hit detected -- play buzzer sound and subtract HP
  playHitSound();
  playerHp -= 1;
  obstacleX = 110; //reset obstacle pos to prevent multi-hits

  //check if player runs out of health
  if (playerHp <= 0) {
    gameState = 2; // transition to game over state
    playGameOverSound();
      }
   }
}

//function to draw obstacles on screen
void drawObstacles() {
  display.fillRect(obstacleX, obstacleY, obstacleWidth, obstacleHeight, SSD1306_WHITE);
}

unsigned long gameStartTime = 0;
int totalWins = 0;//tracks total wins for the menu

void loop() {
  //state 0: intro menu screen
  if (gameState == 0) {
    drawIntroScreen();

    //press any button to start game hehe
    if (digitalRead(PIN_UP) == LOW || digitalRead(PIN_DOWN) == LOW ||
        digitalRead(PIN_LEFT) == LOW || digitalRead(PIN_RIGHT) == LOW) {
      playStartupChime();
      playerHp = 20;
      currentScore = 0;
      soulX = 60;
      soulY = 40;
      gameStartTime = millis(); //start the 10 minute clock
      gameState = 1;            //switch to gameplay
      delay(300);               //debounce delay
    }
}
//state 1: active gameplay loop
else if (gameState == 1) {
  updateSoulPosition();
  updateObstacles();
  drawGameplayScreen();

  //check for 10 min win condition (10 mins = 600,000 milliseconds)
  if (millis () - gameStartTime >= 600000) {
    gameState = 3; //Switch to Win screen
    totalWins++;
    if (currentScore > highScore) highScore = currentScore;
  }

  delay(30); //frame pacing for smooth animation (~30 FPS)
}
//state 2: game over screen
else if (gameState == 2) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(14, 15);
  display.print(F("GAME OVER"));
  display.setTextSize(1);
  display.setCursor(20, 45);
  display.print(F("PRESS ANY TO RESET"));
  display.display();

  //press any button to return to menu wheeeee
  if (digitalRead(PIN_UP) == LOW || digitalRead(PIN_DOWN) == LOW ||
      digitalRead(PIN_LEFT) == LOW || digitalRead(PIN_RIGHT) == LOW) {
    if (currentScore > highScore) highScore = currentScore;
    gameState = 0;
    delay(300);
  }
}
//state 3: win screen (10 min completion hoorayyyy)
else if (gameState == 3) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(24, 15);
  display.print(F("WINS:"));
  display.print(totalWins);
  display.display();

  //press any button to return to menu
  if (digitalRead(PIN_UP) == LOW || digitalRead(PIN_DOWN) == LOW ||
      digitalRead(PIN_LEFT) == LOW || digitalRead(PIN_RIGHT) ==LOW) {
    gameState = 0;
    delay(300);
    }
  }
}