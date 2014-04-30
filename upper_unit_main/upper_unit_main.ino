///////////////////////////////////////////////////////////////////////////////
//                       ECE445 SENIOR DESIGN - UIUC                     
//                         GROUP 1 - SKI SPEDOMETER                      
//                CHEIF ENGINEERS: CHRIS GERTH & BRIAN VAUGHN           
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// FILE: upper_unit_main.ino
// MAJOR REVISION: 0
// MINOR REVISION: See repository version
// DESCRIPTION: Main Arduino Sketch running on the upper unit computer
//              Facilitates data aquisition, logging, and display
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// SECTION: Library Inclusions & Debug Controls
// DESCRIPTION: External libraries included to drive perephials.
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

//If these are defined, code will be compiled with printf satements to return
//debugging information. Should not be turned on for production-ready code.
//#define DEBUG_PRINT_MASTER

#ifdef DEBUG_PRINT_MASTER
#define DEBUG_PRINT //for this file
#define DEBUG_PRINT_EEPROM
//#define DEBUG_PRINT_PC
//#define DEBUG_PRINT_RTC
//#define DEBUG_PRINT_ACCEL
//#define DEBUG_PRINT_DISPLAY
//#define DEBUG_PRINT_MENU
//#define DEBUG_PRINT_SPEED
//#define DEBUG_PRINT_TEMP_BARO
//#define DEBUG_PRINT_BUTTONS
#endif

//if this is defined, we will do process timing based on RTC ticks
//otherwise, timing is extracted from less accurate internal timers.
#define TIME_SOURCE_EXT

//If this is defined, the EEPROM will be wiped every time the unit is reset
//Useful for speeding up the debugging process, but should NOT be used
//for final production code (defeats the purpose of the EEPROM
//#define CLEAR_EEPROM_ON_RESET

//If this is defined, the EEPROM memory clear function will set all the values
//to zero when called. If not defined, it simply resets the number of runs to
//zero. It should only be defined for debugging purposes.
//#define EEPROM_FULL_CLEAR

//Wire library is used for I2C communications. See 
//http://playground.arduino.cc/Main/WireLibraryDetailedReference for details
//of how it works. It is packaged with the Arduino IDE, version 1.0.5
#include <Wire.h>

//EEPROM library used to store user settings on the ATMEGA's non-volatile memory
//note this is not the data EEPROM on the board, this is on the processor itself
#include <EEPROM.h>

//Adafruit has written a library set for their display that we utilized
//we downloaded it from https://github.com/adafruit/Adafruit_SSD1306 and https://github.com/adafruit/Adafruit-GFX-Library
//See their website for more details and tutorials.
//Both of these libraries are put in our Repo, but you must add them to the Arduino IDE so it knows where to find them.
//Go Sketch->Import Library->Add library..., navigate to the code section of the repo, and select the folder of the library. 
// -This will have to be done twice, once per library.
//#include <Adafruit_GFX.h>
//#include <Adafruit_SSD1306.h>

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// SECTION: Global Constants
// DESCRIPTION: Constant values used throughout the software
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

//temp/baro global defines
#define TEMP_BARO_SEA_LEVEL_PRESS_DEFAULT 101325L

//speed sensor definitions
#define SPEED_AVG_FILT_LEN 10

//Button State Definitions (booleans and chars)
#define BUTTON_DOWN true 
#define BUTTON_UP false
#define BUTTON_NO_CHANGE 2
#define BUTTON_PRESSED 3
#define BUTTON_RELEASED 4
#define NUM_BUTTONS 4
#define BUTTON_DEBOUNCE_LOOPS_NOT_MENU 3
#define BUTTON_DEBOUNCE_LOOPS_MENU 30

//Button number definitions
#define USER_BUTTON_UP 0 //also doubles as "start/stop record"
#define USER_BUTTON_DOWN 1 //also doubles as "switch display"
#define USER_BUTTON_MENU 2
#define USER_BUTTON_EXIT 3

//Pin number definitions
#define SERIAL_RX_PIN 0 //automatically set up by Serial. library
#define SERIAL_TX_PIN 1 //automatically set up by Serial. library
#define I2C_SDA_PIN A4 //automatically set up by Wire. library
#define I2C_SCL_PIN A5 //automatically set up by Wire. library
#define RADAR_FREQ_PIN 2 //must be 2 or 3 so it can be an interrupt
#define RTC_INT_PIN 3 //1Hz square wave applied here for timing purposes
#define COLLISION_PENDING_PIN 4 //asserted when pending collision is detected
#define USER_INPUT_UP_PIN 5  //user input buttons
#define USER_INPUT_DOWN_PIN 6
#define USER_INPUT_MENU_PIN 7
#define USER_INPUT_EXIT_PIN 8
#define STATUS_LED_PIN 13 //LED is built into board

//operational mode definitions
#define MODE_STANDBY 0 //only read sensor data (default)
#define MODE_START_CAPTURE 1 //initalize sensor data record session
#define MODE_CAPTURE_DATA 2 //record sensor data to EEPROM
#define MODE_END_CAPTURE 3 //close out sensor data record session
#define MODE_MENU 4 //display user menus
#define MODE_PC_CONNECT 5 //talk to PC for data exchange

//PC communication constants
#define PC_MSG_TERM_CHAR '\n'
#define MAX_PC_MSG_LENGTH 64

//Arduino Hardware constants
#define CPU_FREQ 8000000L //our cpu frequency is 8MHZ

//Arduino internal EEPROM constants
#define INTERNAL_EEPROM_DISPLAY_DATA_ADDR 0
#define INTERNAL_EEPROM_SEA_LEVEL_PRESS_DATA_ADDR 1


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// SECTION: Global Data
// DESCRIPTION: Volatile data structures which hold data used by functions
//              Most data which is to be shared across functions or tasks is
//              kept in this global format. Since only one physical unit is 
//              present, it didn't make much sense to define a large struct
//              for this data and pass around a pointer. Rather, everything
//              is declared as global, and the compiler takes care of getting
//              the data out of the heap. In this way, the globals act like
//              a single class, and all the functions are methods on that
//              class.
// NOTE: All globally-visible data should have the prefix "global_". ANY 
//       function which can touch a global variable needs to have that 
//       stated in the "OUTPUT" field of the function comment header.
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

//Microcontroller operation data
unsigned char global_loop_counter = 0;
unsigned char global_current_mode = MODE_STANDBY;
boolean global_capture_ready; //determines if the data capture is ready to commence
boolean global_capture_stopped; //determines if the data capture has been stopped.
unsigned long global_last_sample_time_ms = 0;

//Temp & Baro pressure sensor data
float global_outside_temp_C = 0;
float global_outside_temp_F = 0; 
float global_altitude_m = 0;
unsigned long global_sea_level_press_Pa = TEMP_BARO_SEA_LEVEL_PRESS_DEFAULT;
unsigned char global_baro_temp_sensor_status = -1; //0 = normal, -1 = error

//eeprom data
byte global_next_available_recording_descriptor = 0;
unsigned int global_next_available_datapoint = 0;
unsigned char global_eeprom_status = -1; //-1 = error, 0 = normal, 1 = full

//timing data
volatile boolean global_run_1HZ_tasks = false;
volatile boolean global_1HZ_tasks_missed = false;

//rtc data 
char global_time_sec = 0;
char global_time_min = 0;
char global_time_hour = 1; //12 hour system
boolean global_time_currently_AM = false;
char global_day_of_week = 1;
char global_date = 1;
char global_month = 1;
char global_year = 0;
byte global_rtc_status = -1; //0 = normal, -1 = error, 2 = running but time not set

//accelerometer sensor data
float global_accel_x = 0;
float global_accel_y = 0;
float global_accel_z = 0;
unsigned char global_accel_sensor_status = -1; //0 = normal, -1 = error

//speed sensor data
volatile unsigned long global_speed_delta_t[SPEED_AVG_FILT_LEN];
volatile byte global_speed_t_index = 0;
volatile unsigned long global_speed_prev_t;
unsigned int global_user_speed_MPH = 0; //in units of 0.5MPH/LSB
unsigned int global_user_speed_KPH = 0; //in units of 0.5KPH/LSB
unsigned int global_frequency = 0;
unsigned int global_prev_frequency = 0;
unsigned int global_unchanged_freq_count;
unsigned char global_radar_sensor_status = -1; //0 = normal, -1 = error


//button array order is up, down, menu, exit
boolean global_user_button_states[NUM_BUTTONS][BUTTON_DEBOUNCE_LOOPS_MENU]; 
boolean global_user_button_cur_state_debounced[NUM_BUTTONS];
boolean global_user_button_prev_state_debounced[NUM_BUTTONS];
byte global_user_button_deltas[NUM_BUTTONS] = {BUTTON_NO_CHANGE, BUTTON_NO_CHANGE, BUTTON_NO_CHANGE, BUTTON_NO_CHANGE}; 
boolean global_user_button_new_state_available = false;


//PC connection variables
char global_pc_current_cmd[MAX_PC_MSG_LENGTH+1];
boolean global_pc_new_cmd_available = false;

//Display variables
boolean global_display_available = false;
char global_display_data = 0; //0=Speed(MPH), 1=Speed(KPH), 2=Lateral Accel, 3=Longititudinal_accel, 4=Altitude (m), 5=Temperature(F), 6=Temperature(C)
volatile char global_prev_disp[4] = {' ', ' ', ' ', ' '};
volatile char global_prev_dca = 0x00;

#ifndef TIME_SOURCE_EXT
unsigned long global_prev_millis = 0;
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// SECTION: Arduino Functions
// DESCRIPTION: The setup and loop functions required by Arduino. 
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// FUNCTION: setup()
// DESCRIPTION: Tasks to be run once right after power-on.
// ARGUMENTS: Null
// OUTPUTS: Null
// RETURN VAL: Null
///////////////////////////////////////////////////////////////////////////////
void setup()
{
  Serial.begin(115200); //open serial port to computer
  
  #ifdef DEBUG_PRINT
  Serial.println("Booting Up...");
  Serial.println("Setting Pin Modes");
  #endif
  
  //set proper pin modes
  pinMode(RADAR_FREQ_PIN, INPUT);
  pinMode(RTC_INT_PIN, INPUT);
  pinMode(COLLISION_PENDING_PIN, INPUT);
  pinMode(USER_INPUT_UP_PIN, INPUT);
  pinMode(USER_INPUT_DOWN_PIN , INPUT);
  pinMode(USER_INPUT_MENU_PIN, INPUT);
  pinMode(USER_INPUT_EXIT_PIN, INPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);

  #ifdef DEBUG_PRINT
  Serial.println("Setting up I2C (Fast Mode)");
  #endif
  
  Wire.begin(); //initalize I2C communications 
  TWBR = ((CPU_FREQ / 400000L) - 16) / 2; //set up high-speed (400KHZ) I2C comm's
  
  #ifdef DEBUG_PRINT
  Serial.println("Initalizing hardware peripherials");
  #endif
  display_init(); //initalize the display
  display_show_progress(0);
  rtc_init(); //initalize real-time clock
  accel_init(); //initalize the accelerometer
  display_show_progress(1);
  tempBaro_init(); //initalize the temperature and barometric pressure sensors
  tempBaro_set_sea_level_press(); //ensure the default sea-level pressure has been set.
  EEPROM_init(); //must happen after RTC init, as it might pull the current date and time.
  display_show_progress(2);

  #ifdef DEBUG_PRINT
  Serial.println("Attaching interrupts");
  #endif
  display_show_progress(3);
  attachInterrupt(0, speed_freq_input_pin_rising_edge, RISING); //interupt 0 is on pin 2.
  attachInterrupt(1, rtc_timer_freq_input_pin_trigger, RISING); //interupt 1 is on pin 3.
  //note that RTC generates a 1Hz square wave on pin 3, so rtc_timer_freq_input_pin_trigger fires
  // every 1000ms. The plan is to use that for task timing, esp. writing to EEPROM.
  
  #ifdef DEBUG_PRINT
  Serial.println("Retrieving User settings from Internal EEPROM");
  #endif

  global_display_data = EEPROM.read(INTERNAL_EEPROM_DISPLAY_DATA_ADDR);
  global_sea_level_press_Pa = (unsigned long)(EEPROM.read(INTERNAL_EEPROM_SEA_LEVEL_PRESS_DATA_ADDR));
  global_sea_level_press_Pa += (unsigned long)(EEPROM.read(INTERNAL_EEPROM_SEA_LEVEL_PRESS_DATA_ADDR+1))<<8;
  global_sea_level_press_Pa += (unsigned long)(EEPROM.read(INTERNAL_EEPROM_SEA_LEVEL_PRESS_DATA_ADDR+2))<<16;
  global_sea_level_press_Pa += (unsigned long)(EEPROM.read(INTERNAL_EEPROM_SEA_LEVEL_PRESS_DATA_ADDR+3))<<24;
  
  #ifdef DEBUG_PRINT
  Serial.print("Got Display Data Value: ");
  Serial.println(global_display_data);
  Serial.print("Got sea_level_press: ");
  Serial.println(global_sea_level_press_Pa);
  #endif
  
  
  display_show_progress(4);
  #ifdef DEBUG_PRINT
  Serial.println("Boot Complete!");
  #endif
  
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION: loop()
// DESCRIPTION: Run once every main execution loop
// ARGUMENTS: Null
// OUTPUTS: global_loop_count
// RETURN VAL: Null
///////////////////////////////////////////////////////////////////////////////
void loop()
{
  //run approprate tasks for this main loop
  tasks_main_execution_loop();

  //if we are doing timing internally, set the 1Hz flag approximately every 1000 ms.
  #ifndef TIME_SOURCE_EXT
  if(millis() - global_prev_millis > 1000)
  {
    global_run_1HZ_tasks = true;
    global_prev_millis = millis();
  }
  #endif
  
  if(global_run_1HZ_tasks == true)
    tasks_1HZ_Update();
    
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// SECTION: Loop Task Functions
// DESCRIPTION: Functions which contain tasks to be run at a given frequency.
//              loop() will call a subset of these functions during each main
//              loop iteration depending on the task frequency. Prioritizing
//              these tasks allows proper operation with minimal processor
//              load. 
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// FUNCTION: tasks_main_execution_loop()
// DESCRIPTION: Tasks to be run once every main execution loop
// ARGUMENTS: Null
// OUTPUTS: Null
// RETURN VAL: Null
///////////////////////////////////////////////////////////////////////////////
void tasks_main_execution_loop()
{
  get_pc_cmd();
  get_user_buttons();
  determine_current_mode(); //update the system with the current operational state
  process_PC_cmd();
  
  if(global_current_mode == MODE_MENU)
  {
    run_main_menu(); //run through the menu until exit
    display_update_main();
    return;
  }
  
  if(global_current_mode != MODE_MENU && global_current_mode != MODE_PC_CONNECT)
  {
    calculate_average_filtered_speed();
    accel_readData();
    tempBaro_readData();
    if(global_user_button_deltas[USER_BUTTON_DOWN] == BUTTON_PRESSED)
    {
      global_display_data = (global_display_data + 1)%7; //rotate through display options
      EEPROM.write(INTERNAL_EEPROM_DISPLAY_DATA_ADDR, global_display_data);
    }
  }
  
  if(global_current_mode == MODE_START_CAPTURE) //see if we need to start data capture
  {
    EEPROM_start_record_session();
    global_capture_ready = true; 
  }
  else if(global_current_mode == MODE_END_CAPTURE) //see if we need to stop data capture
  {
    EEPROM_stop_record_session();
    global_capture_stopped = true;
  }

  display_update_main();

}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION: tasks_1HZ_Update()
// DESCRIPTION: Tasks to be run once every second
// ARGUMENTS: Null
// OUTPUTS: Null
// RETURN VAL: Null
///////////////////////////////////////////////////////////////////////////////
void tasks_1HZ_Update()
{
  #ifdef DEBUG_PRINT
  Serial.println("Running 1Hz Tasks");
  #endif
  if(global_current_mode == MODE_CAPTURE_DATA)
    EEPROM_record_data_point(); //record a datapoint if we're in record mode
  
  rtc_read_time(); //get current time from RTC
  
  global_run_1HZ_tasks = false; //clear flag for running 1Hz update functions
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION: determine_current_state()
// DESCRIPTION: Sets up the state for this loop based on data set in previous loop
// ARGUMENTS: Null
// OUTPUTS: updates global_current_mode variable
// RETURN VAL: Null
///////////////////////////////////////////////////////////////////////////////
void determine_current_mode()
{
  int prev_mode = global_current_mode; //get the previous loop's mode

  //main state machine switch case
  switch(prev_mode) //make next state decisions first based on current state
  {
    case MODE_STANDBY:
      if(global_user_button_new_state_available)
      { 
        if(global_user_button_deltas[USER_BUTTON_MENU] == BUTTON_PRESSED)
        {
          #ifdef DEBUG_PRINT
          Serial.println("Going to Mode MENU");
          #endif
          global_current_mode = MODE_MENU; //go into the menu if that button was pressed
        }
       else if(global_user_button_deltas[USER_BUTTON_UP] == BUTTON_PRESSED && global_eeprom_status == 0) 
       {
         #ifdef DEBUG_PRINT
         Serial.println("Going to Mode Start Capture");
         #endif
         global_capture_ready = false; //set that we aren't ready to start capture yet, but we should prepare to.
                                       //someone else will set this to true when the capture is ready to begin
         global_current_mode = MODE_START_CAPTURE; 
       }
       else if(global_pc_new_cmd_available && global_pc_current_cmd[0] == '$' && global_pc_current_cmd[1] == 'S')
       {
         #ifdef DEBUG_PRINT
         Serial.println("Going to Mode PC Connect");
         #endif
         global_pc_new_cmd_available = 0; //we have processed the command.
         global_current_mode = MODE_PC_CONNECT;
         Serial.print("$R\n"); //tell the computer we are now ready for communication.
         Serial.flush();
       }
       else //if none of the above apply, we're still in standby.
         global_current_mode = MODE_STANDBY;
      }
    break;
    
    case MODE_START_CAPTURE: //wait in start_capture mode until the EEPROM has been set up to recieve data.
      if(global_capture_ready == true)
      {
        #ifdef DEBUG_PRINT
        Serial.println("Going to Mode Capture Data");
        #endif
        if(global_eeprom_status == 1)
          global_current_mode = MODE_STANDBY;
        else
          global_current_mode = MODE_CAPTURE_DATA;
      }
      else
      {
        global_current_mode = MODE_START_CAPTURE;
        Serial.print("Still waiting for capture to start");
        Serial.println(global_current_mode);
      }
    break;
    
    case MODE_CAPTURE_DATA: //while capturing data, check if the stop capture button was pressed
      if(global_user_button_new_state_available)
      { 
        if(global_user_button_deltas[USER_BUTTON_UP] == BUTTON_PRESSED)
        {
          global_capture_stopped = false;
          #ifdef DEBUG_PRINT
          Serial.println("Going to Mode End Capture");
          #endif
          global_current_mode = MODE_END_CAPTURE; //stop recording if the button was pressed
        }
      }
      else if(global_eeprom_status != 0) //check to see if the eeprom is full
      {
        global_capture_stopped = false;
        #ifdef DEBUG_PRINT
        Serial.println("ERROR - EEPROM not able to accept more data. Going to Mode End Capture");
        #endif
        global_current_mode = MODE_END_CAPTURE; //stop recording if the button was pressed
      }
      else
        global_current_mode = MODE_CAPTURE_DATA;
    break;
    
    case MODE_END_CAPTURE:
       if(global_capture_stopped == true)
       {
         #ifdef DEBUG_PRINT
         Serial.println("Going to Mode Standby");
         #endif
         global_current_mode = MODE_STANDBY;
       }
      else
        global_current_mode = MODE_END_CAPTURE;
    break;
    
    case MODE_MENU:
    //once we've done one loop in menu mode, just go back to standby.
       #ifdef DEBUG_PRINT
       Serial.println("Going to Mode Standby");
       #endif
       global_current_mode = MODE_STANDBY;
    break;
    
    case MODE_PC_CONNECT:
      //check to see if PC has attempted to terminate the connection.
      if(global_pc_new_cmd_available && global_pc_current_cmd[0] == '$' && global_pc_current_cmd[1] == 'E')
      {
        #ifdef DEBUG_PRINT
        Serial.println("Going to Mode Standby");
        #endif
        global_pc_new_cmd_available = 0; //we have processed the command.
        global_current_mode = MODE_STANDBY;
      }
      else //stay connected to computer.
      {
        global_current_mode = MODE_PC_CONNECT;
      }
    break;
    
    default:
      global_current_mode = MODE_STANDBY; //catch/recover from errors and just go to standby
    break;
  }
  
  return;
  
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// SECTION: Interrupt Service Routines
// DESCRIPTION: Functions that get called from interrupt contexts.
//              Must be as light-weight as possible
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// FUNCTION: speed_freq_input_pin_rising_edge()
// DESCRIPTION: records the delta_t in milliseconds
// ARGUMENTS: Null
// OUTPUTS: updates global_current_mode variable
// RETURN VAL: Null
// Note micros() rolls over once every 70 minutes, causing a speed glitch.
// I'll assume it's minor for now, we can always add a check to throw out
// that datapoint if need be. However, because of the infrequency of the 
// glitch, I'll assume it's not worth the overhead of an extra check in the ISR
///////////////////////////////////////////////////////////////////////////////
void speed_freq_input_pin_rising_edge()
{
  //noInterrupts();
  global_speed_t_index = (global_speed_t_index + 1)%SPEED_AVG_FILT_LEN;
  long cur_time = micros();
  global_speed_delta_t[global_speed_t_index] = cur_time - global_speed_prev_t; //record the delta_t
  global_speed_prev_t = cur_time;
  
  #ifdef DEBUG_PRINT
  //Serial.print("Speed ISR Called - Delta_T (us)= ");
  //Serial.println(global_speed_delta_t[global_speed_t_index]);
  #endif
  //interrupts();
  return; 
  
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION: rtc_timer_freq_input_pin_trigger()
// DESCRIPTION: ISR to run when the rtc timer pin goes high
// ARGUMENTS: Null
// OUTPUTS: updates global_current_mode variable
// RETURN VAL: Null
///////////////////////////////////////////////////////////////////////////////
void rtc_timer_freq_input_pin_trigger()
{
  #ifdef TIME_SOURCE_EXT //only do this stuff if we have an external timing source
  digitalWrite(STATUS_LED_PIN, HIGH);
  //make sure we haven't missed a task run
  if(global_run_1HZ_tasks == true)
    global_1HZ_tasks_missed = true;
  else
    global_1HZ_tasks_missed = false;
  
  //tell the main loop that we should run 1Hz tasks ASAP.
  global_run_1HZ_tasks = true;
  
  #ifdef DEBUG_PRINT
  //Serial.println("RTC 1s Task ISR Called");
  #endif
  digitalWrite(STATUS_LED_PIN, LOW);
  #endif
  
  return; 
}


