///////////////////////////////////////////////////////////////////////////////
//                       ECE445 SENIOR DESIGN - UIUC                     
//                         GROUP 1 - SKI SPEDOMETER                      
//                CHEIF ENGINEERS: CHRIS GERTH & BRIAN VAUGHN           
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// FILE: upper_unit_PC_interface.ino
// MAJOR REVISION: 0
// MINOR REVISION: See repository version
// DESCRIPTION: contains functions used to interact with the PC when connected
//////////////////////////////////////////////////////////////////////////////

#define MODE_PC_CONNECT 5 //talk to PC for data exchange
#define EEPROM_MAX_ADDR 0x7FFF //maximum address in eeprom memory address space

///////////////////////////////////////////////////////////////////////////////
// FUNCTION: get_PC_cmd()
// DESCRIPTION: checks if the PC has sent a command, and reads it in if so
// ARGUMENTS: Null
// OUTPUTS: updates global_pc_current_cmd and global_pc_new_cmd_available
// RETURN VAL: Null
// NOTE: does not currently check the validity of what it reads, just reads it in.
///////////////////////////////////////////////////////////////////////////////
void get_pc_cmd()
{
  
  if(Serial.available() > 0 && global_pc_new_cmd_available == 0) //be sure we don't overwrite an unprocessed command
  {
    digitalWrite(STATUS_LED_PIN,HIGH);
    memset(global_pc_current_cmd, 0, sizeof(global_pc_current_cmd)); //clear out command string 
    Serial.readBytesUntil(PC_MSG_TERM_CHAR, global_pc_current_cmd, MAX_PC_MSG_LENGTH);
    global_pc_new_cmd_available = 1;
    digitalWrite(STATUS_LED_PIN,LOW);
  }
  
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION: process_PC_cmd()
// DESCRIPTION: executes actions if the PC has sent a command
// ARGUMENTS: Null
// OUTPUTS: writes to global_pc_new_cmd_available, and possibly serial port
// RETURN VAL: Null
// NOTE:
///////////////////////////////////////////////////////////////////////////////
void process_PC_cmd()
{
  byte temp_buf = 0;
  
  if(global_pc_new_cmd_available == 1 && global_current_mode == MODE_PC_CONNECT)
  {

    if(global_pc_current_cmd[0] == '$' && global_pc_current_cmd[1] == 'I')//command - Identify by printing out what this unit is
      Serial.print("$SKICOMP\n");
    else if(global_pc_current_cmd[0] == '$' && global_pc_current_cmd[1] == 'X') //command - clear EEPROM memory
      EEPROM_clear(); 
    else if(global_pc_current_cmd[0] == '$' && global_pc_current_cmd[1] == 'T') //command - dump EEPROM memory to serial port
    {
      EEPROM_dump_contents_to_PC();
    }
    else
    { //do nothing besides say the command has been recieved
      #ifdef DEBUG_PRINT_PC
      Serial.println("Invalid command recieved:");
      Serial.println(global_pc_current_cmd);
      #endif
    }
  }
  global_pc_new_cmd_available = 0;
  
}
