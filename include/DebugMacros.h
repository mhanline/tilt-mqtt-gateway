
//***************************************************************
// From: https://forum.arduino.cc/index.php?topic=215334.15
//examples:
//  This  converts to >>>>----------------------->  This OR a Blank Line.  
// DPRINTLN("Testing123");                          Serial.println("Testing123");  
// DPRINTLN(0xC0FFEEul,DEC);                        Serial.println(0xC0FFEEul,DEC);
// DPRINTLN(12648430ul,HEX);                        Serial.println(12648430ul,HEX);
// DPRINTLNF("This message came from flash");       Serial.println(F("This message came from your flash"));
// DPRINT(myVariable);                              Serial.print(myVariable);
// DELAY(100);                                      delay(100);
// PINMODE(9600);                                   pinMode(9600);
//
// Also, this works  #define INFO(...)  { Console->printf("INFO: "); Console->printf(__VA_ARGS__); }   >>>--->   where {} allows multiple lines of code.
// See: http://forum.arduino.cc/index.php?topic=511393.msg3485833#new

#ifndef DEBUGMACROS_H
#define DEBUGMACROS_H
#ifdef DEBUGMACROS
//examples:
#define SERIALBEGIN(...)   Serial.begin(__VA_ARGS__)
#define DPRINT(...)        Serial.print(__VA_ARGS__)
#define DPRINTLN(...)      Serial.println(__VA_ARGS__)
#define DPRINTF(...)       Serial.print(F(__VA_ARGS__))
#define DPRINTLNF(...)     Serial.println(F(__VA_ARGS__))
#define DELAY(...)         delay(__VA_ARGS__)
#define PINMODE(...)       pinMode(__VA_ARGS__)
#define DPRINTFMT(...)      Serial.printf(__VA_ARGS__)

#define DEBUG_PRINT(...)   Serial.print(F(#__VA_ARGS__" = ")); Serial.print(__VA_ARGS__); Serial.print(F(" "))
#define DEBUG_PRINTLN(...) DEBUG_PRINT(__VA_ARGS__); Serial.println()

//***************************************************************
#else
#define SERIALBEGIN(...)  
#define DPRINT(...)      
#define DPRINTLN(...)    
#define DPRINTF(...)      
#define DPRINTLNF(...)    
#define DELAY(...)        
#define PINMODE(...)          
#define DPRINTFMT(...)

#define DEBUG_PRINT(...)    
#define DEBUG_PRINTLN(...)  

#endif

#endif
//***************************************************************
