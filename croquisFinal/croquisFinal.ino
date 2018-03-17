
#define GLOBAL_VOLUME         1.0f   // [0.0 - 1.0]
#define PIXEL_MAX_INTENSITY   150    // [0 - 255]
#define PIXEL_REFRESH_RATE    60     // Hz

/*********** SD CARD CONTENT ****************/

#define SOUND_STARTUP                     "startup.wav"
#define SOUND_CHANGE_PRESET_UP            "up.wav"
#define SOUND_CHANGE_PRESET_DOWN          "down.wav"

#define SOUND_RANDOM_PATH                 "random/%03lu.wav"
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

struct Vec2
{
 byte x, y;
};

struct Color
{
  byte r, g, b;
};

Color operator*(float a, const Color& color) // a = [0.0 - 1.0]
{ 
  return {(byte)(a*color.r), (byte)(a*color.g), (byte)(a*color.b)}; 
} 

struct Leds
{
  float switches[SWITCH_COUNT];
  Color pad[PAD_WIDTH][PAD_WIDTH];
};

struct Audio
{
  float rmsGlobal;
  float rmsFx;
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
  Audio  audio;
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

unsigned long padJoystickTime[PAD_WIDTH][PAD_WIDTH] = {0};
short joystickCoordX, joystickCoordY = 0;
short lastJoystickCoordX, lastJoystickCoordY = 0;
unsigned long lastJoystickCoordDebounceToggle = 0;
unsigned long joystickCoordDebounceDuration = 5; //ms
void updateJoystickCoord()
{
  short readX, readY;
  bool joystickInUsed = getPadIndexFromAnalogicJoystick(readX, readY);

  if ((readX != lastJoystickCoordX) || (readY != lastJoystickCoordY))
    lastJoystickCoordDebounceToggle = millis();
    
  if ((millis() - lastJoystickCoordDebounceToggle) > joystickCoordDebounceDuration)
  {
    if (joystickInUsed && (readX != joystickCoordX || readY != joystickCoordY))
    {
      
      padJoystickTime[readY][readX] = millis();
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



unsigned long lastPixelRefresh = 0;
void updatePadLeds()
{
  if (millis() - lastPixelRefresh < PIXEL_REFRESH_PERIOD)
    return;

  //updateLedPattern();

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
AudioPlaySdWav           playSdWav1;     //xy=349,401
AudioPlaySdWav           playSdWav2;     //xy=352,438
AudioPlaySdWav           playSdWav4;     //xy=352,517
AudioPlaySdWav           playSdWav3;     //xy=353,478
AudioMixer4              mixer1;         //xy=519,454
AudioAnalyzeRMS          rmsFx;          //xy=544,541
AudioAnalyzeFFT256       fft256_1;       //xy=547,611
AudioAnalyzeRMS          rmsGlobal;      //xy=727,607
AudioPlaySdRaw           playSdRaw1;     //xy=735,499
AudioEffectFlange        flange1;        //xy=744,423
AudioInputI2S            i2s1;           //xy=877,343
AudioMixer4              mixer2;         //xy=918,473
AudioRecordQueue         queue1;         //xy=1017,338
AudioOutputAnalog        dac1;           //xy=1060,473
AudioConnection          patchCord1(playSdWav1, 0, mixer1, 0);
AudioConnection          patchCord2(playSdWav2, 0, mixer1, 1);
AudioConnection          patchCord3(playSdWav4, 0, mixer1, 3);
AudioConnection          patchCord4(playSdWav4, 0, fft256_1, 0);
AudioConnection          patchCord5(playSdWav4, 0, rmsFx, 0);
AudioConnection          patchCord6(playSdWav3, 0, mixer1, 2);
AudioConnection          patchCord7(mixer1, rmsGlobal);
AudioConnection          patchCord8(mixer1, 0, mixer2, 0);
AudioConnection          patchCord9(playSdRaw1, 0, mixer2, 1);
AudioConnection          patchCord10(i2s1, 0, queue1, 0);
AudioConnection          patchCord11(mixer2, dac1);
AudioControlSGTL5000     sgtl5000_1;     //xy=685,52
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
  AudioMemory(100);
  sgtl5000_1.enable();

  sgtl5000_1.inputSelect(AUDIO_INPUT_MIC);
  sgtl5000_1.micGain(36);
  
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
  mixer2.gain(1, 1.0f);
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
  mixer2.gain(1, channelVolume);

  //s_freq = 8.f * rawSensorValue[ANALOG_SLIDER] / 1024.0f;
  //flange1.voices(s_idx, s_depth, s_freq);

  if(rmsGlobal.available())
    DB.audio.rmsGlobal = constrain(5.0f * rmsGlobal.read(), 0.0f, 1.0f);
    
  if(rmsFx.available())
    DB.audio.rmsFx = constrain(5.0f * rmsFx.read(), 0.0f, 1.0f);
    
  if(fft256_1.available())
  {
    //playingSoundAmplitude[0] = fft256_1.read(0, 4);
    //playingSoundAmplitude[1] = fft256_1.read(3, 5);
    //playingSoundAmplitude[2] = fft256_1.read(4, 6);
    //0playingSoundAmplitude[3] = fft256_1.read(5, 7);
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

void updateSamplePath(char* path, byte presetIndex, byte sampleIndex)
{
  path[PAD_SAMPLE_PATH_BANK_OFFSET] = 'A' + presetIndex;
  path[PAD_SAMPLE_PATH_SAMPLE_OFFSET] = 'A' + sampleIndex;
}

void playAudioSamplesFromPad()
{
  static char path[] = PAD_SAMPLE_PATH;
  
  for (byte y = 0; y<PAD_WIDTH; ++y)
  {
    for (byte x = 0; x<PAD_WIDTH; ++x)
    {
      if (DB.events.bt.pad[y][x] != Events::Down)
        continue;

      updateSamplePath(path, DB.inputs.preset, x + y*PAD_WIDTH);
      
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
    DB.leds.switches[i] = constrain((DB.audio.rmsGlobal/0.08f)-i, 0.0f, 1.0f);
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
  animSwitchLed(0, DB.leds.switches, randomizeSettings);

  if (millis() - lastPadUpdate > 20)
  {
    lastPadUpdate = millis();

    static const float introFinalColor = random(4096)/4096.0f;
        
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

/*
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
*/

void executeTransition()
{
  static byte goToPreset = -1;
  static float transitionColor;
  if (goToPreset != DB.inputs.preset)
  {
    if (DB.events.preset == Events::Up)
      playFxAudio(SOUND_CHANGE_PRESET_UP);
    else
      playFxAudio(SOUND_CHANGE_PRESET_DOWN);
      
    goToPreset = DB.inputs.preset; 
    transitionColor = random(999999)/999999.0f;
    memset(&DB.leds.pad, 0, PAD_WIDTH*PAD_WIDTH*sizeof(Color));
  }
  animSwitchLed_Random(DB.leds.switches, 100);//50, -1);

  static unsigned long lastPadUpdate = 0;
  if (millis() - lastPadUpdate > 10)
  {
    lastPadUpdate = millis();

    int x = random(0, 5);
    int y = random(0, 5);
    if (DB.leds.pad[y][x].r || DB.leds.pad[y][x].g || DB.leds.pad[y][x].b)
      DB.leds.pad[y][x] = {0, 0, 0};
    else
      DB.leds.pad[y][x] = getColorFromFloat(fmod(transitionColor + (0.15)*(random(4096)/4096.0f), 1.0f));
  }

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
  updateJoystickCoord();
  playAudioSamplesFromPad();
  
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
    DB.leds.pad[PAD_WIDTH-1-(currentIndex%4)][PAD_WIDTH-1-(currentIndex/4)] = {(byte)(50 * intensity), (byte)(0 * intensity), (byte)(255 * intensity)};

  currentIndex = newIndex;
}


File frec; // recording file
unsigned long recByteSaved = 0;
bool recording = false;

void startRecording(const char* fileName)
{
  Serial.print("recording start ");
  Serial.println(fileName);
  
  recording = true;

  if (SD.exists(fileName)) {
    // The SD library writes new data to the end of the
    // file, so to start a new recording, the old file
    // must be deleted before new data is written.
    SD.remove(fileName);
  }
  frec = SD.open(fileName, FILE_WRITE);
  if (frec)
    queue1.begin();

  recByteSaved = 0;
}

unsigned long ChunkSize = 0L;
unsigned long Subchunk1Size = 16;
unsigned int AudioFormat = 1;
unsigned int numChannels = 1;
unsigned long sampleRate = 44100;
unsigned int bitsPerSample = 16;
unsigned long byteRate = sampleRate * numChannels * (bitsPerSample / 8); // samplerate x channels x (bitspersample / 8)
unsigned int blockAlign = numChannels * bitsPerSample / 8;
byte byte1, byte2, byte3, byte4;

void writeOutHeader() // WAV header
{
  unsigned long Subchunk2Size = recByteSaved;
  unsigned long ChunkSize = Subchunk2Size + 36;
  frec.seek(0);
  frec.write("RIFF");
  byte1 = ChunkSize & 0xff;
  byte2 = (ChunkSize >> 8) & 0xff;
  byte3 = (ChunkSize >> 16) & 0xff;
  byte4 = (ChunkSize >> 24) & 0xff;
  frec.write(byte1);  frec.write(byte2);  frec.write(byte3);  frec.write(byte4);
  frec.write("WAVE");
  frec.write("fmt ");
  byte1 = Subchunk1Size & 0xff;
  byte2 = (Subchunk1Size >> 8) & 0xff;
  byte3 = (Subchunk1Size >> 16) & 0xff;
  byte4 = (Subchunk1Size >> 24) & 0xff;
  frec.write(byte1);  frec.write(byte2);  frec.write(byte3);  frec.write(byte4);
  byte1 = AudioFormat & 0xff;
  byte2 = (AudioFormat >> 8) & 0xff;
  frec.write(byte1);  frec.write(byte2);
  byte1 = numChannels & 0xff;
  byte2 = (numChannels >> 8) & 0xff;
  frec.write(byte1);  frec.write(byte2);
  byte1 = sampleRate & 0xff;
  byte2 = (sampleRate >> 8) & 0xff;
  byte3 = (sampleRate >> 16) & 0xff;
  byte4 = (sampleRate >> 24) & 0xff;
  frec.write(byte1);  frec.write(byte2);  frec.write(byte3);  frec.write(byte4);
  byte1 = byteRate & 0xff;
  byte2 = (byteRate >> 8) & 0xff;
  byte3 = (byteRate >> 16) & 0xff;
  byte4 = (byteRate >> 24) & 0xff;
  frec.write(byte1);  frec.write(byte2);  frec.write(byte3);  frec.write(byte4);
  byte1 = blockAlign & 0xff;
  byte2 = (blockAlign >> 8) & 0xff;
  frec.write(byte1);  frec.write(byte2);
  byte1 = bitsPerSample & 0xff;
  byte2 = (bitsPerSample >> 8) & 0xff;
  frec.write(byte1);  frec.write(byte2);
  frec.write("data");
  byte1 = Subchunk2Size & 0xff;
  byte2 = (Subchunk2Size >> 8) & 0xff;
  byte3 = (Subchunk2Size >> 16) & 0xff;
  byte4 = (Subchunk2Size >> 24) & 0xff;
  frec.write(byte1);  frec.write(byte2);  frec.write(byte3);  frec.write(byte4);
  frec.close();
  //  Serial.println("header written");
  //  Serial.print("Subchunk2: ");
  //  Serial.println(Subchunk2Size);
}

void stopRecording()
{
  Serial.println("recording stop");
  recording = false;

  queue1.end();
  while (queue1.available() > 0) {
    frec.write((byte*)queue1.readBuffer(), 256);
    recByteSaved += 256;
    queue1.freeBuffer();
  }
  writeOutHeader();
  frec.close(); 
}

void continueRecording()
{
  if (queue1.available() >= 2) 
  {
    byte buffer[512];
    // Fetch 2 blocks from the audio library and copy
    // into a 512 byte buffer.  The Arduino SD library
    // is most efficient when full 512 byte sector size
    // writes are used.
    memcpy(buffer, queue1.readBuffer(), 256);
    queue1.freeBuffer();
    memcpy(buffer+256, queue1.readBuffer(), 256);
    queue1.freeBuffer();
    // write all 512 bytes to the SD card
    elapsedMicros usec = 0;
    frec.write(buffer, 512);
    recByteSaved += 512;
    // Uncomment these lines to see how long SD writes
    // are taking.  A pair of audio blocks arrives every
    // 5802 microseconds, so hopefully most of the writes
    // take well under 5802 us.  Some will take more, as
    // the SD library also must write to the FAT tables
    // and the SD card controller manages media erase and
    // wear leveling.  The queue1 object can buffer
    // approximately 301700 us of audio, to allow time
    // for occasional high SD card latency, as long as
    // the average write time is under 5802 us.
    //Serial.print("SD write, us=");
    //Serial.println(usec);
  }
}

void stopAllPlayingAudio()
{
  if (playSdWav1.isPlaying()) playSdWav1.stop();
  if (playSdWav2.isPlaying()) playSdWav2.stop();
  if (playSdWav3.isPlaying()) playSdWav3.stop();
  if (playSdWav4.isPlaying()) playSdWav4.stop();
}

void executePresetC()
{
  static bool recordingMode = false;
  static char path[] = PAD_SAMPLE_PATH;
  static Vec2 recordingCoord = {0, 0};
  const Color recordingColor = {150, 0, 0};
  
  if (DB.events.bt.arcade == Events::Down)
  {
    if (recording)
      stopRecording();

    memset(&DB.leds, 0, sizeof(DB.leds));
    DB.events.bt.arcade = Events::None;

    recordingMode = !recordingMode;
    
    Serial.print("recordingMode: ");
    Serial.println(recordingMode);
  }

  if (recordingMode)
  {
    DB.leds.switches[0] = 2.0 + 2*cos(millis()*0.003);
    
    if (!recording)
    {
      for (byte y = 0; y<PAD_WIDTH; ++y)
      {
        for (byte x = 0; x<PAD_WIDTH; ++x)
        {
          if (DB.events.bt.pad[y][x] == Events::Down && !recording)
          {
            recordingCoord = {x, y};
            updateSamplePath(path, DB.inputs.preset, x + y*PAD_WIDTH);

            stopAllPlayingAudio();
            startRecording(path);
            DB.leds.pad[y][x] = {255, 0, 0};
            continue;
          }
        }
      }
    }
  
    if (DB.events.bt.pad[recordingCoord.y][recordingCoord.x] == Events::Up)
    {
      stopRecording();
      DB.leds.pad[recordingCoord.y][recordingCoord.x] = {0, 0, 0};
      
      delay(100);
      wavePlayer[0]->play(path);
    }
  
    if (recording)
      continueRecording();
  }
  else
  { 
    executePresetA();
  }
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
    executePresetC,
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
  DB.mode = Preset; // Skip the intro

  // get initial inputs
  updateInputs(true);
}



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

  static float mainColorFloat = 0;
  mainColorFloat += 0.00015f * DB.inputs.slider/1024.f;
  mainColorFloat = fmod(mainColorFloat, 1.0f);

  Color mainColor = getColorFromFloat(mainColorFloat);
  Color compColor = getColorFromFloat(fmod(mainColorFloat + 0.5f, 1.0f));

  Color colorBackground = 0.2 * compColor;
  Color colorRmsBackground = min(1.0, 0.2 + 1.5*DB.audio.rmsFx) * compColor;
  Color colorPattern = 0.5 * compColor;
    
  unsigned long currentTime = millis();
  static AnimPattern_Cross pattern[PAD_WIDTH][PAD_WIDTH];

  // mask update
  static bool bgMask[PAD_WIDTH][PAD_WIDTH] = {{1, 1, 1, 1},{1, 1, 1, 1},{1, 1, 1, 1},{1, 1, 1, 1}};
  
  static unsigned long lastMaskUpdate = 0;
  if (millis() - lastMaskUpdate > 50)
  {
    lastMaskUpdate = millis();
    
    const int x = random(0, 4);
    const int y = random(0, 4);
    bgMask[y][x] = !bgMask[y][x];
  }
  

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

      const unsigned long joystickTime = millis() - padJoystickTime[y][x];
      const unsigned long joystickTrailDuration = 500; // ms
 
      if (DB.inputs.bt.pad[y][x]) // bt pressed
      {
         DB.leds.pad[y][x] = mainColor; 
      }
      else if(joystickTime < joystickTrailDuration) // joystick trail
      {
        float trailColorFloat = fmod(mainColorFloat + (joystickTime/(float)joystickTrailDuration)*0.5f, 1.0f);
        DB.leds.pad[y][x] = getColorFromFloat(trailColorFloat);
      }
      else
      {  
        // get local pattern value
        bool patternValue = false;
        for (byte py = 0; py<PAD_WIDTH; ++py)
          for (byte px = 0; px<PAD_WIDTH; ++px)
            patternValue |= pattern[py][px].isActive(currentTime, x, y);
  
        Color localColorBackground;
        localColorBackground = colorBackground;
        
        DB.leds.pad[y][x] = patternValue ? colorPattern : ((fxPlayer.isPlaying() && bgMask[y][x]) ? colorRmsBackground : colorBackground);
      }
      
    }
  }

  //lastPixelRefresh = millis();
}

/************** MAIN LOOP *******************/

void loop() 
{
  memset(&DB.events, 0, sizeof(DB.events)); // clear events from previous iteration
  updateInputs(); 
  //printInputs();
  executeLogic();
  updateLeds();
  updateAudio();
}

