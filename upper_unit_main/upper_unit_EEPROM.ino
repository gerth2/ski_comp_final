///////////////////////////////////////////////////////////////////////////////
//                       ECE445 SENIOR DESIGN - UIUC                     
//                         GROUP 1 - SKI SPEDOMETER                      
//                CHEIF ENGINEERS: CHRIS GERTH & BRIAN VAUGHN           
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// FILE: upper_unit_EEPROM.ino
// MAJOR REVISION: 0
// MINOR REVISION: See repository version
// DESCRIPTION: contains functions for communication with the EEPROM memory chip
///////////////////////////////////////////////////////////////////////////////

extern void display_write_val(char* vals, char dca_byte); //forward-definition
#define NUM_DIGITS 4

//a few notes: memory_descriptor is the 16 byte region that defines the whole filesystem
//                -memory_descriptor is always at the start of the filesystem (addr's 0x0-0xF)
//                -memory_descriptor contains a pointer to the start of the recording_descriptors and their size
//                -memory_descriptor contains a pointer to the start of the datapoints and their size
//             recording_descriptor is one of 32 16-byte regions which describe a recording
//             datapoint is one of 4030 8-byte regions which describe data recorded at a single moment.

//EEPROM constants
#define EEPROM_I2C_ADDRESS 0x50 //address of eeprom memory
#define EEPROM_MAX_ADDR 0x7FFF //maximum address in eeprom memory address space
#define EEPROM_MAGIC_NUMBER_1 0x42 //constants which determine the initilization state of the eeprom
#define EEPROM_MAGIC_NUMBER_2 0x2F
#define EEPROM_RECORDING_DESCRIPTOR_SIZE 0x10 //size in bytes of each recording descriptor
#define EEPROM_DATA_POINT_SIZE 0x08 //size in bytes of each data sample point record
#define EEPROM_MEMORY_DESCRIPTOR_SIZE 0x10 //size in bytes of the memory descriptor
#define EEPROM_MEMORY_DESCRIPTOR_START 0x00 //location of the eeprom memory descriptor
#define EEPROM_RECORDING_DESCRIPTOR_PTR EEPROM_MEMORY_DESCRIPTOR_SIZE //location of the first recording descriptor, whcih is right after memory_descriptor
#define EEPROM_DATA_REGION_START 0x0210 //location of the first datapoint
#define EEPROM_MAX_RECORDINGS 32 //maximum of 32 recordings
#define EEPROM_PAGE_SIZE 64 //size of page in EEPROM
#define EEPROM_NUM_PAGES 4096 //number of pages present in EEPROM

//EEPROM filestructure offset constants. See excel sheet for memory region definitions
#define EEPROM_MD_MAGIC_NUMBER_1_OFFSET 0x00
#define EEPROM_MD_NUM_RECORDINGS_OFFSET 0x01
#define EEPROM_MD_PTR_TO_RECORDINGS_OFFSET 0x02
#define EEPROM_MD_RECORDING_DESCRIPTOR_SIZE_OFFSET 0x04
#define EEPROM_MD_DATA_SIZE_OFFSET 0x05
#define EEPROM_MD_RECORDING_IN_PROGRESS_OFFSET 0x06
#define EEPROM_MD_MAGIC_NUMBER_2_OFFSET 0x07
#define EEPROM_MD_INIT_DATE_OFFSET 0x08
#define EEPROM_MD_INIT_MONTH_OFFSET 0x09
#define EEPROM_MD_INIT_YEAR_OFFSET 0x0A
#define EEPROM_MD_INIT_HOUR_OFFSET 0x0B
#define EEPROM_MD_INIT_MIN_OFFSET 0x0C
#define EEPROM_MD_INIT_AM_PM_OFFSET 0x0D
#define EEPROM_MD_NEXT_AVAIL_DATAPT_MSB_OFFSET 0x0E
#define EEPROM_MD_NEXT_AVAIL_DATAPT_LSB_OFFSET 0x0F

#define EEPROM_RD_START_SEC_OFFSET      0x01
#define EEPROM_RD_START_MIN_OFFSET      0x02
#define EEPROM_RD_START_HOUR_OFFSET     0x03
#define EEPROM_RD_START_AM_PM_OFFSET    0x04
#define EEPROM_RD_START_DATE_OFFSET     0x05
#define EEPROM_RD_START_MONTH_OFFSET    0x06
#define EEPROM_RD_START_YEAR_OFFSET     0x07
#define EEPROM_RD_NUM_DATAPOINTS_OFFSET 0x08
#define EEPROM_RD_DATA_POINTER_OFFSET   0x0A

#define EEPROM_DP_DELTA_T_OFFSET 0x00
#define EEPROM_DP_TEMP_C_OFFSET 0x01
#define EEPROM_DP_ALTITUDE_M_OFFSET 0x02
#define EEPROM_DP_ACCEL_X_OFFSET 0x04
#define EEPROM_DP_ACCEL_Y_OFFSET 0x05
#define EEPROM_DP_ACCEL_Z_OFFSET 0x06
#define EEPROM_DP_SPEED_OFFSET 0x07

///////////////////////////////////////////////////////////////////////////////
// FUNCTION: EEPROM_init()
// DESCRIPTION: verifies filesystem for EEPROM is vaild, sets it up if not
// ARGUMENTS: Null
// OUTPUTS: updates global_eeprom_status, global_next_available_datapoint, global_next_available_recording_descriptor with values from eeprom
// RETURN VAL: Null
///////////////////////////////////////////////////////////////////////////////
void EEPROM_init()
{
  byte buffer[EEPROM_MEMORY_DESCRIPTOR_SIZE]; //temp buffer for working with system
  
  //this section provides debugging functionality to allow developer to reset
  //the EEPROM each time the device is booted. Should not be left on for
  //production code, as it will wipe all runs every power cycle (bad!)
  #ifdef CLEAR_EEPROM_ON_RESET
  EEPROM_clear();
  #endif
  
  EEPROM_I2C_readRegisters(EEPROM_I2C_ADDRESS, EEPROM_MEMORY_DESCRIPTOR_START, EEPROM_MEMORY_DESCRIPTOR_SIZE, buffer); //pull in current memory descriptor
  if(buffer[EEPROM_MD_MAGIC_NUMBER_1_OFFSET] == EEPROM_MAGIC_NUMBER_1 && buffer[EEPROM_MD_MAGIC_NUMBER_2_OFFSET] == EEPROM_MAGIC_NUMBER_2)
  {
    //magic numbers check out, the EEPROM is ready to use.
  }
  else //we need to initalize the EEPROM. This case should only be hit if EEPROM was corrupted, first power-on, or something like that...
  {
    rtc_read_time();//get current time so we know when we initalized it.
    buffer[EEPROM_MD_MAGIC_NUMBER_1_OFFSET] = EEPROM_MAGIC_NUMBER_1;
    buffer[EEPROM_MD_NUM_RECORDINGS_OFFSET] = 0;
    buffer[EEPROM_MD_PTR_TO_RECORDINGS_OFFSET] = EEPROM_RECORDING_DESCRIPTOR_PTR >> 8; //MSB
    buffer[EEPROM_MD_PTR_TO_RECORDINGS_OFFSET+1] = EEPROM_RECORDING_DESCRIPTOR_PTR & 0xFF; //LSB
    buffer[EEPROM_MD_RECORDING_DESCRIPTOR_SIZE_OFFSET] = EEPROM_RECORDING_DESCRIPTOR_SIZE;
    buffer[EEPROM_MD_DATA_SIZE_OFFSET] = EEPROM_DATA_POINT_SIZE;
    buffer[EEPROM_MD_RECORDING_IN_PROGRESS_OFFSET] = 0;
    buffer[EEPROM_MD_MAGIC_NUMBER_2_OFFSET] = EEPROM_MAGIC_NUMBER_2;
    buffer[EEPROM_MD_INIT_DATE_OFFSET] = global_date;
    buffer[EEPROM_MD_INIT_MONTH_OFFSET] = global_month;
    buffer[EEPROM_MD_INIT_YEAR_OFFSET] = global_year;
    buffer[EEPROM_MD_INIT_HOUR_OFFSET] = global_time_hour;
    buffer[EEPROM_MD_INIT_MIN_OFFSET] = global_time_min;
    buffer[EEPROM_MD_INIT_AM_PM_OFFSET] = byte(!global_time_currently_AM);
    buffer[EEPROM_MD_NEXT_AVAIL_DATAPT_MSB_OFFSET] = (EEPROM_RECORDING_DESCRIPTOR_PTR+EEPROM_RECORDING_DESCRIPTOR_SIZE*EEPROM_MAX_RECORDINGS) >> 8;
    buffer[EEPROM_MD_NEXT_AVAIL_DATAPT_LSB_OFFSET] = (EEPROM_RECORDING_DESCRIPTOR_PTR+EEPROM_RECORDING_DESCRIPTOR_SIZE*EEPROM_MAX_RECORDINGS) & 0xFF;
    
    //write whole memory descriptor into memory
    EEPROM_I2C_writeRegister_multiple(EEPROM_I2C_ADDRESS, EEPROM_MEMORY_DESCRIPTOR_START, EEPROM_MEMORY_DESCRIPTOR_SIZE, buffer);  
  }
  EEPROM_I2C_readRegisters(EEPROM_I2C_ADDRESS, EEPROM_MEMORY_DESCRIPTOR_START+EEPROM_MD_NUM_RECORDINGS_OFFSET, 1, &global_next_available_recording_descriptor); //pull out the next available descriptor
  EEPROM_I2C_readRegisters(EEPROM_I2C_ADDRESS, EEPROM_MEMORY_DESCRIPTOR_START+EEPROM_MD_NEXT_AVAIL_DATAPT_MSB_OFFSET, 2, buffer); //pull out the next available datapoint
  global_next_available_datapoint = ((unsigned int)buffer[0]) << 8 + ((unsigned int)buffer[1]);
  

  
  if(global_next_available_recording_descriptor >= 32) //ensure that the EEPROM is not already full.
    global_eeprom_status = 1;
  else if(global_next_available_datapoint >= EEPROM_MAX_ADDR) 
    global_eeprom_status = 1;
  else
    global_eeprom_status = 0; //by now, the eeprom should be good.
  
  #ifdef DEBUG_PRINT_EEPROM
  Serial.print("Next available datapoint = 0x");
  Serial.println(global_next_available_datapoint, HEX);
  Serial.print("Next available recording descriptor = ");
  Serial.println(global_next_available_recording_descriptor);
  Serial.print("EEPROM Status: ");
  Serial.println(global_eeprom_status, DEC);
  #endif
  
  
  
  //DEBUG Code:
  /*
  for(unsigned int i = 0; i < 16; i++)
    buffer[i] = i;
  for(unsigned int i = 0; i < (EEPROM_MAX_ADDR>>4); i++)
    EEPROM_I2C_writeRegister_multiple(EEPROM_I2C_ADDRESS, i<<4, 0x10, buffer); 
    */
  /*
  EEPROM_I2C_readRegisters(EEPROM_I2C_ADDRESS, 0x2000, 5, buffer);
  
  Serial.println("First Read Back:");
  for(int i = 0; i < 5; i++)
    Serial.println(buffer[i]);
  
  buffer[0] = 0x10;
  buffer[1] = 0x20;
  buffer[2] = 0x30;
  buffer[3] = 0x40;
  buffer[4] = buffer[4] + 1;
  
  EEPROM_I2C_writeRegister_multiple(EEPROM_I2C_ADDRESS, 0x2000, 5, buffer);
  
  EEPROM_I2C_readRegisters(EEPROM_I2C_ADDRESS, 0x2000, 5, buffer);
  
  Serial.println("Read after Write:");
  for(int i = 0; i < 5; i++)
    Serial.println(buffer[i]);
    
  while(1);
  
  //END DEBUG CODE
  */
  return;
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION: EEPROM_start_record_session()
// DESCRIPTION: starts sets up filesystem to start tracking data
// ARGUMENTS: Null
// OUTPUTS: updates EEPROM recording_descriptor, global_last_sample_time_ms, 
// RETURN VAL: Null
///////////////////////////////////////////////////////////////////////////////
void EEPROM_start_record_session()
{
  byte recording_descriptor_buf[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  byte temp_buf[2]; //temporary working buffer
  
  if(global_eeprom_status != 0)//make sure eeprom is ready to do stuff before we try and write to it.
  {
      #ifdef DEBUG_PRINT_EEPROM
      Serial.print("Error, EEPROM not initalized or full. Error is ");
      Serial.println(global_eeprom_status, DEC);
      #endif
      return;
  }
  
  if(global_next_available_recording_descriptor > 32) //check to see that we have space first before we try writing to EEPROM
  {
      #ifdef DEBUG_PRINT_EEPROM
      Serial.println("Error, too many recordings.");
      #endif
      global_eeprom_status = 1; //set that the EEPROM is full
      return;
  }
  
  EEPROM_I2C_writeRegister(EEPROM_I2C_ADDRESS, EEPROM_MEMORY_DESCRIPTOR_START+EEPROM_MD_RECORDING_IN_PROGRESS_OFFSET, 0x01); //set that a recording is now in progress 
  #ifdef DEBUG_PRINT_EEPROM
  Serial.println("Set Recording in Progress bit");
  #endif
  recording_descriptor_buf[EEPROM_RD_START_SEC_OFFSET] = global_time_sec;      
  recording_descriptor_buf[EEPROM_RD_START_MIN_OFFSET] = global_time_min;          
  recording_descriptor_buf[EEPROM_RD_START_HOUR_OFFSET] = global_time_hour;         
  recording_descriptor_buf[EEPROM_RD_START_AM_PM_OFFSET] = (byte)(!global_time_currently_AM);        
  recording_descriptor_buf[EEPROM_RD_START_DATE_OFFSET] = global_date;         
  recording_descriptor_buf[EEPROM_RD_START_MONTH_OFFSET] = global_month;        
  recording_descriptor_buf[EEPROM_RD_START_YEAR_OFFSET] = global_year;         
  recording_descriptor_buf[EEPROM_RD_NUM_DATAPOINTS_OFFSET] = 0; //no points so far
  recording_descriptor_buf[EEPROM_RD_NUM_DATAPOINTS_OFFSET+1] = 0;
  
  //populate pointer to data with the next available datapoint pointer from the memory descriptor. This read command takes care of all 16 bits at once.
  EEPROM_I2C_readRegisters(EEPROM_I2C_ADDRESS, EEPROM_MEMORY_DESCRIPTOR_START+EEPROM_MD_NEXT_AVAIL_DATAPT_MSB_OFFSET, 2, recording_descriptor_buf+EEPROM_RD_DATA_POINTER_OFFSET);
  global_next_available_datapoint = ((unsigned int)recording_descriptor_buf[EEPROM_RD_DATA_POINTER_OFFSET] << 8)+((unsigned int)recording_descriptor_buf[EEPROM_RD_DATA_POINTER_OFFSET+1]);
  
  if(global_next_available_datapoint > EEPROM_MAX_ADDR)
  {
     #ifdef DEBUG_PRINT_EEPROM
     Serial.print("Error: Next available datapoint is 0x");
     Serial.print(global_next_available_datapoint, HEX);
     Serial.println(", which exceededs memory capacity. Not recording.");
     #endif
     global_eeprom_status = 1;
     return;
  }
  
  #ifdef DEBUG_PRINT_EEPROM
  Serial.print("Read in pointer to next available datapoint = 0x");
  Serial.println(global_next_available_datapoint, HEX);
  #endif
  
  //populate recording_descriptor with data
  EEPROM_I2C_writeRegister_multiple(EEPROM_I2C_ADDRESS, EEPROM_RECORDING_DESCRIPTOR_PTR+(EEPROM_RECORDING_DESCRIPTOR_SIZE*global_next_available_recording_descriptor), EEPROM_RECORDING_DESCRIPTOR_SIZE, recording_descriptor_buf);
  #ifdef DEBUG_PRINT_EEPROM
  Serial.println("Populated recording descriptor");
  #endif

  
  global_last_sample_time_ms = millis(); //record the time so we can track the delta to the first datapoint more accurately than the RTC
  #ifdef DEBUG_PRINT_EEPROM
  Serial.println("Recorded sample time");
  #endif
  
  return;
  
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION: EEPROM_record_data_point()
// DESCRIPTION: records single data point from current data in memory
// ARGUMENTS: Null
// OUTPUTS: updates global_last_sample_time_ms, global_next_available_datapoint, and EEPROM 
// RETURN VAL: Null
///////////////////////////////////////////////////////////////////////////////
void EEPROM_record_data_point()
{
  byte datapoint_buf[EEPROM_DATA_POINT_SIZE] = {0,0,0,0,0,0,0,0};
  byte tempbuf[2] = {0,0};
  
  if(global_eeprom_status != 0) //don't do anything if the eeprom isn't initalized properly
    return;
  
  datapoint_buf[EEPROM_DP_DELTA_T_OFFSET] = (byte)((millis()-global_last_sample_time_ms)/50); //in units of 50ms/bit
  datapoint_buf[EEPROM_DP_TEMP_C_OFFSET] = (char)(global_outside_temp_C); //needs to still be signed
  datapoint_buf[EEPROM_DP_ALTITUDE_M_OFFSET] = ((byte)(global_altitude_m))>>8; //MSB
  datapoint_buf[EEPROM_DP_ALTITUDE_M_OFFSET+1] = ((byte)(global_altitude_m)) & 0xFF; //LSB
  datapoint_buf[EEPROM_DP_ACCEL_X_OFFSET] = (byte)(global_accel_x*16); //in units of 1/16 G/bit
  datapoint_buf[EEPROM_DP_ACCEL_Y_OFFSET] = (byte)(global_accel_y*16); //in units of 1/16 G/bit
  datapoint_buf[EEPROM_DP_ACCEL_Z_OFFSET] = (byte)(global_accel_z*16); //in units of 1/16 G/bit
  datapoint_buf[EEPROM_DP_SPEED_OFFSET] = (byte)(global_user_speed_MPH); //in units of 1/2 MPH/bit

  global_last_sample_time_ms = millis(); //mark the time of this sample for calculating the next delta

  //write datapoint to EEPROM
  EEPROM_I2C_writeRegister_multiple(EEPROM_I2C_ADDRESS, global_next_available_datapoint, EEPROM_DATA_POINT_SIZE, datapoint_buf);
  
  #ifdef DEBUG_PRINT_EEPROM
  Serial.print("Wrote Datapoint at 0x");
  Serial.println(global_next_available_datapoint, HEX);
  #endif
  
  global_next_available_datapoint = global_next_available_datapoint + (unsigned int)EEPROM_DATA_POINT_SIZE; //update the next available datapoint
  tempbuf[0] = (global_next_available_datapoint >> 8);
  tempbuf[1] = (global_next_available_datapoint & 0xFF);
  EEPROM_I2C_writeRegister_multiple(EEPROM_I2C_ADDRESS,EEPROM_MEMORY_DESCRIPTOR_START+EEPROM_MD_NEXT_AVAIL_DATAPT_MSB_OFFSET, 2, tempbuf); //update memory_descriptor with the next available datapoint
  
  #ifdef DEBUG_PRINT_EEPROM
  Serial.print("Updated next available datapoint to 0x");
  Serial.println(global_next_available_datapoint, HEX);
  #endif
  
  if(global_next_available_datapoint > EEPROM_MAX_ADDR)
  {
     #ifdef DEBUG_PRINT_EEPROM
     Serial.print("Error: Next available datapoint is 0x");
     Serial.print(global_next_available_datapoint, HEX);
     Serial.println(", which exceededs memory capacity. Not recording.");
     #endif
     global_eeprom_status = 1;
     return;
  }
  return;
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION: EEPROM_stop_record_session()
// DESCRIPTION: closes out a recording in memory.
// ARGUMENTS: Null
// OUTPUTS: updates EEPROM, global_next_available_recording_descriptor, 
// RETURN VAL: Null
///////////////////////////////////////////////////////////////////////////////
void EEPROM_stop_record_session()
{
  byte tempbuf[2] = {0,0};
  unsigned int start_datapoint;
  unsigned int num_pts;
  

  
  //get the start of the datapoints for the current RD
  EEPROM_I2C_readRegisters(EEPROM_I2C_ADDRESS, EEPROM_RECORDING_DESCRIPTOR_PTR+ (EEPROM_RECORDING_DESCRIPTOR_SIZE*global_next_available_recording_descriptor) + EEPROM_RD_DATA_POINTER_OFFSET, 2, tempbuf);
  start_datapoint = (((unsigned int)tempbuf[0]) << 8) + ((unsigned int)tempbuf[1]);
  num_pts = (global_next_available_datapoint - start_datapoint)/EEPROM_DATA_POINT_SIZE; //calculate number of points based on offset between start and next abvailable datapoint addresses
  tempbuf[0] = num_pts >> 8;
  tempbuf[1] = num_pts & 0xFF;
  //write number of points back to the recording descriptor
  EEPROM_I2C_writeRegister_multiple(EEPROM_I2C_ADDRESS, EEPROM_RECORDING_DESCRIPTOR_PTR + EEPROM_RECORDING_DESCRIPTOR_SIZE*global_next_available_recording_descriptor + EEPROM_RD_NUM_DATAPOINTS_OFFSET, 2, tempbuf);
  
  //move to the next recording descriptor for the next record session
  global_next_available_recording_descriptor = global_next_available_recording_descriptor+1;
  
  if(global_next_available_recording_descriptor == 32)
  {
      #ifdef DEBUG_PRINT_EEPROM
      Serial.println("Recordings full...");
      #endif
      global_eeprom_status = 1; //set that the EEPROM is full
  }
    
  EEPROM_I2C_writeRegister(EEPROM_I2C_ADDRESS, EEPROM_MEMORY_DESCRIPTOR_START+EEPROM_MD_NUM_RECORDINGS_OFFSET, (global_next_available_recording_descriptor)); //set that a new recording has been added to eeprom
  EEPROM_I2C_writeRegister(EEPROM_I2C_ADDRESS, EEPROM_MEMORY_DESCRIPTOR_START+EEPROM_MD_RECORDING_IN_PROGRESS_OFFSET, 0x00); //set that a recording is no longer in progress
  
  
  #ifdef DEBUG_PRINT_EEPROM
  Serial.print("Recording Started at ");
  Serial.println(start_datapoint, HEX);
  Serial.print("Recording had ");
  Serial.print(num_pts);
  Serial.println(" datapoints.");
  Serial.print("Set number of recordings to ");
  Serial.println(global_next_available_recording_descriptor);
  #endif
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION: EEPROM_dump_contents_to_PC()
// DESCRIPTION: transfers entire EEPROM contents to PC. Interleavs I2C reads 
//              and serial writes
// ARGUMENTS: Null
// OUTPUTS: writes to serial port. Blocks until complete. Stops interrupts
// RETURN VAL: Null
///////////////////////////////////////////////////////////////////////////////
void EEPROM_dump_contents_to_PC()
{
  byte data_buffer;
  unsigned int addr;
  digitalWrite(STATUS_LED_PIN,HIGH);//set status led to show we're dumping...
  for(addr = 0x0000; addr <= EEPROM_MAX_ADDR; addr++)
  { //I2C read-Serial write cycle - not the most efficent, but the PC has to write these
    //to a disc file, so hopefully the slowness here will give the PC time to react...
    EEPROM_I2C_readRegisters(EEPROM_I2C_ADDRESS, addr, 1, &data_buffer);

    #ifdef DEBUG_PRINT_EEPROM
    Serial.print("Addr:"); //human-readable version
    Serial.println(addr);
    Serial.print("Data:");
    Serial.println(data_buffer);
    #else
    Serial.write(data_buffer); //computer-version
    #endif 
  }
  Serial.print("$END\n");
  digitalWrite(STATUS_LED_PIN,LOW);//clear status led to show we're done
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION: EEPROM_clear()
// DESCRIPTION: resets EEPROM to be properly initalized and empty. Note this 
//              does not actually clear out the data, just changes state 
//              variables so the existing data is treated as garbage and
//              gets written-over. Actual EEPROM wipes should be avoided
//              to maximize life of EEPROM.
// ARGUMENTS: Null
// OUTPUTS: updates global_next_available_recording_descriptor, global_next_available_datapoint, global_eeprom_status, and EEPROM
// RETURN VAL: Null
///////////////////////////////////////////////////////////////////////////////
void EEPROM_clear()
{
  byte buffer[64]; //temporary work buffer
  char done_disp[NUM_DIGITS] = {'C','L','R','D'};
  
  //set total number of recordings to zero
  EEPROM_I2C_writeRegister(EEPROM_I2C_ADDRESS,EEPROM_MEMORY_DESCRIPTOR_START+EEPROM_MD_NUM_RECORDINGS_OFFSET,0);
  
  //set next-available-datapoint pointers back to start of data region
  EEPROM_I2C_writeRegister(EEPROM_I2C_ADDRESS,EEPROM_MEMORY_DESCRIPTOR_START+EEPROM_MD_NEXT_AVAIL_DATAPT_MSB_OFFSET,(EEPROM_DATA_REGION_START) >> 8);
  EEPROM_I2C_writeRegister(EEPROM_I2C_ADDRESS,EEPROM_MEMORY_DESCRIPTOR_START+EEPROM_MD_NEXT_AVAIL_DATAPT_LSB_OFFSET,(EEPROM_DATA_REGION_START) & 0xFF);
  
  //read back results into Arduino's memory
  EEPROM_I2C_readRegisters(EEPROM_I2C_ADDRESS, EEPROM_MEMORY_DESCRIPTOR_START+EEPROM_MD_NUM_RECORDINGS_OFFSET, 1, &global_next_available_recording_descriptor); //pull out the next available descriptor
  EEPROM_I2C_readRegisters(EEPROM_I2C_ADDRESS, EEPROM_MEMORY_DESCRIPTOR_START+EEPROM_MD_NEXT_AVAIL_DATAPT_MSB_OFFSET, 2, buffer); //pull out the next available datapoint
  global_next_available_datapoint = ((unsigned int)buffer[0]) << 8 + ((unsigned int)buffer[1]);
  
  global_eeprom_status = 0;
  
  //for debugging purposes, functionality is added to allow developer to fully wipe the EEPROM
  //and not just reset pointer positions.
  #ifdef EEPROM_FULL_CLEAR
  for(unsigned int i = 0; i < 64; i++)
    buffer[i] = 0x00;
  for(unsigned int i = 0; i < (EEPROM_MAX_ADDR/64); i++)
    EEPROM_I2C_writeRegister_multiple(EEPROM_I2C_ADDRESS, i/64, 64, buffer); 
  #endif
  
  display_write_val(done_disp, 0x00);
  
  #ifdef DEBUG_PRINT_EEPROM
  Serial.println("Cleared EEPROM.");
  #endif
  delay(1000);
  
}

//NOTE: EEPROM NEEDS SPECIAL I2C FUNCTIONS BEAUSE OF ITS 16-BIT ADDRESSING

///////////////////////////////////////////////////////////////////////////////
// FUNCTION: EEPROM_I2C_readRegisters()
// DESCRIPTION: reads multiple bytes from an I2C device
// ARGUMENTS: byte I2C_addr - I2C address of device to read from
//            byte addressToRead - memory location to start read from
//            unsigned int bytesToRead - number of bytes to read from device 
//            byte* dest - buffer to place the read bytes into
// OUTPUTS: puts bytes into buffer dest.
// NOTE: blocks until I2C data has been returned.
// RETURN VAL: Null
///////////////////////////////////////////////////////////////////////////////
void EEPROM_I2C_readRegisters(byte I2C_addr, unsigned int addressToRead, unsigned int bytesToRead, byte * dest)
{
  
  Wire.beginTransmission(I2C_addr);
  Wire.write((byte)(addressToRead>>8));
  Wire.write((byte)(addressToRead & 0x00FF));
  Wire.endTransmission();
  
  Wire.requestFrom((int)I2C_addr, bytesToRead); //Ask for bytes, once done, bus is released by default
  while(Wire.available() < bytesToRead); //Hang out until we get the # of bytes we expect
  for(int x = 0 ; x < bytesToRead ; x++)
  {
    dest[x] = Wire.read();    
  }
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION: EEPROM_I2C_writeRegister()
// DESCRIPTION: writes a byte to one device address
// ARGUMENTS: byte I2C_addr - I2C address of device to write to
//            byte addressToWrite - memory location to write to
//            byte dataToWrite - data to be written to that mem. address.
// OUTPUTS: Null
// RETURN VAL: Null
///////////////////////////////////////////////////////////////////////////////
void EEPROM_I2C_writeRegister(byte I2C_addr, unsigned int addressToWrite, byte dataToWrite)
{
  Wire.beginTransmission(I2C_addr);
  Wire.write((byte)(addressToWrite>>8));
  Wire.write((byte)(addressToWrite & 0xFF));
  Wire.write(dataToWrite);
  Wire.endTransmission(); //Stop transmitting
  
  delay(5);//Ensure the EEPROM has time to actually store the data
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION: EEPROM_I2C_writeRegister_multiple()
// DESCRIPTION: writes multiple bytes to sequential device addresses
// ARGUMENTS: byte addressToWrite - memory location start to write to
//            byte * source - data buffer to be written
//            unsigned int bytesToWrite - number of bytes to write
// OUTPUTS: Null
// RETURN VAL: Null
///////////////////////////////////////////////////////////////////////////////
void EEPROM_I2C_writeRegister_multiple(byte I2C_addr, unsigned int addressToWrite, unsigned int bytesToWrite, byte * source)
{
  Wire.beginTransmission(I2C_addr);
  Wire.write((byte)(addressToWrite>>8));
  Wire.write((byte)(addressToWrite & 0xFF));
  for(int i = 0; i < bytesToWrite; i++)
  {
    Wire.write(source[i]);
  }
  Wire.endTransmission(); //Stop transmitting
  
  delay(5); //ensure the EEPROM has time to store the actual data
}


