///////////////////////////////////////////////////////////////////////////////
//                       ECE445 SENIOR DESIGN - UIUC                     
//                         GROUP 1 - SKI SPEDOMETER                      
//                CHEIF ENGINEERS: CHRIS GERTH & BRIAN VAUGHN           
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// FILE: upper_unit_accel.ino
// MAJOR REVISION: 0
// MINOR REVISION: See repository version
// DESCRIPTION: contains functions for communication with the accelerometer.
///////////////////////////////////////////////////////////////////////////////

//Accelerometer constants
#define ACCEL_GSCALE 8 //2, 4, or 8 to represent the range of the accelerometer output
#define ACCEL_I2C_ADDR 0x1D //I2C Address for accelerometer
#define ACCEL_OUT_X_MSB 0x01 //register locations
#define ACCEL_XYZ_DATA_CFG  0x0E
#define ACCEL_WHO_AM_I   0x0D
#define ACCEL_CTRL_REG1  0x2A



///////////////////////////////////////////////////////////////////////////////
// FUNCTION: accel_init()
// DESCRIPTION: initalizes the accelerometer
// ARGUMENTS: Null
// OUTPUTS: Null
// RETURN VAL: Null
// CODE SOURCE: Based on example code from https://www.sparkfun.com/products/10955
///////////////////////////////////////////////////////////////////////////////
void accel_init()
{
  byte c;
  I2C_readRegisters(ACCEL_I2C_ADDR, ACCEL_WHO_AM_I, 1, &c);  // Read WHO_AM_I register
  if (c == 0x2A) // WHO_AM_I should always be 0x2A
  {  
    #ifdef DEBUG_PRINT_ACCEL
    Serial.println("Accelerometer is online, initalizing...");
    #endif
  }
  else
  {
    #ifdef DEBUG_PRINT_ACCEL
    Serial.print("Error, no response from Accelerometer");
    #endif
    return;
  }
  
  //Put unit into standby mode
  I2C_readRegisters(ACCEL_I2C_ADDR, ACCEL_CTRL_REG1, 1, &c);
  I2C_writeRegister(ACCEL_I2C_ADDR, ACCEL_CTRL_REG1, (c & ~(0x01))); 

  // Set up the full scale range to 2, 4, or 8g.
  byte fsr = ACCEL_GSCALE;
  fsr >>= 2; // Neat trick, see page 22 of datasheet. 00 = 2G, 01 = 4A, 10 = 8G
  I2C_writeRegister(ACCEL_I2C_ADDR, ACCEL_XYZ_DATA_CFG, fsr);

  //The default data rate is 800Hz and we don't modify it in this code
  
  //Set the accelerometer back to active mode
  I2C_readRegisters(ACCEL_I2C_ADDR, ACCEL_CTRL_REG1, 1, &c);
  I2C_writeRegister(ACCEL_I2C_ADDR, ACCEL_CTRL_REG1, (c | 0x01)); 
  global_accel_sensor_status = 0;
  
  #ifdef DEBUG_PRINT_ACCEL
  Serial.println("Accelerometer initalization complete.");
  #endif
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION: accel_readData()
// DESCRIPTION: pulls in X/Y/Z acceleration data from accelerometer
// ARGUMENTS: Null
// OUTPUTS: writes to global values for accelerometer x/y/z data
// RETURN VAL: Null
// CODE SOURCE: Based on example code from https://www.sparkfun.com/products/10955
///////////////////////////////////////////////////////////////////////////////
void accel_readData()
{
  byte rawData[6];  // x/y/z accel register data stored here

  I2C_readRegisters(ACCEL_I2C_ADDR, ACCEL_OUT_X_MSB, 6, rawData);  // Read the six raw data registers into data array

  // Loop to calculate 12-bit ADC and g value for each axis
  for(int i = 0; i < 3 ; i++)
  {
    int gCount = (rawData[i*2] << 8) | rawData[(i*2)+1];  //Combine the two 8 bit registers into one 12-bit number
    gCount >>= 4; //The registers are left align, here we right align the 12-bit integer

    // If the number is negative, we have to make it so manually (no 12-bit data type)
    if (rawData[i*2] > 0x7F)
    {  
      gCount = ~gCount + 1;
      gCount *= -1;  // Transform into negative 2's complement #
    }
    switch(i)
    {
      case 0:
        global_accel_x = (float) gCount/((1<<12)/(2*ACCEL_GSCALE)); //Record this gCount into the global data structure
        break;
      case 1:
        global_accel_y = (float) gCount/((1<<12)/(2*ACCEL_GSCALE)); //scale to real-world value
        break;
      case 2:
        global_accel_z = (float) gCount/((1<<12)/(2*ACCEL_GSCALE));
        break;
    }
  }
  
  #ifdef DEBUG_PRINT_ACCEL
  Serial.print("Accel X Y Z = ");
  Serial.print(global_accel_x,4);
  Serial.print(", ");
  Serial.print(global_accel_y,4);
  Serial.print(", ");
  Serial.println(global_accel_z,4);
  #endif
}



