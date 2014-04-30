///////////////////////////////////////////////////////////////////////////////
//                       ECE445 SENIOR DESIGN - UIUC                     
//                         GROUP 1 - SKI SPEDOMETER                      
//                CHEIF ENGINEERS: CHRIS GERTH & BRIAN VAUGHN           
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// FILE: upper_unit_display.ino
// MAJOR REVISION: 0
// MINOR REVISION: See repository version
// DESCRIPTION: contains functions for communication with the 7-Segment display
// Code in this file was based off of sample code from sparkfun
// linked at https://www.sparkfun.com/products/11442
//////////////////////////////////////////////////////////////////////////////

#define SEVSEG_I2C_ADDR 0x71 
#define SEVSEG_CLR_DISP_CMD 0x76
#define SEVSEG_DCA_CTRL_CMD 0x77
#define SEVSEG_BGHT_CTRL_CMD 0x7A
#define SEVSEG_BGHT_LVL 0xFF

///////////////////////////////////////////////////////////////////////////////
// FUNCTION: display_init()
// DESCRIPTION: initalizes the 7-Segment display and associated data structures
// ARGUMENTS: Null
// OUTPUTS: ???
// RETURN VAL: Null
///////////////////////////////////////////////////////////////////////////////
void display_init()
{
  
  Wire.beginTransmission(SEVSEG_I2C_ADDR);
  Wire.write(SEVSEG_CLR_DISP_CMD);//Send the reset command to the display - this forces the cursor to return to the beginning of the display
  Wire.write(SEVSEG_BGHT_CTRL_CMD); //set up the proper brightness level...
  Wire.write(SEVSEG_BGHT_LVL);
  Wire.endTransmission();
  global_display_available = true;
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION: display_show_progress()
// DESCRIPTION: displays boot progress
// ARGUMENTS: Null
// OUTPUTS: ???
// RETURN VAL: Null
///////////////////////////////////////////////////////////////////////////////
void display_show_progress(int prog)
{
 char prog_0[4] = {' ', ' ', ' ', ' '};
 char prog_1[4] = {'-', ' ', ' ', ' '};
 char prog_2[4] = {'-', '-', ' ', ' '};
 char prog_3[4] = {'-', '-', '-', ' '};
 char prog_4[4] = {'-', '-', '-', '-'};
 
 switch(prog)
 {
   case 0:
     display_write_val(prog_0, 0);
   break;
   case 1:
     display_write_val(prog_1, 0);
   break;
   case 2:
     display_write_val(prog_2, 0);
   break;
   case 3:
     display_write_val(prog_3, 0);
   break;
   case 4:
     display_write_val(prog_4, 0);
   break;
 }
 delay(50); //be sure the user sees how pretty this is!
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION: display_update_main()
// DESCRIPTION: updates and displays data for homepage of device.
//              This means displaying the velocity.
// ARGUMENTS: Null
// OUTPUTS: ???
// RETURN VAL: Null
///////////////////////////////////////////////////////////////////////////////
void display_update_main()
{
  char pc_connect_disp[4] = {' ','p', 'C', ' '};
  int val_to_write = 0;
  int half_digit = 0;
  char disp_vals[4];
  volatile char dca_val;
  int i;
  float temp_float;
  
  if(global_current_mode == MODE_PC_CONNECT)
  {
    display_write_val(pc_connect_disp, 0x00);
    return;
  }
  else if (global_current_mode == MODE_STANDBY || global_current_mode == MODE_CAPTURE_DATA)
  {
    switch(global_display_data)
    {
      case 0: //user speed in MPH
        //break speed down into four digits (three integer digits and one half-integer digit)
        half_digit = global_user_speed_MPH & 0x01;
        val_to_write = global_user_speed_MPH >> 1;
        disp_vals[0] = val_to_write / 100;
        val_to_write = val_to_write % 100;
        disp_vals[1] = val_to_write / 10;
        val_to_write = val_to_write % 10;
        disp_vals[2] = val_to_write;
        if(half_digit)
          disp_vals[3] = '5';
        else
          disp_vals[3] = '0';
        
        //make leading zeros blank
        for(i = 0; i < 2; i++)
        {
          if(disp_vals[i] == 0)
            disp_vals[i] = ' ';
          else
            break;
        }
        
        dca_val = 0b00000100;//turn on proper decimal point
      break;
      
      case 1://user speed in KPH
        //break speed down into four digits (three integer digits and one half-integer digit)
        half_digit = global_user_speed_KPH & 0x01;
        val_to_write = global_user_speed_KPH >> 1;
        disp_vals[0] = val_to_write / 100;
        val_to_write = val_to_write % 100;
        disp_vals[1] = val_to_write / 10;
        val_to_write = val_to_write % 10;
        disp_vals[2] = val_to_write;
        if(half_digit)
          disp_vals[3] = '5';
        else
          disp_vals[3] = '0';
        
        //make leading zeros blank
        for(i = 0; i < 2; i++)
        {
          if(disp_vals[i] == 0)
            disp_vals[i] = ' ';
          else
            break;
        }
        
        dca_val = 0b00000100;//turn on proper decimal point
      
      break;
      
      case 2: //lateral Acceleration (x)  
        temp_float = (float)global_accel_x;
        
        if(temp_float > 0)
          disp_vals[0] = ' ';
        else
        {
          disp_vals[0] = '-';
          temp_float = -temp_float;
        }
        disp_vals[1] = ((char)floor(temp_float))%10;
        disp_vals[2] = ((char)floor(temp_float*10))%10;
        disp_vals[3] = ((char)floor(temp_float*100))%10;
        dca_val = 0b00000010;
      break;
      
      case 3: //longititudinal Acceleration (y)
        temp_float = (float)global_accel_y;
        
        if(temp_float > 0)
          disp_vals[0] = ' ';
        else
        {
          disp_vals[0] = '-';
          temp_float = -temp_float;
        }
        disp_vals[1] = ((char)floor(temp_float))%10;
        disp_vals[2] = ((char)floor(temp_float*10))%10;
        disp_vals[3] = ((char)floor(temp_float*100))%10;
        dca_val = 0b00000010;
      break;
      
      case 4: //Alt - m
        if(global_altitude_m < 0) //don't handle negative altitudes, just say it's zero
        {
          disp_vals[0] = 0;
          disp_vals[1] = 0;
          disp_vals[2] = 0;
          disp_vals[3] = 0; 
        }
        else
        {
          val_to_write = (int)global_altitude_m;
          disp_vals[0] = val_to_write / 1000;
          val_to_write = val_to_write % 1000;
          disp_vals[1] = val_to_write / 100;
          val_to_write = val_to_write % 100;
          disp_vals[2] = val_to_write / 10;
          val_to_write = val_to_write % 10;
          disp_vals[3] = val_to_write;       
        }
        
        //make leading zeros blank
        for(i = 0; i < 3; i++)
        {
          if(disp_vals[i] == 0)
            disp_vals[i] = ' ';
          else
            break;
        }
        dca_val = 0b00001000;
      break;
      
      case 5: //Temp (F)
        temp_float = (float)global_outside_temp_F;
        
        if(temp_float > 0)
          disp_vals[0] = ' ';
        else
        {
          disp_vals[0] = '-';
          temp_float = -temp_float;
        }
        disp_vals[1] = ((char)floor(temp_float/10))%10;
        disp_vals[2] = ((char)floor(temp_float))%10;
        disp_vals[3] = (char)(((int)floor(temp_float*10))%10);
        dca_val = 0b00000100;
      break;
      
      case 6: //Temp (C)
        temp_float = (float)global_outside_temp_C;
        
        if(temp_float > 0)
          disp_vals[0] = ' ';
        else
        {
          disp_vals[0] = '-';
          temp_float = -temp_float;
        }
        disp_vals[1] = ((char)floor(temp_float/10))%10;
        disp_vals[2] = ((char)floor(temp_float))%10;
        disp_vals[3] = (char)(((int)floor(temp_float*10))%10);
        dca_val = 0b00000100;
      break;
    }
    
    //turn on recording light if we're recording
    if(global_current_mode == MODE_CAPTURE_DATA)
      dca_val = dca_val | 0b00100000;
      
    display_write_val(disp_vals, dca_val); //write the data to the display! finally!
  }

}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION: display_write_val()
// DESCRIPTION: displays four characters on the display, along with approprate decimal places
// ARGUMENTS: char* vals - four byte buffer containing the four characters to write
//                    to the display
//            char dca_byte - byte to be written for the DCA control register
// OUTPUTS: ???
// RETURN VAL: Null
// NOTE: Will not write anything if the previous thing written matches the current thing to be written.
///////////////////////////////////////////////////////////////////////////////
void display_write_val(char* vals, char dca_byte)
{
  byte i = 0;
  boolean difference_flag = false;
  
  //check if the last display was identical
  for(i = 0; i < 4; i++)
  {
    if(global_prev_disp[i] != vals[i])
    {
      difference_flag = true; 
    }
    global_prev_disp[i] = vals[i];
  }
  
  if(global_prev_dca != dca_byte)
  {
    difference_flag = true;
  }
  global_prev_dca = dca_byte;
  

  
  if(difference_flag == false)
    return; //nothing to do

  //else write the new thing to the display
  Wire.beginTransmission(SEVSEG_I2C_ADDR);
  Wire.write(0x79);//set curser to first position
  Wire.write(0x00);
  Wire.write(vals[0]);
  Wire.write(vals[1]);
  Wire.write(vals[2]);
  Wire.write(vals[3]);
  Wire.write(SEVSEG_DCA_CTRL_CMD);
  Wire.write(dca_byte);
  Wire.endTransmission();
  
  #ifdef DEBUG_PRINT_DISPLAY
  Serial.print(vals[0], DEC);
  Serial.print(' ');
  Serial.print(vals[1], DEC);
  Serial.print(' ');
  Serial.print(vals[2], DEC);
  Serial.print(' ');
  Serial.print(vals[3], DEC);
  Serial.print(" DCA: 0b");
  Serial.println(dca_byte, BIN);
  
  
  #endif
  
  return;
}


