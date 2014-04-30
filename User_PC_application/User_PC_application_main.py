###############################################################################
##                       ECE445 SENIOR DESIGN - UIUC                     
##                         GROUP 1 - SKI SPEDOMETER                      
##                CHEIF ENGINEERS: CHRIS GERTH & BRIAN VAUGHN           
###############################################################################
##  File - User_PC_Application_Main.py
##  Purpose - Serves as the main computer application for retrieving EEPROM
##            memory dumps from the unit, and displaying them graphically
###############################################################################
import numpy as np #numerical manipluation enviroment (matlab-like)
import pylab as pl #plotting envirment that makes nice graphs
import serial #serial communication library
import sys #basic OS-dependent functions
import os #basic os-specific functions
import time #timing functions


#switch between the proper working directory, depending on whether I'm running
#this on my desktop or laptop. User working directories should be put here too.
workingdir = "B:\Documents\Senior Design\Ski Software Dumpfiles"
if(not(os.path.isdir(workingdir))):
    workingdir = "C:\Users\Chris Gerth\Documents\Senior Design\EEPROM_dump_files"
    
os.chdir(workingdir)

#make sure the user's save-file path exists. Create it if not (ex, first run of this script)
#also change to that directory

print("Ski Computer Main PC Interface")

cur_working_file = 0 #file descriptor to the current working memory dump file
cur_working_file_name = "" #filepath and name for current working memory dump file
cur_working_file_open = False #tells if we have an open memory dump file
exit_flag = 0

while(exit_flag == 0): #main program loop
    #print menu out first, then get user input
    print("Select a menu option:")
    print("1 - Upload Memory Contents from Ski Computer")
    print("2 - Select working Memory Dump")
    print("3 - Display a Run")
    print("4 - Exit")
    selection = input()
    
    #do whatever the user asked
    if(selection == 1):
        portnum = input("Input Comport number of device:")
        print("Connecting to device...")
        #open actual serial port with proper baudrate and a 1s timeout
        try:
            ser = serial.Serial(portnum-1, 115200, timeout=1, stopbits=serial.STOPBITS_TWO) 
        except serial.SerialException as e:
            print("ERROR: could not open requested serial port.")
            continue #go back to start of menu loop
        except ValueError as e:
            print("ERROR: Serial port number out of valid range")
            continue

        
        #note opening a serial port flashes the RTS line, which puts
        #the arduino into a reset cycle.
        
        time.sleep(3) #wait for arduino to reset and boot
  
        #Send PC Connect command, listen for response
        ser.write("$S\n") #send command to connect to PC
        in_str = ser.readline() #listen for proper response
        if(in_str != "$R\n"):
            print("ERROR: did not recieve PC connect response from Ski Computer")
            print(in_str)
            print("Verify connections and try again.")
            ser.close()
            continue

        ser.write("$I\n") #check that this is actually a ski computer
        in_str = ser.readline() #listen for acknowledgement
        if(in_str != "$SKICOMP\n"):
            print("ERROR: identify command did not get proper response.")
            print(in_str)
            print("Did you choose the right comport?")
            ser.close()
            continue

        newfile = raw_input("Input the location for the dump file:")
        newfile = newfile + ".scdmp" #append our file extension
        #open file for writing, catch errors
        try:
            f = open(newfile,'wb')
        except IOError as e:
            print("ERROR: could not create requested file.")
            print(" System Error:({0}): {1}".format(e.errno, e.strerror))
            ser.write("$E\n") #end PC connect session
            ser.close()
            continue
            

        #Send Memory Dump command, record response
        ser.write("$T\n")
        file_good = True
        
        progress_counter = 0
        time.sleep(0.3) #wait for first data to come in from Ski Computer
        EOF_check_buf = [chr(0),chr(0),chr(0),chr(0),chr(0)]
        while(EOF_check_buf != ['$', 'E', 'N', 'D','\n']):
            EOF_check_buf[0] = EOF_check_buf[1] #shift array
            EOF_check_buf[1] = EOF_check_buf[2]
            EOF_check_buf[2] = EOF_check_buf[3]
            EOF_check_buf[3] = EOF_check_buf[4]
            EOF_check_buf[4] = ser.read() #read in next byte
            f.write(EOF_check_buf[4])  #write response to new file
            #print out progress in terms of the EEPROM pages recieved.
            progress_counter = progress_counter + 1
            if(progress_counter % 64 == 0):
                print("Recording Page {0}".format(progress_counter/64))
        
        #handle premature ending
        if(file_good == False):
            #end PC connection
            ser.write("$E\n") #end PC connect session
            #close serial port
            ser.close()
            f.close()
            os.remove(newfile) #delete any data we have already
            continue
            
        
        print("Got all data from unit")
        #close file
        f.close()
        #end PC connection
        ser.write("$E\n") #end PC connect session
        #close serial port
        ser.close()
        #open file for reading
        try:
            if(cur_working_file_open == True):
                    cur_working_file_open = False
                    cur_working_file.close()
            cur_working_file = open(newfile,'rb')
            cur_working_file_open = True
            cur_working_file_name = newfile
            print("New working file set:")
            print(cur_working_file_name)
        except IOError as e:
            print("ERROR: could not open new memory dump file. System error:")
            print(" System Error:({0}): {1}".format(e.errno, e.strerror))
            continue

    elif(selection == 2):
        newfile = str(raw_input("Input file location:"))
        
        try:
            #close current file
            if(cur_working_file_open == True):
                    cur_working_file_open = False
                    cur_working_file.close()
            #open new file for reading
            cur_working_file = open(newfile,'rb')
            cur_working_file_open = True
            cur_working_file_name = newfile
            print("New working file set:")
            print(cur_working_file_name)
        except IOError as e:
            print("ERROR: could not open new memory dump file. System error:")
            print(" System Error:({0}): {1}".format(e.errno, e.strerror))
            continue
        
    elif(selection == 3):
        #verify there is a file to work with
        if(cur_working_file_open == False):
            print("No current file open. Select a memory dump file with option 2")
            continue
        #re-open that file to start reading from the begining 
        try:
            cur_working_file.close()
            cur_working_file = open(cur_working_file_name, 'rb')
        except IOError as e: #catch anything odd with the filesystem
            print("ERROR: could not re-open memory dump file. System error:")
            print(" System Error:({0}): {1}".format(e.errno, e.strerror))
            continue
        #read in the memory descriptor (first 16 bytes of file)
        memory_Descriptor = []
        for i in range(0,16):
            memory_Descriptor.append(ord(cur_working_file.read(1)))
        #check for errors in magic numbers
        if(memory_Descriptor[0] != 0x42 or memory_Descriptor[7] != 0x2F):
            print("ERROR: Memory dump is corrupted. Cannot use this file.")
            cur_working_file.close() #close file and return to menu
            cur_working_file_open = False
            continue
       
        #get number of runs in current working memory dump
        print("There are {0} runs on file.".format(memory_Descriptor[1]))
        run_num = int(input("Select Run Number:"))-1
        
        #use memory structure to extract data to RAM
        if(run_num < 0 or run_num >= memory_Descriptor[1]):
            print("Invalid run number")
            continue
        #file pointer is currently at start of recording descriptors section.
        
        #skip down to desired recording descriptor
        cur_working_file.read(16*run_num)
        
        #read recording descriptor
        recording_Descriptor = []
        for i in range(0,16):
            recording_Descriptor.append(ord(cur_working_file.read(1)))
        data_start_offset = (recording_Descriptor[0xB] + 256 * recording_Descriptor[0xA]) - 0x0210
        num_datapoints = (recording_Descriptor[0x9] + 256 * recording_Descriptor[0x8])
        
        #skip to start of data section
        cur_working_file.read(16*(32-run_num-1))
        #skip to start of actual data for this run
        cur_working_file.read(data_start_offset)
        
        #extract datapoints
        
        datapoint_list = [0]*num_datapoints*8
        for i_iter in range(0,num_datapoints):
            for j_iter in range(0,8):
                #python 2d indexing still escapes me...
                datapoint_list[i_iter*8+j_iter] = ord(cur_working_file.read(1))
        
        cum_time_ms = 0
        sample_time_s = [0]*num_datapoints
        temp_C = [0]*num_datapoints
        Alt_m = [0]*num_datapoints
        Accel_X_G = [0]*num_datapoints
        Accel_Y_G = [0]*num_datapoints
        Accel_Z_G = [0]*num_datapoints
        Speed_mph = [0]*num_datapoints
        #make vectors of real-world data to plot
        for i_iter in range(0,num_datapoints):
            
            sample_time_s[i_iter] = cum_time_ms/1000
            delta_t = datapoint_list[i_iter*8 + 0]
            delta_t = delta_t * 50
            cum_time_ms = delta_t + cum_time_ms
            
            temp_C[i_iter] = datapoint_list[i_iter*8+1]
            if(temp_C[i_iter] > 0x7F): #convert to signed integer
                temp_C[i_iter] = -(256 - temp_C[i_iter])
                
            Alt_m[i_iter] = datapoint_list[i_iter*8+2]*256+datapoint_list[i_iter*8+3]
            
            Accel_X_G[i_iter] = datapoint_list[i_iter*8+4]
            if(Accel_X_G[i_iter] > 0x7F): #convert to signed integer
                Accel_X_G[i_iter] = -(256 - Accel_X_G[i_iter])
            Accel_X_G[i_iter] = Accel_X_G[i_iter]*0.0625

            Accel_Y_G[i_iter] = datapoint_list[i_iter*8+5]
            if(Accel_Y_G[i_iter] > 0x7F): #convert to signed integer
                Accel_Y_G[i_iter] = -(256 - Accel_Y_G[i_iter])
            Accel_Y_G[i_iter] = Accel_Y_G[i_iter]*0.0625

            Accel_Z_G[i_iter] = datapoint_list[i_iter*8+6]
            if(Accel_Z_G[i_iter] > 0x7F): #convert to signed integer
                Accel_Z_G[i_iter] = -(256 - Accel_Z_G[i_iter])
            Accel_Z_G[i_iter] = Accel_Z_G[i_iter]*0.0625

            Speed_mph[i_iter] = datapoint_list[i_iter*8+7]*0.5
                    
        #generate timestamp
        Months_of_year = ["Jan", "Feb","Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"]
        Am_Or_Pm = ["AM","PM"]
        Sec = recording_Descriptor[1]
        Min = recording_Descriptor[2]
        Hour = recording_Descriptor[3]
        AmPm = Am_Or_Pm[recording_Descriptor[4]]
        Date = recording_Descriptor[5]
        Month = Months_of_year[recording_Descriptor[6]-1]
        Year = 2000 + recording_Descriptor[7]
        Timestamp = "{0} {1}, {2} - {3:02d}:{4:02d}:{5:02d} {6}".format(Month, Date, Year, Hour, Min, Sec, AmPm)

        
        #plot data
        pl.figure()
        pl.plot(sample_time_s, temp_C)
        pl.title("Temperature - {0}".format(Timestamp))
        pl.xlabel("Time (s)")
        pl.ylabel("Temp. (C)")
        pl.show()

        pl.figure()
        pl.plot(sample_time_s, Alt_m)
        pl.title("Altitude - {0}".format(Timestamp))
        pl.xlabel("Time (s)")
        pl.ylabel("Altitude (m)")
        pl.show()

        pl.figure()
        pl.plot(sample_time_s, Speed_mph)
        pl.title("Speed - {0}".format(Timestamp))
        pl.xlabel("Time (s)")
        pl.ylabel("Speed (mph)")
        pl.show()

        pl.figure()
        xplot, = pl.plot(sample_time_s, Accel_X_G)
        yplot, = pl.plot(sample_time_s, Accel_Y_G)
        zplot, = pl.plot(sample_time_s, Accel_Z_G)
        pl.title("3D Acceleration - {0}".format(Timestamp))
        pl.xlabel("Time (s)")
        pl.ylabel("Acceleration (G-force)")
        pl.legend((xplot,yplot,zplot),("X-Axis", "Y-Axis", "Z-Axis"), 'best')
        pl.show()



        cur_working_file.close() #close file and return to menu
        break #exit so plots display

        

    #DEBUG ONLY
    elif(selection == 5):
        f = open('testwrite.scdmp','wb')
        for i in range(0,0xFF):
            f.write(chr(i))
        f.close()
        continue
        
    else:
        if(cur_working_file_open == True):
            cur_working_file.close() #close file and return to menu
            cur_working_file_open = False
        exit_flag = 1 #get the heck outta here, we're done!
        

#Program will now return
