///////////////////////////////////////////////////////////////////////////////
//                       ECE445 SENIOR DESIGN - UIUC                     
//                         GROUP 1 - SKI SPEDOMETER                      
//                CHEIF ENGINEERS: CHRIS GERTH & BRIAN VAUGHN           
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// FILE: upper_unit_RTC.ino
// MAJOR REVISION: 0
// MINOR REVISION: See repository version
// DESCRIPTION: contains functions for communication with the RTC
///////////////////////////////////////////////////////////////////////////////

//RTC Constants
#define RTC_I2C_ADDRESS 0x68 //I2C Address for RTC
#define RTC_SEC_REG 0x00 //registers
#define RTC_MIN_REG 0x01
#define RTC_HOUR_REG 0x02
#define RTC_DAY_REG 0x03
#define RTC_DATE_REG 0x04
#define RTC_MONTH_REG 0x05
#define RTC_YEAR_REG 0x06
#define RTC_CAL_CFG1_REG 0x07
#define RTC_SFKEY1_REG 0x20
#define RTC_SFKEY2_REG 0x21
#define RTC_KEY1 0x5E
#define RTC_KEY2 0xC7
#define RTC_SF_REG 0x22


///////////////////////////////////////////////////////////////////////////////
// FUNCTION: rtc_init()
// DESCRIPTION: sets up real-time clock
// ARGUMENTS: Null
// OUTPUTS: updates global_rtc_status 
// RETURN VAL: Null
///////////////////////////////////////////////////////////////////////////////
void rtc_init()
{
  byte keyring[2] = {RTC_KEY1, RTC_KEY2}; //for unlocking the special function register
  
  #ifdef DEBUG_PRINT_RTC
  Serial.println("Starting RTC Initalization...");
  #endif
  
  //no WHOAMI register, so we'll just have to assume the RTC is present and functional
  byte timedata;

  //special procedure to set square wave output to one Hz - must write special keys to two registers
  I2C_writeRegister_multiple(RTC_I2C_ADDRESS, RTC_SFKEY1_REG, 2, keyring);
  I2C_writeRegister(RTC_I2C_ADDRESS,RTC_SF_REG, 0x01); //set 1hz mode
    
  I2C_writeRegister(RTC_I2C_ADDRESS, RTC_CAL_CFG1_REG, 0xC0); //enable square wave output
  
  delay(500); //delay .5 s to ensure crystal has time to stabilize 
  
  #ifdef DEBUG_PRINT_RTC
  Serial.println("RTC initalization complete.");
  #endif
  
  return;
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION: rtc_read_time()
// DESCRIPTION: retrieves time from RTC
// ARGUMENTS: Null
// OUTPUTS: updates all RTC time variables
// RETURN VAL: Null
///////////////////////////////////////////////////////////////////////////////
void rtc_read_time()
{
  byte timedata[8];
  
  I2C_readRegisters(RTC_I2C_ADDRESS, RTC_SEC_REG, 8, timedata); //read in all data from RTC
  if(timedata[7] && 0x20) //check OSF flag
    global_rtc_status = 1; // signal that time needs to be set
  else
    global_rtc_status = 0;
    
  //convert BCD data structures to our globabl data format
  //see datasheet for bitfield descriptions (eg, why these bitmasks work)
  global_time_sec = 10*((timedata[0] & 0x70)>>4) + (timedata[0] & 0x0F);
  global_time_min = 10*((timedata[1] & 0x70)>>4) + (timedata[1] & 0x0F);
  global_time_hour = cvt_hr_to_12h_format((10*((timedata[2] & 0x30)>>4) + (timedata[2] & 0x0F)), &global_time_currently_AM);
  global_day_of_week = timedata[3] & 0x07;
  global_date = 10*((timedata[4] & 0x30)>>4) + (timedata[4] & 0x0F);
  global_month = 10*((timedata[5] & 0x10)>>4) + (timedata[5] & 0x0F);
  global_year = 10*((timedata[6] & 0xF0)>>4) + (timedata[6] & 0x0F);
  
  #ifdef DEBUG_PRINT_RTC
  Serial.print(global_time_hour, DEC);
  Serial.print(":");
  Serial.print(global_time_min, DEC);
  Serial.print(":");
  Serial.print(global_time_sec, DEC);
  Serial.print("AM?=");
  Serial.print(global_time_currently_AM, BIN);
  Serial.print(" on ");
  Serial.print(global_month, DEC);
  Serial.print("-");
  Serial.print(global_date, DEC);
  Serial.print("-20");
  Serial.println(global_year, DEC);
  #endif
  
  return;
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION: rtc_set_time()
// DESCRIPTION: Sets a given time to the RTC
// ARGUMENTS: sec, minute, hour, date, month all = bytes which should be self explanatory
//            year should be last two digits of current year (eg, 14 instead of 2014)
//            is_AM is true if the time should be AM, false if it should be PM.
//            Day of the week is automatically calculated.
// OUTPUTS: registers on the RTC.
// RETURN VAL: Null
///////////////////////////////////////////////////////////////////////////////
void rtc_set_time(byte sec, byte minute, byte hour, boolean is_AM, byte date, byte month, byte year)
{
  byte timedata[7];
  byte temp_24_hr = cvt_hr_to_24h_format(hour, is_AM);
  
  //convert numbers into bcd bits within a register block
  timedata[0] = (((sec/10) << 4) | (sec % 10)) & 0x7F; //be sure CH bit is still cleared
  timedata[1] = (((minute/10) << 4) | (minute % 10));
  timedata[2] = (((temp_24_hr/10) << 4)& 0x30) | (temp_24_hr % 10) & 0x0F; 
  timedata[3] = rtc_calc_day_of_week(date, month, (unsigned int)year+2000) & 0x07;
  timedata[4] = (((date/10) << 4) | (date % 10));
  timedata[5] = (((month/10) << 4) | (month % 10));
  timedata[6] = (((year/10) << 4) | (year % 10));
  
  //note we can do sequential writes just like sequential reads
  //but I chose the inneficent way instead to minimize function overhead.
  //The time is set infrequently, so time performance is not crucial here...
  #ifdef DEBUG_PRINT_RTC
  Serial.println("Writing time to RTC:");
  #endif
  for(byte i = 0; i < 7; i++)
  {
    //We should be using the #defines's for registers here, but to save program space, it's been compacted to a for loop with i
    #ifdef DEBUG_PRINT_RTC
    Serial.print("0b");
    Serial.println(timedata[i], BIN);
    #endif
    I2C_writeRegister(RTC_I2C_ADDRESS, i, timedata[i]); 
  }

}
///////////////////////////////////////////////////////////////////////////////
// FUNCTION: rtc_calc_day_of_week()
// DESCRIPTION: takes the current date/month/year, and outputs the day of the week usable with the RTC
// ARGUMENTS: current date and month, as well as whole 4-digit year (2014, not 14)
// OUTPUTS: Null
// RETURN VAL: byte of day of week
// NOTE: formula for day-of-week calculation from http://en.wikipedia.org/wiki/Determination_of_the_day_of_the_week
///////////////////////////////////////////////////////////////////////////////
byte rtc_calc_day_of_week(byte date, byte month, unsigned int year)
{
   static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4}; //table for month offset
   year -= month < 3;
   return ((year + year/4 - year/100 + year/400 + t[month-1] + date) % 7) + 1; //1 = sunday, 7 = saturday
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION: cvt_hr_to_12h_format()
// DESCRIPTION: returns 12-hour format of a 24-hour hour representation
// ARGUMENTS: current hour in 24 hour format, pointer to boolean for AM/PM bit
// OUTPUTS: isAM_ptr - pointer toboolean to be set to 1 if it is AM, 0 if it is PM
// RETURN VAL: 12-hour representation
///////////////////////////////////////////////////////////////////////////////
byte cvt_hr_to_12h_format(byte hour, boolean * isAM_ptr)
{
  if(hour < 12)
  {
    *isAM_ptr = 1;
    if(hour == 0)
      return 12;
    else
      return hour;
  }
  else
  {
    *isAM_ptr = 0;
    if(hour == 12)
      return 12;
    else
      return hour-12;
  }
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION: cvt_hr_to_24h_format()
// DESCRIPTION: returns 24-hour format of a 12-hour hour representation
// ARGUMENTS: current hour in 12 hour format, am/pm bit
// OUTPUTS: 
// RETURN VAL: 24-hour representation
///////////////////////////////////////////////////////////////////////////////
byte cvt_hr_to_24h_format(byte hour, boolean isAM)
{
  if(isAM)
  {
    if(hour == 12)
      return 0;
    else
      return hour;
  }
  else
  {
    if(hour == 12)
      return 12;
    else
      return (12+hour);
  }
  
}



