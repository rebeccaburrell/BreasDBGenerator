#pragma once
#include "stdafx.h"
#pragma pack(1)
typedef int8_t          IBYTE;  // Definition of singed byte  8 bit
typedef uint8_t         UBYTE;  // Definition of unsigned byte 8 bit
typedef int16_t         IWORD;  // Definition of signed word 16 bit same as int
typedef uint16_t        UWORD;  // Definition of unsigned word 16 bit same as unsigned int
typedef int32_t         ILONG;  // Definition of singed long 32 bit*/
typedef UBYTE       *   PBYTE;  // Pointer to unsigned byte
typedef UWORD       *   PWORD;  // Pointer to unsigned word
typedef UWORD           UINT16; // Definition of unsigned 16 bit same as unsigned int

typedef struct
{
  ULONG timestamp;         /**< 4 bytes session timestamp */
  UBYTE ms;                /**< 1 byte ms*4 */
  UWORD patientPressure;   /**< 2 bytes momentary patient pressure (0.01 cmH2O) */
  IWORD patientFlow;       /**< 2 bytes momentary patient flow */
  IWORD totalFlow;         /**< 2 bytes momentary total flow */
  WORD  tidalVolume;       /**< 2 bytes momentary Tidal volume */
  UWORD leakage;           /**< 2 bytes momentary Leakage  l/min */
  UWORD momentaryCO2;      /**< 2 byte Momentary CO2, in 0.1 %, 0xFFFF not connected  */
  UWORD atmosphericP;      /**< 2 bytes atmospheric pressure in mbar*/
  UBYTE fIO2Perc;          /**< 1 byte FiO2 value % (also in TBreathData so session averages can be calculated)*/
  IWORD thoracic;          /**< 2 bytes thoracic effort, no unit */
  IWORD abdominal;         /**< 2 bytes abdominal effort, no unit */
  UBYTE trigger;           /**< 1 byte 0x01 = patient triggered 0x00 = machine triggered */
  UBYTE sigh;              /**< 1 byte 0x01 = sigh active 0x00 = sigh inactive */
  UBYTE breathFlag;          /**< CR44: 1 byte 0x01 = inspiration, 0 = expiration */
} TRealtimeData;           // 27 bytes

typedef struct
{
  ULONG breathId;                       /**< The time in seconds from 1970 */
  ULONG timestampId;                   /**< The trailing ms from 1970 */
                                        /* Inspiration values */
  ULONG timestamp;     /**< Peak inspiration patient flow */
  UBYTE ms;                /**< Inspiration time stored as value*20 */
  UBYTE leakage;                       /**< Rise time */
  UBYTE exptrig;  /**< Rate of patient triggered breaths */
  UBYTE fio2;         /**< Patient triggered breath */
  UBYTE etco2pct;                       /**< Fraction of inspired O2 */
  UBYTE etco2kpa;                    /**< Irma Inspiratory CO2 */
  UBYTE etco2mmgh;                 /**< Irma CO2 Atmospheric pressure */
  UBYTE pulse;
  UWORD vT;                       /**< Onyx O2 saturation */
  UBYTE ppCM;                          /**< Onyx Pulse value */
  UBYTE ppHpa;                     /**< Transcutaneous carbon dioxide  CO2  PtCO2, in 0.01 kPa, 0 not connected, 65535 error data  */
  UBYTE ppMb;                      /**< Transcutaneous carbon dioxide  CO2 PtCO2, in 0.01 mmHg, 0 not connected, 65535 error data  */
  UBYTE pCM;                        /**< Breath Leakage */
  UBYTE pHpa;                    /**< Inspiration/expiration ratio inspiration part */
  UBYTE pMB;              /**< Peak pressure */
  UBYTE totalBR;                /**< Peak pressure */
  UBYTE spontBR;               /**< Peak pressure */
  UBYTE spO2;                      /**< Positive end-expiratory pressure */
  UBYTE trigger;                        /**< Positive end-expiratory pressure */
  UBYTE inspCO2;                       /**< Positive end-expiratory pressure */
  UWORD atmCO2;                 /**< Breath expiration trigger */
  UBYTE tid;                     /**< Inspiration/expiration ratio expiration part */
  UBYTE temp;                /**< Total breath rate */
  UBYTE averageLeakage;                 /**< Mean leakage */
  UBYTE peakflow;                             /**< Tidal volume */
  UBYTE inspTime;                      /**< IRMA EtCO2, in 0.1 %, 0xFF not connected  */
  UBYTE riseTime;                       /**< IRMA EtCO2, in 0.1 kPa, 0xFF not connected  */
  UBYTE iexpRatio;                      /**< IRMA EtCO2, in 1 mmHg, 0xFF not connected   */
                                        /* Misc values */
  UWORD ieratio;                 /**< Temperature is measured at different places. This id shows where */
  UBYTE ptCO2kpa;                   /**< Temperature */
  UBYTE ptCO2mmHg;                        /**< thoracic effort */
}TBreathData102;

typedef enum
{
  EventRecord,                          /**< recordId is of type Event */
  AlarmRecord                          /**< recordId is of type Alarm */
}TRecordType;

typedef struct
{
  unsigned int startTimeDate;                 /**< The epoch time when the session started */
  unsigned short msStart; 					   /**< milliseconds when the session started*/
  unsigned int timeDate;                      /**< The epoch time when the event occured*/
  unsigned short milliSeconds;	               /**< milliSeconds within current timeDate */
                                     /* Expiration values */
  unsigned short averagePeakPressure;           /**< Average peak pressure for the session in 0.01 CMH2O */
  unsigned short averagePEEP;                   /**< Average PEEP in 0.01 CMH2O*/
  unsigned short averageVt;                     /**< Average tidal volume in 1.0 mL*/
  unsigned short averageMV;                     /**< Average minute volume in 0.1 L/min */
  unsigned short patientTriggeredBreathPercent; /**< Patient triggered breath percent */
  unsigned short averageEtCO2;                  /**< Average EtCO2 values in 0.1 % */
  unsigned short averageAtmP;                   /**< Average atmospheric pressure in hpa or mbar */
                                       /* Inspiration values */
  unsigned short averageLeakage;                /**< Average Leakage in 0.1 L/min*/
  UBYTE averageFiO2;                   /**< Average FiO2 values in 1 % */
  UBYTE averageSpO2;                   /**< Average SpO2 values in 1 % */
  unsigned short averagePtCO2;                  /**< Average PtCO2 values in 0.1 %*/
}TUsageSessionData;

/**
* Struct for breath data
*/
typedef struct
{
  ULONG timeDate;                       /**< The time in seconds from 1970 */
  UBYTE milliSeconds;                   /**< The trailing ms from 1970 */

  UWORD peakInspirationPatientFlow;     /**< Peak inspiration patient flow in ml/s*/
  UWORD peakPressure;                   /**< Peak pressure in 0.01 cmH2O*/
  UWORD peep;                           /**< Positive end-expiratory pressure in .01 CmH2O*/
  UWORD meanAirP;                       /**< 2 bytes Mean air pressure (0.01 cmH2O) */
  UWORD maxTidalVolume;                 /**< 2 bytes Max insp tidal volume ml */
  UWORD inspExpRatio;                   /**< 2 bytes Insp./Exp. ratio, 0x10xx - 0x1099 / 0x1110 - 0x9910 */
  UWORD leakage;                        /**< Mean leakage in 0.1 L/min */
  UWORD minuteVolume;                   /**< 2 bytes minute volume 0.1 L/min */
  UBYTE risetime;                       /**< Rise time in 0.1 sec */
  UWORD calcInspTime;                   /**< 2 bytes calculated inspiratory time .1 sec*/
  UBYTE patientTriggeredBreath;         /**< Patient triggered breath (1 = patient, 0 = machine)*/
  UBYTE spontBreathRate;                /**< Rate of patient triggered breaths in %*/
  UBYTE totalBreathRate;                /**< Total breath rate */
  UWORD percTargetVol;                  /**< 2 bytes Target volume percentage, 00-100 (%) */
  UWORD PercSpontBreathRate;            /**< 2 bytes Spont. breath percentage, 00 - 100 % */
  UWORD etCO2;                          /**< IRMA EtCO2, in 0.1 %, 0xFFFF not connected  */
  UWORD inspCO2Perc;                    /**< Irma Inspiratory CO2 0.1% 0xFFFF not connected*/
  UWORD atmPressureCO2;                 /**< Irma CO2 Atmospheric pressure mbar or hpa*/
  UBYTE spO2Perc;                       /**< Onyx O2 saturation 1%*/
  UBYTE pulse;                          /**< Onyx Pulse value */
  UBYTE sigh;							/**< sigh breath indication */
  UBYTE fiO2perc;                       /**< Average FiO2 values in 1 % - needed here for calculating session average*/
  UWORD ptCO2;                          /**< Transcutaneous carbon dioxide  CO2 PtCO2, in 0.1%, 0xFFFF not connected  */
  UWORD atmosphericPressure;            /**< measured atmospheric pressure in mbar */

                                        /* Misc values */
  UBYTE temperatureID;                 /**< Temperature is measured at different places. This id shows where */
  IBYTE temperature;                   /**< Temperature in degrees */

}TBreathData;

typedef enum
{
  DF_SWE = 0,    /**< yyyy-mm-dd Sweden/China */
  DF_GLOBAL,   /**< dd/mm/yyyy Most common */
  DF_US,       /**< mm/dd/yyyy United States */
  NUMBER_OF_DATEFORMATS /**< Total number of date formats */
} TDateFormat;

/**
* Time Formats
*/
typedef enum
{
  TF_12H = 0,               /**< 12h (AM, PM) */
  TF_24H,                 /**< 24h */
  NUMBER_OF_TIMEFORMATS   /**< Total number of time formats */
} TTimeFormat;

typedef struct
{
  ULONG l_CTime;     /**< Calendar time is the number of seconds since 1/1/1970 */
  UWORD ui_TimeMs;   /**< Millisecond in the second (resolution 1 ms) */
} TDateTime;

#define PROFILENAME_LENGTH  (12)         // Length of profile names (11 + \0).
//#define VIVO45_VCV_DISABLED
/******************************************************************************
*                                TYPE DEFINITIONS
******************************************************************************/
/**
* Ventilation Modes
*/
typedef enum
{
  VENT_PRESSURE = 0, /**< Pressure */
  VENT_CPAP = 1, /**< CPAP */
#ifndef VIVO45_VCV_DISABLED
  VENT_VOLUME = 2, /**< Volume */
#endif
  NUMBER_OF_VENT_MODES /**< Total number of ventilation modes */
} TVentilationMode;

/**
* Changes to this enum must be made in the eToPatientMode operation.
* Patient Modes
*/
typedef enum
{
  PAT_ADULT = 0, /**< Adult */
                 //#ifndef VIVO45_PEADIATRIC_DISABLED
                 PAT_PAEDIATRIC, /**< Paediatric */
                                 //#endif
                                 NUMBER_OF_PATIENT_MODES /**< Total number of patient modes */
} TPatientMode;

/**
* Device Modes
*/
typedef enum
{
  DEV_CLINICAL = 0, /**< Clinical mode */
  DEV_HOME, /**< Home mode */
  NUMBER_OF_DEVICE_MODES /**< Total number of device modes */
} TDeviceMode;

/**
* Breath Modes
*/
typedef enum
{
  BREATH_SUPPORT = 0, /**< Support */
  BREATH_ASSIST_CONTROL, /**< Assist/Control */
  NUMBER_OF_BREATH_MODES /**< Total number of breath modes */
} TBreathMode;

/**
* Circuit Types
*/
typedef enum
{
  CIR_LEAKAGE = 0, /**< Leakage */
  CIR_SINGLE, /**< Single */
  CIR_DUAL, /**< Dual */
  NUMBER_OF_CIRCUITS /**< Total number of circuits */
} TCircuitType;

/**
* Profile Id
*/
typedef enum
{
  PROFILE_1 = 0, /**< Profile Id for first profile */
  PROFILE_2, /**< Profile Id for second profile */
  PROFILE_3, /**< Profile Id for third profile */
  PROFILE_SHADOW_UI, /**< Profile Id for UI Shadow */
  PROFILE_SHADOW_PC, /**< Profile Id for PC Shadow */
  PROFILE_SHADOW_PC_SERVICE, /**< Profile Id for PC Shadow */
  NUMBER_OF_PROFILES_AND_SHADOW /**< Total number of profiles and shadow profiles */
} TProfileNumber;

typedef char TProfileName[PROFILENAME_LENGTH];
/**
* Struct used to store the data connected to the settings log
*/
typedef struct
{
  ULONG timeDate;         /**< The time in seconds from 1970 */
  UBYTE milliSeconds;     /**< The trailing ms from 1970 */
  UBYTE inspPress;        /**< Inspiration pressure */
  UBYTE peep;             /**< Positive end-expiratory pressure */
  UBYTE inspTrig;         /**< Inspiration trigger */
  UBYTE expTrig;          /**< Expiratory trigger */
  UBYTE riseTimeVolume;   /**< Volume rise time  */
  UBYTE riseTimePressure; /**< Pressure rise time */
  UWORD targetVolume;     /**< Target volume CR44: changed to UWORD*/
  UBYTE inspPressMax;     /**< Max inspiratory pressure */
  UBYTE inspPressMin;     /**< Min inspiratory pressure */
  UBYTE backupRate;       /**< Backup rate */
  UBYTE inspTimeMin;      /**< Min inspiratory time */
  UBYTE inspTimeMax;      /**< Max inspiratory time */
  UBYTE setInspTime;      /**< Set inspiratory time */
  UWORD setVol;           /**< Set volume CR44: changed to UWORD*/
  UBYTE cpap;             /**< Continuous positive airway pressure */
  UBYTE ramp;             /**< ramp */
  UBYTE rampStartPressure; /**< ramp start pressure */
  UBYTE rampStart;         /** < ramp autostart */
  UBYTE humidifier;          /**< Humidifier off and heat setting */
  UWORD heatedWire;     /**< Heated wire off and heat setting */
  UWORD heatedWireF;       /**< Heated wire fahrenheit setting */
  UBYTE tempUnit;           /**< temp unit 0 = celcius, 1 = Fahrenheit*/
  UBYTE sigh;  				/**< Sigh on/off*/
  UBYTE sighRate;			/**< Sigh Rate */
  UBYTE sighPercent;		/**< Sigh Percent */
  UBYTE flowPattern;        /**< flowPattern */
  TVentilationMode ventMode;    /**< Ventilation  mode pressure, volume, CPAP*/
  TBreathMode breathMode;       /**< Breath mode support, assist, simv*/
  TPatientMode patientMode;     /**< Patient mode */
  TDeviceMode deviceMode;       /**< Device mode */
  UBYTE activatedProfiles;  /**< Activated profiles */
  UBYTE selectedProfile;    /**< Selected profiles */
  UBYTE complianceTime;     /**< compliance time */
  UBYTE adjustmentInHome;   /**< Enables limited adjustment of treatment parameters in home mode */
  UBYTE circuitRecognition; /**< No = 0, Yes = 1 */
  UBYTE temperatureUnit;    /**< 0 = Celcius, 1 = Fahrenheit */
  UBYTE alarmSoundLevel;    /**< Alarm sound level 1-9 */
  UBYTE pressHighAlarm;     /**< High alarm limit for Pressure */
  UBYTE pressLowAlarm;      /**< Low alarm limit for Pressure */
  UBYTE flowHighAlarm;     /**< High alarm limit for Flow */
  UBYTE flowLowAlarm;      /**< Low alarm limit for Flow */
  UBYTE peepHighAlarm;      /**< High alarm limit for PEEP */
  UBYTE peepLowAlarm;       /**< Low alarm limit for PEEP */
  UBYTE breathRateHighAlarm;/**< High alarm limit for Breath Rate */
  UBYTE breathRateLowAlarm; /**< Low alarm limit for Breath Rate */
  UBYTE apneaAlarm;         /**< Apnea alarm on/off */
  UBYTE disconnectionAlarm; /**< Disconnection alarm limit */
  UBYTE obstructionAlarm;   /**< Obstruction alarm setting */
  UBYTE rebreathingAlarm;   /**< Rebreathing alarm limit */
  UWORD vtHighAlarm;        /**< High alarm limit for Tidal Volume */
  UWORD vtLowAlarm;         /**< Low alarm limit for Tidal Volume */
  UWORD mvHighAlarm;        /**< High alarm limit for Insp Minute Volume */
  UWORD mvLowAlarm;         /**< Low alarm limit for Insp Minute Volume */
  UBYTE pulseRateHighAlarm; /**< High alarm limit for Pulse Rate */
  UBYTE fiO2HighAlarm;      /**< High alarm limit for FIO2 */
  UBYTE fiO2LowAlarm;       /**< Low alarm limit for FIO2 */
  UBYTE etCO2Unit;          /**< mmHg=0, kPa=1, Percent=2 (Te_EtCO2Unit) */
  UBYTE spO2HighAlarm;      /**< High alarm limit for SPO2 */
  UBYTE spO2LowAlarm;       /**< Low alarm limit for SPO2 */
  UBYTE pulseRateLowAlarm;  /**< Low alarm limit for Pulse Rate */
  UWORD circuitResistance;  /**< Patient circuit resistance factor. */
  UWORD circuitCompliance;  /**< Patient circuit compliance factor. */
  UBYTE humidifierOnOff;    /**< humidifier on or off */
  UBYTE heatedWireOnOff;    /**< heatedWire on or off */
  UBYTE etCO2HighPercentAlarm;  /**< High alarm limit for End Tidal CO2 (%) */
  UBYTE etCO2HighmmHgAlarm;     /**< High alarm limit for End Tidal CO2 (mmHg) */
  UBYTE etCO2HighkPaAlarm;      /**< High alarm limit for End Tidal CO2 (kPa) */
  UBYTE etCO2LowPercentAlarm;   /**< Low alarm limit for End Tidal CO2 (%) */
  UBYTE etCO2LowmmHgAlarm;      /**< Low alarm limit for End Tidal CO2 (mmHg) */
  UBYTE etCO2LowkPaAlarm;       /**< Low alarm limit for End Tidal CO2 (kPa) */
  UBYTE inspCO2HighPercentAlarm;  /**< High alarm limit for Insp CO2 (%) */
  UBYTE inspCO2HighmmHgAlarm;     /**< High alarm limit for Insp CO2 (mmHg) */
  UBYTE inspCO2HighkPaAlarm;      /**< High alarm limit for Insp CO2 (kPa) */
  TProfileName activeProfileName;      /**< The string representing the active profile name */

}TSettingsData;

/**<
* Struct used to store event specific data. Stored as UWORD internally
*/
typedef struct
{
  UWORD eventData1; /**< Event specific data */
  UWORD eventData2; /**< Event specific data */
  UWORD eventData3; /**< Event specific data */
  UWORD eventData4; /**< Event specific data */
  UWORD eventData5; /**< Event specific data */
  UWORD eventData6; /**< Event specific data */
  UWORD eventData7; /**< Event specific data */
  UWORD eventData8; /**< Event specific data */
} TUsageEventData;

/**<
* Struct used to store event data
*/
typedef struct {
  ULONG timeDate;         /**< Time and date the event occurred */
  UWORD milliSeconds;     /**< Trailing milliseconds */
  TRecordType recordType; /**< Type of record. 0 = event, 1 = alarm */
  UWORD recordId;         /**< Event specific id */
  TUsageEventData data;   /**< Contains event specific data */
} TUsageEventLogRecord;

/**<
* Struct for timestamp data
*/
typedef struct {
  ULONG timeDate;       /**< Time and date the timestamp  */
  UWORD milliSeconds;   /**< Trailing milliseconds */
  UBYTE timeStampType;  /**< Type of timestamp, db create, start, end, sync or 24h  */
  UBYTE logVersion;     /**< Version of the log */
} TTimestampData;

typedef struct
{
  ULONG timeDate;        /**< The time in seconds from 1970 */
  UWORD peakPressure;    /**< Peak pressure */
  UWORD peep;            /**< Peep */
  UBYTE totalBreathRate;  /**< Total breath rate */
  UBYTE spontBreathRate; /**< patient triggered breath rate*/
  UBYTE spO2;                    /**< sp02 */
  UWORD etCO2;               /**< end tidal co2  */
  UWORD leakage;                 /**< Leakage*/
  UWORD vt;                      /**< Tidal volume */
  UWORD ptCO2;                   /**< Transcutaneous carbon dioxide  CO2  */
}TTrendData;

/**
*
*/
typedef struct {
  long totalSum;
  long value;
} TTrendDataRecord;
/**
*
*/
typedef struct
{
  TTrendDataRecord ppeak;
  TTrendDataRecord peep;
  TTrendDataRecord spO2;
  TTrendDataRecord totalRate;
  TTrendDataRecord spontRate;
  TTrendDataRecord etCO2;
  TTrendDataRecord ptcCO2;
  TTrendDataRecord tidalVolume;
  TTrendDataRecord leakage;
  long samples;
} TTrendDataValues;
/**
*
*/
#define TOTAL_SAMPLES 390
typedef struct {
  TTrendDataValues values[TOTAL_SAMPLES];
  unsigned int head;
  unsigned int tail;
  unsigned int oldestTimeStamp;
  unsigned int samplesPerPixel;
} TTrendDataHolder;