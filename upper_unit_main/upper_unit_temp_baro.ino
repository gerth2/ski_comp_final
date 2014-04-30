///////////////////////////////////////////////////////////////////////////////
//                       ECE445 SENIOR DESIGN - UIUC                     
//                         GROUP 1 - SKI SPEDOMETER                      
//                CHEIF ENGINEERS: CHRIS GERTH & BRIAN VAUGHN           
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// FILE: upper_unit_temp_baro.ino
// MAJOR REVISION: 0
// MINOR REVISION: See repository version
// DESCRIPTION: contains functions for communication with the temp & altitude
//              sensor chip.
///////////////////////////////////////////////////////////////////////////////


//Temp/Baro Sensor constants
#define TEMP_BARO_I2C_ADDRESS 0x60 // 7-bit I2C address
#define TEMP_BARO_OS_RATE    4 //oversample rate - see datasheet for acceptable values (0-7)
#define TEMP_BARO_STATUS     0x00 //register addresses
#define TEMP_BARO_OUT_P_MSB  0x01
#define TEMP_BARO_WHO_AM_I   0x0C
#define TEMP_BARO_CTRL_REG1  0x26
#define TEMP_BARO_PT_DATA_CFG_REG 0x13
#define TEMP_BARO_SEA_LEVEL_PRESS_MSB 0x14
#define TEMP_BARO_SEA_LEVEL_PRESS_LSB 0x15




///////////////////////////////////////////////////////////////////////////////
// FUNCTION: tempBaro_init()
// DESCRIPTION: initalizes the Temperature/Barometer sensor
// ARGUMENTS: Null
// OUTPUTS: updates global_baro_temp_sensor_status
// RETURN VAL: Null
// CODE SOURCE: Based on example code from https://www.sparkfun.com/products/11084
///////////////////////////////////////////////////////////////////////////////
void tempBaro_init()
{
  byte tempSetting; //working variable, temporary
  
  //verify that we can actually talk to the temp/baro sensor
  I2C_readRegisters(TEMP_BARO_I2C_ADDRESS, TEMP_BARO_WHO_AM_I, 1, &tempSetting);
  if(tempSetting == 196)
  { 
    #ifdef DEBUG_PRINT_TEMP_BARO
    Serial.println("Temp/Baro Sensor online, initalizing...");
    #endif
  }
  else
  {
    #ifdef DEBUG_PRINT_TEMP_BARO
    Serial.println("Error, No response from Temp/Baro sensor.");
    #endif
    return; //don't do anything else if we can't find the sensor
  }
    
  //set up sensor to work as altimiter  
  I2C_readRegisters(TEMP_BARO_I2C_ADDRESS, TEMP_BARO_CTRL_REG1, 1, &tempSetting); //Read current settings
  tempSetting |= (1<<7); //Set ALT bit
  I2C_writeRegister(TEMP_BARO_I2C_ADDRESS, TEMP_BARO_CTRL_REG1, tempSetting);
  
  //set up sensor oversample rate
  I2C_readRegisters(TEMP_BARO_I2C_ADDRESS, TEMP_BARO_CTRL_REG1, 1, &tempSetting); //Read current settings
  tempSetting &= 0b11000111; //Clear out old OS bits
  tempSetting |= (TEMP_BARO_OS_RATE<<3); //Mask in new OS bits, align for CTRL_REG1 register
  I2C_writeRegister(TEMP_BARO_I2C_ADDRESS, TEMP_BARO_CTRL_REG1, tempSetting);
  
  //Set the event flags to be set in the status register whenever new data is available
  I2C_writeRegister(TEMP_BARO_I2C_ADDRESS, TEMP_BARO_PT_DATA_CFG_REG, 0x07);
  
  //mark initalization as completed
  global_baro_temp_sensor_status = 0;
  
  #ifdef DEBUG_PRINT_TEMP_BARO
  Serial.println("Temp/Baro sensor initalization complete.");
  #endif
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION: tempBaro_set_sea_level_press()
// DESCRIPTION: initalizes the Temperature/Barometer sensor
// ARGUMENTS: Null
// OUTPUTS: writes global_sea_level_press_Pa to Temp/Baro sensor for proper altitude measurement
// RETURN VAL: Null
// CODE SOURCE: Based on example code from https://www.sparkfun.com/products/11084
///////////////////////////////////////////////////////////////////////////////
void tempBaro_set_sea_level_press()
{
  byte lsb_to_write;
  byte msb_to_write;
  
  //need to write pressure value divided by two
  lsb_to_write = (byte)((global_sea_level_press_Pa >> 1) & 0x000000FF);
  msb_to_write = (byte)(((global_sea_level_press_Pa >> 1) & 0x0000FF00) >> 8);
  I2C_writeRegister(TEMP_BARO_I2C_ADDRESS, TEMP_BARO_SEA_LEVEL_PRESS_MSB, msb_to_write);
  I2C_writeRegister(TEMP_BARO_I2C_ADDRESS, TEMP_BARO_SEA_LEVEL_PRESS_LSB, lsb_to_write);
  
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION: tempBaro_readData()
// DESCRIPTION: reads temp and altitude from sensor
// ARGUMENTS: Null
// OUTPUTS: updates temp and barometric pressure global data variables
// RETURN VAL: Null
// CODE SOURCE: Based on example code from https://www.sparkfun.com/products/11084
///////////////////////////////////////////////////////////////////////////////
void tempBaro_readData()
{
  byte tempReadBuf[5]; //temporary I2C reading buffer
  //0 - Pressure MSB
  //1 - Pressure CSB
  //2 - Pressure LSB
  //3 - Temp MSB
  //4 - Temp LSB
  
  //trigger a reading by setting the OST bit
  I2C_readRegisters(TEMP_BARO_I2C_ADDRESS, TEMP_BARO_CTRL_REG1, 1, tempReadBuf);
  tempReadBuf[0] = tempReadBuf[0] | 0x02; //set OST
  I2C_writeRegister(TEMP_BARO_I2C_ADDRESS, TEMP_BARO_CTRL_REG1, tempReadBuf[0]);
  
  //wait for temperature and pressure data to become available
  I2C_readRegisters(TEMP_BARO_I2C_ADDRESS, TEMP_BARO_STATUS, 1, tempReadBuf);
  while( (tempReadBuf[0] & (0x06)) == 0)
  {
    I2C_readRegisters(TEMP_BARO_I2C_ADDRESS, TEMP_BARO_STATUS, 1, tempReadBuf);
  }
  
 // Read pressure and temp registers
  I2C_readRegisters(TEMP_BARO_I2C_ADDRESS, TEMP_BARO_OUT_P_MSB, 5, tempReadBuf);
  
  
  // The least significant bytes l_altitude and l_temp are 4-bit,
  // fractional values, so you must cast the calulation in (float),
  // shift the value over 4 spots to the right and divide by 16 (since 
  // there are 16 values in 4-bits). 
  float temporarycsb = (tempReadBuf[2]>>4)/16.0;
  global_altitude_m = (float)( (tempReadBuf[0] << 8) | tempReadBuf[1]) + temporarycsb; //set altitude variable based on pressure
  
  float temporarylsb = (tempReadBuf[4]>>4)/16.0; //temp, fraction of a degree. same logic as above

  global_outside_temp_C = (float)(tempReadBuf[3] + temporarylsb); //populate the temperature variables
  global_outside_temp_F = (((float)(tempReadBuf[3]+ temporarylsb))*9/5)+32;
  
  #ifdef DEBUG_PRINT_TEMP_BARO 
  Serial.print("Alt(m),Temp(C), Temp(F)=");
  Serial.print(global_altitude_m);
  Serial.print(", ");
  Serial.print(global_outside_temp_C);
  Serial.print(", ");
  Serial.println(global_outside_temp_F);
  #endif
  
  return;

  
}




