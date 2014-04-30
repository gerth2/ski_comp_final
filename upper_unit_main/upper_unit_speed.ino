///////////////////////////////////////////////////////////////////////////////
//                       ECE445 SENIOR DESIGN - UIUC                     
//                         GROUP 1 - SKI SPEDOMETER                      
//                CHEIF ENGINEERS: CHRIS GERTH & BRIAN VAUGHN           
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// FILE: upper_unit_speed.ino
// MAJOR REVISION: 0
// MINOR REVISION: See repository version
// DESCRIPTION: functions for calculating the speed
///////////////////////////////////////////////////////////////////////////////

#define SPEED_AVG_FILT_LEN 10
#define DATASHEET_FREQ_SPD_CONVERT_MPH 31.36 //radar sensor datasheet-given conversion factors
#define DATASHEET_FREQ_SPD_CONVERT_KPH 19.49
#define FREQ_DOWNCONVERSION_FACTOR 16 //factor by which the frequency is downconverted to fit within the bandwidth allowable by the XBEES
#define BYPASS_AVG_FILT_THRESH_US 50000L
#define ANGLE_CORRECTION_FACTOR 0.707 //correction for the fact that the radar sensor is at an angle
                                      //will also include a "fudgefactor" to correct for system innacuracies
#define ZERO_FREQ_TIMEOUT 15
                                      

///////////////////////////////////////////////////////////////////////////////
// FUNCTION: calculate_average_filtered_speed()
// DESCRIPTION: calculates filtered speed from delta_t's
// ARGUMENTS: Null
// OUTPUTS: updates global_user_speed_MPH, global_user_speed_KPH, global_frequency
// RETURN VAL: Null
///////////////////////////////////////////////////////////////////////////////
void calculate_average_filtered_speed()
{
  unsigned long avg_delta_t_us = 0;
  unsigned char i;
  
  if(global_speed_delta_t[global_speed_t_index]>BYPASS_AVG_FILT_THRESH_US) //case period too long, use current period
  {
    avg_delta_t_us = global_speed_delta_t[global_speed_t_index];
  }
  else //case period long enough to use averaging filter
  {
    for(i = 0; i < SPEED_AVG_FILT_LEN; i++)
    {
      //pre-divide by ten. I know it decreases accuracy, but I also don't want avg_delta_t to overflow
      avg_delta_t_us +=global_speed_delta_t[i]/10; 
    }
  } 
  
  global_prev_frequency = global_frequency; //save old value
  global_frequency = (float)(1000000.0/(float)(avg_delta_t_us)*(float)FREQ_DOWNCONVERSION_FACTOR); //calculate frequency
  
  //record how many of the same frequencies we have seen
  if(global_prev_frequency  == global_frequency)
    global_unchanged_freq_count++;
  else
    global_unchanged_freq_count = 0;
 
  //if we haven't gotten a new frequency in a while (eg, no new edge), assume frequency is actually zero
  //and therefore the speeds should be zero.  
  if(global_unchanged_freq_count > ZERO_FREQ_TIMEOUT)
  {
    global_unchanged_freq_count = ZERO_FREQ_TIMEOUT;
    global_user_speed_MPH = 0; 
    global_user_speed_KPH = 0;
  }
  else //otherwise calculate the speed.
  {
    global_user_speed_MPH = 2*global_frequency / DATASHEET_FREQ_SPD_CONVERT_MPH / ANGLE_CORRECTION_FACTOR; //update speeds
    global_user_speed_KPH = 2*global_frequency / DATASHEET_FREQ_SPD_CONVERT_KPH / ANGLE_CORRECTION_FACTOR; 
  }
  
  #ifdef DEBUG_PRINT_SPEED
  Serial.print("Measured Speed (Hz MPH KPH): ");
  Serial.print(global_frequency);
  Serial.print(" ");
  Serial.print((float)global_user_speed_MPH/2.0);
  Serial.print(" ");
  Serial.println((float)global_user_speed_KPH/2.0);
  #endif
}

