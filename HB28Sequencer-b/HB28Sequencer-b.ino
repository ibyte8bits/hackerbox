//Simple Block Step Sequencer Demo
//For use on JamBox (HackerBox #0028)
//  audio output on external I2S PCM5102 module
//  each "row" plays a one of eight wavetables
//  buttons set/clear use of wavetable for each time step
//  mutiple waves can be combined in each step
//  fifth knob sets cycle speed
//  slow speed to more easily set buttons
//  raise speed to hear melody
//

#include <SPI.h>
#include "LedMatrix.h"
#include "driver/i2s.h"
#include "freertos/queue.h"

//LED Matrix Pins
#define NUMBER_OF_DEVICES 4 
#define CS_PIN 15
#define CLK_PIN 14
#define MISO_PIN 2 //Not Used
#define MOSI_PIN 12

#define fs  16000  //sample rate in Hz
#define pi2 6.288319
#define samplelen 100

LedMatrix ledMatrix = LedMatrix(NUMBER_OF_DEVICES, CLK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
byte gridstate[32];
int curcol = 0;

static const i2s_port_t i2s_num = (i2s_port_t)0; // i2s port number

static const i2s_config_t i2s_config = {
     .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
     .sample_rate = fs,
     .bits_per_sample = (i2s_bits_per_sample_t) 16,
     .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,  
     .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
     .intr_alloc_flags = 0,
     .dma_buf_count = 8,
     .dma_buf_len = 64 
};

static const i2s_pin_config_t pin_config = {
    .bck_io_num = 26,
    .ws_io_num = 25,
    .data_out_num = 22,
    .data_in_num = I2S_PIN_NO_CHANGE
};

int wavetable1[samplelen];
int wavetable2[samplelen];
int wavetable3[samplelen];
int wavetable4[samplelen];
int wavetable5[samplelen];
int wavetable6[samplelen];
int wavetable7[samplelen];
int wavetable8[samplelen];
  
void setup() {
  i2s_driver_install(i2s_num, &i2s_config, 0, NULL);   //install and start i2s driver
  i2s_set_pin(i2s_num, &pin_config);
  i2s_set_sample_rates(i2s_num, fs); //set sample rates

  loadwavetable(1, 264);
  loadwavetable(2, 297);
  loadwavetable(3, 329);
  loadwavetable(4, 352);
  loadwavetable(5, 396);
  loadwavetable(6, 440);
  loadwavetable(7, 494);
  loadwavetable(8, 555);
  ledMatrix.init();
  pinMode(4, INPUT_PULLDOWN);
  pinMode(5, INPUT_PULLDOWN);
  pinMode(16, INPUT_PULLDOWN);
  pinMode(17, INPUT_PULLDOWN);
  pinMode(18, INPUT_PULLDOWN);
  pinMode(19, INPUT_PULLDOWN);
  pinMode(21, INPUT_PULLDOWN);
  pinMode(23, INPUT_PULLDOWN);

  analogReadResolution(10);
  pinMode(32, INPUT);
  pinMode(33, INPUT);
  pinMode(34, INPUT);
  pinMode(35, INPUT);
  pinMode(36, INPUT);

  for (int i=0; i<32; i++)
    gridstate[i]=0;

  Serial.begin(9600);

  introscroll();
}

void loop() 
{
  
  byte btns;
  int pots[5];
  ledMatrix.clear();

  readinputs(&btns, pots);  

//  gridstate[curcol] ^= btns; 
 
  while ( pots[1] < 512 )
  {

    readinputs(&btns, pots);
    curcol = ( float(pots[0]) / 1023.0 ) * 31.0;

    ledMatrix.setColumn(curcol, 0xff);
    ledMatrix.commit();
    delay(10);
    ledMatrix.clear();
    for ( int i = 0 ; i < 32 ; i++ )
    {
      ledMatrix.setColumn(i, gridstate[i]);
      ledMatrix.commit();
    }
    
    gridstate[curcol] ^= btns;
 
    if ( btns )
    {
      ledMatrix.setColumn(curcol, gridstate[curcol] ^ 255);
    }
    else
    {
      ledMatrix.setColumn(curcol, gridstate[curcol]);
    }
    
    Serial.print("gridstate[");
    Serial.print( curcol );
    Serial.print("] = ");
    Serial.println( gridstate[curcol], BIN );
     
    playwavevect(gridstate[curcol], 100);
    ledMatrix.commit();
    delay(200);
  }

  Serial.print("Buttons = ");
  Serial.println( btns, BIN );
  Serial.print("Pot 0 = ");
  Serial.println( pots[0] );
  Serial.print("Pot 1 = ");
  Serial.println( pots[1] );
  Serial.print("Pot 4 = ");
  Serial.println( pots[4] );
  Serial.print("Column = ");
  Serial.println( curcol );

  //read range is 0-1023
  //map to:
  //slowest delay is 800 fastest delay is 60
  int tt = ((((1023-pots[4])*(800-60))/1023)+60);

  for (int i=0; i<32; i++)
  {
    ledMatrix.setColumn(i, gridstate[i]);

    setcolmask(i, 0xff);
    ledMatrix.commit();
    delay(10);
    ledMatrix.clear();    
    setcolmask(i, gridstate[i]); 
    ledMatrix.commit();
    playwavevect(gridstate[i], tt);

  }
}

void introscroll()
{
  ledMatrix.setText("Jam");
  for (int i=0; i<28; i++)
  {
    ledMatrix.clear();
    ledMatrix.scrollTextLeft();
    ledMatrix.drawText();
    ledMatrix.commit();
    delay(100);
  }
  delay(500);
}

void readinputs(byte *buttons, int *potentiometers)
{
    *buttons = 0;
    if (digitalRead(4) == HIGH)  *buttons+=1;
    if (digitalRead(5) == HIGH)  *buttons+=2;
    if (digitalRead(16) == HIGH) *buttons+=4;
    if (digitalRead(17) == HIGH) *buttons+=8;
    if (digitalRead(18) == HIGH) *buttons+=16;
    if (digitalRead(19) == HIGH) *buttons+=32;
    if (digitalRead(21) == HIGH) *buttons+=64;
    if (digitalRead(23) == HIGH) *buttons+=128;
    
    potentiometers[0] = analogRead(32); 
    potentiometers[1] = analogRead(33); 
    potentiometers[2] = analogRead(34);
    potentiometers[3] = analogRead(35);
    potentiometers[4] = analogRead(36); 
}

//sets an LED column according to bitmask
void setcolmask(int col, char bits)
{
    for (int i=0; i<8; i++)
    {
       if (bits & (1<<i))
         ledMatrix.setPixel(col,i);
    }
}

//sets an LED column from the bottom up with value 0-8
void setcolbotval(int col, char val)
{
    for (int i=0; i<8; i++)
    {
       int cmp = i+1;
       if (val>=cmp)
         ledMatrix.setPixel(col,(7-i));
    }
}

//load fixed tone into a wave table
//f is tone (apx in Hz)
void loadwavetable(int channel, int f)  
{
   int *curtable;
   if (channel==1)
     curtable = wavetable1;
   if (channel==2)
     curtable = wavetable2;
   if (channel==3)
     curtable = wavetable3;
   if (channel==4)
     curtable = wavetable4;
   if (channel==5)
     curtable = wavetable5;
   if (channel==6)
     curtable = wavetable6;
   if (channel==7)
     curtable = wavetable7;
   if (channel==8)
     curtable = wavetable8;
   float dt =  pi2 * f / (fs);
   for (int c=0; c<samplelen; c++)
   {
     float sample; 
     sample = sin( dt * c );
     sample = sample * 60;
     sample += 128; 
     int s = (int) sample;
     curtable[c] = (s<<12);
   }
}

//play one wave (channel #) for apx t (ms) 
void playwavetime(int channel, int t)
{
  int *curtable;
  if (channel==1)
    curtable = wavetable1;
  if (channel==2)
    curtable = wavetable2;
  if (channel==3)
    curtable = wavetable3;
  if (channel==4)
    curtable = wavetable4;
  if (channel==5)
    curtable = wavetable5;
  if (channel==6)
    curtable = wavetable6;
  if (channel==7)
    curtable = wavetable7;
  if (channel==8)
    curtable = wavetable8;
  long msperchan = 1000 * samplelen / (2*fs);
  long count = 2 * t / msperchan;
  for (int i=0; i<count; i++)
    i2s_write_bytes((i2s_port_t)i2s_num, (char *) curtable, samplelen, 100);
}

//play combined waves in bitvector for apx t (ms) 
void playwavevect(char vect, int t)
{
  int combinedwave[samplelen];
  int weight=0;
  for (int i=0; i<8; i++)
  {
    if ( vect & (1<<i) )
      weight++;
  }
  for (int i=0; i<samplelen; i++)
  {
    combinedwave[i] = 0;
    if (vect & 1)
      combinedwave[i] += wavetable1[i];
    if (vect & 2)
      combinedwave[i] += wavetable2[i];
    if (vect & 4)
      combinedwave[i] += wavetable3[i];
    if (vect & 8)
      combinedwave[i] += wavetable4[i];    
    if (vect & 16)
      combinedwave[i] += wavetable5[i];
    if (vect & 32)
      combinedwave[i] += wavetable6[i];
    if (vect & 64)
      combinedwave[i] += wavetable7[i];
    if (vect & 128)
      combinedwave[i] += wavetable8[i];      
    
    if (weight)
    {
      combinedwave[i] /= weight;
    }
  }
 
  long msperchan = 1000 * samplelen / (2*fs);
  long count = 2 * t / msperchan;
  for (int i=0; i<count; i++)
    i2s_write_bytes((i2s_port_t)i2s_num, (char *) combinedwave, samplelen, 100);
}
