/*
 * telemetry.h
 *
 *  Created on: Jun 26, 2018
 *      Author: vthakur
 */

#ifndef telemetry_service
#define telemetry_service


//__________________________________________________________
// INCLUDE                                                 /
//________________________________________________________/

/*----- C++ STANDARD ----- */

/*----- THIRD PARTY  ----- */

/*----- FRAMEWORK -----*/


/*----- PROJECT      -----*/
#include "utils/util.h"
#include "rtos/rtos.h"
#include "services/telemetry/alarmHeader.h"
#include "services/telemetry/customActionHeader.h"
#include "services/telemetry/telmHeader.h"

#define TELEMETRY_INDEX_OFFSET 0x000000
#define TELEMETRY_FILE1_LOCATION 0x000FFF
#define TELEMETRY_FILE2_LOCATION 0x000FFF
#define TELM_MAX_FILE_SIZE 2048
#define TELM_MINUMUM_INTERVAL 	(5*60) /* After every 5*60 second a telemetry record will be averaged and stored to flash.*/
#define TELM_CAPTURING_INTERVAL	(10) /* After every 10 seconds a Telmetery record will be captured.*/
#define MIN(x,y) (x<y?x:y)
#define MAX(x,y) (x<y?y:x)

typedef enum {
	EMPTY = 0,
	FULL = 1,
	ACTIVE = 2,
} FileState;

typedef enum {
	TELM_FILE_1 = 0,
	TELM_FILE_2 = 1,
	TELM_MAX_FILE
}TelmFile;
typedef enum {
	MAGIC_BYTES = 0,
	FLASH_LOCATION = 1,
	WRITE_OFFSET = 2,
	FILE_STATE = 3,
	START_TIME = 4,
	END_TIME = 5,
	MAX_FILE_SIZE = 6,
	NO_OF_RECORDS = 7
}TelmMap;

typedef struct {
	uint32_t telm_magic_byte;
	uint32_t telm_flash_location;
	uint32_t telm_write_offset;
	FileState  telm_file_state;
	uint32_t telm_start_time;
	uint32_t  telm_end_time;
	uint16_t  telm_max_file_size;
	uint8_t  telm_no_of_records;
}__attribute__((packed, aligned(1))) TelemetryFileIndex;
    /**
     * @brief
     */
    class TelemetryService
    {
          //________________________________________________________
         //enum                                                    /
        //________________________________________________________/
    public:

          //________________________________________________________
         //typedef                                                 /
        //________________________________________________________/
    public:


          //________________________________________________________
         //construct,destruct                                      /
        //________________________________________________________/
    public:
    	TelemetryService();

        virtual ~TelemetryService() {}


          //________________________________________________________
         //abstract                                                /
        //________________________________________________________/
    public:
        uint32_t avgCount;
        uint32_t timeInterval;
        TelemetryData telemetryInfo;
        /* Maintaining two files for telemetry will be used alternately.*/
        /* Will start file 1 once full move to file 2 */
        TelemetryFileIndex telmetry_file[TELM_MAX_FILE];
          //________________________________________________________
         //my function                                             /
        //________________________________________________________/
    public:
        /*Function to handle service request*/
        TelemetryData* readTelemetryRecords(TelmFile file);
        TelmFile getActiveFile();
        void collectAlarmStatus();
        void collectCustomAction();
        void collectTelemetryRecord();
        void getTimeInterval(Slip& slip,uint32_t& startTime, uint32_t& endTime);
        void initTelmFile(TelmFile file,uint32_t file_location,FileState state);
        void initTelmFileSystem();
        void resetTelemetryParameters();
        void setTelemeytryInterval(uint32_t interval);
        void telReadRecords(Slip& slip, uint8_t* telmData, uint16_t* noOfRecords, uint16_t* size);
        void updateTelmFile(TelmFile file, TelmMap mapId, uint32_t value);
        void updateTelemetryRecord();
        void writeTelemetryRecord();


        /*Task related*/
        void init();
        void join();
        void printInfo();
        void startup();
        void start();
        void telemetryServiceHandler();

    protected:
          //________________________________________________________
         //thread                                                  /
        //________________________________________________________/
    protected:

    public:
        static void taskTelemetryService(void const* argument);


    };

        //________________________________________________________
       //inline App::                                   /
      //________________________________________________________/

extern TelemetryService telemetryServ;


#endif
