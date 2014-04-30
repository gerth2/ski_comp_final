    ///////////////////////////////////////////////////////////////////////////////
//                       ECE445 SENIOR DESIGN - UIUC                     
//                         GROUP 1 - SKI SPEDOMETER                      
//                CHEIF ENGINEERS: CHRIS GERTH & BRIAN VAUGHN           
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// FILE: upper_unit_unit_test.ino
// MAJOR REVISION: 0
// MINOR REVISION: See repository version
// DESCRIPTION: unit tests functions to run - not used in final software
///////////////////////////////////////////////////////////////////////////////

void time_unit_test()
{
  boolean temp_isAM;
  byte temp_hour12;
  for(byte i = 0; i < 24; i++)
  { 
    temp_hour12 = cvt_hr_to_12h_format(i, &temp_isAM);
    Serial.print("Hour24 = ");
    Serial.print(i, DEC);
    Serial.print(" Hour12 = ");
    Serial.print(temp_hour12, DEC);
    Serial.print(" AM? = ");
    Serial.print(temp_isAM, BIN);
    Serial.print(" Hour24 = ");
    Serial.println(cvt_hr_to_24h_format(temp_hour12, temp_isAM));
    
  }
  while(1);
  
  
  
}
