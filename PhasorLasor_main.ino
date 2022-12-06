#include <IRremote.h>
#include <FastLED.h>
#include <SoftwareSerial.h>
#include <DFMiniMp3.h>

#define NUM_LEDS 98
#define LED_PIN 2
#define RECV_PIN 7
#define BLADE_SPEED 3
#define BLADE_RETRACT_SPEED 6

/* Below are the different "states" that the Phasor Lasor can be in
 * The Phasor Lasor is in OFF in state 0 and ON in state 1-9
 * States 3-9 are "effects"
 */
#define OFF 0
#define SOLID 1 // Whenever the Phasor Lasor is showing a solid color of any RGB value

#define FADING 2 // A special state that is only used for if the Phasor is powered off while fading, 
                 // so it can make the color be whatever the fade fuction was fading to after the phasor is powered back on
#define FIRE 3
#define STROBE 4
#define WORM 5
#define RAINBOW 6
#define COLORCYCLE 7
#define CARAMELLDANSEN 8
#define TWINKLE 9

#define AMBIENT_SOLID 01


#define FADE_TIME 1000
#define FADE_TIMESTEPS 100

bool HasAudioPlayer = true;  //MAKE SURE THIS IS FALSE IF YOU DO NOT HAVE THE MP3 MODULE ATTACHED *WITH* THE SD CARD

CRGB led[NUM_LEDS];
IRrecv irrecv(RECV_PIN);
decode_results results;
int state = OFF;
int prev_state = SOLID;
int wave_increasing = 0;
int red = 0;
int green = 0;
int blue = 0;
//int speed = 100000;  // Why is this here :/
//int on = 0;
int timer = 0;
int volume = 5;
bool ambience_playing = false;
bool sound_on = true;
bool sound_change = false; // Signals when a new sound plays


// setup for software serial (if you want to use) NOTE: does not currently work with volume commands using current DFmp3 library
/*class Mp3Notify;
SoftwareSerial secondarySerial(10, 11); // RX, TX
typedef DFMiniMp3<SoftwareSerial, Mp3Notify> DfMp3;
DfMp3 dfmp3(secondarySerial);
class Mp3Notify
{
  public:
  static void PrintlnSourceAction(DfMp3_PlaySources source, const char* action)
  {
    if (source & DfMp3_PlaySources_Sd) 
    {
        Serial.print("SD Card, ");
    }
    if (source & DfMp3_PlaySources_Usb) 
    {
        Serial.print("USB Disk, ");
    }
    if (source & DfMp3_PlaySources_Flash) 
    {
        Serial.print("Flash, ");
    }
    Serial.println(action);
  }
  static void OnError([[maybe_unused]] DfMp3& mp3, uint16_t errorCode)
  {
    // see DfMp3_Error for code meaning
    Serial.println();
    Serial.print("Com Error ");
    Serial.println(errorCode);
  }
  static void OnPlayFinished([[maybe_unused]] DfMp3& mp3, [[maybe_unused]] DfMp3_PlaySources source, uint16_t track)
  {
    Serial.print("Play finished for #");
    Serial.println(track);  
  }
  static void OnPlaySourceOnline([[maybe_unused]] DfMp3& mp3, DfMp3_PlaySources source)
  {
    PrintlnSourceAction(source, "online");
  }
  static void OnPlaySourceInserted([[maybe_unused]] DfMp3& mp3, DfMp3_PlaySources source)
  {
    PrintlnSourceAction(source, "inserted");
  }
  static void OnPlaySourceRemoved([[maybe_unused]] DfMp3& mp3, DfMp3_PlaySources source)
  {
    PrintlnSourceAction(source, "removed");
  }
};*/


class Mp3Notify;
typedef DFMiniMp3<HardwareSerial, Mp3Notify> DfMp3;
DfMp3 dfmp3(Serial1);

class Mp3Notify
{
public:
  static void PrintlnSourceAction(DfMp3_PlaySources source, const char* action)
  {
    if (source & DfMp3_PlaySources_Sd) 
    {
        Serial.print("SD Card, ");
    }
    if (source & DfMp3_PlaySources_Usb) 
    {
        Serial.print("USB Disk, ");
    }
    if (source & DfMp3_PlaySources_Flash) 
    {
        Serial.print("Flash, ");
    }
    Serial.println(action);
  }
  static void OnError([[maybe_unused]] DfMp3& mp3, uint16_t errorCode)
  {
    // see DfMp3_Error for code meaning
    Serial.println();
    Serial.print("Com Error ");
    Serial.println(errorCode);
  }
  static void OnPlayFinished([[maybe_unused]] DfMp3& mp3, [[maybe_unused]] DfMp3_PlaySources source, uint16_t track)
  {
    Serial.print("Play finished for #");
    Serial.println(track);  
  }
  static void OnPlaySourceOnline([[maybe_unused]] DfMp3& mp3, DfMp3_PlaySources source)
  {
    PrintlnSourceAction(source, "online");
  }
  static void OnPlaySourceInserted([[maybe_unused]] DfMp3& mp3, DfMp3_PlaySources source)
  {
    PrintlnSourceAction(source, "inserted");
  }
  static void OnPlaySourceRemoved([[maybe_unused]] DfMp3& mp3, DfMp3_PlaySources source)
  {
    PrintlnSourceAction(source, "removed");
  }
};

void setup() {
  //RFID Code
  Serial.begin(115200);  // <--- BAUD rate
  irrecv.enableIRIn();
  dfmp3.begin();

  //Run this startup once for led initialization
  FastLED.addLeds < NEOPIXEL, LED_PIN>(led, NUM_LEDS);
  for (int i = 0; i < NUM_LEDS; i++) {
    led[i] = CRGB(0, 0, 0);  // Initializes each LED to no color
    set_colors(255, 255, 255); //Then sets all LEDs to white
  }
  FastLED.show();
  
  //uint16_t volume = dfmp3.getVolume();
  //Serial.print("volume ");
  //Serial.println(volume);
  if (HasAudioPlayer)
  {
    dfmp3.getVolume();
    dfmp3.setVolume(volume);
  }

  //uint16_t count = dfmp3.getTotalTrackCount(DfMp3_PlaySource_Sd);
  //Serial.print("files ");
  //Serial.println(count);
}

void waitMilliseconds(uint16_t msWait)
{
  uint32_t start = millis();
  
  while ((millis() - start) < msWait)
  {
    // if you have loops with delays, its important to 
    // call dfmp3.loop() periodically so it allows for notifications 
    // to be handled without interrupts
    dfmp3.loop(); 
    delay(1);
  }
}
// Think of this as the Arduino's "internal" RGB colors to later apply to the LEDs
void set_colors(int r, int g, int b) {
  red = r;
  green = g;
  blue = b;
}
// Sets a single LED to the chosen RGB values
void set_pixel(int index, int red, int green, int blue) {
  led[index] = CRGB(red, green, blue);
}
// "Shifts" all of colors on each LED to the LED below it
void step() {
  for (int i = NUM_LEDS; i > 0; i++) {
    led[i] = led[i - 1];
  }
  led[0] = CRGB(red, green, blue);
}
// Sets all LEDs as a unified RGB color
void set_solid() {
  for (int i = 0; i < NUM_LEDS; i++) {
    set_pixel(i, red, green, blue);
  }
}
// Turns on all of the LEDs, starting with the closest one like a lightsaber
void extend() {
  Serial.println("track 2");
  if (sound_on)
  {
    dfmp3.playMp3FolderTrack(2); // sd:/mp3/0002.wav
  }
  
  for (int i = 0; i < NUM_LEDS; i++) {
    set_pixel(i, red, green, blue);
    FastLED.show();
    delay(BLADE_SPEED);
  }
}
// Makes all of the LEDs RGB values = 0 starting with the LED furthest away
void retract() {
  Serial.println("track 3");
  if (sound_on)
  {
    dfmp3.playMp3FolderTrack(3); // sd:/mp3/0002.wav
  }
  for (int i = NUM_LEDS - 1; i >= 0; i--) {
    set_pixel(i, 0, 0, 0);
    FastLED.show();
    delay(BLADE_RETRACT_SPEED);
  }
}

void change_brightness(int8_t change) {
  // Unsigned integer of 8 bits -> 1 byte
  static uint8_t brightness = 8;
  static uint32_t last_brightness_change = 0 ;
  uint8_t bright_value;

    if (millis() - last_brightness_change < 300) {
      Serial.print("Too soon... Ignoring brightness change from ");
      Serial.println(brightness);
      return;
    }
    last_brightness_change = millis(); // millis() is a delay function that isn't an inturrupt
    brightness = constrain(brightness + change, 1, 8); // Constrains the brightness to a min of 1 and max of 8
    bright_value = (1 << brightness) - 1; // Converts bright_value from 1-8 to 1-255
    FastLED.setBrightness(bright_value);
    Serial.print("Changing brightness ");
    Serial.print(change);
    Serial.print(" to level ");
    Serial.print(brightness);
    Serial.print(" value ");
    Serial.println(bright_value);
    FastLED.show();
}

/*************************************************************/

void Fire(int Cooling, int Sparking, int SpeedDelay) {
  static byte heat[NUM_LEDS];
  int cooldown;
 
  // Step 1.  Cool down every cell a little
  for(int i = 5; i < NUM_LEDS; i++) {
    cooldown = random(0, ((Cooling * 10) / NUM_LEDS) + 2);
   
    if(cooldown>heat[i]) {
      heat[i]=0;
    } else {
      heat[i]=heat[i]-cooldown;
    }
  }
 
  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for(int k = NUM_LEDS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }
   
  // Step 3.  Randomly ignite new 'sparks' near the bottom
  if(random(255) < Sparking ) {
    int y = random(7);
    heat[y] = heat[y] + random(160,255);
    //heat[y] = random(160,255);
  }

  // Step 4.  Convert heat to LED colors
  for(int j = 0; j < NUM_LEDS; j++) {
    setPixelHeatColor(j, heat[j] );
  }

  if (irrecv.isIdle())
    {
      FastLED.show();
    }
  if (check_IR(SpeedDelay)) 
    {
      //dfmp3.stop();
      return;
    }
    Fire(55,120,15);
}

void setPixelHeatColor (int Pixel, byte temperature) {
  // Scale 'heat' down from 0-255 to 0-191
  byte t192 = round((temperature/255.0)*191);
 
  // calculate ramp up from
  byte heatramp = t192 & 0x3F; // 0..63
  heatramp <<= 2; // scale up to 0..252
 
  // figure out which third of the spectrum we're in:
  if( t192 > 0x80) {                     // hottest
    set_pixel(Pixel, 255, 255, heatramp);
  } else if( t192 > 0x40 ) {             // middle
    set_pixel(Pixel, 255, heatramp, 0);
  } else {                               // coolest
    set_pixel(Pixel, heatramp, 0, 0);
  }
}

 // RAINBOW
 /*void loop2() {
  rainbowCycle(20);
} */

void rainbowCycle(int SpeedDelay) {
  /* https://www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/#LEDStripEffectRainbowCycle
   * ^ This is where we found the code to create the Rainbow Cycle effect
   */
  byte *c;
  uint16_t i, j;

  for(j=0; j<256*5; j++) 
  { // 5 cycles of all colors on wheel
    for(i=0; i< NUM_LEDS; i++) 
    {
      c = Wheel(((i * 256 / NUM_LEDS) + j) & 255); // Does a bitwise AND 
      set_pixel(i, *c, *(c+1), *(c+2));
    }
     if (irrecv.isIdle())
    {
      FastLED.show();
    }
    if (check_IR(SpeedDelay)) 
    {
      return;
    }
    //delay(SpeedDelay);
  }
  rainbowCycle(20);
}
// See the link under rainbowCycle for how the Wheel function works
byte * Wheel(byte WheelPos) {
  static byte c[3];
 
  if(WheelPos < 85) {
   c[0]=WheelPos * 3;
   c[1]=255 - WheelPos * 3;
   c[2]=0;
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   c[0]=255 - WheelPos * 3;
   c[1]=0;
   c[2]=WheelPos * 3;
  } else {
   WheelPos -= 170;
   c[0]=0;
   c[1]=WheelPos * 3;
   c[2]=255 - WheelPos * 3;
  }

  return c;
}
/*************************************************************/
// STROBE
// Turns on and off all the LEDs rapidly, creating a strobe effect
void Strobe(byte red, byte green, byte blue, int FlashDelay){
  for(int j = 0; j < NUM_LEDS; j++){
    // Turn all LEDs on
    set_colors(red, green, blue);
    set_solid();
    if (irrecv.isIdle())
      {
        FastLED.show();
      }
    if (check_IR(FlashDelay))
      {
        return;
      }
    // Turn all LEDs off
    set_colors(0, 0, 0);
    set_solid();
    if (irrecv.isIdle())
      {
        FastLED.show();
      }
    if (check_IR(FlashDelay))
      {
        return;
      }
  }
  Strobe(red, green, blue, FlashDelay);
}
/***********************************************************/
// STROBE... but using a rainbow
void RainbowStrobe(int SpeedDelay)
{
  byte *c;
  uint16_t i;

  // 5 cycles of all colors on wheel
    for(i=0; i< NUM_LEDS; i++) 
    {
      c=Wheel(((i * 256 / NUM_LEDS)));
      set_pixel(i, *c, *(c+1), *(c+2));
    }
     if (irrecv.isIdle())
    {
      FastLED.show();
    }
    if (check_IR(SpeedDelay)) 
    {
      return;
    }
    //delay(SpeedDelay);
    set_colors(0, 0, 0);
    set_solid();
    if (irrecv.isIdle())
      {
        FastLED.show();
      }
    if (check_IR(SpeedDelay))
      {
        return;
      }
  
  RainbowStrobe(50);
}
/***********************************************************/
// Sends alittle bug up and own the length of the blade
void Caterpillar(byte red, byte green, byte blue, int BugSize, int BugSpeed, int ReturnDelay){
  for(int i = 0; i < NUM_LEDS-BugSize-2; i++) {
    // Turn all LEDs off
    set_colors(0,0,0);
    set_solid();
    // Set the LED in the space of the bug's butt to the bug's color
    set_pixel(i, red/10, green/10, blue/10);
    for(int j = 1; j <= BugSize; j++) {
      // Shift the bug up one LED
      set_pixel(i+j, red, green, blue);
    }
    // Set the LED in front of the bug to the bug's color
    set_pixel(i+BugSize+1, red/10, green/10, blue/10);
    if (irrecv.isIdle())
    {
      FastLED.show();
    }
    if (check_IR(BugSpeed)) 
    {
      return;
    }
  }

  if (check_IR(ReturnDelay)) 
    {
      return;
    }

  for(int i = NUM_LEDS-BugSize-2; i > 0; i--) {
    set_colors(0,0,0);
    set_solid();
    set_pixel(i, red/10, green/10, blue/10);
    for(int j = 1; j <= BugSize; j++) {
      set_pixel(i+j, red, green, blue);
    }
    set_pixel(i+BugSize+1, red/10, green/10, blue/10);
    if (irrecv.isIdle())
    {
      FastLED.show();
    }
    if (check_IR(BugSpeed)) 
    {
      return;
    }
  }
  if (check_IR(ReturnDelay)) 
    {
      return;
    }
}
// Fades the LEDs in and out
int FadeInOut(byte red, byte green, byte blue, int speedDelay){
  float r, g, b;
  // Increases the intensity of the RGB value chosen
  for(int k = 0; k < 256; k+=1) {
    r = (k/256.0)*red;
    g = (k/256.0)*green;
    b = (k/256.0)*blue;
    set_colors(r,g,b);
    set_solid();
    if (irrecv.isIdle())
    {
      FastLED.show();
    }
     if (check_IR(speedDelay)) 
    {
      return 9;
    }
  
  }
  // Decreases the intensity of the RGB values twice as fast
  for(int k = 255; k >= 0; k-=2) {
    r = (k/256.0)*red;
    g = (k/256.0)*green;
    b = (k/256.0)*blue;
    set_colors(r,g,b);
    set_solid();
    if (irrecv.isIdle())
    {
      FastLED.show();
    }
     if (check_IR(speedDelay)) 
    {
      return 9;
    }
  }
}
// Fades the LEDs in and out but changes colors during
void ColorCycleFade(int speedDelay)
{
  int output;
 
  for(int j = 0; j < 9; j++)
  switch(j)
 {
  case 0: 
  output = FadeInOut(255,0,0,2); // FadeInOut returns a 9 as a sort of edge case check
  if(output == 9)
  {
    return; 
  }
  break; //red
  
  case 1:
  output = FadeInOut(255,20,0,2);
   if(output == 9)
  {
    return; 
  }
    break; // orange
    
  case 2: 
  output = FadeInOut(150,55,0,2);
  if(output == 9)
  {
    return; 
  }
    break; // yellow
  
  case 3: 
  output = FadeInOut(0,255,0,2);
   if(output == 9)
  {
    return; 
  }
  break;  // green
  
  case 4: 
  output = FadeInOut(2,255,34,2); 
     if(output == 9)
  {
    return; 
  }
  break; // pine

   case 5: 
  output = FadeInOut(13,250,97,2);
     if(output == 9)
  {
    return; 
  }
  break; // teal
  
  case 6: 
  output = FadeInOut(0,0,255,2);
    if(output == 9)
  {
    return; 
  }
  break;  // blue
 
  case 7: 
  output = FadeInOut(255,0,230,2); 
     if(output == 9)
  {
    return; 
  }
  break; //purple
  
  case 8: 
  output = FadeInOut(255,0,70,2); 
     if(output == 9)
  {
    return; 
  }
  break; //rose
} 
ColorCycleFade(10);
}

//****************************************************************
// Strobes different colors while Caramelldansen plays on the speakers
void Caramelldansen(byte red, byte green, byte blue, int ColorFlashDelay){
  if (HasAudioPlayer)
  {
    //Serial.println("track 4");
    //dfmp3.playMp3FolderTrack(4);
  }
  for(int i = 0; i < NUM_LEDS; i++){
    set_colors(255, 0, 0); // Red
    set_solid();
    if (irrecv.isIdle())
      {
        FastLED.show();
      }
    if (check_IR(ColorFlashDelay))
      {
        sound_change = true;
        Serial.println("sound change true");
        return;
      }
    set_colors(0, 255, 0); // Green
    set_solid();
    if (irrecv.isIdle())
      {
        FastLED.show();
      }
    if (check_IR(ColorFlashDelay))
      {
        sound_change = true;
        Serial.println("sound change true");
        return;
      }
    set_colors(0, 0, 255); // Blue
    set_solid();
    if (irrecv.isIdle())
      {
        FastLED.show();
      }
    if (check_IR(ColorFlashDelay))
      {
        sound_change = true;
        Serial.println("sound change true");
        return;
      }
    set_colors(255, 20, 0); // Red with a little bit of green
    set_solid();
    if (irrecv.isIdle())
      {
        FastLED.show();
      }
    if (check_IR(ColorFlashDelay))
      {
        sound_change = true;
        Serial.println("sound change true");
        return;
      }
    set_colors(255, 0, 30); // Red with a little bit of blue
    set_solid();
    if (irrecv.isIdle())
      {
        FastLED.show();
      }
    if (check_IR(ColorFlashDelay))
      {
        sound_change = true;
        Serial.println("sound change true");
        return;
      }
    set_colors(150, 55, 0); // Less Red, more green
    set_solid();
    if (irrecv.isIdle())
      {
        FastLED.show();
      }
    if (check_IR(ColorFlashDelay))
      {
        sound_change = true;
        Serial.println("sound change true");
        return;
      }
    set_colors(13, 250, 97); // Mostly Blue with some green and a little red
    set_solid();
    if (irrecv.isIdle())
      {
        FastLED.show();
      }
    if (check_IR(ColorFlashDelay))
      {
        sound_change = true;
        Serial.println("sound change true");
        return;
      }
  }
  Caramelldansen(red, green, blue, ColorFlashDelay);
}

/****************************************************************/
// Makes the LEDs "twinkle" across the blade
void Twinkle(int Count, int SpeedDelay, boolean One) {
  set_colors(0,0,0);
  set_solid();
  for (int i=0; i<Count; i++) {
    // Sets the pixel to a random color
    set_pixel(random(NUM_LEDS),random(0,255),random(0,255),random(0,255));
    if (irrecv.isIdle())
      {
        FastLED.show();
      }
    if (check_IR(SpeedDelay))
      {
        return;
      }
    if(One) {
      // if One is true,turn the colors off
      set_colors(0,0,0);
      set_solid();
      }
  }
  if (check_IR(SpeedDelay))
      {
        return;
      }
  Twinkle(Count, SpeedDelay, One);
}

/*****************************************************************/
// check_IR first checks if there is any
bool check_IR (int delay_time) {  
  delay(delay_time);
  check_audio();
  if (irrecv.decode(&results)) {
    irrecv.resume();

    //initial volume setting required here
    if (HasAudioPlayer)
    {
      //dfmp3.getVolume();
      dfmp3.setVolume(volume);
    }
        
    Serial.println(results.value);
      switch (results.value) {
        /* This guy is just a massive switch case that we used to tell the arduino 
         * what code to run depending on which button was pressed. If your remote is a different make or model,
         * just change the hex codes below to match the hex keys on your remote :3
         */
        case 0xFF02FD: // ON/OFF button
          Serial.println("ON/OFF");
          // If turned off and back on, the arduino checks to the what the last state was before turning the blade off and turns it back on to that stae
          if (state == OFF) {
            switch (prev_state) {
              case SOLID:
                extend();
                state = SOLID;
                break;
              case FADING:
                extend();
                state = SOLID;
                break;
              case FIRE:
                state = FIRE;
                Fire(55,120,15);
                break;
              case STROBE:
                state = STROBE;
                Strobe(red, green, blue, 50); 
                break;
              case WORM:
                state = WORM;
                Caterpillar(0, 255, 0, 4, 200, 50);
                break;
              case RAINBOW:
                state = RAINBOW;
                rainbowCycle(20);
                break;
              case COLORCYCLE:
                state = COLORCYCLE;
                ColorCycleFade(2);
                break;
              case CARAMELLDANSEN:
                state = CARAMELLDANSEN;
                Caramelldansen(red, green, blue, 181);
                break;
            }
            return 1;            
          } else {
            // "Turns off" the blade
            retract();
            prev_state = state;
            state = OFF;
            return 1; 
          }
          

        case 0xFF3AC5: // INCREASE BRIGHTNESS
          change_brightness(+1);
          Serial.println("Got IR: Bright");
          return 0;

        case 0xFFBA45: // DECREASE BRIGHTNESS
          change_brightness(-1);
          Serial.println("Got IR: Dim");
          return 0;

        case 0xFF22DD: // WHITE
          Serial.println("WHITE");
          if (state == (SOLID || COLORCYCLE || FADING)) {
            fade(255, 210, 255);
          } else {
            set_colors(255, 210, 255);
            extend();
          }
          state = SOLID;
          sound_change = true;
          return 1;

        case 0xFF1AE5: // RED
          Serial.println("RED");
          if (state == (SOLID || COLORCYCLE || FADING)) {
            fade(255, 0, 0);
          } else {
            set_colors(255, 0, 0);
            extend();
          }
          state = SOLID;
          sound_change = true;
          Serial.println("returned 1");
          return 1;

        case 0xFF9A65: // GREEN
          Serial.println("GREEN");
          if (state == (SOLID || COLORCYCLE || FADING)) {
            fade(0, 255, 0);
          } else {
            set_colors(0, 255, 0);
            extend();
          }
          state = SOLID;
          sound_change = true;
          Serial.println("returned 1");
          return 1;

        case 0xFFA25D: // BLUE
          Serial.println("BLUE");
          if (state == (SOLID || COLORCYCLE || FADING)) {
            fade(0, 0, 255);
          } else {
            set_colors(0, 0, 255);
            extend();
          }
          state = SOLID;
          sound_change = true;
          Serial.println("returned 1");
          return 1;

        case 0xFF2AD5: // REDOR
          Serial.println("REDOR");
          if (state == (SOLID || COLORCYCLE || FADING)) {
            fade(220, 10, 0); //255, 28, 0
          } else {
            set_colors(220, 10, 0);
            extend();
          }
          state = SOLID;
          sound_change = true;
          return 1;

        case 0xFF0AF5: // ORANGE
          Serial.println("ORANGE");
          if (state == (SOLID || COLORCYCLE || FADING)) {
            fade(255, 20, 0);
          } else {
            set_colors(255, 20, 0);
            extend();
          }
          state = SOLID;
          sound_change = true;
          return 1;

        case 0xFF38C7: // YELLOR
          Serial.println("YELLOR");
          if (state == (SOLID || COLORCYCLE || FADING)) {
            fade(230, 30, 0);
          } else {
            set_colors(230, 30, 0);
            extend();
          }
          state = SOLID;
          sound_change = true;
          return 1;

        case 0xFF18E7: // YELLOW
          Serial.println("YELLOW");
          if (state == (SOLID || COLORCYCLE || FADING)) {
            fade(150, 55, 0); //240, 60, 2 //255, 50, 0
          } else {
            set_colors(150, 55, 0);
            extend();
          }
          state = SOLID;
          sound_change = true;
          return 1;

        case 0xFFAA55: // LIME GREEN
          Serial.println("LIMEGR");
          if (state == (SOLID || COLORCYCLE || FADING)) {
            fade(130, 240, 0);
          } else {
            set_colors(130, 240, 0);//51, 204, 0 //70, 204, 2
            extend();
          }
          state = SOLID;
          sound_change = true;
          return 1;

        case 0xFF8A75: // CYAN
          Serial.println("CYAN");
          if (state == (SOLID || COLORCYCLE || FADING)) {
            fade(50, 240, 255); //0, 255, 239
          } else {
            set_colors(50, 240, 255);
            extend();
          }
          state = SOLID;
          sound_change = true;
          return 1;

        case 0xFFB847: // TEAL
          Serial.println("TEAL");
          if (state == (SOLID || COLORCYCLE || FADING)) {
            fade(13, 250, 97); //3, 147, 97
          } else {
            set_colors(13, 250, 97);
            extend();
          }
          state = SOLID;
          sound_change = true;
          return 1;

        case 0xFF9867: // PINE
          Serial.println("PINE");
          if (state == (SOLID || COLORCYCLE || FADING)) {
            fade(2, 255, 34); //2, 100, 239 //2, 100, 39
          } else {
            set_colors(2, 255, 34);
            extend();
          }
          state = SOLID;
          sound_change = true;
          return 1;

        case 0xFF926D: // CERULEAN
          Serial.println("CERULEAN");
          if (state == (SOLID || COLORCYCLE || FADING)) {
            fade(24, 78, 255);
          } else {
            set_colors(24, 78, 255);
            extend();
          }
          state = SOLID;
          sound_change = true;
          return 1;

        case 0xFFB24D: // PURPLE
          Serial.println("PURPLE");
          if (state == (SOLID || COLORCYCLE || FADING)) {
            fade(255, 0, 230);
          } else {
            set_colors(255, 0, 230);
            extend();
          }
          state = SOLID;
          sound_change = true;
          return 1;

        case 0xFF7887: // VIOLET
          Serial.println("VIOLET");
          if (state == (SOLID || COLORCYCLE || FADING)) {
            fade(255, 0, 70);
          } else {
            set_colors(255, 0, 70);
            extend();
          }
          state = SOLID;
          sound_change = true;
          return 1;

        case 0xFF58A7: // ROSE
          Serial.println("ROSE");
          if (state == (SOLID || COLORCYCLE || FADING)) {
            fade(255, 0, 30);
          } else {
            set_colors(255, 0, 30);
            extend();
          }
          state = SOLID;
          sound_change = true;
          return 1;
        case 0xFF12ED: // PINKT
          Serial.println("PINKT");
          if (state == (SOLID || COLORCYCLE || FADING)) {
            fade(255, 40, 40);
          } else {
            set_colors(255, 40, 40);
            extend();
          }
          state = SOLID;
          sound_change = true;
          return 1;

        case 0xFFF807: // BLUET
          Serial.println("BLUET");
          if (state == (SOLID || COLORCYCLE || FADING)) {
            fade(137, 207, 240);
          } else {
            set_colors(137, 207, 240);
            extend();
          }
          state = SOLID;
          sound_change = true;
          return 1;

        case 0xFF32CD: // PINKB
          Serial.println("PINKB");
          if (state == (SOLID || COLORCYCLE || FADING)) {
            fade(174, 14, 14);
          } else {
            set_colors(174, 14, 14);
            extend();
          }
          state = SOLID;
          sound_change = true;
          return 1;

        case 0xFFD827: // BLUEB
          Serial.println("BLUEB");
          if (state == (SOLID || COLORCYCLE || FADING)) {
            fade(41, 90, 175);
          } else {
            set_colors(41, 90, 175);
            extend();
          }
          state = SOLID;
          sound_change = true;
          return 1;
           

        case 0xFF28D7: // RED INCREASE
          Serial.println("RED INCREASE");
          if (red < 225){
            red += 25;
          } else {
            Serial.println("Red at max value");
            red = 255;
          }
          fade(red, green, blue);
          return 1;

        case 0xFF08F7: // RED DECREASE
          Serial.println("RED DECREASE");
          if (red > 25){
            red -= 25;
          } else {
            Serial.println("Red at min value");
            red = 0;
          }
          fade(red, green, blue);
          return 1;
          
        case 0xFFA857: // GREEN INCREASE
          Serial.println("GREEN INCREASE");
          if (green < 225){
            green += 25;
          } else {
            Serial.println("Green at max value");
            green = 255;
          }
          fade(red, green, blue);
          return 1;
          
        case 0xFF8877: // GREEN DECREASE
          Serial.println("GREEN DECREASE");
          if (green > 25){
            green -= 25;
          } else {
            Serial.println("Green at min value");
            green = 0;
          }
          fade(red, green, blue);
          return 1;

        case 0xFF6897: // BLUE INCREASE
          Serial.println("BLUE INCREASE");
          if (blue < 225){
            blue += 25;
          } else {
            Serial.println("Blue at max value");
            blue = 255;
          }
          fade(red, green, blue);
          return 1;
 

        case 0xFF48B7: // BLUE DECREASE
          Serial.println("BLUE DECREASE");
          if (blue > 25){
            blue -= 25;
          } else {
            Serial.println("Blue at min value");
            blue = 0;
          }
          fade(red, green, blue);
          return 1;

        /*Fire: Sends red, orange, and white lights up the length of the blade in a flame like fashion */
        case 0xFF30CF: // DIY1 FIRE
          /*if (HasAudioPlayer)
          {
            Serial.println("track 5");
            dfmp3.loopFolder(02);
          }*/
          //if (state == OFF) {
            //set_colors(0, 0, 0);
            //extend();
          //}
          state = FIRE;
          sound_change = true;
          Serial.println("sound change true");
          //while(irrecv.isIdle())
          //{
            Fire(55,120,15);
          //}
          return 1;
  
          /*Strobe: Appalling flashing light*/
          case 0xFFB04F: //DIY2 Strobe
          dfmp3.stop();
          prev_state = state;
          state = STROBE;
          sound_change = true;
          if(prev_state == RAINBOW)
          {
              RainbowStrobe(50);
          }
          else
          {
            if(red == 0 & green == 0 & blue == 0 & state != OFF) {
              set_colors(255,255,255);
            }
            Strobe(red, green, blue, 50); 
          }
        return 1;
        
        /*Caterpillar: A small caterpillar crawls up and down the blade*/
          case 0xFF708F: //DIY3
          state = WORM;
          sound_change = true;
          Caterpillar(0, 255, 0, 4, 200, 50);
        return 1;

        /*Rainbow: GAY*/
        case 0xFF10EF: //DIY4
          //if (state == OFF) {
            //set_colors(0, 0, 0);
            //extend();
          //}
          state = RAINBOW;
          sound_change = true;
          rainbowCycle(20);
        return 1;

        /*Color Wipe: Cycles through all the colors*/
        case 0xFF906F: // DIY5
          /*if (state == OFF) {
            set_colors(0, 0, 0);
            extend();
          }*/
          state = COLORCYCLE;
          sound_change = true;
          ColorCycleFade(2);
        return 1;

        case 0xFF50AF: // DIY6
          state = CARAMELLDANSEN;
          sound_change = true;
          Serial.println("sound change true");
          Caramelldansen(red, green, blue, 181);
        return 1;
        
        //rainbow twinkle
        case 0xFFA05F: // JUMP7
          state = TWINKLE;
          sound_change = true;
          Twinkle(20, 100, false);
        return 1;

        case 0xFFE817: // QUICK - VOLUME UP
          if (HasAudioPlayer && sound_on)
          {
            volume++;
            if (volume > 30) {volume = 30;}
            dfmp3.setVolume(volume);
          }
        return 0;

        case 0xFFC837: // SLOW - VOLUME DOWN
          if (HasAudioPlayer && sound_on)
          {
            volume--;
            if (volume < 0) {volume = 0;}
            dfmp3.setVolume(volume);
          }          
        return 0;

        case 0xFFF00F: // AUTO - MUTE/UNMUTE AUDIO
          if (!sound_on)
          {
            sound_on = true;        
          }
          else
          {
            sound_on = false;
          }
          check_audio();
        return 0;


        case 0xFFFFFFFF: //JUNK
          Serial.println("JUNK,returned 0");
          return 0;

        default: //OTHER
          Serial.println("OTHER,returned 0");
          return 0;
      }
      Serial.println(results.value, HEX);
      
  }
  
  return 0;
}

void check_audio() {
  // checks to see if the MP3 module is attached to the arduino with the SD card
  if (HasAudioPlayer)
  {
    if (sound_on && (!ambience_playing || sound_change))
    {
      // Runs a certain audio file depending on the state that the blade is currently in
      switch (state) {
        case OFF:
          break;
          
        case SOLID:
          dfmp3.loopFolder(01);
          ambience_playing = true;
          break;
          
        case FADING:
          break;
          
        case FIRE:
          dfmp3.loopFolder(02);
          ambience_playing = true;
          break;
          
        case STROBE:
          //dfmp3.loopFolder(01);
          //ambience_playing = true;
          break;

        case WORM:
          dfmp3.loopFolder(01);
          ambience_playing = true;
          break;

        case RAINBOW:
          dfmp3.loopFolder(01);
          ambience_playing = true;
          break;

        case COLORCYCLE:
          dfmp3.loopFolder(01);
          ambience_playing = true;
          break;

        case CARAMELLDANSEN:
          dfmp3.loopFolder(03);
          ambience_playing = true;
          break;

        case TWINKLE:
          dfmp3.loopFolder(01);
          ambience_playing = true;
          break;
      }
      sound_change = false;
    }
    else if (ambience_playing)
    {
      //Serial.println(dfmp3.getCurrentTrack());
      if (!sound_on || state == OFF) // checks if the sound should be commanded to stop playing
      {
        dfmp3.stop();
        ambience_playing = false;
      }
       
       /*switch (state) {
        case OFF:
          dfmp3.stop();
          ambience_playing = false;
          break;
          
        case SOLID:

        case FADING:

        case FIRE:

        case STROBE:

        case WORM:

        case RAINBOW:

        case COLORCYCLE:

        case CARAMELLDANSEN:

        case TWINKLE:*/
    }
  }
}

/* The fade function takes the source color and destination color and uses some epic math
 * to smooth the transition between the two colors 
 */
void fade(int r, int g, int b) {
  Serial.println("ENTERED FADE");
  double r_temp = (double) red;
  double g_temp = (double) green;
  double b_temp = (double) blue;
  double r_grad = (r - r_temp) / FADE_TIMESTEPS;
  double g_grad = (g - g_temp) / FADE_TIMESTEPS;
  double b_grad = (b - b_temp) / FADE_TIMESTEPS;

  for (int i = 0; i < FADE_TIMESTEPS; i++) {
    r_temp += r_grad;
    g_temp += g_grad;
    b_temp += b_grad;
    red = (int) r_temp;
    green = (int) g_temp;
    blue = (int) b_temp;
    set_solid();
    if (irrecv.isIdle())
    {
      FastLED.show();
    }
    if (check_IR(FADE_TIME / FADE_TIMESTEPS)) 
    {
      Serial.println("FADE EARLY TERMINATE");
      return;
    }
  }
  // Finally ends on the destination color
  red = r;
  green = g;
  blue = b;
  set_solid();
  if (irrecv.isIdle())
  {
    FastLED.show();
  }
  Serial.println("FADE FINISHED");
}
/* The main loop that runs, essentially just looking at the IR sensor,
 * then running the associated code, then checking the audio, and playing
 * the correct sounds :p
 */
void loop() {
  check_IR(1);
  check_audio();
}
