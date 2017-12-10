#ifndef _DATA_ACQ_H
#define _DATA_ACQ_H

#ifndef __APPLE__
#include <sys/inotify.h>
#include "dm75xx_library.h"
#endif /* __APPLE__ */
#include <thread>

#include "log.h"
#include "UsbManager.h"
#include "ZynqManager.h"
#include "ThermManager.h"
#include "pdmdata.h"
#include "data_format.h"
#include "ConfigManager.h"
#include "SynchronisedFile.h"

#define DATA_DIR "/home/minieusouser/DATA"
#define DONE_DIR "/home/minieusouser/DONE"
#define USB_MOUNTPOINT_0 "/media/usb0"
#define USB_MOUNTPOINT_1 "/media/usb1"

/* for use with inotify in ProcessIncomingData() */
#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (1024 * (EVENT_SIZE + 16))

/* for use with analog readout functions */
#define CHANNELS 16
#define FIFO_DEPTH 64
#define BURST_RATE 1000000
#define PACER_RATE 100000
#define PH_CHANNELS 4

/* acquisition structure for analog readout */
typedef struct
{
  float val [FIFO_DEPTH][CHANNELS];
} AnalogAcq;

/* class for controlling the acquisition */
class DataAcqManager {   
public:  
  std::string cpu_main_file_name;
  std::string cpu_sc_file_name;
  std::shared_ptr<SynchronisedFile> CpuFile;
  Access * RunAccess;
  ThermManager * ThManager = new ThermManager();
  
  enum RunType : uint8_t {
    CPU = 0,
    SC = 1,
  };

  DataAcqManager();
  int CreateCpuRun(RunType run_type, Config * ConfigOut);
  int CloseCpuRun(RunType run_type);
  int CollectSc(Config * ConfigOut);
  int CollectData(Config * ConfigOut, uint8_t instrument_mode = 0, uint8_t test_mode = 0, bool single_run = false, bool test_mode_on = false);
  static int WriteFakeZynqPkt();
  
private:
  std::string CreateCpuRunName(RunType run_type, Config * ConfigOut);
  static uint32_t BuildCpuFileHeader(uint32_t type, uint32_t ver);
  static uint32_t BuildCpuPktHeader(uint32_t type, uint32_t ver);
  static uint32_t BuildCpuTimeStamp();
  SC_PACKET * ScPktReadOut(std::string sc_file_name, Config * ConfigOut);
  ZYNQ_PACKET * ZynqPktReadOut(std::string zynq_file_name, Config * ConfigOut);
  AnalogAcq * AnalogDataCollect();
  HK_PACKET * AnalogPktReadOut(AnalogAcq * acq_output);
  int WriteScPkt(SC_PACKET * sc_packet);
  int WriteCpuPkt(ZYNQ_PACKET * zynq_packet, HK_PACKET * hk_packet, Config * ConfigOut);
  int ProcessIncomingData(Config * ConfigOut, bool single_run);
  int ProcessThermData();
  
};

#endif
/* _DATA_ACQ_H */
