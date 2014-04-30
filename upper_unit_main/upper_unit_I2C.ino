///////////////////////////////////////////////////////////////////////////////
//                       ECE445 SENIOR DESIGN - UIUC                     
//                         GROUP 1 - SKI SPEDOMETER                      
//                CHEIF ENGINEERS: CHRIS GERTH & BRIAN VAUGHN           
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// FILE: upper_unit_I2C.ino
// MAJOR REVISION: 0
// MINOR REVISION: See repository version
// DESCRIPTION: wrapper functions for Wire library for uniform accessing scheme
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// FUNCTION: I2C_readRegisters()
// DESCRIPTION: reads multiple bytes from an I2C device
// ARGUMENTS: byte I2C_addr - I2C address of device to read from
//            byte addressToRead - memory location to start read from
//            unsigned int bytesToRead - number of bytes to read from device 
//            byte* dest - buffer to place the read bytes into
// OUTPUTS: puts bytes into buffer dest.
// NOTE: blocks until I2C data has been returned.
// RETURN VAL: Null
///////////////////////////////////////////////////////////////////////////////
void I2C_readRegisters(byte I2C_addr, byte addressToRead, unsigned int bytesToRead, byte * dest)
{
  Wire.beginTransmission(I2C_addr);
  Wire.write(addressToRead);
  Wire.endTransmission(false);

  Wire.requestFrom((int)I2C_addr, bytesToRead); //Ask for bytes, once done, bus is released by default

  while(Wire.available() < bytesToRead); //Hang out until we get the # of bytes we expect

  for(int x = 0 ; x < bytesToRead ; x++)
    dest[x] = Wire.read();    
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION: I2C_writeRegister()
// DESCRIPTION: writes a byte to one device address
// ARGUMENTS: byte I2C_addr - I2C address of device to write to
//            byte addressToWrite - memory location to write to
//            byte dataToWrite - data to be written to that mem. address.
// OUTPUTS: Null
// RETURN VAL: Null
///////////////////////////////////////////////////////////////////////////////
void I2C_writeRegister(byte I2C_addr, byte addressToWrite, byte dataToWrite)
{
  Wire.beginTransmission(I2C_addr);
  Wire.write(addressToWrite);
  Wire.write(dataToWrite);
  Wire.endTransmission(); //Stop transmitting
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION: I2C_writeRegister_multiple()
// DESCRIPTION: writes multiple bytes to sequential device addresses
// ARGUMENTS: byte addressToWrite - memory location start to write to
//            byte * source - data buffer to be written
//            unsigned int bytesToWrite - number of bytes to write
// OUTPUTS: Null
// RETURN VAL: Null
///////////////////////////////////////////////////////////////////////////////
void I2C_writeRegister_multiple(byte I2C_addr, byte addressToWrite, unsigned int bytesToWrite, byte * source)
{
  Wire.beginTransmission(I2C_addr);
  Wire.write(addressToWrite);
  for(int i = 0; i < bytesToWrite; i++)
  {
    Wire.write(source[i]); 
  }
  Wire.endTransmission(); //Stop transmitting
}



