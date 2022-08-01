/*!
 *  @file LC709203F.cpp
 *
 *  @mainpage LC709203F Battery Monitor code
 *
 *  @section intro_sec Introduction
 *
 * 	I2C Driver for the LC709203F Battery Monitor IC
 *
 *  @section author Author
 *
 *  Daniel deBeer (EzSBC)
 *
 * 	@section license License
 *
 * 	BSD (see license.txt)
 *
 * 	@section  HISTORY
 *
 *     v1.0 - First release
 */

#include "Arduino.h"
#include <Wire.h>
#include "LC709203F.h"

/*!
 *    @brief  Instantiates a new LC709203F class
 */
LC709203F::LC709203F(void) {}

LC709203F::~LC709203F(void) {}

uint8_t i2c_address = LC709203F_I2C_ADDR ;

/*!
 *    @brief  Sets up the hardware and initializes I2C
 *    @param  
 *    @return True if initialization was successful, otherwise false.
 */
bool LC709203F::begin( void ) 
{
  Wire.begin();
  setPowerMode(LC709203F_POWER_OPERATE) ;
  setCellCapacity(LC709203F_APA_500MAH) ;
  setTemperatureMode(LC709203F_TEMPERATURE_THERMISTOR) ;
  
  return true;
}


/*!
 *    @brief  Get IC version
 *    @return 16-bit value read from LC709203F_RO_ICVERSION registers
 */
uint16_t LC709203F::getICversion(void) 
{
  uint16_t vers = 0;
  vers = read16(LC709203F_RO_ICVERSION);
  return vers;
}


/*!
 *    @brief  Initialize the RSOC algorithm
 *    @return
 */
void LC709203F::initRSOC(void) 
{
  write16(LC709203F_WO_INITRSOC, 0xAA55);
}


/*!
 *    @brief  Get battery voltage
 *    @return Cell voltage in milliVolt
 */
uint16_t LC709203F::cellVoltage_mV(void) 
{
  uint16_t mV = 0;
  mV = read16(LC709203F_RO_CELLVOLTAGE);
  return 1000 * ( mV / 1000.0 ) ;
}


/*!
 *    @brief  Get cell remaining charge in percent (0-100%)
 *    @return point value from 0 to 1000
 */
uint16_t LC709203F::cellRemainingPercent10(void) 
{
  uint16_t percent = 0;
  percent = read16(LC709203F_RO_ITE );
  return percent ;
}

/*!
 *    @brief  Get battery state of charge in percent (0-100%)
 *    @return point value from 0 to 100
 */
uint16_t LC709203F::cellStateOfCharge(void)
{
  uint16_t percent = 0;
  percent = read16(LC709203F_RW_RSOC );
  return percent ;
}


/*!
 *    @brief  Get battery thermistor temperature
 *    @return value from -20 to 60 *C  // CdB Needs testing, no thermistor on ESP32_Bat_R2 board
 */
uint16_t LC709203F::getCellTemperature(void) 
{
  uint16_t temp = 0;
  temp = read16(LC709203F_RW_CELLTEMPERATURE );
  return temp ;
}


/*!
 *    @brief  Set the temperature mode (external or internal)
 *    @param t The desired mode: LC709203F_TEMPERATURE_I2C or
 * LC709203F_TEMPERATURE_THERMISTOR
 */
void LC709203F::setTemperatureMode(lc709203_tempmode_t t) 
{
  return write16(LC709203F_RW_STATUSBIT, (uint16_t)t);
}


/*!
 *    @brief  Set the cell capacity, 
 *    @param apa The lc709203_adjustment_t enumerated approximate cell capacity
 */
void LC709203F::setCellCapacity(lc709203_adjustment_t apa) 
{
  write16(LC709203F_RW_APA, (uint16_t)apa);
}


/*!
 *    @brief  Set the alarm pin to respond to an RSOC percentage level
 *    @param percent The threshold value, set to 0 to disable alarm
 */
void LC709203F::setAlarmRSOC(uint8_t percent) 
{
  write16(LC709203F_RW_ALARMRSOC, percent);
}


/*!
 *    @brief  Set the alarm pin to respond to a battery voltage level
 *    @param voltage The threshold value, set to 0 to disable alarm
 */
void LC709203F::setAlarmVoltage(float voltage) 
{
  write16(LC709203F_RW_ALARMVOLT, voltage * 1000);
}


/*!
 *    @brief  Set the power mode, LC709203F_POWER_OPERATE or
 *            LC709203F_POWER_SLEEP
 *    @param t The power mode desired
 *    @return 
 */
void LC709203F::setPowerMode(lc709203_powermode_t t) 
{
  write16(LC709203F_RW_POWERMODE, (uint16_t)t);
}

/*!
 *    @brief  Set cell type 
 *    @param t The profile, Table 8.  Normally 1 for 3.7 nominal 4.2V Full carge cells
 *    @return
 */
void LC709203F::setCellProfile(lc709203_cell_profile_t t) 
{
  write16(LC709203F_RW_PROFILE, (uint16_t)t);
}

/*!
 *    @brief  Get the thermistor Beta value //For completeness since we have to write it.
 *    @return The uint16_t Beta value
 */
uint16_t LC709203F::getThermistorBeta(void) 
{
  uint16_t val = 0;
  val = read16(LC709203F_RW_THERMISTORB);
  return val;
}


/*!
 *    @brief  Set the thermistor Beta value
 *    @param b The value to set it to
 *    @return 
 */
void LC709203F::setThermistorB(uint16_t beta) 
{
  write16(LC709203F_RW_THERMISTORB, beta);
}


//
// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
/*
    INTERNAL I2C FUNCTIONS and CRC CALCULATION
*/


/**
 * Performs a CRC8 calculation on the supplied values.
 *
 * @param data  Pointer to the data to use when calculating the CRC8.
 * @param len   The number of bytes in 'data'.
 *
 * @return The computed CRC8 value.
 */
static uint8_t crc8(uint8_t *data, int len) 
{
  const uint8_t POLYNOMIAL(0x07);
  uint8_t crc(0x00);

  for (int j = len; j; --j) {
    crc ^= *data++;

    for (int i = 8; i; --i) {
      crc = (crc & 0x80) ? (crc << 1) ^ POLYNOMIAL : (crc << 1);
    }
  }
  return crc;
}


// writes a 16-bit word (d) to register pointer regAddress
// when selecting a register pointer to read from, data = 0
void LC709203F::write16(uint8_t regAddress, uint16_t data)
{
  // Setup array to hold bytes to send including CRC-8
  uint8_t crcArray[5];
  crcArray[0] = 0x16;
  crcArray[1] = regAddress;
  crcArray[2] = lowByte(data);
  crcArray[3] = highByte(data);
  // Calculate crc of preceding four bytes and place in crcArray[4]
  crcArray[4] = crc8( crcArray, 4 );
  // Device address
  Wire.beginTransmission(i2c_address);
  // Register address
  Wire.write(regAddress);
  // low byte
  Wire.write(crcArray[2]);
  // high byte
  Wire.write(crcArray[3]);
  // Send crc8 
  Wire.write(crcArray[4]);
  Wire.endTransmission();
}

int16_t LC709203F::read16( uint8_t regAddress)
{
  int16_t data = 0;
  Wire.beginTransmission(i2c_address);
  Wire.write(regAddress);
  Wire.endTransmission(false);
  Wire.requestFrom(i2c_address, 2);
  uint8_t lowByteData = Wire.read();
  uint8_t highByteData = Wire.read();
  data = word(highByteData, lowByteData);
  return( data );
}
