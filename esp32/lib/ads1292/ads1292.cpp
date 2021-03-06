//////////////////////////////////////////////////////////////////////////////////////////
//
//   Arduino Library for ADS1292R Shield/Breakout
//
//   Copyright (c) 2017 ProtoCentral
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
//
/////////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include <ads1292.h>
#include <SPI.h>

char* ads1292::ads1292_Read_Data()
{
  static char SPI_Dummy_Buff[10];
   
  digitalWrite(ADS1292_CS_PIN, LOW);
	for (int i = 0; i < 9; ++i)
	{
		SPI_Dummy_Buff[i] = SPI.transfer(CONFIG_SPI_MASTER_DUMMY);
	}
  digitalWrite(ADS1292_CS_PIN, HIGH);
	
	return SPI_Dummy_Buff;
}

void ads1292::spi_Init(uint8_t sck, uint8_t miso, uint8_t mosi, uint8_t ss)
{
  // start the SPI library:
  SPI.begin(sck, miso, mosi, ss);
  SPI.setBitOrder(MSBFIRST); 
  //CPOL = 0, CPHA = 1
  SPI.setDataMode(SPI_MODE1);
  // Selecting 8Mhz clock for SPI
  SPI.setClockDivider(SPI_CLOCK_DIV2);
}

void ads1292::ads1292_Init(volatile bool *is_init)
{
  ads1292_Reset();
  delay(1000);
  ads1292_Disable_Start();
  ads1292_Enable_Start();
  
  ads1292_Hard_Stop();
  ads1292_Start_Data_Conv_Command();
  ads1292_Soft_Stop();
  delay(50);
  ads1292_Stop_Read_Data_Continuous();	// SDATAC command
  delay(300);

  ads1292_Reg_Write(ADS1292_REG_CONFIG1, 0b00000001); //Set sampling rate to 250 SPS
  delay(10);
  ads1292_Reg_Write(ADS1292_REG_CONFIG2, 0b10110000);	//Lead-off comp on, test signal disabled, 4.033-Vref (for 5V analog supply)
  delay(10);
  ads1292_Reg_Write(ADS1292_REG_CH1SET, 0b01100000);	//Ch 1 enabled, gain 12, 
  delay(10);
  ads1292_Reg_Write(ADS1292_REG_CH2SET, 0b01100000);	//Ch 2 enabled, gain 12, 
  delay(10);
  ads1292_Reg_Write(ADS1292_REG_LOFF, 0b00010000);		//Lead-off defaults
  delay(10);
  ads1292_Reg_Write(ADS1292_REG_RLDSENS, 0b00101100);	//RLD settings: fmod/16, RLD enabled, RLD inputs from Ch2 only
  delay(10);
  ads1292_Reg_Write(ADS1292_REG_LOFFSENS, 0x0f);		//LOFF settings: all disabled
  delay(10);
  ads1292_Reg_Write(ADS1292_REG_RESP1, 0b00000010);		//Respiration: MOD/DEMOD turned only, phase 0
  delay(10);
  ads1292_Reg_Write(ADS1292_REG_RESP2, 0b00000011);		//Respiration: Calib OFF, respiration freq defaults
  delay(10);
  *is_init = true;
  ads1292_Start_Read_Data_Continuous(); // RDATAC command
  delay(10);
  ads1292_Enable_Start();
}

void ads1292::ads1292_Test_Mode()
{
  ads1292_Hard_Stop();
  ads1292_Soft_Stop();
  delay(50);
  ads1292_Stop_Read_Data_Continuous(); // SDATAC command
  delay(300);
  ads1292_Reg_Write(ADS1292_REG_CONFIG2, 0b10110011);	//Lead-off comp off, test signal enabled
  delay(10);
  ads1292_Reg_Write(ADS1292_REG_CH1SET, 0b00000101);	//Ch 1 enabled, gain 6, input shorten for noise measurement
  delay(10);
  ads1292_Reg_Write(ADS1292_REG_CH2SET, 0b10000001);	//Ch 2 enabled, gain 6, input shorten for noise measurement
  delay(10);
  ads1292_Start_Read_Data_Continuous(); // RDATAC command
  delay(10);
  ads1292_Enable_Start();
}

void ads1292::ads1292_Reset()
{
  ads1292_SPI_Command_Data(RESET);
}

void ads1292::ads1292_Disable_Start()
{
  digitalWrite(ADS1292_START_PIN, LOW);
  delay(20);
}

void ads1292::ads1292_Enable_Start()
{
  digitalWrite(ADS1292_START_PIN, HIGH);
  delay(20);
}

void ads1292::ads1292_Hard_Stop (void)
{
  digitalWrite(ADS1292_START_PIN, LOW);
  delay(100);
}


void ads1292::ads1292_Start_Data_Conv_Command (void)
{
  ads1292_SPI_Command_Data(START);					// Send 0x08 to the ADS1x9x
}

void ads1292::ads1292_Soft_Stop (void)
{
  ads1292_SPI_Command_Data(STOP);                   // Send 0x0A to the ADS1x9x
}

void ads1292::ads1292_Start_Read_Data_Continuous (void)
{
  ads1292_SPI_Command_Data(RDATAC);					// Send 0x10 to the ADS1x9x
}

void ads1292::ads1292_Stop_Read_Data_Continuous (void)
{
  ads1292_SPI_Command_Data(SDATAC);					// Send 0x11 to the ADS1x9x
}

void ads1292::ads1292_SPI_Command_Data(unsigned char data_in)
{
  digitalWrite(ADS1292_CS_PIN, HIGH);
  digitalWrite(ADS1292_CS_PIN, LOW);
  delayMicroseconds(10);
  SPI.transfer(data_in);
  delayMicroseconds(10);
  digitalWrite(ADS1292_CS_PIN, HIGH);
}

//Sends a write command to SCP1000
void ads1292::ads1292_Reg_Write (unsigned char READ_WRITE_ADDRESS, unsigned char DATA)
{
  switch (READ_WRITE_ADDRESS)
  {
    case 1:
      DATA = DATA & 0x87;
	    break;
    case 2:
      DATA = DATA & 0xFB;
	    DATA |= 0x80;		
	    break;
    case 3:
	    DATA = DATA & 0xFD;
	    DATA |= 0x10;
	    break;
    case 7:
	    DATA = DATA & 0x3F;
	    break;
    case 8:
    	DATA = DATA & 0x5F;
	    break;
    case 9:
	    DATA |= 0x02;
	    break;
    case 10:
	    DATA = DATA & 0x87;
	    DATA |= 0x01;
	    break;
    case 11:
	    DATA = DATA & 0x0F;
	    break;
    default:
	    break;		
  }
  
  // now combine the register address and the command into one byte:
  byte dataToSend = READ_WRITE_ADDRESS | WREG;
  // take the chip select low to select the device:
  digitalWrite(ADS1292_CS_PIN, LOW);
  delayMicroseconds(10);
  SPI.transfer(dataToSend); //Send register location
  SPI.transfer(0x00);		//number of register to wr
  SPI.transfer(DATA);		//Send value to record into register
  delayMicroseconds(10);
  // take the chip select high to de-select:
  digitalWrite(ADS1292_CS_PIN, HIGH);
}

byte ads1292::ads1292_Reg_Read (unsigned char READ_WRITE_ADDRESS)
{
  byte temp;
  digitalWrite(ADS1292_CS_PIN, LOW);
  SPI.transfer(READ_WRITE_ADDRESS | RREG); //Send register location
  SPI.transfer(0x03);		//number of register to read
  temp = SPI.transfer(0xff);
  digitalWrite(ADS1292_CS_PIN, HIGH);
  return temp;
}