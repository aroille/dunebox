
#define GLOBAL_VOLUME         0.99f   // [0.0 - 1.0]
#define PIXEL_MAX_INTENSITY   100    // [0 - 255]
#define PIXEL_REFRESH_RATE    60     // Hz

/*********** SD CARD CONTENT ****************/

#define SOUND_STARTUP                     "startup.wav"
#define SOUND_CHANGE_PRESET               "preset1.wav"

#define SOUND_RANDOM_PATH                 "random/%03d.wav"
#define SOUND_RANDOM_PATH_SIZE            15
#define SOUND_RANDOM_FILE_COUNT           58

#define SWITCH_SAMPLE_PATH                "switches/X.wav"
#define SWITCH_SAMPLE_PATH_OFFSET         9

#define PAD_SAMPLE_PATH                   "banks/X/X.wav"
#define PAD_SAMPLE_PATH_BANK_OFFSET       6
#define PAD_SAMPLE_PATH_SAMPLE_OFFSET     8

/************** PIN SETUP *******************/

#define PIN_POT          A10
#define PIN_PRESET       A12
#define PIN_SLIDER       A15
#define PIN_JOYSTICK_X   A13
#define PIN_JOYSTICK_Y   A11

#define PIN_ARCADE_BT    0

#define PIN_SWITCH_0     15
#define PIN_SWITCH_1     16
#define PIN_SWITCH_2     17
#define PIN_SWITCH_3     20
#define PIN_SWITCH_4     21

#define PIN_SWITCH_LED_0 5
#define PIN_SWITCH_LED_1 4
#define PIN_SWITCH_LED_2 3
#define PIN_SWITCH_LED_3 25
#define PIN_SWITCH_LED_4 32

#define PIN_PAD_INPUT_0  28
#define PIN_PAD_INPUT_1  27
#define PIN_PAD_INPUT_2  29
#define PIN_PAD_INPUT_3  2

#define PIN_PAD_OUTPUT_0 30
#define PIN_PAD_OUTPUT_1 31
#define PIN_PAD_OUTPUT_2 33
#define PIN_PAD_OUTPUT_3 24

#define PIN_PIXEL        1

#define PIN_SDCARD_CS    10
#define PIN_SDCARD_MOSI  7
#define PIN_SDCARD_SCK   14

/*************** BASIC HELPERS *****************/ 

#define SWITCH_COUNT 5
const short switchPin[SWITCH_COUNT]    = {    PIN_SWITCH_0,     PIN_SWITCH_1,     PIN_SWITCH_2,     PIN_SWITCH_3,     PIN_SWITCH_4};
const short switchLedPin[SWITCH_COUNT] = {PIN_SWITCH_LED_0, PIN_SWITCH_LED_1, PIN_SWITCH_LED_2, PIN_SWITCH_LED_3, PIN_SWITCH_LED_4};
const byte ledSwitchMultiplier[SWITCH_COUNT] = {        50,               50,               50,              200,               50};

#define PAD_WIDTH 4
const short padInputPin[PAD_WIDTH]  = { PIN_PAD_INPUT_0,  PIN_PAD_INPUT_1,  PIN_PAD_INPUT_2,  PIN_PAD_INPUT_3}; // orange wires
const short padOutputPin[PAD_WIDTH] = {PIN_PAD_OUTPUT_0, PIN_PAD_OUTPUT_1, PIN_PAD_OUTPUT_2, PIN_PAD_OUTPUT_3}; // white wires

#define BT_COUNT (PAD_WIDTH*PAD_WIDTH + SWITCH_COUNT + 1) // pad + switches + arcade button

/************** DUNEBOX STRUCTS ***************/  

struct Inputs
{
  short slider = 0;    // [0-1023]
  short joystickX = 0; // [0-1023]
  short joystickY = 0; // [0-1023]
  short rotary = 0;    // [0-1023]
  byte  preset = 0;    // [0-9]
  union
  {
    bool bt_as_array[BT_COUNT];
    struct
    {
      bool  arcade;
      bool  switches[SWITCH_COUNT];
      bool  pad[PAD_WIDTH][PAD_WIDTH];
    }bt;
  };
};

struct Events
{
  enum Event
  {
    None,
    Up,
    Down
  }; 

  Event preset;
  union
  {
    Event bt_as_array[BT_COUNT];
    struct
    {
      Event  arcade;
      Event  switches[SWITCH_COUNT];
      Event  pad[PAD_WIDTH][PAD_WIDTH];
    }bt;
  };
};

struct Color
{
  byte r, g, b;
};

struct Leds
{
  float switches[SWITCH_COUNT];
  Color pad[PAD_WIDTH][PAD_WIDTH];
};

enum Mode
{
  Intro,
  Preset,
  Transition,
  ModeCount
};

struct DuneBox
{
  Mode   mode;
  Inputs inputs;
  Events events;
  Leds   leds;
  float  audioRms;
};

DuneBox DB;

/**************** DEBUG ******************/  
void printInput(const char* inputName, short value)
{
  Serial.print(inputName);
  Serial.print("  ");
  Serial.print(value);
  Serial.print("\n");
}

void printInputs()
{
  Serial.print("-- INPUTS --------\n");
  
  // analogic
  printInput("top pot   ", DB.inputs.rotary);
  printInput("joystick X", DB.inputs.joystickX);
  printInput("joystick Y", DB.inputs.joystickY);
  printInput("slider    ", DB.inputs.slider);
  printInput("selector  ", DB.inputs.preset);

  // bt switches
  Serial.print("switches    ");
  for (byte i = 0; i<SWITCH_COUNT; ++i)
  {
    Serial.print(DB.inputs.bt.switches[i]);
    Serial.print(" ");
  }
  Serial.print("\n");

  // bt arcade
  printInput("arcade bt ", DB.inputs.bt.arcade);

  // bt pad
  for (byte y = 0; y<PAD_WIDTH; ++y)
  {
    if (y==0)
      Serial.print("pad         ");
    else
      Serial.print("            ");
      
    for (byte x = 0; x<PAD_WIDTH; ++x)
    {
      Serial.print(DB.inputs.bt.pad[y][x]);
      Serial.print(" ");
    }
    Serial.print("\n");
  }
}


/**********************************************/  
#define BT_BOUNCE 15

void debounceInputs(const Inputs& rawInputs, Inputs& inputs, Events& events)
{
  // debounce buttons
  {
    static byte btBounce[BT_COUNT] = {0};
    for (byte i = 0; i<BT_COUNT; ++i)
    {
      const bool newState = rawInputs.bt_as_array[i];
      const bool currentState = inputs.bt_as_array[i];
      
      if (currentState == newState)
        btBounce[i] = 0;
      else
        ++btBounce[i];

      if (btBounce[i] > BT_BOUNCE)
      {
        btBounce[i] = 0;
        inputs.bt_as_array[i] = newState;
        events.bt_as_array[i] = newState ? Events::Down : Events::Up;
      }
    }
  }

  // debounce preset
  {
    static byte presetBounce = 0;
    
    const byte newState = rawInputs.preset;
    const byte currentState = inputs.preset;

     if (currentState == newState)
        presetBounce = 0;
      else
        ++presetBounce;

      if (presetBounce > BT_BOUNCE)
      {
        presetBounce = 0;
        events.preset = newState > currentState ? Events::Up : Events::Down;
        inputs.preset = newState;
      }
  }
  
  inputs.slider    = rawInputs.slider;
  inputs.joystickX = rawInputs.joystickX;
  inputs.joystickY = rawInputs.joystickY;
  inputs.rotary    = rawInputs.rotary;
}

void getRawInputs(Inputs& inputs)
{
  /**** Analogic inputs ****/
  
  inputs.slider    = analogRead(PIN_SLIDER);
  inputs.joystickX = analogRead(PIN_JOYSTICK_X);
  inputs.joystickY = analogRead(PIN_JOYSTICK_Y);
  inputs.rotary    = analogRead(PIN_POT);
  inputs.preset   = (analogRead(PIN_PRESET)+(113/2)) / 113;

  /**** Digital inputs ****/
  
  inputs.bt.arcade = !digitalRead(PIN_ARCADE_BT);

  // scan pad
  for (byte x = 0; x<PAD_WIDTH; ++x)
  {
    digitalWrite(padOutputPin[x], LOW);
    for (byte y = 0; y<PAD_WIDTH; ++y)
      inputs.bt.pad[y][x] = !digitalRead(padInputPin[y]);
    digitalWrite(padOutputPin[x], HIGH);
    
    delayMicroseconds(1);
  }

  // scan switch
  for (byte i = 0; i<SWITCH_COUNT; ++i)
  {
    analogWrite(switchLedPin[i], 0);
    inputs.bt.switches[i] = digitalRead(switchPin[i]);
    analogWrite(switchLedPin[i], ledSwitchMultiplier[i] * DB.leds.switches[i]);
  }
}

void remapInputs(Inputs& inputs)
{
  inputs.slider    = constrain(map(inputs.slider,   15, 1015, 0, 1023), 0, 1023);
  inputs.joystickX = constrain(map(inputs.joystickX, 5, 1023, 0, 1023), 0, 1023);
  inputs.joystickY = constrain(map(inputs.joystickY, 5, 1018, 0, 1023), 0, 1023);
  inputs.rotary    = constrain(map(inputs.rotary,    5, 1018, 0, 1023), 0, 1023);
}

void updateInputs(bool init = false)
{
  Inputs rawInputs;
  getRawInputs(rawInputs);
  remapInputs(rawInputs);

  if (init)
    DB.inputs = rawInputs;
  else
    debounceInputs(rawInputs, DB.inputs, DB.events); // This function call is updating DB.inputs and DB.events
}

#define PIXEL_REFRESH_PERIOD (1000/PIXEL_REFRESH_RATE) // ms
#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(PAD_WIDTH*PAD_WIDTH, PIN_PIXEL, NEO_GRB + NEO_KHZ800);

void updateLeds()
{
  // Update leds at the specified frequency
  static unsigned long lastPixelRefresh = 0;
  if (millis() - lastPixelRefresh < PIXEL_REFRESH_PERIOD)
    return;

  // Update switches leds
  for (byte i=0; i<SWITCH_COUNT; ++i)
    analogWrite(switchLedPin[i], ledSwitchMultiplier[i] * DB.leds.switches[i]);

  for (byte y = 0; y<PAD_WIDTH; ++y)
  {
    for (byte x = 0; x<PAD_WIDTH; ++x)
    {
      const Color& pixel = DB.leds.pad[y][x];
      pixels.setPixelColor(getLedIndex(y, x), pixels.Color(pixel.r, pixel.g, pixel.b));
    }
  }
  
  noInterrupts();
  pixels.show();
  interrupts();

  lastPixelRefresh = millis();
}


short joystickCoordX, joystickCoordY = 0;
short lastJoystickCoordX, lastJoystickCoordY = 0;
long lastJoystickCoordDebounceToggle = 0;
long joystickCoordDebounceDuration = 5; //ms

bool getPadIndexFromAnalogicJoystick(short& padX, short& padY)
{
  if (DB.inputs.joystickX<512-400)
    padX = 0;
  else if (DB.inputs.joystickX<512)
    padX = 1;
  else if (DB.inputs.joystickX<512+505) 
    padX = 2;
  else
    padX = 3;

  if (DB.inputs.joystickY<512-350)
    padY = 0;
  else if (DB.inputs.joystickY<512)
    padY = 1;
  else if (DB.inputs.joystickY<512+490) 
    padY = 2;
  else
    padY = 3;

  padY = 3 - padY;;

  if ((abs(DB.inputs.joystickX - 512) < 64 ) && (abs(DB.inputs.joystickY - 512) < 80 ))
  {
    padX = -1;
    padY = -1;
    return false;
  }
  
  return true;
}

float padLedIntensityFromJoystick[PAD_WIDTH][PAD_WIDTH] = {0};
void updateJoystickCoord()
{
  short readX, readY;
  bool joystickInUsed = getPadIndexFromAnalogicJoystick(readX, readY);

  if ((readX != lastJoystickCoordX) || (readY != lastJoystickCoordY))
    lastJoystickCoordDebounceToggle = millis();
    
  if ((millis() - lastJoystickCoordDebounceToggle) > joystickCoordDebounceDuration)
  {
    if (readX != -1 && readY != -1 && (readX != joystickCoordX || readY != joystickCoordY))
    {
      DB.events.bt.pad[readY][readX] = Events::Down;
      padLedIntensityFromJoystick[readY][readX] = 1.0f;
    }
      
    joystickCoordX = readX;
    joystickCoordY = readY;
  }
  
  lastJoystickCoordX = readX;
  lastJoystickCoordY = readY;
}

/************** PAD LEDS *******************/

byte getLedIndex(byte y, byte x)
{
  return (PAD_WIDTH-1-x) + (PAD_WIDTH-1-y)*PAD_WIDTH;
}

struct Color lerp(Color c1, Color c2, float x)
{ 
  Color result;
  result.r = (byte)((float)c1.r + x * ((float)c2.r - (float)c1.r));
  result.g = (byte)((float)c1.g + x * ((float)c2.g - (float)c1.g));
  result.b = (byte)((float)c1.b + x * ((float)c2.b - (float)c1.b));
  return result;
}

Color getColorFromFloat(float x)
{
  Color color = {0, 0, 0};
  float v = 0.f;
  if (x<0.333)
  {
    v = x*3;
    color = lerp({255, 0, 0}, {0, 255, 0}, v);
  }
  else if (x<0.666)
  {
    v = (x-0.333)*3;
    color = lerp({0, 255, 0}, {0, 0, 255}, v);
  }
  else
  {
    v = (x-0.666)*3;
    color = lerp({0, 0, 255}, {255, 0, 0}, v);
  }
  return color;
}

float playingSoundAmplitude[4] = {0.0f};
void updateLedPattern()
{
  static byte currentLine = 0;
  static byte linePeriod = 10; // change line every 5 update calls
  static short linePeriodCount = 0;
  static float lineDirection = 1.f;
  static float colorSpeed = 0.005f;
  static float colorFloat = 0.f;
  
  linePeriodCount++;
  colorFloat += colorSpeed;
  
  if (colorFloat > 1.f)
    colorFloat -= 1.f;
  if (colorFloat < 0.f)
    colorFloat += 1.f;

  // update current line
  if (linePeriodCount>=linePeriod)
  {
    if ((currentLine == 0) || (currentLine == (PAD_WIDTH-1)))
      lineDirection *= -1;
    currentLine = (currentLine+lineDirection);
    
    linePeriodCount = 0;
  }

  Color color = getColorFromFloat(colorFloat);
  /*
  Color color = {255, 0, 0};
  color.r = 0;
  color.g = 255*2;
  color.b = 255*2;
 */
  for (byte y = 0; y<PAD_WIDTH; ++y)
  {
    for (byte x = 0; x<PAD_WIDTH; ++x)
    {
      padLedIntensityFromJoystick[y][x] -= 0.05f;
      float ledIntensity = padLedIntensityFromJoystick[y][x];
      //float ledIntensity = (y == currentLine) ? 1.0f : 0.0f;// + 0.1 * ((y+1)%PAD_WIDTH == currentLine);
      //float ledIntensity = (y == currentLine) ? 1.0f : 0.0f;// + 0.1 * ((y+1)%PAD_WIDTH == currentLine);
      //float ledIntensity = playingSoundAmplitude * (y == 0) * (x == 0);
      if (DB.inputs.bt.pad[y][x])
        padLedIntensityFromJoystick[y][x] = 1.0f;

      DB.leds.pad[y][x] = {color.r * ledIntensity, color.g * ledIntensity, color.b * ledIntensity};
    }
  }
}



unsigned long lastPixelRefresh = 0;
void updatePadLeds()
{
  if (millis() - lastPixelRefresh < PIXEL_REFRESH_PERIOD)
    return;

  updateLedPattern();

  lastPixelRefresh = millis();
}

/**************** AUDIO *********************/

#define WAVEPLAYER_COUNT 3



#define AUDIO_PARAM_REFRESH_RATE 30 // Hz
#define AUDIO_PARAM_REFRESH_PERIOD (1000/AUDIO_PARAM_REFRESH_RATE) // ms

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioPlaySdWav           playSdWav1;     //xy=437,269
AudioPlaySdWav           playSdWav2;     //xy=440,306
AudioPlaySdWav           playSdWav4; //xy=440,385
AudioPlaySdWav           playSdWav3; //xy=441,346
AudioMixer4              mixer1;         //xy=607,322
AudioAnalyzeFFT256       fft256_1;       //xy=635,479
AudioAnalyzeRMS          rms1;           //xy=817,481
AudioEffectFlange        flange1;        //xy=835,322
AudioMixer4              mixer2;         //xy=1006,341
AudioOutputAnalog        dac1;           //xy=1148,341
AudioConnection          patchCord1(playSdWav1, 0, mixer1, 0);
AudioConnection          patchCord2(playSdWav2, 0, mixer1, 1);
AudioConnection          patchCord3(playSdWav4, 0, mixer1, 3);
AudioConnection          patchCord4(playSdWav4, 0, fft256_1, 0);
AudioConnection          patchCord5(playSdWav3, 0, mixer1, 2);
AudioConnection          patchCord6(mixer1, flange1);
AudioConnection          patchCord7(mixer1, rms1);
AudioConnection          patchCord8(flange1, 0, mixer2, 0);
AudioConnection          patchCord9(mixer2, dac1);
AudioControlSGTL5000     sgtl5000_1;     //xy=671,174
// GUItool: end automatically generated code

#define FLANGE_DELAY_LENGTH (24*AUDIO_BLOCK_SAMPLES)
// Allocate the delay lines for left and right channels
short delayline[FLANGE_DELAY_LENGTH];
int s_idx = FLANGE_DELAY_LENGTH/4;
int s_depth = FLANGE_DELAY_LENGTH/4;
double s_freq = 10;



AudioPlaySdWav* wavePlayer[WAVEPLAYER_COUNT] = {&playSdWav1, &playSdWav2, &playSdWav3};
AudioPlaySdWav& fxPlayer = playSdWav4;
unsigned long wavePlayerLastPlayTime[WAVEPLAYER_COUNT] = {0};


byte getWavePlayerIndex()
{
  byte index = 0;
  for (uint i=0; i<WAVEPLAYER_COUNT; ++i)
  {
    if (wavePlayerLastPlayTime[i] <= wavePlayerLastPlayTime[index])
      index = i;
  }
  wavePlayerLastPlayTime[index] = millis();
  return index;
}

void playFxAudio(const char* path)
{
  AudioNoInterrupts();
  fxPlayer.play(path);
  AudioInterrupts();
  delay(5);
}

void stopFxAudio()
{
  AudioNoInterrupts();
  fxPlayer.stop();
  AudioInterrupts();
}

void setupAudio()
{
  AudioMemory(50);
  sgtl5000_1.enable();
  dac1.analogReference(EXTERNAL);
  SPI.setMOSI(PIN_SDCARD_MOSI);
  SPI.setSCK(PIN_SDCARD_SCK);
  if (!(SD.begin(PIN_SDCARD_CS))) 
  {
    while (1) 
    {
      Serial.println("Unable to access the SD card");
      delay(100);
    }
  }

  mixer1.gain(0, 0.25f);
  mixer1.gain(1, 0.25f);
  mixer1.gain(2, 0.25f);
  mixer1.gain(3, 0.50f);

  mixer2.gain(0, 1.0f);
  mixer2.gain(1, 0.0f);
  mixer2.gain(2, 0.0f);
  mixer2.gain(3, 0.0f);

  //flange1.begin(delayline,FLANGE_DELAY_LENGTH,s_idx,s_depth,s_freq);
  flange1.begin(delayline,FLANGE_DELAY_LENGTH,0,0,0);
}

long lastAudioParamRefresh = 0;
void updateAudio()
{
  if (millis() - lastAudioParamRefresh < AUDIO_PARAM_REFRESH_PERIOD)
    return;

  const float volumeMax = 1.00f; // [0.0 - 1.0]
  const float volumeMin = 0.05f; // [0.0 - 1.0]
  float channelVolume = GLOBAL_VOLUME * map(DB.inputs.rotary,   0, 1023, volumeMin*1023, volumeMax*1023) / 1023.f;
  mixer2.gain(0, channelVolume);

  //s_freq = 8.f * rawSensorValue[ANALOG_SLIDER] / 1024.0f;
  //flange1.voices(s_idx, s_depth, s_freq);

  if(rms1.available())
    DB.audioRms = constrain(5.0f * rms1.read(), 0.0f, 1.0f);//constrain(map(rms1.read(),   0.05, 0.2, 0, 1.0), 0, 1.0); 

  if(fft256_1.available())
  {
    playingSoundAmplitude[0] = fft256_1.read(0, 4);
    playingSoundAmplitude[1] = fft256_1.read(3, 5);
    playingSoundAmplitude[2] = fft256_1.read(4, 6);
    playingSoundAmplitude[3] = fft256_1.read(5, 7);
  }

  lastAudioParamRefresh = millis();
}

/************ SWITCH LEDS ANIMATIONS ****************/ 

// Anim: RANDOM SOFT BLINKING
void animSwitchLed_Random(float* led, long refreshPeriod)
{
    static float localLed[SWITCH_COUNT] = {0};
  
    const long currentTime = millis();
    
    static long lastRefresh = 0;
    if ((currentTime - lastRefresh) > refreshPeriod)
    {
      lastRefresh = currentTime;

      for (byte i=0; i<SWITCH_COUNT; ++i)
        localLed[i] = 0;

      localLed[random(SWITCH_COUNT)] = 1;
      localLed[random(SWITCH_COUNT)] = 1;
    }

    const float decayValue = 0.2f / refreshPeriod;

    for (byte i=0; i<SWITCH_COUNT; ++i)
      led[i] = led[i] + decayValue * ((led[i] > localLed[i]) ? -1 : 1);
}

void _animSwitchLed_Random(float* led, bool randomizeSettings)
{
    static long refreshPeriod = 200;

    if (randomizeSettings)
      refreshPeriod = random(100, 250);

    animSwitchLed_Random(led, refreshPeriod);
}

// Anim: CLASSIC TRAIN
void animSwitchLed_Train(float* led, long refreshPeriod, int direction)
{
    const long currentTime = millis();
    static long lastRefresh = 0;
    static byte currentIndex = 0;
    
    if ((currentTime - lastRefresh) > refreshPeriod)
    {
      lastRefresh = currentTime;
      currentIndex = (currentIndex + SWITCH_COUNT + direction) % SWITCH_COUNT;
      for (byte i=0; i<SWITCH_COUNT; ++i)
        led[i] = (i == currentIndex);
    }
}

void _animSwitchLed_Train(float* led, bool randomizeSettings)
{
    static int direction = 1;
    static long refreshPeriod = 100;
    
    if (randomizeSettings)
    {
      direction *= -1;
      refreshPeriod = random(50, 150);
    }

    animSwitchLed_Train(led, refreshPeriod, direction);
}

// Generic function that will randomly choose a new animation with new settings when "changeSettings" is True
#define SWITCH_ANIM_COUNT 2
void animSwitchLed(int animIndex, float* led, bool changeSettings)
{
  static void (*animFunction[SWITCH_ANIM_COUNT]) (float* led, bool changeSettings) = {_animSwitchLed_Random, _animSwitchLed_Train};

  if(animIndex == -1)
  {
    static int randomIndex = 0;
    if (changeSettings)
      randomIndex = random(SWITCH_ANIM_COUNT);

    animFunction[randomIndex](led, changeSettings);
  }
  else
    animFunction[animIndex](led, changeSettings);
}

/************ PRESET COMMUN ******************/ 

void playAudioSamplesFromPad()
{
  static char path[] = PAD_SAMPLE_PATH;
  
  for (byte y = 0; y<PAD_WIDTH; ++y)
  {
    for (byte x = 0; x<PAD_WIDTH; ++x)
    {
      if (DB.events.bt.pad[y][x] != Events::Down)
        continue;
      
      path[PAD_SAMPLE_PATH_BANK_OFFSET] = 'A' + (DB.inputs.preset%4);
      path[PAD_SAMPLE_PATH_SAMPLE_OFFSET] = 'A' + (x + y*PAD_WIDTH);

      byte playerIndex = getWavePlayerIndex();
      
      Serial.print(DB.inputs.preset);
      Serial.print("[");
      Serial.print(playerIndex);
      Serial.print("] play ");
      Serial.println(path);
  
      AudioNoInterrupts();
      wavePlayer[playerIndex]->play(path);
      AudioInterrupts();
      delay(5);
    }
  }
}

void playAudioSampleFromSwitches()
{
  static char path[] = SWITCH_SAMPLE_PATH;
  
  for (byte i=0; i<SWITCH_COUNT; ++i)
  {
    if (DB.events.bt.switches[i] == Events::Down)
    {
      path[SWITCH_SAMPLE_PATH_OFFSET] = 'A' + i;
      
      byte playerIndex = getWavePlayerIndex();
      
      Serial.print("play ");
      Serial.println(path);
  
      AudioNoInterrupts();
      wavePlayer[playerIndex]->play(path);
      AudioInterrupts();
      delay(5);    
    }
  }
}

void arcadeButtonPlaysRandomSoundAndAnimateSwitchLeds()
{
  // Play a randon sound
  if (DB.events.bt.arcade == Events::Down)
  {
    char soundPath [SOUND_RANDOM_PATH_SIZE];
    sprintf(soundPath, SOUND_RANDOM_PATH, random(SOUND_RANDOM_FILE_COUNT));
    
    playFxAudio(soundPath);
  }
/*
  // If a random sound is playing, animate the switch leds,
  // else switch leds states = swtich states.
  if (fxPlayer.isPlaying())
  {
    const bool changeSettings = (DB.events.bt.arcade == Events::Down);
    animSwitchLed(-1, DB.leds.switches, changeSettings);
  }
  else
  {
    for (byte i=0; i<SWITCH_COUNT; ++i)
      DB.leds.switches[i] = DB.inputs.bt.switches[i];
  }
  */
  for (byte i=0; i<SWITCH_COUNT; ++i)
    DB.leds.switches[i] = constrain((DB.audioRms/0.08f)-i, 0.0f, 1.0f);
}


/************** INTRO *********************/

void executeIntro()
{
  static bool startPlaying = true;
  static unsigned long lastPadUpdate = 0;
  
  // Start intro
  if (startPlaying)
    playFxAudio(SOUND_STARTUP);

  const bool randomizeSettings = startPlaying;
  animSwitchLed(-1, DB.leds.switches, randomizeSettings);

  if (millis() - lastPadUpdate > 20)
  {
    lastPadUpdate = millis();

    const long introDuration = 5000;
    static const float introFinalColor = random(4096)/4096.0f;

    float relativeTime = (introDuration - millis()) / (float)introDuration;
        
    int x = random(0, 5);
    int y = random(0, 5);
    if (DB.leds.pad[y][x].r || DB.leds.pad[y][x].g || DB.leds.pad[y][x].b)
      DB.leds.pad[y][x] = {0, 0, 0};
    else
      DB.leds.pad[y][x] = getColorFromFloat(fmod(introFinalColor + (0.1)*(random(4096)/4096.0f), 1.0f)); //{255 - DB.leds.pad[y][x].r, 0, 0};
    /*
    const long fadeOutStartTime = 3000;
    const long fadeOutDuration = 2000;
    if(millis() > fadeOutStartTime)
    {
      DB.leds.pad[y][x] = lerp(DB.leds.pad[y][x], {0, 0, 0}, constrain(millis()-fadeOutStartTime, 0, fadeOutDuration)/(float)fadeOutDuration);
    }
    */
  }


  if (startPlaying)
    startPlaying = false;

  // End intro
  if (!fxPlayer.isPlaying())
    DB.mode = Mode::Preset;
}

/************** TRANSITION *********************/

void executeTransition()
{
  static byte goToPreset = -1;
  static Color color;
  static Color halfColor;
  static long startTransitionTime;
  static bool mask;
  
  if (goToPreset != DB.inputs.preset)
  {
    playFxAudio(SOUND_CHANGE_PRESET);
    
    goToPreset = DB.inputs.preset; 
    startTransitionTime = millis();
    mask = true;
    color = getColorFromFloat(random(1024)/1023.f);
    halfColor = {color.r * 0.4, color.g * 0.4, color.b * 0.4};
  }

  // the full pad will blink with a random color during transition
  const bool newMask = false;//((millis() - startTransitionTime) % 150) > 75;
  if (newMask != mask)
  {
    Color currentColor = mask ? color : halfColor;

    for (byte y = 0; y<PAD_WIDTH; ++y)
      for (byte x = 0; x<PAD_WIDTH; ++x)
        DB.leds.pad[y][x] = currentColor;    
  }
  mask = newMask;

  animSwitchLed_Random(DB.leds.switches, 100);//50, -1);

  // End transition
  if (!fxPlayer.isPlaying())
    DB.mode = Mode::Preset;
}  

/************** PRESET A *********************/

void testAnim();

void executePresetA()
{
  arcadeButtonPlaysRandomSoundAndAnimateSwitchLeds();
  playAudioSampleFromSwitches();
  playAudioSamplesFromPad();
  updateJoystickCoord();
  //updatePadLeds();
  testAnim();
}

/************** PRESET B *********************/

void executePresetB()
{
  static int currentIndex = -1;
  static char path[] = PAD_SAMPLE_PATH;

  int newIndex = currentIndex;
  for (byte y = 0; y<PAD_WIDTH; ++y)
  {
    for (byte x = 0; x<PAD_WIDTH; ++x)
    {
      if (DB.events.bt.pad[y][x] == Events::Down)
      {
        const int index = getLedIndex(x, y);
        newIndex = (newIndex == index) ? -1 : index;
      }
    }
  }

  // select a new random music
  if ((currentIndex >=0) && (newIndex >= 0) && !fxPlayer.isPlaying())
    while(newIndex == currentIndex)
      newIndex = random(0, 16);

  // play/stop music
  if (newIndex == -1)
    stopFxAudio();
  else if (newIndex != currentIndex)
  {
    path[PAD_SAMPLE_PATH_BANK_OFFSET] = 'A' + (DB.inputs.preset);
    path[PAD_SAMPLE_PATH_SAMPLE_OFFSET] = 'A' + newIndex;
      
    playFxAudio(path); // to replace 
  }
  
  // update led state
  const float period = 3.0; // seconds
  const float speedFactor =  TWO_PI / (1000.0 * period);
  float intensity = 0.6 + 0.4 * sin((millis()%600000) * speedFactor);
  intensity *= 0.2 + 0.8 * DB.inputs.slider / 1024.0;
  memset(&DB.leds, 0, sizeof(DB.leds));
  if (currentIndex >= 0)
    DB.leds.pad[PAD_WIDTH-1-(currentIndex%4)][PAD_WIDTH-1-(currentIndex/4)] = {50 * intensity, 0 * intensity, 255 * intensity};

  currentIndex = newIndex;
}

/************** PRESET UPDATE FUNCTIONS *********************/


void executePreset() 
{ 
  static void (*executePresetPtr[10]) () = {
    executePresetA, 
    executePresetA,
    executePresetA,
    executePresetA,
    executePresetA,
    executePresetA,
    executePresetA,
    executePresetA,
    executePresetA,
    executePresetB
  };
  executePresetPtr[DB.inputs.preset](); 
}

/************** MODE UPDATE FUNCTIONS *********************/

void executeLogic() 
{
  if (DB.events.preset != Events::None)
    DB.mode = Mode::Transition;
    
  switch(DB.mode)
  {
    case Intro:  executeIntro();      break;
    case Preset: executePreset();     break;    
    default:     executeTransition(); break;
  }
}

/**************** SETUP *********************/

void setup() 
{
  // serial com. speed
  Serial.begin(9600);
  
  // setup random seed using connected pins as we don't have any free one.
  randomSeed(analogRead(PIN_POT) + (analogRead(PIN_JOYSTICK_X) << 16));

  // setup switch
  for (byte i=0; i<SWITCH_COUNT; ++i)
  {
    pinMode(switchPin[i], INPUT_PULLUP);
    analogWrite(switchLedPin[i], 0);
  }
  // setup arcade button
  pinMode(PIN_ARCADE_BT, INPUT_PULLUP);

  // setup pad buttons
  for (byte i=0; i<PAD_WIDTH; ++i)
  {
    pinMode(padInputPin[i], INPUT_PULLUP);
    pinMode(padOutputPin[i], OUTPUT);
    digitalWrite(padOutputPin[i], HIGH);
  }

  // setup pad leds (neo pixels)
  pixels.begin(); // this initializes the NeoPixel library
  pixels.setBrightness(PIXEL_MAX_INTENSITY);

  setupAudio();

  // init DuneBox
  memset(&DB, 0, sizeof(DB));
  //DB.mode = Preset; // Skip the intro

  // get initial inputs
  updateInputs(true);
}

struct Vec2
{
  byte x, y;
};

/*
class AnimPattern
{
  public:
    virtual bool isActive(unsigned long currentTime, byte x, byte y) = 0;

    unsigned long startTime;
    byte          startX, startY;
};

class AnimPattern_Cross : public AnimPattern
{
  public:
    bool isActive(unsigned long currentTime, byte x, byte y) override
    {     
      long t = (currentTime - startTime) / 100;
      //Serial.print(t);
      //Serial.print("\n");

      if (t > 50)
        return false;

      if ((y == startY) || (x == startX))
        if((abs(x-startX)+abs(y-startY))==t%5)
          return true;

      return false;
        
      if ((abs(x-startX)+abs(y-startY))==t)
        return true;

      return false;
    }
};
*/

class AnimPattern
{
  public:
    virtual bool isActive(unsigned long currentTime, byte x, byte y) = 0;

    unsigned long startTime;
    byte          startX, startY;
};

class AnimPattern_Cross : public AnimPattern
{
  public:
    bool isActive(unsigned long currentTime, byte x, byte y) override
    {     
      long t = (currentTime - startTime) / 100;
      //Serial.print(t);
      //Serial.print("\n");

      if (t > 4)
        return false;

      if ((y == startY) || (x == startX))
        if((abs(x-startX)+abs(y-startY))==t%5)
          return true;

      return false;
    }
};

void testAnim()
{ 
  //if (millis() - lastPixelRefresh < PIXEL_REFRESH_PERIOD)
  //  return;

  static float baseFloatColor = 0;
  baseFloatColor += 0.00015f * DB.inputs.slider/1024.f;
  baseFloatColor = fmod(baseFloatColor, 1.0f);

  Color colorPressed = getColorFromFloat(baseFloatColor);
  Color colorSlider  = getColorFromFloat(fmod(baseFloatColor + 0.5f, 1.0f));

  float in = 0.25f;
  Color colorBackground = {colorSlider.r *in, colorSlider.g *in, colorSlider.b *in};
  
  in = 1.0f;
  Color colorPattern = {colorSlider.r *in, colorSlider.g *in, colorSlider.b *in};
    
  unsigned long currentTime = millis();
  static AnimPattern_Cross pattern[PAD_WIDTH][PAD_WIDTH];

  for (byte y = 0; y<PAD_WIDTH; ++y)
  {
    for (byte x = 0; x<PAD_WIDTH; ++x)
    {
      if(DB.events.bt.pad[y][x] == Events::Down)
      {
        pattern[y][x].startTime = currentTime;
        pattern[y][x].startX = x;
        pattern[y][x].startY = y;
      }
      //else if(DB.events.bt.pad[y][x] == Events::Up)
      //  pattern[y][x].startTime = 0;
      
      bool patternValue = false;
      for (byte py = 0; py<PAD_WIDTH; ++py)
        for (byte px = 0; px<PAD_WIDTH; ++px)
          patternValue |= pattern[py][px].isActive(currentTime, x, y);

      const bool isPressed = DB.inputs.bt.pad[y][x];
      DB.leds.pad[y][x] = isPressed ? colorPressed : (patternValue ? colorPattern : colorBackground);
    }
  }

  //lastPixelRefresh = millis();
}

/************** MAIN LOOP *******************/

void loop() 
{
  // clear events from previous iteration
  memset(&DB.events, 0, sizeof(DB.events));
  
  updateInputs(); 
  //printInputs();
  
  executeLogic();
  //testAnim();

  updateLeds();
  updateAudio();
}
