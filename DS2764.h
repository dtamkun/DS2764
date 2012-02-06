//DS2764.h
// DMT 2/5/2012 - Arduino 1.0 Compatibility

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif


// constants
//Bit Masks for Gas Gauge Settings
#define DS00PS        			0x80	// Power Switch bit        in bit 7 of the Special Features Register
#define DS00OV        			0x80	// Over Voltage Indicator  in bit 7 of the Protection Reg.
#define DS00UV        			0x40	// Under Voltage Indicator in bit 6 of the Protection Reg.
#define DS00COC       			0x20	// Charge Over Current     in bit 5 of the Protection Reg.
#define DS00DOC       			0x10	// Discharge Over Current  in bit 4 of the Protection Reg.
#define DS00CC        			0x08	// Charging On/Off         in bit 3 of the Protection Reg.
#define DS00DC        			0x04	// Discharge On/Off        in bit 2 of the Protection Reg.
#define DS00CE        			0x02	// Charging Enabled        in bit 1 of the Protection Reg.
#define DS00DE        			0x01	// Discharging Enabled     in bit 0 of the Protection Reg.
#define DS00SLP       			0x20	// Sleep Enable bit        in bit 5 of the Status Register
#define DS_ADDRESS			0x34	// Default i2c Address for the chip.

// Registers and Memory Locations for various settings
#define DS_PROTECTION_REGISTER	        0x00
#define DS_STATUS_REGISTER		0x01
#define DS_SPECIAL_FEATURE_REG	        0x08
#define DS_VOLT_REG_HIBYTE		0x0C
#define DS_VOLT_REG_LOBYTE		0x0D
#define DS_CURRENT_REG_HIBYTE	        0x0E
#define DS_CURRENT_REG_LOBYTE	        0x0F
#define DS_ACC_CURRENT_REG_HI	        0x10
#define DS_ACC_CURRENT_REG_LO	        0x11
#define DS_TEMP_REG_HIBYTE		0x18
#define DS_TEMP_REG_LOBYTE		0x19
#define DS_EEPROM_BLOCK0_START	        0x20
#define DS_BATTERY_CAP_ADDR		0x20
#define DS_EEPROM_BLOCK1_START	        0x30
#define DS_CURRENT_OFFSET_REG	        0x33
#define DS_EEPROM_BLOCK2_START	        0x40
#define DS_FUNCTION_REGISTER	        0xFE
#define DS_SLEEP_MODE_ADDR		0x31	//in 2nd byte of EEPROM Block 1

// Function Commands - write to DS_FUNCTION_REGISTER to invoke
// SAVE/COPY   => Copy from Shadow RAM to EEPROM's non-volital storage
// RECALL      => Copy from EEPROM to Shadow RAM
#define DS_SAVE_EEPROM_BLK_0	    0x42
#define DS_SAVE_EEPROM_BLK_1	    0x44
#define DS_SAVE_EEPROM_BLK_2	    0x48
#define DS_RECALL_EEPROM_BLK_0	    0xB2
#define DS_RECALL_EEPROM_BLK_1	    0xB4
#define DS_RECALL_EEPROM_BLK_2	    0xB8

#define DS_PROTECTION_CLEAR_ENABLE     0x03	// internal use
#define DS_PROTECTION_CLEAR_DISABLE    0x00	// internal use

#define DS_RESET_ENABLE		        1	// use with dsResetProtection -  enables charge and discharge
#define DS_RESET_DISABLE	        0	// use with dsResetProtection - disables charge and discharge


#define DS_VOLTS_LOW			1
#define DS_VOLTS_OK			2
#define DS_VOLTS_HI			3

#define DS_CHARGE_CURRENT_OK	1
#define DS_CHARGE_CURRENT_HI	2

#define DS_DISCHARGE_CURRENT_OK	1
#define DS_DISCHARGE_CURRENT_HI	2

#define DS_CHARGE_ON		1
#define DS_CHARGE_OFF		0
#define DS_CHARGE_ENABLED	1
#define DS_CHARGE_DISABLED	0

#define DS_DISCHARGE_ON		1
#define DS_DISCHARGE_OFF	0
#define DS_DISCHARGE_ENABLED	1
#define DS_DISCHARGE_DISABLED	0

#define DS_POWER_SWITCH_ON	1
#define DS_POWER_SWITCH_OFF	0

#define DS_SLEEP_ENABLED	1
#define DS_SLEEP_DISABLED	0


class DS2764 {

    public:
        void	dsInit(void);
	void	dsRefresh(void);
	void    dsResetProtection(int);
	float	dsGetCurrent(void);
	int	dsGetAccumulatedCurrent();
	int	dsGetBatteryVoltage(void);
	int	dsGetBatteryCapacity(void);
	float	dsGetBatteryCapacityPercent(void);
	int	dsGetVoltageStatus(void);
	int	dsGetChargeStatus(void);
	int	dsGetDischargeStatus(void);
	boolean	dsIsChargeOn(void);
	boolean	dsIsChargeEnabled(void);
	boolean	dsIsDischargeOn(void);
	boolean	dsIsDischargeEnabled();
	float	dsGetTempF(void);
	float	dsGetTempC(void);
	boolean	dsIsSleepEnabled(void);
        boolean dsIsPowerOn(void);
		
	void	dsSetAccumCurrent(int);
	void	dsSetBatteryCapacity(int);
	void    dsSetPowerSwitchOn(void);
	void	dsEnableSleep(void);
	void	dsDisableSleep(void);
		
		
	private:

	int	miProtect;
    	int	miStatus;
    	int	miVolts;
    	int     miBatteryCapacity;
    	boolean	mbPowerOn;
    	boolean mbPowerSwitchOn;
    	boolean	mbSleepEnabled;
    	float	mfCurrent;
    	int 	miAccCurrent;
    	float	mfTempC;
    	float   mfTempF;
    	
    	
    	
    	void    dspGetProtection(void);
        void    dspGetBatteryCapacity(void);
    	void    dspGetVoltageAndCurrent(void);  
    	void    dspGetTemp(void);
    	
        void    dspGetPowerSwitch(void);
        void    dspHandlePower(void);
        
        void    dspSetSleepMode(int);
        void    dspSetPowerSwitchOn(void);    // so we can detect when it's pushed again.

}; // end class DS2764
