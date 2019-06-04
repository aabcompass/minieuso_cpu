#include "ZynqManager.h"

/**
 * constructor 
 * initialises public members
 * hvps_status, instrument_mode, test_mode and telnet_connected
 */
ZynqManager::ZynqManager () {   
  this->hvps_status = ZynqManager::UNDEF;
  this->zynq_mode = ZynqManager::NONE;
  this->test_mode = ZynqManager::T_NONE;
  this->telnet_connected = false;

  /* initialise vector of EC values to 0 */
  for (int i = 0; i < N_EC; i++) {
    this->ec_values.push_back(0);
  }
}


/**
 * check if the telnet socekt is responding as expected
 */
bool ZynqManager::CheckTelnet() {

  bool connected = false;
  int sockfd;
  std::string status_string = "";
  
  /* set up the telnet connection */
  sockfd = ConnectTelnet();

  std::cout << "..." << std::endl;
  if (sockfd > 0) {
    status_string = SendRecvTelnet("instrument status\n", sockfd);
    close(sockfd);
  }

  size_t found = status_string.find("40");
  if (found != std::string::npos) {
    connected = true;
  }
  
  return connected;  
}

/**
 * check telnet connection on ZYNQ_IP (defined in ZynqManager.h)
 * and close the telnet connection after.
 * has a timeout implemented of length CONNECT_TIMEOUT_SEC (defined in ZynqManager.h)
 */
int ZynqManager::CheckConnect() {
 
  clog << "info: " << logstream::info << "checking connection to IP " << ZYNQ_IP  << std::endl;

  /* initilaise timeout timer */
  time_t start = time(0);
  int time_left = CONNECT_TIMEOUT_SEC;

  /* wait for FTP to be up */  
  //while (!CpuTools::CheckFtp()) {

    ///* timeout if no activity after CONNECT_TIMEOUT_SEC reached */
    //time_t end = time(0);
    //time_t time_taken = end - start;
    //time_left = CONNECT_TIMEOUT_SEC - time_taken;
    
  //}
  /* catch FTP timeout */
  //if (!CpuTools::CheckFtp()) {

  //std::cout << "ERROR: FTP server timeout" << std::endl;
  //std::cout << "Try: /etc/init.d/vsftpd start" << std::endl;
  //clog << "error: " << logstream::error << "timing out on setup of FTP server" << std::endl;
  //this->telnet_ed = false;
    
  //return -1;
  //}
  
  /* reinitialise timout timer */
  //start = time(0);
  //time_left = CONNECT_TIMEOUT_SEC;
  
  /* wait for answer on telnet */
  while (!CheckTelnet() && time_left > 0) {
    
    sleep(2);

    /* timeout if no activity after CONNECT_TIMEOUT_SEC reached */
    time_t end = time(0);
    time_t time_taken = end - start;
    time_left = CONNECT_TIMEOUT_SEC - time_taken;
  
  }
  /* catch connection timeout */
  if (!CheckTelnet()) {

    std::cout << "ERROR: Connection timeout to the Zynq board" << std::endl;
    clog << "error: " << logstream::error << "error connecting to " << ZYNQ_IP << " on port " << TELNET_PORT << std::endl;
    
    this->telnet_connected = false;
    return 1;
  }

  this->telnet_connected = true;
  return 0;  
}

/**
 * send and recieve commands over the telnet connection
 * to be used inside a function which opens the telnet connection 
 * @param send_msg the message to be sent
 * @param sockfd the socket field descriptor
 */
std::string ZynqManager::SendRecvTelnet(std::string send_msg, int sockfd) {

  const char * kSendMsg = send_msg.c_str();
  char buffer[256];
  std::string recv_msg;
  std::string err_msg = "error";
  int n;
  
  /* prepare the message to send */
  bzero(buffer, 256);
  strncpy(buffer, kSendMsg, sizeof(buffer));
  send_msg.erase(std::remove(send_msg.begin(), send_msg.end(), '\n'), send_msg.end());
  clog << "info: " << logstream::info << "sending via telnet: " << send_msg << std::endl;
 
  n = write(sockfd, buffer, strlen(buffer));
  if (n < 0) {
    clog << "error: " << logstream::error << "error writing to socket" << std::endl;
    return err_msg;
  }
  bzero(buffer, 256);
  n = read(sockfd, buffer, 255);
  if (n < 0) {
    clog << "error: " << logstream::error << "error reading from socket" << std::endl;
    return err_msg;
  }
  recv_msg = buffer;
  recv_msg.erase(std::remove(recv_msg.begin(), recv_msg.end(), '\r'), recv_msg.end());
  recv_msg.erase(std::remove(recv_msg.begin(), recv_msg.end(), '\n'), recv_msg.end());
  clog << "info: " << logstream::info << "receiving via telnet: " << recv_msg << std::endl;
  return recv_msg;
 }

/**
 * wrapper for use of SendRecvTelnet() in more readable way
 * @param send_msg message to send
 * @param sockfd the socket file descriptor
 * @param print if true, received message is printed
 * returns the recieved telnet response
 */
std::string ZynqManager::Telnet(std::string send_msg, int sockfd, bool print) {

  std::string status_string = "";
  
  if (sockfd > 0) {
    status_string = SendRecvTelnet(send_msg, sockfd);
    if (print) {
      std::cout << status_string << std::endl;
    }
    usleep(SLEEP_TIME);
  }
  else {
    clog << "error: " << logstream::error << "bad socket passed to ZynqManager::Telnet()" << std::endl;
  }
  return status_string;
}

/**
 * connect via telnet to ZYNQ_IP.
 * NB: leaves telnet open to be closed with a separate function 
 */
int ZynqManager::ConnectTelnet() {

  /* definitions */
  int sockfd;
  struct sockaddr_in serv_addr;
  struct hostent * server;
  const char * ip = ZYNQ_IP;
  struct timeval tv;
  fd_set fdset;
  
  cout << "Hi Francesca!! Where are the logs?" << std::endl;
  clog << "info: " << logstream::info << "checking connection to IP " << ZYNQ_IP  << std::endl;

  /* debug */
  clog << "info: " << logstream::info << "setting up telnet connection" << std::endl;  
   
  /* set up the telnet connection */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) { 
    clog << "error: " << logstream::error << "error opening socket" << std::endl;
    return 1;
  }

  /* debug */
  clog << "info: " << logstream::info << "retrieving server" << std::endl;  
   
  server = gethostbyname(ip);
  if (server == NULL) {
    clog << "error: " << logstream::error << "no host found for " << ZYNQ_IP << std::endl;  
    return 1;
  }
  
  /* debug */
  clog << "info: " << logstream::info << "setting server address" << std::endl;  

  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, 
	(char *)&serv_addr.sin_addr.s_addr,
	server->h_length);
  serv_addr.sin_port = htons(TELNET_PORT);

  /* debug */
  clog << "info: " << logstream::info << "set port to non-blocking" << std::endl;  

  /* set non-blocking */
  int opts = fcntl(sockfd, F_SETFL, O_NONBLOCK);
  connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));

  FD_ZERO(&fdset);
  FD_SET(sockfd, &fdset);

  /* add timeout */
  tv.tv_sec = SHORT_TIMEOUT_SEC; 
  tv.tv_usec = 0;

  /* debug */
  clog << "info: " << logstream::info << "connect to socket" << std::endl;  
  
  if (select(sockfd + 1, NULL, &fdset, NULL, &tv) == 1) {
      int so_error;
      socklen_t len = sizeof so_error;
      
      getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &so_error, &len);
      
      if (so_error == 0) {
	clog << "info: " << logstream::info << "connected to " << ZYNQ_IP << " on port " << TELNET_PORT  << std::endl;

	/* clear non-blocking */
	opts = opts & (~O_NONBLOCK);
	fcntl(sockfd, F_SETFL, opts);   
      }
      else {

	clog << "error: " << logstream::error << "error connecting to " << ZYNQ_IP << " on port " << TELNET_PORT << std::endl;
	return -1;
      }
      
    }
  
  return sockfd;   
}

/**
 * check the instrument status 
 */
int ZynqManager::GetInstStatus() {

  int sockfd;

  clog << "info: " << logstream::info << "checking the instrument status" << std::endl;

  /* setup the telnet connection */
  sockfd = ConnectTelnet();

  /* get the instrument status */
  std::cout << "instrument status: ";
  std::string status = Telnet("instrument status\n", sockfd, true);
  close(sockfd);

  /* perform checks */
  size_t found = status.find("40");
  if (found == std::string::npos) {
    clog << "error: " << logstream::error << "instrument status is: " << status << std::endl;
  }

  int reported_zynq_mode = stoi(status.substr(3, std::string::npos));

  if (reported_zynq_mode != this->zynq_mode) {
    clog << "error: " << logstream::error << "zynq_mode is: " << reported_zynq_mode << std::endl;
  }
  
  return 0;
}


/**
 * check the HV status 
 */
int ZynqManager::GetHvpsStatus() {

  int sockfd;

  clog << "info: " << logstream::info << "checking the HVPS status" << std::endl;

  /* setup the telnet connection */
  sockfd = ConnectTelnet();

  /* get the HVPS status */
  std::cout << "HVPS status: ";
  std::string status = Telnet("hvps status gpio\n", sockfd, true);
  close(sockfd);

  /* perform checks */
  /* ! disabled this for now as causing problems ! */
  /*
  std::vector<int> ec_status = CpuTools::DelimStrToVec(status, ' ', N_EC, false);
  for (uint8_t i = 0; i < ec_status.size(); i++) {
    if (this->ec_values[i] != ec_status[i]) {
      
      std::cout << "ERROR: unexpected EC status" << std::endl; 
      clog << "error: " << logstream::error << "unexpected EC HVPS status" << std::endl;
    }
  }
  */

  return 0;
}

/**
 * turn on the HV.
 * ramps up the dynode voltage in steps of 500 HV DAC <=> ~140 V 
 * @param cv the cathode voltage (int from 1-3)
 * @param dv the dynode voltage (HV DAC from 0 to 4096).
 * @param hvps_ec_string string of 9 (each EC unit) comma-separated values, 1 <=> on, 0 <=> off
 * to convert from HV DAC to voltage use (dv/4096 * 2.44 * 466)
 */
int ZynqManager::HvpsTurnOn(int cv, int dv, std::string hvps_ec_string) {

  int sockfd;
  std::string cmd;
 
  clog << "info: " << logstream::info << "turning on the HVPS" << std::endl;

  /* setup the telnet connection */
  sockfd = ConnectTelnet();
  
  /* set the cathode voltage */
  /* make the command string from config file values */
  cmd = CpuTools::BuildStr("hvps cathode", " ", 3, N_EC);
  std::cout << "Set HVPS cathode to " << cv << ": "; 
  Telnet(cmd, sockfd, true);
  
  /* set the dynode voltage to 0 */
  cmd = CpuTools::BuildStr("hvps setdac", " ", 0, N_EC);
  std::cout << "Set HVPS DAC to " << 0 << ": "; 
  Telnet(cmd, sockfd, true);
  
  /* turn on */
  /* make the command string from hvps_ec_string */
  this->ec_values = CpuTools::DelimStrToVec(hvps_ec_string, ',', N_EC, true);
  cmd = CpuTools::BuildStrFromVec("hvps turnon", " ", this->ec_values); 
  std::cout << "Turn on HVPS: ";
  Telnet(cmd, sockfd, true);
  
  /* ramp up in steps of 500 DAC */
  int ramp_dac[8];
  int ramp_step = 500;
  int dac = 0;
  int i = 0;
  for (i = 0; i < 8; i++) {
    dac += ramp_step;
    ramp_dac[i] = dac;
  }
  
  i = 0;
  bool ramp_done = false;
  while (!ramp_done && i < 8) {  
    if (dv > ramp_dac[i]) {
      cmd = CpuTools::BuildStr("hvps setdac", " ", ramp_dac[i], N_EC);
      std::cout << "Set HVPS DAC to " << ramp_dac[i] << ": ";
      Telnet(cmd, sockfd, true);

      i += 1;
    }
    else {
      ramp_done = true;
    }
  }
      
  /* set the final DAC */
  cmd = CpuTools::BuildStr("hvps setdac", " ", dv, N_EC);
  std::cout << "Set HVPS DAC to " << dv << ": ";
  Telnet(cmd, sockfd, true);
  
  /* check the status */
  std::cout << "HVPS status: ";
  Telnet("hvps status gpio\n", sockfd, true);
  
  /* update the HvpsStatus */
  this->hvps_status = ZynqManager::ON;
  
  close(sockfd);
  return 0;
}


/**
 * turn off the HV 
 */
int ZynqManager::HvpsTurnOff() {

  int sockfd;
  std::string cmd;

  clog << "info: " << logstream::info << "turning off the HVPS" << std::endl;

  /* setup the telnet connection */
  sockfd = ConnectTelnet();

  /* turn off */
  std::cout << "HVPS turn off: ";
  cmd = CpuTools::BuildStr("hvps turnoff", " ", 1, N_EC);
  Telnet(cmd, sockfd, true);

  /* check the status */
  std::cout << "HVPS status: ";
  Telnet("hvps status gpio\n", sockfd, true);
  close(sockfd); 

  /* update the HvpsStatus */
  this->hvps_status = ZynqManager::OFF;

  /* update the ec_values */
  for (uint8_t i = 0; i < ec_values.size(); i++) {
    this->ec_values[i] = 0;
  }
  
  return 0;
}


/**
 * take an scurve 
 */
int ZynqManager::Scurve(int start, int step, int stop, int acc) {
  
  int sockfd;
  std::string cmd;
  std::stringstream conv;
  std::string status_string;
  
  clog << "info: " << logstream::info << "taking an S-curve" << std::endl;

  /* setup the telnet connection */
  sockfd = ConnectTelnet();
  
  /* take an s-curve */
  std::cout << "S-Curve acquisition starting" << std::endl;
  conv << "acq scurve " << start << " " << step << " " << stop << " " << acc << std::endl;
  cmd = conv.str();
  std::cout << cmd;
  
  status_string = SendRecvTelnet(cmd, sockfd);

  while(!this->CheckScurve(sockfd)) {
    sleep(1);
  }
  
  close(sockfd);
  return 0;
}


/**
 * check the S-curve acquisition status and return true on completion
 */
bool ZynqManager::CheckScurve(int sockfd) {

  bool scurve_status = false;
  std::string status_string;
  
  status_string = SendRecvTelnet("acq scurve status\n", sockfd);
  std::cout << "acq scurve status: " << status_string << std::endl;

  size_t noacq_found = status_string.find("GatheringInProgress=0");
  if (noacq_found != std::string::npos) {

    /* scurve gathering is done */
    scurve_status = true;
  }
  else {
    clog << "error: " << logstream::error << "bad socket in ZynqManager::CheckScurve()" << std::endl;
    
  }
  
  return scurve_status;
}

/**
 * set the ASIC DAC on the SPACIROCs 
 * @param dac_level (0 - 1000)
 */
int ZynqManager::SetDac(int dac_level) {

  /* definitions */
  std::string status_string;
  int sockfd;
  std::string cmd;
  std::stringstream conv;

  clog << "info: " << logstream::info << "set the dac level to the SPACIROCs" << std::endl;

  /* setup the telnet connection */
  sockfd = ConnectTelnet();
  
  /* set the dac level */
  conv << "slowctrl all dac " << dac_level << std::endl;
  cmd = conv.str();
  std::cout << cmd;
  
  Telnet(cmd, sockfd, false);

  close(sockfd);
  return 0;
}


/**
 * acquire one GTU frame from the SPACIROCs 
 */
int ZynqManager::AcqShot() {

  /* definitions */
  std::string status_string;
  int sockfd;
  std::string cmd;
  std::stringstream conv;

  clog << "info: " << logstream::info << "acquiring a single frame from the SPACIROCs" << std::endl;

  /* setup the telnet connection */
  sockfd = ConnectTelnet();
  
  /* take a single frame */
  conv << "acq shot" << std::endl;
  cmd = conv.str();
  std::cout << cmd;

  Telnet(cmd, sockfd, false);

  close(sockfd);
  return 0;
}

/**
 * set the acquisition mode 
 * @param input_mode the desired mode to set
 */
uint8_t ZynqManager::SetZynqMode() {

  /* definitions */
  std::string status_string;
  int sockfd;
  std::string cmd;
  std::stringstream conv;

  uint8_t input_mode = this->zynq_mode;

  clog << "info: " << logstream::info << "ZynqManager switching to zynq mode " << (int)input_mode << std::endl;

  /* setup the telnet connection */
  sockfd = ConnectTelnet();

  /* define the command to send via telnet */
  uint32_t timestamp = time(NULL);
  conv << "instrument mode " << (int)this->zynq_mode << " " << timestamp << std::endl;
  cmd = conv.str();
  Telnet(cmd, sockfd, false);  

  /* check the status */
  std::string status = Telnet("instrument status\n", sockfd, false);

  try {
    int reported_zynq_mode = std::stoi(status.substr(2,5));
    if (reported_zynq_mode != (int)this->zynq_mode) {

      std::cout << "ERROR: zynq mode set incorrectly" << std::endl; 
      clog << "error: " << logstream::error
	   << "reported zynq mode is: " << reported_zynq_mode
	   << "ZynqManager zynq mode is: " << (int)this->zynq_mode
	   << std::endl;
    }
  }
  catch (std::invalid_argument) {
    std::cout << "ERROR: problem reading instrument status" << std::endl;
  }

  close(sockfd); 
  return this->zynq_mode;
}


/**
 * set the test acquisition mode 
 * @param input_mode the desired mode to be set
 */
ZynqManager::TestMode ZynqManager::SetTestMode() {

  /* definitions */
  std::string status_string;
  int sockfd;
  std::string cmd;
  std::stringstream conv;

  ZynqManager::TestMode input_mode = this->test_mode;
  
  clog << "info: " << logstream::info << "switching to zynq test mode " << input_mode << std::endl;

  /* setup the telnet connection */
  sockfd = ConnectTelnet();

  /* define the command to send over telnet */
  conv << "acq test " << (int)this->test_mode << std::endl;
  cmd = conv.str();
  
  Telnet(cmd, sockfd, false);
  
  close(sockfd);
  return this->test_mode;
}


/**
 * static function to stop acquisition by setting the instrument to NONE 
 */
int ZynqManager::StopAcquisition() {

  /* definitions */
  std::string status_string;
  int sockfd;

  clog << "info: " << logstream::info << "switching off the Zynq acquisition" << std::endl;

  /* setup the telnet connection */
  sockfd = ConnectTelnet();
  Telnet("instrument mode 0\n", sockfd, false);
  
  close(sockfd);
  return 0;
}


/**
 * set the number of packets for D1 and D2 required every 5.24 s
 * @param N1 the number of packets for data level 1 (1 - 4)
 * @param N2 the number of packets for data level 2 (1 - 4)
 */
int ZynqManager::SetNPkts(int N1, int N2) {

  /* definitions */
  std::string status_string;
  int sockfd;
  std::string cmd1, cmd2;
  std::stringstream conv1, conv2;

  clog << "info: " << logstream::info << "setting N1 to " << N1 << " and N2 to " << N2 << std::endl;

  /* create the command */
  conv1 << "mmg N1 " << N1 << std::endl;
  cmd1 = conv1.str();
  conv2 << "mmg N2 " << N2 << std::endl;
  cmd2 = conv2.str();

  /* setup the telnet connection */
  sockfd = ConnectTelnet();
  Telnet(cmd1, sockfd, false);
  Telnet(cmd2, sockfd, false);
 
  close(sockfd);
  return 0;
}


/**
 * set the configurable parameters of the L2 trigger
 * @param n_bg the number of times above background level the threshold is set
 * @param low_thresh the lowest possible value of the threshold
 */
int ZynqManager::SetL2TrigParams(int n_bg, int low_thresh) {

  /* definitions */
  std::string status_string;
  int sockfd;
  std::string cmd1, cmd2;
  std::stringstream conv1, conv2;

  clog << "info: " << logstream::info << "setting L2 parameters to N_BG: " << n_bg << " and LOW_THRESH: " << low_thresh << std::endl;

  /* create the command */
  conv1 << "trig n_bg " << n_bg << std::endl;
  cmd1 = conv1.str();
  conv2 << "trig low_thresh " << low_thresh << std::endl;
  cmd2 = conv2.str();

  /* setup the telnet connection */
  sockfd = ConnectTelnet();
  Telnet(cmd1, sockfd, false);
  Telnet(cmd2, sockfd, false);
 
  close(sockfd);
  return 0;
}


/**
 * get the Zynq version info
 */
std::string ZynqManager::GetZynqVer() {

  std::string zynq_ver = "";
  std::string cmd = "instrument ver\n";
  int sockfd;
  
  /* setup the telnet connection */
  sockfd = ConnectTelnet();

  /* ask for the version */
  zynq_ver = SendRecvTelnet(cmd, sockfd);
  
  close(sockfd);
  return zynq_ver;
} 


/**
 * Hide the corrupted Pixels give in DeadPixelMask.txt
 */
int ZynqManager::HidePixels() {

  int sockfd;
  
  clog << "info: " << logstream::info << "Hiding corrupted pixels" << std::endl;
  
  // Object containing the command list to send by telnet
  DeadPixelMask mask;
  
  // Check that DI R_USB0 etc are defined in the right place
  int n_max=mask.c2send.size();
  
  if(n_max>0){
    
    sockfd = ConnectTelnet();
    
    for(int i=0;i<n_max;i++){

      Telnet(mask.c2send[i].line,sockfd,true);
      Telnet(mask.c2send[i].asic,sockfd,true);
      Telnet(mask.c2send[i].pixel,sockfd,true);
      Telnet("slowctrl mask 1",sockfd,true);
    }
    
    close(sockfd);
    
  }
  else{

    //*it is possible to print mask.readed_file*//
    clog << "info: " << logstream::info << "No DeadPixelMask.txt found or the file is incorrect" << std::endl;
    
  }
  return 0;
}

