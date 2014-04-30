///////////////////////////////////////////////////////////////////////////////
//                       ECE445 SENIOR DESIGN - UIUC                     
//                         GROUP 1 - SKI SPEDOMETER                      
//                CHEIF ENGINEERS: CHRIS GERTH & BRIAN VAUGHN           
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// FILE: upper_unit_user_buttons.ino
// MAJOR REVISION: 0
// MINOR REVISION: See repository version
// DESCRIPTION: contains functions for reading the user input buttons
///////////////////////////////////////////////////////////////////////////////

//Button State Definitions (booleans and chars)
#define BUTTON_DOWN true 
#define BUTTON_UP false
#define BUTTON_NO_CHANGE 2
#define BUTTON_PRESSED 3
#define BUTTON_RELEASED 4
#define NUM_BUTTONS 4

//Button number definitions
#define USER_BUTTON_UP 0 //also doubles as "start/stop record"
#define USER_BUTTON_DOWN 1 //also doubles as "switch display"
#define USER_BUTTON_MENU 2
#define USER_BUTTON_EXIT 3





///////////////////////////////////////////////////////////////////////////////
// FUNCTION: get_user_buttons()
// DESCRIPTION: updates the global data struct with the current state of each
//              of the user input buttons, and sets the flag indicating new
//              data is available.
// ARGUMENTS: Null
// OUTPUTS: Updates:
// global_user_button_states
// global_user_button_prev_states
// global_user_button_deltas
// global_user_button_new_state_available
// RETURN VAL: Null
///////////////////////////////////////////////////////////////////////////////
void get_user_buttons()
{
  int i; //index variables
  int j;
  boolean button_state_stable;
  int debounce_length = 0;
  
  //determine length of debounce
  if(global_current_mode == MODE_MENU)
	debounce_length = BUTTON_DEBOUNCE_LOOPS_MENU;
  else
    debounce_length = BUTTON_DEBOUNCE_LOOPS_NOT_MENU;
  
  //save old state
  for(i = 0; i < NUM_BUTTONS; i++)
  {
    for(j = BUTTON_DEBOUNCE_LOOPS_MENU-1; j > 0 ; j--)
      global_user_button_states[i][j] = global_user_button_states[i][j-1];
    global_user_button_prev_state_debounced[i] = global_user_button_cur_state_debounced[i];
  }
  
  //get new state
  global_user_button_states[0][0] = (boolean)digitalRead(USER_INPUT_UP_PIN);
  global_user_button_states[1][0] = (boolean)digitalRead(USER_INPUT_DOWN_PIN);
  global_user_button_states[2][0] = (boolean)digitalRead(USER_INPUT_MENU_PIN);
  global_user_button_states[3][0] = (boolean)digitalRead(USER_INPUT_EXIT_PIN);
  
  
  
  for(i = 0; i < NUM_BUTTONS; i++)
  { //evaluate debounced button value
    button_state_stable = 1;
    for(j = 0; j < debounce_length; j++)
    {
      if(global_user_button_states[i][j] != global_user_button_states[i][0]) //see if the buffer is not yet all the same. if so, we are not outside the debounce stage yet
        button_state_stable = 0;
    }
    if(button_state_stable)
      global_user_button_cur_state_debounced[i] = global_user_button_states[i][0]; 
    
    
    if(global_user_button_cur_state_debounced[i] == global_user_button_prev_state_debounced[i])
    {
      global_user_button_deltas[i] = BUTTON_NO_CHANGE;
    }
    else if(global_user_button_cur_state_debounced[i] == BUTTON_DOWN && global_user_button_prev_state_debounced[i] == BUTTON_UP)
    {
      #ifdef DEBUG_PRINT_BUTTONS
      Serial.print("Button ");
      Serial.print(i);
      Serial.println(" Pressed.");
      #endif
      global_user_button_deltas[i] = BUTTON_PRESSED; 
    }
    else
    {
      #ifdef DEBUG_PRINT_BUTTONS
      Serial.print("Button ");
      Serial.print(i);
      Serial.println(" Released.");
      #endif
      global_user_button_deltas[i] = BUTTON_RELEASED; 
    }
  }
  
  //set that new data is available
  global_user_button_new_state_available = true;
}


