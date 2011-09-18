// DS2764.cpp
#include <Wire.h>
#include <avr/pgmspace.h>
#include <WProgram.h>
#include <util/delay.h>
#include <stdlib.h>

#include "DS2764.h"


// Public Methods
void DS2764::dsInit(void) {

    // initialize all member variables
    miProtect           = 0;
    miStatus            = 0;
    miVolts             = 0;
    miBatteryCapacity   = 0;
    mbPowerOn           = false;
    mbPowerSwitchOn     = false;
    mbSleepEnabled      = false;
    mfCurrent           = 0.0;
    miAccCurrent        = 0;
    mfTempC             = 0.0;
    mfTempF             = 0.0;
    
    
    mbPowerOn           = true;
    dsResetProtection(DS_RESET_ENABLE);
    dsSetPowerSwitchOn();
    
    dspGetBatteryCapacity();
    dspGetProtection();
    dspGetVoltageAndCurrent();  
    dspGetTemp();
    
}   // end init()




int DS2764::dsGetAccumulatedCurrent(void) {
    return miAccCurrent;
}

int DS2764::dsGetBatteryVoltage(void) {
    return miVolts;
}
    
int DS2764::dsGetBatteryCapacity(void) {
    return miBatteryCapacity;
}


float DS2764::dsGetBatteryCapacityPercent(void) {
    return (((float) miAccCurrent / (float) miBatteryCapacity) * 100.0);
}






int DS2764::dsGetChargeStatus(void) {

    if (miProtect & DS00COC) { 
          //Serial.println("   Charge Current: *** OVER  ***"); 
          return DS_CHARGE_CURRENT_HI;
    }
    else {
          //Serial.println("   Charge Current: OK");
          return DS_CHARGE_CURRENT_OK;
    }
}
 

 
int DS2764::dsGetDischargeStatus(void) {
 
    if (miProtect & DS00DOC) { 
        //Serial.println("Discharge Current: *** OVER  ***"); 
        return DS_DISCHARGE_CURRENT_HI;
        }
    else {
        //Serial.println("Discharge Current: OK");
        return DS_DISCHARGE_CURRENT_OK;
    }
}





        

float DS2764::dsGetCurrent(void) {
    return mfCurrent;
}







float   DS2764::dsGetTempC(void) {
    return mfTempC;
}

float   DS2764::dsGetTempF(void) {
    return mfTempF;
}




int DS2764::dsGetVoltageStatus(void) {

    if (!(miProtect & (DS00OV + DS00UV))) {
        //Serial.println("      Voltage: OK");
        return DS_VOLTS_OK;
    }
    else if (miProtect & DS00OV) { 
        //Serial.println("      Voltage: *** OVER  ***"); 
        return DS_VOLTS_HI;
    }
    else if (miProtect & DS00UV) { 
        //Serial.println("      Voltage: *** UNDER ***"); 
        return DS_VOLTS_LOW;
    }
    else {
        return 0;
    }
}



void DS2764::dsDisableSleep(void) {
    dspSetSleepMode(DS_SLEEP_DISABLED);
}

void DS2764::dsEnableSleep(void) {
    dspSetSleepMode(DS_SLEEP_ENABLED);
}

boolean DS2764::dsIsChargeOn(void) {
    return !(miProtect & DS00CC);
}

boolean DS2764::dsIsChargeEnabled(void) {
    return (miProtect & DS00CE);
}

boolean DS2764::dsIsDischargeOn(void) {
    return !(miProtect & DS00DC);
}

boolean DS2764::dsIsDischargeEnabled(void) {
    return (miProtect & DS00DE);
}




boolean DS2764::dsIsPowerOn(void) {
    return mbPowerOn;   
}


        
boolean DS2764::dsIsSleepEnabled(void) {
// check bit 5 in the Status Register.
// this value is read only, but the default
// value is set in Byte 2 of EEPROM block 1, aka Addr 0x31
    return (miStatus & DS00SLP);
}




void DS2764::dsRefresh(void) {
    
    dspHandlePower();            // check if powerbutton was pushed
    dspGetProtection();
    dspGetVoltageAndCurrent();  
    dspGetTemp();
}



// Apparently the Charge and Discharge Over Current flags do not get reset by this function,
// but by the chip once the problem is corrected.
void DS2764::dsResetProtection(int aiOn) {
    int dsProtect  = 0;
    int dsStatus   = 0;

    // Read Protection Register
    
    if(aiOn == DS_RESET_ENABLE) {
        dsProtect = DS_PROTECTION_CLEAR_ENABLE;     //Clear OV and UV and enable both charge and discharge
    }
    else {
        dsProtect = DS_PROTECTION_CLEAR_DISABLE;    //Clear OV and UV and disable both charge and discharge
    }
    
    //Serial.print("About to send Protection Flags: ");
    //Serial.println(lowByte(dsProtect), HEX);
    
    Wire.beginTransmission(DS_ADDRESS);
    Wire.send(DS_PROTECTION_REGISTER);
    Wire.send(lowByte(dsProtect));      
    Wire.endTransmission();
    //delay(10);
    Wire.requestFrom(DS_ADDRESS, 2);
    if(1 <= Wire.available())                    //if two bytes were received 
    { 
        miProtect = Wire.receive();
        miStatus  = Wire.receive(); 
    }
    else {
      //  Serial.println("Nothing received from resetdsProtection Request");
        
        miProtect = -1;
        miStatus  = -1;        
    }
    //Serial.print("dsResetProtection miProtect: ");
    //Serial.println(miProtect, HEX);
    //Serial.print("dsResetProtection  miStatus: ");
    //Serial.println(miStatus, HEX);

}







void DS2764::dsSetAccumCurrent(int iNewVal) {
  
    // Convert our new value in mAh to units of .25mAh
    int acurrent = iNewVal / 0.25;
    byte loByte = 0;
    byte hiByte = 0;
        
    loByte = acurrent & 0x00FF;
    hiByte = acurrent >> 8;
        
    // Send Data to Accumulated Current Variable
    Wire.beginTransmission(DS_ADDRESS);
    Wire.send(DS_ACC_CURRENT_REG_HI);
    delay(5);
        
    Wire.send(hiByte);
    Wire.send(loByte);    
    Wire.endTransmission();
    
    miAccCurrent = iNewVal;

}









void DS2764::dsSetBatteryCapacity(int aiValue) {

    int     iCapacity   = 0;
    byte    bHi         = 0;
    byte    bLow        = 0;
    byte    bCheck      = 0;
    byte    bFill       = 0xA;

    // Recall EEPROM Block 0 Data to Shadow RAM
    // This automatically happens at Power Up, so this
    // step probably isn't necessary.
    Wire.beginTransmission(DS_ADDRESS);
    Wire.send(DS_FUNCTION_REGISTER);        // Write value to Function Command Address
    Wire.send(DS_RECALL_EEPROM_BLK_0);      // Request refresh of Block 0 Shadow RAM from EEPROM
    Wire.endTransmission();
    delay(500);
        
        
    // Read first 4 bytes of Block 0 Shadow RAM
    Wire.beginTransmission(DS_ADDRESS);
    Wire.send(DS_BATTERY_CAP_ADDR);
    Wire.endTransmission();
    delay(100);
    Wire.requestFrom(DS_ADDRESS, 4);

    if(4 <= Wire.available()) { 
        bHi     = Wire.receive();
        bLow    = Wire.receive();
        bCheck  = Wire.receive();
        bFill   = Wire.receive();
    }

    // Calculate our checksum by OR'ing the hi and low
    // bytes of the battery capacity.
    bCheck  = highByte(aiValue) ^ lowByte(aiValue);
    
    // Sat the last byte to 0xA == 1010 in binary
    // as a marker to indicate that we set these 4 bytes
    // in the Gas Gauge Memory and to even out
    // our memory usage to an even number of bytes.
    bFill   = 0xA;
        
    // Write the data back to Address 0x20, which is the
    // "shadow RAM" for EEPROM Block 0.    
    Wire.beginTransmission(DS_ADDRESS);
    Wire.send(DS_BATTERY_CAP_ADDR);
    Wire.send(highByte(aiValue));
    Wire.send(lowByte(aiValue));
    Wire.send(bCheck);
    Wire.send(bFill);
    Wire.endTransmission();
    delay(10);
        
    // Now that we've written to the Shadow RAM for EEPROM
    // Block 0, we need to ask the Gas Gauge to Save the
    // Block 0 Shadow RAM back to the EEPROM memory.    
    Wire.beginTransmission(DS_ADDRESS);
    Wire.send(DS_FUNCTION_REGISTER);        // Write value to Function Command Address
    Wire.send(DS_SAVE_EEPROM_BLK_0);        // Request Write of Block 0 Shadow RAM back into EEPROM
    Wire.endTransmission();
    delay(10);
        
    miBatteryCapacity = aiValue;
}



void DS2764::dsSetPowerSwitchOn(void) {
    dspSetPowerSwitchOn();
}







// private methods








//------------------------------------------------------------------------------
// dspGetBatteryCapacity
//
// Retrieves the Battery Capacity in mAh from the Gas Gauge Chip's EEPROM Block
// 0 memory that was saved previously.  Block 0 starts at address 0x20.
//
// The first 2 bytes hold the Battery Capacity in mAh
//
// The third byte is a checksum (calculated by XOR'ing the first two bytes
// together.
//
// The 4th and final byte is the value 0xA which is used to indicate that
// these 4 bytes contain a battery capacity that was saved previously.
// 
//
// Arguments:
//     None
//
//------------------------------------------------------------------------------
void DS2764::dspGetBatteryCapacity() { 

    byte    bHi         = 0;
    byte    bLow        = 0;
    byte    bCheck      = 0;
    byte    bFill       = 0;

    // Read 4 bytes from the start of EEPROM Address 20, which is 
    // EEPROM Block 0.  First two bytes should be the battery
    // capacity in mAH, and the next byte should be the first
    // two bytes ORed together.  The 4th byte should be 0xA to
    // indicate that this is memory that's been set by this program
    // 
    Wire.beginTransmission(DS_ADDRESS);
    Wire.send(DS_BATTERY_CAP_ADDR);
    Wire.endTransmission();
    
    Wire.requestFrom(DS_ADDRESS, 4);
    
    if(4 <= Wire.available()) { 
        bHi     = Wire.receive();
        bLow    = Wire.receive();
        bCheck  = Wire.receive();
        bFill   = Wire.receive();  // should be set to 0xA
        
        if ((bHi ^ bLow == bCheck) && bFill == 0xA) {
            // checksum matches and our filler character
            // matches.
            miBatteryCapacity = word(bHi, bLow);
        }
        else {
            // these 4 bytes may not have been initialized
            // with the battery capacity yet, use the
            // 10 mAh as a default
            //Serial.println("Defaulting Battery Capacity to 999 mAH");
            miBatteryCapacity = 10;
        }
        
        if(miBatteryCapacity < 1) {
            // always set capacity to 1 if it's less than 1
            // this prevents divide by zero problems when
            // calculating remaining battery percent.
        	miBatteryCapacity = 1;
        }
    }
}








//------------------------------------------------------------------------------
// dspGetPowerSwitch
//
// Retrieves the value of the PowerSwitch bit, which is Bit 7 of the 
// special features register at address 0x08.  Only the Arduino can set 
// this bit to 1, but the bit will be set to 0 is the Power Button is pushed,
// which brings the voltage on the PS pin of the chip to LOW.
// 
//
// Arguments:
//     None
//
// Return Value:
//     1 if the bit value is 1, meaning that the power is "on"
//     0 if the bit value is 0, signaling a request to turn power off.
//    -1 if no value retrieved
//------------------------------------------------------------------------------
void DS2764::dspGetPowerSwitch(void) {
    int dsSpecial  = 0;

    // Read Protection Register
    Wire.beginTransmission(DS_ADDRESS);
    Wire.send(DS_SPECIAL_FEATURE_REG);
    Wire.endTransmission();
    Wire.requestFrom(DS_ADDRESS, 1);
    if(1 <= Wire.available()) {                 // if one byte was received 

        dsSpecial = Wire.receive();

        if (dsSpecial & DS00PS) { 
            //Serial.println("PsON");
            mbPowerSwitchOn = true;
        }
        else {
            //Serial.println("PsOFF");
            mbPowerSwitchOn = false;
        }
    }
}





//------------------------------------------------------------------------------
// dspGetProtection
//
// Gets the Protection Flags and Status Settings from the Gas Gauge Chip
// memory.
//
// The protection flags include:
// Charging Enabled
// Charging On/Off
// Charging Over Current Threshold
//
// Discharge Enabled
// Discharge On/Off
// Discharge Current Over Threshold
//
// Over Voltage
// Under Voltage
//
// The Status Flags indicate if Sleep Mode is Enabled or Disabled.  When
// the gas gauge chip is "sleeping", less power is drained from the Lipo
// battery.
//
// Arguments:
//     None
//
// Return Value:
//     int - byte containing the Protection Flags.  Also sets giProtect and 
//           giStatus global variables.
//------------------------------------------------------------------------------
void DS2764::dspGetProtection(void) {

    // Read Protection Register
    Wire.beginTransmission(DS_ADDRESS);
    Wire.send(DS_PROTECTION_REGISTER);
    Wire.endTransmission();
    Wire.requestFrom(DS_ADDRESS, 2);
    if(2 <= Wire.available()) {     // if two bytes were received 
    
        miProtect = Wire.receive();
        miStatus  = Wire.receive();
        
        /*
        Serial.println("from getdsProtection");
        Serial.print("Status Register from Address 01h: ");
        Serial.println(dsStatus, HEX);
        Serial.print("     Status Register in giStatus: ");
        Serial.println(giStatus, HEX);
        */
        
#ifdef DEBUG && (DEBUG > 1)
        if (!miProtect & (DS00OV + DS00UV)) {
          Serial.println("      Voltage: OK");
        }
        else if (miProtect & DS00OV) { 
          Serial.println("      Voltage: *** OVER  ***"); 
        }
        else if (miProtect & DS00UV) { 
          Serial.println("      Voltage: *** UNDER ***"); 
        }
        
        if (miProtect & DS00COC) { 
          Serial.println("   Charge Current: *** OVER  ***"); 
        }
        else {
          Serial.println("   Charge Current: OK");
        }
        
        if (miProtect & DS00DOC) { 
          Serial.println("Discharge Current: *** OVER  ***"); 
        }
        else {
          Serial.println("Discharge Current: OK");
        }
        
        
        if (miProtect & DS00CC) { 
            Serial.println("       Charging: *** OFF ***");
        }
        else {
            Serial.println("       Charging: ON");
        }
        
        if (miProtect & DS00CE) { 
            Serial.println("       Charging: Enabled"); 
        }
        else {
            Serial.println("       Charging: Disabled");
        }
    
        if (miProtect & DS00DC) { 
            Serial.println("    Discharging: *** OFF ***"); 
        }
        else {
            Serial.println("    Discharging: ON");
        }
        
        if (miProtect & DS00DE) { 
            Serial.println("    Discharging: Enabled");
        }
        else {
            Serial.println("    Discharging: Disabled");
        }
        
        if (miProtect & DS00SLP) { 
            Serial.println("     Sleep mode: Enabled");
        }
        else {
            Serial.println("     Sleep mode: Disabled");
        }
#endif
    }
    else {
        //Serial.println("Nothing Received from Get Protection Settings Request");
        miProtect    = -1;
        miStatus     = -1;

    }

    //Serial.print("dsStatus & DS00SLP: ");
    //Serial.print(dsStatus, HEX);
    //Serial.print(" ");
    //Serial.println(DS00SLP, HEX);
}







//------------------------------------------------------------------------------
// dspGetTemp
//
// Gets the Temperature from the Gas Gauge Chip
//
// Arguments:
//     None
//
// Return Value:
//     float - the temperature in Celsius
//------------------------------------------------------------------------------
void DS2764::dspGetTemp(void) {
    int reading = 0;
    
    // Read Voltage Register
    Wire.beginTransmission(DS_ADDRESS);
    Wire.send(DS_TEMP_REG_HIBYTE);
    Wire.endTransmission();

    Wire.requestFrom(DS_ADDRESS, 2);
    
    if(2 <= Wire.available()) {     // if two bytes were received 
        reading = Wire.receive();   // receive high byte (overwrites previous reading) 
        reading = reading << 8;     // shift high byte to be high 8 bits 
        reading += Wire.receive();  // receive low byte as lower 8 bits 
        reading = reading >> 5;
         
        mfTempC = reading * 0.125;
        
        // Convert temperature to Fahrenheit        
        mfTempF  = (mfTempC * 9.0 / 5.0) + 32.0;
    }
    else {
        //Serial.println("Nothing Received from Get Temp Request");
        mfTempC = 0.0;
        mfTempF = 0.0;
    }
}





//------------------------------------------------------------------------------
// dspGetVoltageAndCurrent
//
// This actually gets the current Voltage, current Current Draw, and the
// Accumulated Current Count.  Since the registers are all next to each other
// in gas gauge memory, it's easiest to get them all with one request.
// 
//
// Arguments:
//     None
//
// Return Value:
//     float - The current Voltage, but routine also sets the giVolts, 
//             gdCurrent, and giAccCurrent Global Variables.
//------------------------------------------------------------------------------
void DS2764::dspGetVoltageAndCurrent(void) {
    int voltage   = 0;
    int current   = 0;
    int acurrent  = 0;

    // Read Voltage Register
    Wire.beginTransmission(DS_ADDRESS);
    Wire.send(DS_VOLT_REG_HIBYTE);
    Wire.endTransmission();

    // read 6 bytes, hi and lo byte for voltage
    // hi and low byte for current
    // hi and low byte for accumulated current
    Wire.requestFrom(DS_ADDRESS, 6);
    delay(4);
    if(6 <= Wire.available())     // if six bytes were received 
    { 
        voltage = Wire.receive();
        voltage = voltage << 8;
        voltage += Wire.receive();
        voltage = voltage >> 5;
        voltage = voltage * 4.88;
        
        current = Wire.receive();
        current = current << 8;
        current += Wire.receive();
        
        acurrent = Wire.receive();
        acurrent = acurrent << 8;
        acurrent += Wire.receive();
        
        if ((current & 0x80) == 0x80) {
            current = (current ^ 0xFFFFFFFF);
            current = current * -1;
        }
        
        current = current >> 3;

        double c = (current * 0.625);
        
        acurrent = acurrent * 0.25;

        miVolts      = voltage;
        mfCurrent    = c;
        miAccCurrent = acurrent;
    }
    else {
        //Serial.println("Nothing received from get Voltage Request");

        miVolts         = 0;
        mfCurrent       = 0.0;
        miAccCurrent    = 0;
    }
}






void DS2764::dspHandlePower(void) {  
  
    dspGetPowerSwitch();  

    if ((!mbPowerSwitchOn) && mbPowerOn) {
        // power down
        mbPowerOn = false;
        dsResetProtection(DS_RESET_DISABLE);
        dspSetPowerSwitchOn();    // so we can detect when it's pushed again.
    }
    else if((!mbPowerSwitchOn) && (!mbPowerOn)) {
        // power up
        mbPowerOn = true;
        dsResetProtection(DS_RESET_ENABLE);
        dspSetPowerSwitchOn();
    }

}





//------------------------------------------------------------------------------
// dspSetPowerSwitchOn
//
// Sets the value of the PowerSwitch bit, which is Bit 7 of the 
// special features register at address 0x08, to 1.  Only the Arduino can set 
// this bit to 1, but the bit will be set to 0 if the Power Button is pushed,
// which brings the voltage on the PS pin of the chip to LOW.
// 
//
// Arguments:
//     None
//
// Return Value:
//     1 if the bit value was set to 1, meaning that the power is "on"
//     0 if the bit value was not set and is still 0
//    -1 if no value was returned when trying to confirm the setting was made.
//------------------------------------------------------------------------------
void DS2764::dspSetPowerSwitchOn(void) {
    int dsSpecial  = 0;

    // Read Protection Register
    Wire.beginTransmission(DS_ADDRESS);
    Wire.send(DS_SPECIAL_FEATURE_REG);
    Wire.send(DS00PS);
    Wire.endTransmission();
    delay(10);
    Wire.requestFrom(DS_ADDRESS, 1);
    if(1 <= Wire.available())     // if one byte was received 
    { 
        dsSpecial = Wire.receive();

        if (dsSpecial & DS00PS) { 
            //Serial.println("PsSetON");
            mbPowerSwitchOn = true;
        }
        else {
            //Serial.println("PsSetOFF");
            mbPowerSwitchOn = false;
        }      
    }
}









//------------------------------------------------------------------------------
// dspSetSleepMode
//
// Sets the default value for sleep mode on or off depending on the value
// passed in.
//
// The setting for Sleep Mode is set in EEPROM Block 1 in the 5th bit of 
// address 31h.
//
// The effective setting for Sleep Mode is in the Status Register, bit 5
// of address 01h, but this bit is read-only.
// 
//
// Arguments:
//     int aiValue - 1 if sleep mode should be enabled, 0 if it should be
//                   disabled.
//
// Return Value:
//     1 if the bit value was set to 1, meaning that the power is "on"
//     0 if the bit value was not set and is still 0
//    -1 if no value was returned when trying to confirm the setting was made.
//------------------------------------------------------------------------------
void DS2764::dspSetSleepMode(int aiValue) {
    int iReturn    = 0;
    int iJunk      = 0;
    int iShadow    = 0;
        
    // set in bit 5 in Address 31h
        
    // Recall EEPROM Block 1 Data to Shadow RAM
    Wire.beginTransmission(DS_ADDRESS);
    Wire.send(DS_FUNCTION_REGISTER);            // Write value to Function Command Address
    Wire.send(DS_RECALL_EEPROM_BLK_1);          // Request refresh of Block 1 Shadow RAM from EEPROM
    Wire.endTransmission();
    delay(500);
        
        
    // Read second byte of Block 1 Shadow RAM
    Wire.beginTransmission(DS_ADDRESS);
    Wire.send(DS_SLEEP_MODE_ADDR);
    Wire.endTransmission();
    delay(100);
    Wire.requestFrom(DS_ADDRESS, 1);

    if(1 <= Wire.available()) { 
        iShadow = Wire.receive();
        //Serial.print("BEFORE Address 31h, Shadow Block 1 Byte 1: ");
        //Serial.println(iShadow, HEX);
        
        //Serial.print("BEFORE giStatus Variable from Address 02h: ");
        //Serial.println(miStatus, HEX);
        }
    //else {
    //    Serial.println("NOData on Sleep Bit");
    //}

    // set data
    if(aiValue > 0) {
        //Serial.print("BEFORE Shadow OR 0x20: ");
        //Serial.print(iShadow, HEX);
        //Serial.print(" | ");
        //Serial.println(DS00SLP, HEX);
        iShadow = iShadow | DS00SLP;  // This ensures bit 5 is set and nothing else is changed.
    }
    else {
        //Serial.print("BEFORE Shadow XOR 0x20: ");
        //Serial.print(iShadow, HEX);
        //Serial.print(" ^ ");
        //Serial.println(DS00SLP, HEX);
        iShadow = iShadow ^ DS00SLP;  // this flips bit 5 to 0 if it was 1, which ensure sleep mode is disabled   
    }
    
    //Serial.print("AFTER, value to send      : ");
    //Serial.println(iShadow, HEX);
    
//#ifdef DEBUG
//    Serial.print("Setting Sleep Mode - about to send: ");
//    Serial.println(iShadow, HEX);
//#endif
        
    Wire.beginTransmission(DS_ADDRESS);
    Wire.send(DS_SLEEP_MODE_ADDR);
    Wire.send(iShadow);
    Wire.endTransmission();
    delay(10);
        
        
    // copy data back into EEPROM
    Wire.beginTransmission(DS_ADDRESS);
    Wire.send(DS_FUNCTION_REGISTER);        // Write value to Function Command Address
    Wire.send(DS_SAVE_EEPROM_BLK_1);        // Request Write of Block 1 Shadow RAM back into EEPROM
    Wire.endTransmission();
    delay(1000);
    
    // now, that we've updated the EEPROM memory, lets request a refresh of the shadow memory.
    // do another EEPROM refresh.
    
    // Recall EEPROM Block 1 Data to Shadow RAM
    Wire.beginTransmission(DS_ADDRESS);
    Wire.send(DS_FUNCTION_REGISTER);          // Write value to Function Command Address
    Wire.send(DS_RECALL_EEPROM_BLK_1);        // Request refresh of Block 1 Shadow RAM from EEPROM
    Wire.endTransmission();
    delay(1000);
        
        
    // Read second byte of Block 1 Shadow RAM
    Wire.beginTransmission(DS_ADDRESS);
    Wire.send(DS_SLEEP_MODE_ADDR);
    Wire.endTransmission();
    delay(100);
    Wire.requestFrom(DS_ADDRESS, 1);

    if(1 <= Wire.available()) { 
        iShadow = Wire.receive();
        //Serial.print("Address 31h - Shadow After Update and Refresh Block 1 Byte 1: ");
        //Serial.println(iShadow, HEX);
        }
    //else {
    //    Serial.println("NOData on Sleep Bit");
    //}
    
    if((iShadow & DS00SLP) > 0) {
        //iReturn = 1;
        mbSleepEnabled = true;
    }
    else {
        //iReturn = 0;
        mbSleepEnabled = false;
    }

}











