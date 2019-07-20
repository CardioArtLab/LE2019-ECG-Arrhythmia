//////////////////////////////////////////////////////////////////////////////////////////
//
//   Arduino Library for ADS1292R Shield/Breakout
//
//   Copyright (c) 2017 ProtoCentral
//   Heartrate and respiration computation based on original code from Texas Instruments
//
//   This software is licensed under the MIT License(http://opensource.org/licenses/MIT).
//
//   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
//   NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
//   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//   Requires g4p_control graphing library for processing.  Built on V4.1
//   Downloaded from Processing IDE Sketch->Import Library->Add Library->G4P Install

// If you have bought the breakout the connection with the Arduino board is as follows:
//
//|ads1292r pin label| Arduino Connection   |Pin Function      |
//|----------------- |:--------------------:|-----------------:|
//| VDD              | +5V                  |  Supply voltage  |
//| PWDN/RESET       | D4                   |  Reset           |
//| START            | D5                   |  Start Input     |
//| DRDY             | D6                   |  Data Ready Outpt|
//| CS               | D7                   |  Chip Select     |
//| MOSI             | D11                  |  Slave In        |
//| MISO             | D12                  |  Slave Out       |
//| SCK              | D13                  |  Serial Clock    |
//| GND              | Gnd                  |  Gnd             |
//
/////////////////////////////////////////////////////////////////////////////////////////

#include <ads1292.h>
#include <SPI.h>
#include <math.h>

ads1292 ADS1292;   // define class

//Packet format
#define  CES_CMDIF_PKT_START_1      0x0A
#define  CES_CMDIF_PKT_START_2      0xFA
#define  CES_CMDIF_TYPE_DATA        0x02
#define  CES_CMDIF_PKT_STOP_1       0x00
#define  CES_CMDIF_PKT_STOP_2       0x0B

#define TEMPERATURE 0
#define FILTERORDER         161
/* DC Removal Numerator Coeff*/
#define NRCOEFF (0.992)
#define WAVE_SIZE  1

//******* ecg filter *********
#define MAX_PEAK_TO_SEARCH         5
#define MAXIMA_SEARCH_WINDOW      25
#define MINIMUM_SKIP_WINDOW       30
#define SAMPLING_RATE           125
#define TWO_SEC_SAMPLES           2 * SAMPLING_RATE
#define QRS_THRESHOLD_FRACTION    0.4
#define TRUE 1
#define FALSE 0
#define maxlen 32

volatile uint8_t  SPI_Dummy_Buff[30];
uint8_t DataPacketHeader[16];
volatile signed long s32DaqVals[8];
uint8_t data_len = 7;
volatile byte SPI_RX_Buff[15] ;
volatile static int SPI_RX_Buff_Count = 0;
volatile char *SPI_RX_Buff_Ptr;
volatile bool ads1292dataReceived = false;
unsigned long uecgtemp = 0;
signed long secgtemp = 0;
int i, j;

//************** ecg *******************
int16_t CoeffBuf_40Hz_LowPass[FILTERORDER] =
{
-0.000475171042833907,  0.0000217100195346327, -0.000390041526728662, -0.000586353810416361, -0.0000136940682261114, -0.000358633251104773, -0.000743175207074032, -0.0000826614568522009, -0.000327685335586005, -0.000952707427489349 ,
-0.000206352705659719, -0.000284806935413356 , -0.00121148441529281 , -0.000412070849754362, -0.000222110734307867 , -0.00150371887858696 , -0.000729692117821011, -0.000140542951230967 ,-0.00180124002372818  ,-0.00118682346814455  ,
-5.35141615763486e-05, -0.00206536921646624  , -0.00180319282068773 ,  0.0000107835966577074,  -0.00225071839748845,  -0.00258492004693512,  8.86650834151988e-06,  -0.00231065312047003 , -0.00351938382027128 , -0.000116443420851730, 
-0.00220393932456236 , -0.00457137778805045  , -0.000432253957117893, -0.00190192068671187 ,-0.00568113506123540   ,-0.00100987782123088  ,-0.00139547828714757  ,-0.00676459808943437  ,-0.00191761115176585  ,-0.000701027154178303, 
-0.00771603475295397 , -0.00321269689755510  ,  0.000135080580279547, -0.00841276783363556 , -0.00493330559476448  ,  0.00103413816197621 ,-0.00872140687746197  ,-0.00709142876168988 , 0.00188466875264091 ,-0.00850454356181324 , 
-0.00966754364344855 , 0.00254347493675400   , -0.00762634103272975 , -0.0126077722394456 ,0.00283676995788007     , -0.00595464430361576 , -0.0158240339641981 ,  0.00255854702003380 ,-0.00335569092926242 , -0.0191974019250447, 
0.00146035816588585  , 0.000326121591435057  , -0.0225845478212937  , -0.000780582245302070 ,0.00532074948081922   , -0.0258268349148500 ,-0.00464888257029395   , 0.0120650904010712  ,-0.0287613283543875 ,-0.0110396341747405, 
0.0215768078542043   ,-0.0312327701325681    , -0.0220874361896617  ,  0.0367824483197825  ,-0.0331054379149274    , -0.0450027387812823 ,0.0697924295553749  ,   -0.0342737886731606 , -0.130458255724722  ,0.279950979829171 ,
0.632076872773548    , 0.279950979829171    ,-0.130458255724722     , -0.0342737886731606  ,  0.0697924295553749  ,-0.0450027387812823   ,  -0.0331054379149274 ,  0.0367824483197825  ,-0.0220874361896617 ,-0.0312327701325681 ,
0.0215768078542043  ,-0.0110396341747405    ,-0.0287613283543875   ,0.0120650904010712     ,-0.00464888257029395  ,-0.0258268349148500   ,0.00532074948081922   , -0.000780582245302070 ,-0.0225845478212937 ,0.000326121591435057 ,
0.00146035816588585 ,-0.0191974019250447    ,-0.00335569092926242  ,0.00255854702003380    ,-0.0158240339641981   ,-0.00595464430361576  ,0.00283676995788007   , -0.0126077722394456 ,-0.00762634103272975  ,0.00254347493675400 ,
-0.00966754364344855,  -0.00850454356181324 , 0.00188466875264091   ,-0.00709142876168988  , -0.00872140687746197 ,  0.00103413816197621 ,-0.00493330559476448  ,  -0.00841276783363556 , 0.000135080580279547 , -0.00321269689755510,
-0.00771603475295397 , -0.000701027154178303, -0.00191761115176585  ,-0.00676459808943437  , -0.00139547828714757 ,-0.00100987782123088  ,-0.00568113506123540  ,  -0.00190192068671187  , -0.000432253957117893, -0.00457137778805045 ,
-0.00220393932456236 , -0.000116443420851730, -0.00351938382027128  ,-0.00231065312047003  ,  0.00000886650834151988,  -0.00258492004693512 , -0.00225071839748845,  0.0000107835966577074,  -0.00180319282068773,  -0.00206536921646624,
-0.0000535141615763486, -0.00118682346814455,  -0.00180124002372818 , -0.000140542951230967,  -0.000729692117821011 ,-0.00150371887858696  ,-0.00022211073430786  , -0.000412070849754362 ,-0.00121148441529281 , -0.000284806935413356 ,
-0.000206352705659719 ,-0.000952707427489349, -0.000327685335586005 ,-0.0000826614568522009, -0.000743175207074032  ,-0.000358633251104773 ,-0.0000136940682261114, -0.000586353810416361, -0.000390041526728662 ,0.0000217100195346327 , -0.000475171042833907
};
int16_t ECG_WorkingBuff[2 * FILTERORDER];

int16_t ECG_WorkingBuff1[2 * FILTERORDER];
unsigned char Start_Sample_Count_Flag = 0;
unsigned char first_peak_detect = FALSE ;
unsigned int sample_index[MAX_PEAK_TO_SEARCH + 2] ;
uint16_t scaled_result_display[150];
uint8_t indx = 0;

int cnt = 0;
int ii = 0,io=0;

volatile uint8_t flag = 0;

int QRS_Second_Prev_Sample = 0 ;
int QRS_Prev_Sample = 0 ;
int QRS_Current_Sample = 0 ;
int QRS_Next_Sample = 0 ;
int QRS_Second_Next_Sample = 0 ;

static uint16_t QRS_B4_Buffer_ptr = 0 ;
/*   Variable which holds the threshold value to calculate the maxima */
int16_t QRS_Threshold_Old = 0;
int16_t QRS_Threshold_New = 0;
unsigned int sample_count = 0 ;

/* Variable which will hold the calculated heart rate */
volatile uint16_t QRS_Heart_Rate = 0 ;
int16_t ecg_wave_buff[6], ecg_filterout[6];
int outp,outp1;
int inp[maxlen]={0};
int inp1[maxlen]={0};
int inp2[maxlen]={0};
volatile uint8_t global_HeartRate = 0;
volatile uint8_t global_RespirationRate = 0;
long status_byte=0;
uint8_t LeadStatus=0;
boolean leadoff_deteted = true;
long time_elapsed=0;
int16_t aVR=0,aVL=0,aVF=0,leadI,leadII, leadIII=0;
void setup()
{
  delay(2000);
  // initalize the  data ready and chip select pins:
  pinMode(ADS1292_DRDY_PIN, INPUT);  //6
  pinMode(ADS1292_CS_PIN, OUTPUT);    //7
  pinMode(ADS1292_START_PIN, OUTPUT);  //5
  pinMode(ADS1292_PWDN_PIN, OUTPUT);  //4

  Serial.begin(2000000);  // Baudrate for serial communica

  ADS1292.ads1292_Init();  //initalize ADS1292 slave

 // Serial.println("Initiliziation is done");
}

void loop()
{
  if ((digitalRead(ADS1292_DRDY_PIN)) == LOW)      // Sampling rate is set to 125SPS ,DRDY ticks for every 8ms
  {
    SPI_RX_Buff_Ptr = ADS1292.ads1292_Read_Data(); // Read the data,point the data to a pointer

    for (i = 0; i < 9; i++)
    {
      SPI_RX_Buff[SPI_RX_Buff_Count++] = *(SPI_RX_Buff_Ptr + i);  // store the result data in array
    }
    ads1292dataReceived = true;
  }


  if (ads1292dataReceived == true)      // process the data
  {
    j = 0;
    for (i = 3; i < 9; i += 3)         // data outputs is (24 status bits + 24 bits Respiration data +  24 bits ECG data)
    {

      uecgtemp = (unsigned long) (  ((unsigned long)SPI_RX_Buff[i + 0] << 16) | ( (unsigned long) SPI_RX_Buff[i + 1] << 8) |  (unsigned long) SPI_RX_Buff[i + 2]);
      uecgtemp = (unsigned long) (uecgtemp << 8);
      secgtemp = (signed long) (uecgtemp);
      secgtemp = (signed long) (secgtemp >> 8);

      s32DaqVals[j++] = secgtemp;  //s32DaqVals[0] is Resp data and s32DaqVals[1] is ECG data
    }

    status_byte = (long)((long)SPI_RX_Buff[2] | ((long) SPI_RX_Buff[1]) <<8 | ((long) SPI_RX_Buff[0])<<16); // First 3 bytes represents the status
    status_byte  = (status_byte & 0x0f8000) >> 15;  // bit15 gives the lead status
    LeadStatus = (unsigned char ) status_byte ;  

    if(!((LeadStatus & 0x1f) == 0 ))
      leadoff_deteted  = true; 
    else
      leadoff_deteted  = false;
    
    ecg_wave_buff[0] = (int16_t)(s32DaqVals[0] >> 8) ;  // ignore the lower 8 bits out of 24bits
    ecg_wave_buff[1] = (int16_t)(s32DaqVals[1] >> 8) ;  // ignore the lower 8 bits out of 24bits
//Serial.print("DATA,TIME,"); 
// Serial.print("leadII: ");
// Serial.print(ecg_wave_buff[0]); 
// Serial.print(",");
// Serial.print("leadI: ");
if(io<maxlen)
{
inp[maxlen-1-io]=ecg_wave_buff[0];
inp1[maxlen-1-io]=abs(inp[maxlen-1-io]);
inp2[maxlen-1-io]=abs(inp1[maxlen-1-io]);
if (io>0)
{
  inp1[maxlen-1-io]=inp[maxlen-1-io]-inp[maxlen-io];
   inp1[maxlen-1-io]=abs( inp1[maxlen-1-io]);
   inp2[maxlen-1-io]=inp1[maxlen-1-io]-inp1[maxlen-io];
   inp2[maxlen-1-io]=abs( inp2[maxlen-1-io]);
}

outp+=inp1[maxlen-1-io];
outp1+=inp2[maxlen-1-io];
io++;

}
if(io>=maxlen)
{
  for( i=1;i<maxlen;i++)
  {
     outp=0;
     outp1=0;
     inp[maxlen-i]=inp[maxlen-1-i];
    inp1[maxlen-i]=inp1[maxlen-1-i];
   inp2[maxlen-i]=inp2[maxlen-1-i];
     outp+= inp1[maxlen-i];
      outp1+= inp2[maxlen-i];
  }
  inp[0]=ecg_wave_buff[1];
  
   inp1[0]= abs(inp[0]- inp[1]);
   inp2[0]= abs(inp1[0]- inp1[1]);
    outp+=inp1[0];
     outp1+=inp2[0];
}
 Serial.print(outp);
 Serial.print(",");
  Serial.print(outp1);
 Serial.print(",");
 Serial.println(outp+outp1);
   
    if(leadoff_deteted == false) 
       {
         leadI= ECG_ProcessCurrSample(&ecg_wave_buff[0], &ecg_filterout[0]);   // filter out the line noise @40Hz cutoff 161 order
        leadII=  ECG_ProcessCurrSample(&ecg_wave_buff[1], &ecg_filterout[1]); 
  leadIII=leadII-leadI;
  aVR=0-(leadI+leadII)/2;
  aVL=leadI-(leadII/2);
  aVF=leadII-(leadI/2);
if (ii==0)
        {  
//          Serial.print(leadI+60);
//          Serial.print(",");
//         Serial.println(leadII+30);
//           Serial.print(",");
//         Serial.print(leadIII+20);
//        Serial.print(",");
//            Serial.print(aVR+10);
//          Serial.print(",");
//         Serial.print(aVL-20);
//           Serial.print(",");
//        Serial.println(aVF-40);
        }ii++;
        if(ii==1){ii=0;}
////       //  Serial.println( ecg_filterout[1] );
           }
    else
       ecg_filterout[0] = 0;


    if(millis() > time_elapsed)  // update every one second
    {
      if(leadoff_deteted == true) // lead in not connected
      Serial.println("ECG lead error!!! ensure the leads are properly connected");
      else
      {
      //  Serial.print("Heart rate: ");
      // Serial.print(global_HeartRate);
      //  Serial.println("BPM");
      }
      time_elapsed += 1000;
    }
    
  }

  ads1292dataReceived = false;
  SPI_RX_Buff_Count = 0;

}





void ECG_FilterProcess(int16_t * WorkingBuff, int16_t * CoeffBuf, int16_t* FilterOut)
{

  int32_t acc = 0;   // accumulator for MACs
  int  k;

  // perform the multiply-accumulate
  for ( k = 0; k < FILTERORDER; k++ )
  {
    acc += (int32_t)(*CoeffBuf++) * (int32_t)(*WorkingBuff--);
  }
  // saturate the result
  if ( acc > 0x3fffffff )
  {
    acc = 0x3fffffff;
  } else if ( acc < -0x40000000 )
  {
    acc = -0x40000000;
  }
  
  // convert from Q30 to Q15
  *FilterOut = (int16_t)(acc >> 15);
  //*FilterOut = *WorkingBuff;
// Serial.println(*FilterOut);
}




int ECG_ProcessCurrSample(int16_t *CurrAqsSample, int16_t *FilteredOut)
{
  static uint16_t ECG_bufStart = 0, ECG_bufCur = FILTERORDER - 1, ECGFirstFlag = 1;
  static int16_t ECG_Pvev_DC_Sample, ECG_Pvev_Sample;/* Working Buffer Used for Filtering*/
  //  static short ECG_WorkingBuff[2 * FILTERORDER];
  int16_t *CoeffBuf;
  int16_t temp1, temp2, ECGData;

  /* Count variable*/
  uint16_t Cur_Chan;
  int16_t FiltOut = 0;
  //  short FilterOut[2];
  CoeffBuf = CoeffBuf_40Hz_LowPass;         // Default filter option is 40Hz LowPass

  if  ( ECGFirstFlag )                // First Time initialize static variables.
  {
    for ( Cur_Chan = 0 ; Cur_Chan < FILTERORDER; Cur_Chan++)
    {
      ECG_WorkingBuff[Cur_Chan] = 0;
    }
    ECG_Pvev_DC_Sample = 0;
    ECG_Pvev_Sample = 0;
    ECGFirstFlag = 0;
  }

temp1 = NRCOEFF * ECG_Pvev_DC_Sample;       //First order IIR
  ECG_Pvev_DC_Sample = CurrAqsSample[0];
  ECG_Pvev_Sample = CurrAqsSample[0];
  temp2 = ECG_Pvev_DC_Sample >> 2;
  ECGData = (int16_t) temp2;

  /* Store the DC removed value in Working buffer in millivolts range*/
  ECG_WorkingBuff[ECG_bufCur] = ECGData;
  ECG_FilterProcess(&ECG_WorkingBuff[ECG_bufCur], CoeffBuf, (int16_t*)&FiltOut);
  /* Store the DC removed value in ECG_WorkingBuff buffer in millivolts range*/
  ECG_WorkingBuff[ECG_bufStart] = ECGData;
//Serial.println(ECGData);
  /* Store the filtered out sample to the LeadInfo buffer*/
  FilteredOut[0] = FiltOut ;//(CurrOut);

//  ECG_bufCur++;
  ECG_bufStart++;

  if ( ECG_bufStart  == (FILTERORDER - 1))
  {
    ECG_bufStart = 0;
    ECG_bufCur = FILTERORDER - 1;
  }
  return ECGData;
}
