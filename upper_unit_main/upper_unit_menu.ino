///////////////////////////////////////////////////////////////////////////////
//                       ECE445 SENIOR DESIGN - UIUC                     
//                         GROUP 1 - SKI SPEDOMETER                      
//                CHEIF ENGINEERS: CHRIS GERTH & BRIAN VAUGHN           
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// FILE: upper_unit_menu.ino
// MAJOR REVISION: 0
// MINOR REVISION: See repository version
// DESCRIPTION: functions related to displaying the menu, and the menu's
//              interaction with the other variables
///////////////////////////////////////////////////////////////////////////////
#define MAX_MENU_OPTION 9 //last menu option
#define MIN_MENU_OPTION 1 //first menu option
#define DCA_DEFAULT_ST_1 0b00010001
#define DCA_NEGATIVE_ST_1 0b00010011
#define DCA_DEFAULT_ST_2 0b00010100
#define DCA_NEGATIVE_ST_2 0b00010110
#define NUM_DIGITS 4

//define the max and min allowed entry values for the user
const int menu_option_range_max[MAX_MENU_OPTION] = {0,  99, 59, 12, 0, 31, 12, 99, 6};
const int menu_option_range_min[MAX_MENU_OPTION] = {0, -99, 0 , 1 , 0, 1 , 1 , 0 , 0};

///////////////////////////////////////////////////////////////////////////////
// FUNCTION: run_main_menu()
// DESCRIPTION: runs the main menu. Blocks until the user exits the menu.
// ARGUMENTS: Null
// OUTPUTS: ???
// RETURN VAL: Null
///////////////////////////////////////////////////////////////////////////////
void run_main_menu()
{
  byte state = 1; //0 - exit menu, 1 - select menu option, 2 - set menu value
  byte cur_menu_option = MIN_MENU_OPTION;
  char disp_out[NUM_DIGITS];
  char dca_out;
  char press_delta_disp;
  
  while(state != 0)
  {
    press_delta_disp = (char)(((long)global_sea_level_press_Pa - (long)TEMP_BARO_SEA_LEVEL_PRESS_DEFAULT)/25L); //calculate delta to display in 0.25 mb/digit
    get_user_buttons();
    
    switch(state)
    {
      case 0: //state, exit menu
        continue;
      
      case 1: //state select menu option
        if(global_user_button_deltas[USER_BUTTON_MENU] == BUTTON_PRESSED)
        {
          state = 2;
        }
        else if(global_user_button_deltas[USER_BUTTON_UP] == BUTTON_PRESSED && cur_menu_option < MAX_MENU_OPTION)
        {
          cur_menu_option = cur_menu_option + 1;
        }
        else if(global_user_button_deltas[USER_BUTTON_DOWN] == BUTTON_PRESSED && cur_menu_option > MIN_MENU_OPTION)
        {
          cur_menu_option = cur_menu_option - 1;
        }
        else if(global_user_button_deltas[USER_BUTTON_EXIT] == BUTTON_PRESSED)
        {
          state = 0;
        }
      break;
      
      case 2://state change menu item value
        
        if(global_user_button_deltas[USER_BUTTON_EXIT] == BUTTON_PRESSED)
        {
          state = 1;
        }
        else
        {
          switch(cur_menu_option)
          {
            case 1: //clear memory
              if(global_user_button_cur_state_debounced[USER_BUTTON_UP] == BUTTON_DOWN && global_user_button_deltas[USER_BUTTON_DOWN] == BUTTON_PRESSED)
              {
                EEPROM_clear();
              }
            break; 
            
            case 2: //set sea level pressure
              //change the internal value if the user requests it.
              if(global_user_button_deltas[USER_BUTTON_UP] == BUTTON_PRESSED && (press_delta_disp < menu_option_range_max[cur_menu_option-1]))
              {
                global_sea_level_press_Pa = global_sea_level_press_Pa + 25; //increase the sea level pressure
                press_delta_disp = (char)(((long)global_sea_level_press_Pa - (long)TEMP_BARO_SEA_LEVEL_PRESS_DEFAULT)/25L); //recalculate display value
                tempBaro_set_sea_level_press(); //set to tempbaro sensor
                //store to internal EEPROM
                EEPROM.write(INTERNAL_EEPROM_SEA_LEVEL_PRESS_DATA_ADDR, (global_sea_level_press_Pa&0x000000FF));
                EEPROM.write(INTERNAL_EEPROM_SEA_LEVEL_PRESS_DATA_ADDR+1, (global_sea_level_press_Pa&0x0000FF00)>>8);
                EEPROM.write(INTERNAL_EEPROM_SEA_LEVEL_PRESS_DATA_ADDR+2, (global_sea_level_press_Pa&0x00FF0000)>>16);
                EEPROM.write(INTERNAL_EEPROM_SEA_LEVEL_PRESS_DATA_ADDR+3, (global_sea_level_press_Pa&0xFF000000)>>24);
                #ifdef DEBUG_PRINT_MENU
                Serial.print("press_delta_disp: ");
                Serial.println(press_delta_disp);                  
                #endif
              }
              else if(global_user_button_deltas[USER_BUTTON_DOWN] == BUTTON_PRESSED && (press_delta_disp > menu_option_range_min[cur_menu_option-1]))
              {
                global_sea_level_press_Pa = global_sea_level_press_Pa - 25; //decrease the sea level pressure
                press_delta_disp = (char)(((long)global_sea_level_press_Pa - (long)TEMP_BARO_SEA_LEVEL_PRESS_DEFAULT)/25L); //recalculate display value
                tempBaro_set_sea_level_press(); //send to tempbaro sensor
                //store to internal EEPROM
                EEPROM.write(INTERNAL_EEPROM_SEA_LEVEL_PRESS_DATA_ADDR, (global_sea_level_press_Pa&0x000000FF));
                EEPROM.write(INTERNAL_EEPROM_SEA_LEVEL_PRESS_DATA_ADDR+1, (global_sea_level_press_Pa&0x0000FF00)>>8);
                EEPROM.write(INTERNAL_EEPROM_SEA_LEVEL_PRESS_DATA_ADDR+2, (global_sea_level_press_Pa&0x00FF0000)>>16);
                EEPROM.write(INTERNAL_EEPROM_SEA_LEVEL_PRESS_DATA_ADDR+3, (global_sea_level_press_Pa&0xFF000000)>>24);
                #ifdef DEBUG_PRINT_MENU
                Serial.print("press_delta_disp: ");
                Serial.println(press_delta_disp);                  
                #endif
              }
            break; 
            
            case 3: //set current minute
              if(global_user_button_deltas[USER_BUTTON_UP] == BUTTON_PRESSED)
              {
                global_time_min = (global_time_min + 1)%menu_option_range_max[cur_menu_option-1]; //increase minute with rollover
                if(global_time_min > menu_option_range_max[cur_menu_option-1])
                  global_time_min=menu_option_range_min[cur_menu_option-1];
                rtc_set_time(0, global_time_min, global_time_hour, global_time_currently_AM, global_date, global_month, global_year); //set time
              }
              else if(global_user_button_deltas[USER_BUTTON_DOWN] == BUTTON_PRESSED)
              {
                global_time_min = (global_time_min - 1)%menu_option_range_max[cur_menu_option-1]; //decrease minute with rollover
                if(global_time_min < menu_option_range_min[cur_menu_option-1])
                  global_time_min=menu_option_range_max[cur_menu_option-1];
                rtc_set_time(0, global_time_min, global_time_hour, global_time_currently_AM, global_date, global_month, global_year); //set time
              }
            break; 
            
            case 4: //set current hour
              if(global_user_button_deltas[USER_BUTTON_UP] == BUTTON_PRESSED)
              {
                global_time_hour = (global_time_hour + 1); //increase hour
                if(global_time_hour > menu_option_range_max[cur_menu_option-1])
                  global_time_hour = menu_option_range_min[cur_menu_option-1];
                rtc_set_time(0, global_time_min, global_time_hour, global_time_currently_AM, global_date, global_month, global_year); //set time
              }
              else if(global_user_button_deltas[USER_BUTTON_DOWN] == BUTTON_PRESSED)
              {
                global_time_hour = (global_time_hour - 1); //decrease hour
                if(global_time_hour < menu_option_range_min[cur_menu_option-1])
                  global_time_hour = menu_option_range_max[cur_menu_option-1];
                rtc_set_time(0, global_time_min, global_time_hour, global_time_currently_AM, global_date, global_month, global_year); //set time
              }
            break;
            
            case 5:
              if(global_user_button_deltas[USER_BUTTON_UP] == BUTTON_PRESSED || global_user_button_deltas[USER_BUTTON_DOWN] == BUTTON_PRESSED)
              {
                global_time_currently_AM = !global_time_currently_AM;
                rtc_set_time(0, global_time_min, global_time_hour, global_time_currently_AM, global_date, global_month, global_year); //set time
              }
            break;
            
            case 6: //set current date
              if(global_user_button_deltas[USER_BUTTON_UP] == BUTTON_PRESSED)
              {
                global_date = (global_date + 1); //increase date
                
                //fix out-of-range with rollover
                if(global_date > menu_option_range_max[cur_menu_option-1])
                  global_date = menu_option_range_min[cur_menu_option-1];
                  
                rtc_set_time(0, global_time_min, global_time_hour, global_time_currently_AM, global_date, global_month, global_year); //set time
              }
              else if(global_user_button_deltas[USER_BUTTON_DOWN] == BUTTON_PRESSED)
              {
                global_date = (global_date - 1); //decrease date
                
                //fix out-of-range with rollover
                if(global_date < menu_option_range_min[cur_menu_option-1])
                  global_date = menu_option_range_max[cur_menu_option-1];
                  
                rtc_set_time(0, global_time_min, global_time_hour, global_time_currently_AM, global_date, global_month, global_year); //set time
              }
            break;
            
            case 7: //set current Month
              if(global_user_button_deltas[USER_BUTTON_UP] == BUTTON_PRESSED)
              {
                global_month = (global_month + 1); //increase month
                
                //fix out-of-range with rollover
                if(global_month > menu_option_range_max[cur_menu_option-1])
                  global_month = menu_option_range_min[cur_menu_option-1];
                  
                rtc_set_time(0, global_time_min, global_time_hour, global_time_currently_AM, global_date, global_month, global_year); //set time
              }
              else if(global_user_button_deltas[USER_BUTTON_DOWN] == BUTTON_PRESSED)
              {
                global_month = (global_month - 1); //decrease month
                
                //fix out-of-range with rollover
                if(global_month < menu_option_range_min[cur_menu_option-1])
                  global_month = menu_option_range_max[cur_menu_option-1];
                  
                rtc_set_time(0, global_time_min, global_time_hour, global_time_currently_AM, global_date, global_month, global_year); //set time
              }
            break;
            
            case 8: //set current Year
              if(global_user_button_deltas[USER_BUTTON_UP] == BUTTON_PRESSED)
              {
                global_year = (global_year + 1); //increase year
                
                //fix out-of-range with rollover
                if(global_year > menu_option_range_max[cur_menu_option-1])
                  global_year = menu_option_range_min[cur_menu_option-1];
                  
                rtc_set_time(0, global_time_min, global_time_hour, global_time_currently_AM, global_date, global_month, global_year); //set time
              }
              else if(global_user_button_deltas[USER_BUTTON_DOWN] == BUTTON_PRESSED)
              {
                global_year = (global_year - 1); //decrease year
                
                //fix out-of-range with rollover
                if(global_year < menu_option_range_min[cur_menu_option-1])
                  global_year = menu_option_range_max[cur_menu_option-1];
                  
                rtc_set_time(0, global_time_min, global_time_hour, global_time_currently_AM, global_date, global_month, global_year); //set time
              }
            break;
            
            case 9: //set display output value
              if(global_user_button_deltas[USER_BUTTON_UP] == BUTTON_PRESSED)
              {
                global_display_data = (global_display_data + 1); //increase data index with rollover
                if(global_display_data > menu_option_range_max[cur_menu_option-1])
                  global_display_data=menu_option_range_min[cur_menu_option-1];
                  
                EEPROM.write(INTERNAL_EEPROM_DISPLAY_DATA_ADDR, global_display_data);
              }
              else if(global_user_button_deltas[USER_BUTTON_DOWN] == BUTTON_PRESSED)
              {
                global_display_data = (global_display_data - 1); //decrease data index with rollover
                if(global_display_data < menu_option_range_min[cur_menu_option-1])
                  global_display_data=menu_option_range_max[cur_menu_option-1];
                EEPROM.write(INTERNAL_EEPROM_DISPLAY_DATA_ADDR, global_display_data);
              }
            break;
            
            default://catch state in case something really weird happens
              state = 0;
            break;
          }
        }
      break;
      
      default: //catch state in case something really weird happens
        state = 0;
      break;
    }
    
    //set display values
    
    //set menu option number to display output
    disp_out[0] = (char)(cur_menu_option/10);
    disp_out[1] = (char)(cur_menu_option%10);
    
    //set param values
    switch(cur_menu_option)
    {
      case 1:
        //display current number of recordings
        disp_out[2] = (char)(global_next_available_recording_descriptor/10);
        disp_out[3] = (char)(global_next_available_recording_descriptor%10);
        if(state == 1)
          dca_out = DCA_DEFAULT_ST_1;
        else
          dca_out = DCA_DEFAULT_ST_2;
      break;
      
      case 2:
        if(press_delta_disp < 0) //see what decimal places to set to indicate if the offset is negative
        {
          if(state == 1)
            dca_out = DCA_NEGATIVE_ST_1;
          else
            dca_out = DCA_NEGATIVE_ST_2;
          press_delta_disp = -press_delta_disp;
        }
        else
        {
          if(state == 1)
            dca_out = DCA_DEFAULT_ST_1;
          else
            dca_out = DCA_DEFAULT_ST_2;
        }
        //write the value to the output buffer
        disp_out[2] = (char)(press_delta_disp / 10);
        disp_out[3] = (char)(press_delta_disp % 10);
      break;
      
      case 3:
        if(state == 1)
          dca_out = DCA_DEFAULT_ST_1;
        else
          dca_out = DCA_DEFAULT_ST_2;
        disp_out[2] = (char)(global_time_min / 10);
        disp_out[3] = (char)(global_time_min % 10);
      break;
      
      case 4:
        if(state == 1)
          dca_out = DCA_DEFAULT_ST_1;
        else
          dca_out = DCA_DEFAULT_ST_2;
        disp_out[2] = (char)(global_time_hour / 10);
        disp_out[3] = (char)(global_time_hour % 10);
      break;
      
      case 5:
        if(state == 1)
          dca_out = DCA_DEFAULT_ST_1;
        else
          dca_out = DCA_DEFAULT_ST_2;
        disp_out[2] = ' ';
        if(global_time_currently_AM)
          disp_out[3] = 'A';
        else
          disp_out[3] = 'p';
      break;
      
      case 6:
        if(state == 1)
          dca_out = DCA_DEFAULT_ST_1;
        else
          dca_out = DCA_DEFAULT_ST_2;
        disp_out[2] = (char)(global_date / 10);
        disp_out[3] = (char)(global_date % 10);
      break;
      
      case 7:
        if(state == 1)
          dca_out = DCA_DEFAULT_ST_1;
        else
          dca_out = DCA_DEFAULT_ST_2;
        disp_out[2] = (char)(global_month / 10);
        disp_out[3] = (char)(global_month % 10);
      break;
      
      case 8:
        if(state == 1)
          dca_out = DCA_DEFAULT_ST_1;
        else
          dca_out = DCA_DEFAULT_ST_2;
        disp_out[2] = (char)(global_year / 10);
        disp_out[3] = (char)(global_year % 10);
      break;
      
      case 9:
        if(state == 1)
          dca_out = DCA_DEFAULT_ST_1;
        else
          dca_out = DCA_DEFAULT_ST_2;
        disp_out[2] = (char)(global_display_data / 10);
        disp_out[3] = (char)(global_display_data % 10);
      
      break;
    }
    
    //replace leading zeros with blanks
    if(disp_out[0] == 0)
      disp_out[0] = ' ';
      
    if(disp_out[2] == 0)
      disp_out[2] = ' ';

    display_write_val(disp_out, dca_out);
    
  } //while(state != 0)
  return;
}
