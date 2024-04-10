#include "rds2.h"
#include <string.h>

/*******************************************************************************
 * RDS implementation
 ******************************************************************************/

char rds_buffer2A[65]; //!<  RDS Radio Text buffer - Program Information
char rds_buffer2B[33]; //!<  RDS Radio Text buffer - Station Informaation
char rds_buffer0A[9];  //!<  RDS Basic tuning and switching information (Type 0
                       //!<  groups)
char rds_time[25];     //!<  RDS date time received information

int rdsTextAdress2A; //!<  rds_buffer2A current position
int rdsTextAdress2B; //!<  rds_buffer2B current position
int rdsTextAdress0A; //!<  rds_buffer0A current position

bool rdsEndGroupA = false;
bool rdsEndGroupB = false;
uint8_t lastTextFlagAB;

si47x_rds_status currentRdsStatus;

/**
 * @ingroup group16 RDS setup
 * @brief Clear RDS buffer 2A (Radio Text / Program Information)
 * @details same clearRdsProgramInformation
 */
inline void SI4735_clearRdsBuffer2A() {
  memset(rds_buffer2A, 0, sizeof(rds_buffer2A));
};
/**
 * @ingroup group16 RDS setup
 * @brief Clear RDS buffer 2A (Radio Text / Program Information)
 * @details same clearRdsBuffer2A
 */
inline void SI4735_clearRdsProgramInformation() {
  memset(rds_buffer2A, 0, sizeof(rds_buffer2A));
};

/**
 * @ingroup group16 RDS setup
 * @brief Clear RDS buffer 2B (text / Station INformation 32 bytes)
 * @details Same clearRdsStationInformation
 */
inline void SI4735_clearRdsBuffer2B() {
  memset(rds_buffer2B, 0, sizeof(rds_buffer2B));
};
/**
 * @ingroup group16 RDS setup
 * @brief Clear RDS buffer 2B (text / Station INformation 32 bytes)
 * @details Same clearRdsBuffer2B
 */
inline void SI4735_clearRdsStationInformation() {
  memset(rds_buffer2B, 0, sizeof(rds_buffer2B));
};

/**
 * @ingroup group16 RDS setup
 * @brief Clear RDS buffer 0A (text / Station Name)
 * @details clearRdsStationName
 */
inline void SI4735_clearRdsBuffer0A() {
  memset(rds_buffer0A, 0, sizeof(rds_buffer0A));
};
/**
 * @ingroup group16 RDS setup
 * @brief Clear RDS buffer 0A (text / Station Name)
 * @details clearRdsBuffer0A
 */
inline void SI4735_clearRdsStationName() {
  memset(rds_buffer0A, 0, sizeof(rds_buffer0A));
};

inline void SI4735_rdsBeginQuery() { SI4735_getRdsStatus(0, 0, 0); };

/**
 * @ingroup group16 RDS
 * @brief Get the Rds Received FIFO
 * @details if FIFO is 1, it means the minimum number of groups was filled
 * @return true if minimum number of groups was filled.
 */
inline bool SI4735_getRdsReceived() { return currentRdsStatus.resp.RDSRECV; };

/**
 * @ingroup group16 RDS
 * @brief Get the Rds Sync Lost object
 * @details returns true (1) if Lost RDS synchronization is detected.
 * @return true if Lost RDS synchronization detected.
 */
inline bool SI4735_getRdsSyncLost() {
  return currentRdsStatus.resp.RDSSYNCLOST;
};

/**
 * @ingroup group16 RDS
 * @brief Get the Rds Sync Found
 * @details return true if found RDS synchronization
 * @return true if found RDS synchronization
 */
inline bool SI4735_getRdsSyncFound() {
  return currentRdsStatus.resp.RDSSYNCFOUND;
};

/**
 * @ingroup group16 RDS
 * @brief Get the Rds New Block A
 *
 * @details Returns true if valid Block A data has been received.
 * @return true or false
 */
inline bool SI4735_getRdsNewBlockA() {
  return currentRdsStatus.resp.RDSNEWBLOCKA;
};

/**
 * @ingroup group16 RDS
 * @brief Get the Rds New Block B
 * @details Returns true if valid Block B data has been received.
 * @return true or false
 */
inline bool SI4735_getRdsNewBlockB() {
  return currentRdsStatus.resp.RDSNEWBLOCKB;
};

/**
 * @ingroup group16 RDS
 * @brief Get the Rds Sync
 * @details Returns true if RDS currently synchronized.
 * @return true or false
 */
inline bool SI4735_getRdsSync() { return currentRdsStatus.resp.RDSSYNC; };

/**
 * @ingroup group16 RDS
 * @brief Get the Group Lost
 * @details Returns true if one or more RDS groups discarded due to FIFO
 * overrun.
 * @return true or false
 */
inline bool SI4735_getGroupLost() { return currentRdsStatus.resp.GRPLOST; };

/**
 * @ingroup group16 RDS
 * @brief Get the Num Rds Fifo Used
 * @details Return the number of RDS FIFO used
 * @return uint8_t Total RDS FIFO used
 */
inline uint8_t SI4735_getNumRdsFifoUsed() {
  return currentRdsStatus.resp.RDSFIFOUSED;
};

/**
 * @ingroup group16 RDS
 * @brief Sets the minimum number of RDS groups stored in the RDS FIFO before
 * RDSRECV is set.
 * @details Return the number of RDS FIFO used
 * @param value from 0 to 25. Default value is 0.
 */
inline void SI4735_setFifoCount(uint16_t value) {
  SI4735_sendProperty(FM_RDS_INT_FIFO_COUNT, value);
};

/**
 * @ingroup group16 RDS
 * @brief Check if 0xD or 0xA special characters were received for group A
 * @see resetEndIndicatorGroupA
 * @return true or false
 */
inline bool SI4735_getEndIndicatorGroupA() { return rdsEndGroupA; }

/**
 * @ingroup group16 RDS
 * @brief Resets 0xD or 0xA special characters condition (makes it false)
 * @see getEndIndicatorGroupA
 */
inline void SI4735_resetEndIndicatorGroupA() { rdsEndGroupA = false; }

/**
 * @ingroup group16 RDS
 * @brief Check if 0xD or 0xA special characters were received for group B
 * @see resetEndIndicatorGroupB
 * @return true or false
 */
inline bool SI4735_getEndIndicatorGroupB() { return rdsEndGroupB; }

/**
 * @ingroup group16 RDS
 * @brief Resets 0xD or 0xA special characters condition (makes it false)
 * @see getEndIndicatorGroupB
 */
inline void SI4735_resetEndIndicatorGroupB() { rdsEndGroupB = false; }

/**
 * @ingroup group16 RDS status
 * @brief Empty FIFO
 * @details  Clear RDS Receive FIFO.
 * @see getRdsStatus
 */
inline void SI4735_rdsClearFifo() { SI4735_getRdsStatus(0, 1, 0); }

/**
 * @ingroup group16 RDS status
 * @brief Clears RDSINT.
 * @details  INTACK Interrupt Acknowledge; 0 = RDSINT status preserved. 1 =
 * Clears RDSINT.
 * @see getRdsStatus
 */
inline void SI4735_rdsClearInterrupt() { SI4735_getRdsStatus(1, 0, 0); }

void SI4735_convertToChar(uint16_t value, char *strValue, uint8_t len,
                          uint8_t dot, uint8_t separator,
                          bool remove_leading_zeros) {
  char d;
  for (int i = (len - 1); i >= 0; i--) {
    d = value % 10;
    value = value / 10;
    strValue[i] = d + 48;
  }
  strValue[len] = '\0';
  if (dot > 0) {
    for (int i = len; i >= dot; i--) {
      strValue[i + 1] = strValue[i];
    }
    strValue[dot] = separator;
  }

  if (remove_leading_zeros) {
    if (strValue[0] == '0') {
      strValue[0] = ' ';
      if (strValue[1] == '0')
        strValue[1] = ' ';
    }
  }
}

/**
 * @ingroup group16 RDS setup
 *
 * @brief  Starts the control member variables for RDS.
 *
 * @details This method is called by setRdsConfig()
 *
 * @see setRdsConfig()
 */
void SI4735_RdsInit() {
  SI4735_clearRdsBuffer2A();
  SI4735_clearRdsBuffer2B();
  SI4735_clearRdsBuffer0A();
  rdsTextAdress2A = rdsTextAdress2B = lastTextFlagAB = rdsTextAdress0A = 0;
}

// See inlines methods / functions on SI4735.h

/**
 * @ingroup group16 RDS status
 *
 * @brief Returns the programa type.
 *
 * @details Read the Block A content
 *
 * @see Si47XX PROGRAMMING GUIDE; AN332 (REV 1.0); pages 77 and 78
 *
 * @return BLOCKAL
 */
uint16_t SI4735_getRdsPI(void) {
  if (SI4735_getRdsReceived() && SI4735_getRdsNewBlockA()) {
    return currentRdsStatus.resp.BLOCKAL;
  }
  return 0;
}

/**
 * @ingroup group16 RDS status
 *
 * @brief Returns the Group Type (extracted from the Block B)
 *
 * @return BLOCKBL
 */
uint8_t SI4735_getRdsGroupType(void) {
  si47x_rds_blockb blkb;

  blkb.raw.lowValue = currentRdsStatus.resp.BLOCKBL;
  blkb.raw.highValue = currentRdsStatus.resp.BLOCKBH;

  return blkb.refined.groupType;
}

/**
 * @ingroup group16 RDS status
 *
 * @brief Returns the current Text Flag A/B
 *
 * @return uint8_t current Text Flag A/B
 */
uint8_t SI4735_getRdsFlagAB(void) {
  si47x_rds_blockb blkb;

  blkb.raw.lowValue = currentRdsStatus.resp.BLOCKBL;
  blkb.raw.highValue = currentRdsStatus.resp.BLOCKBH;

  return blkb.refined.textABFlag;
}

/**
 * @ingroup group16 RDS status
 *
 * @brief Returns the address of the text segment.
 *
 * @details 2A - Each text segment in version 2A groups consists of four
 * characters. A messages of this group can be have up to 64 characters.
 * @details 2B - In version 2B groups, each text segment consists of only two
 * characters. When the current RDS status is using this version, the maximum
 * message length will be 32 characters.
 *
 * @return uint8_t the address of the text segment.
 */
uint8_t SI4735_getRdsTextSegmentAddress(void) {
  si47x_rds_blockb blkb;
  blkb.raw.lowValue = currentRdsStatus.resp.BLOCKBL;
  blkb.raw.highValue = currentRdsStatus.resp.BLOCKBH;

  return blkb.refined.content;
}

/**
 * @ingroup group16 RDS status
 *
 * @brief Gets the version code (extracted from the Block B)
 *
 * @returns  0=A or 1=B
 */
uint8_t SI4735_getRdsVersionCode(void) {
  si47x_rds_blockb blkb;

  blkb.raw.lowValue = currentRdsStatus.resp.BLOCKBL;
  blkb.raw.highValue = currentRdsStatus.resp.BLOCKBH;

  return blkb.refined.versionCode;
}

/**
 * @ingroup group16 RDS status
 *
 * @brief Returns the Program Type (extracted from the Block B)
 *
 * @see https://en.wikipedia.org/wiki/Radio_Data_System
 *
 * @return program type (an integer betwenn 0 and 31)
 */
uint8_t SI4735_getRdsProgramType(void) {
  si47x_rds_blockb blkb;

  blkb.raw.lowValue = currentRdsStatus.resp.BLOCKBL;
  blkb.raw.highValue = currentRdsStatus.resp.BLOCKBH;

  return blkb.refined.programType;
}

/**
 * @ingroup group16 RDS status
 *
 * @brief Process data received from group 2B
 *
 * @param c  char array reference to the "group 2B" text
 */
void SI4735_getNext2Block(char *c) {
  c[1] = currentRdsStatus.resp.BLOCKDL;
  c[0] = currentRdsStatus.resp.BLOCKDH;
}

/**
 * @ingroup group16 RDS status
 *
 * @brief Process data received from group 2A
 *
 * @param c  char array reference to the "group  2A" text
 */
void SI4735_getNext4Block(char *c) {
  c[0] = currentRdsStatus.resp.BLOCKCH;
  c[1] = currentRdsStatus.resp.BLOCKCL;
  c[2] = currentRdsStatus.resp.BLOCKDH;
  c[3] = currentRdsStatus.resp.BLOCKDL;
}

/**
 * @ingroup group16 RDS status
 *
 * @brief Gets the RDS Text when the message is of the Group Type 2 version A
 *
 * @return char*  The string (char array) with the content (Text) received from
 * group 2A
 */
char *SI4735_getRdsText(void) {

  // Needs to get the "Text segment address code".
  // Each message should be ended by the code 0D (Hex)

  if (rdsTextAdress2A >= 16)
    rdsTextAdress2A = 0;

  SI4735_getNext4Block(&rds_buffer2A[rdsTextAdress2A * 4]);

  rdsTextAdress2A += 4;

  return rds_buffer2A;
}

/**
 * @ingroup group16 RDS status
 * @todo RDS Dynamic PS or Scrolling PS
 * @brief Gets the station name and other messages.
 *
 * @return char* should return a string with the station name.
 *         However, some stations send other kind of messages
 */
char *SI4735_getRdsText0A(void) {
  si47x_rds_blockb blkB;

  if (SI4735_getRdsReceived()) {
    if (SI4735_getRdsGroupType() == 0) {
      if (lastTextFlagAB != SI4735_getRdsFlagAB()) {
        lastTextFlagAB = SI4735_getRdsFlagAB();
        SI4735_clearRdsBuffer0A();
      }
      // Process group type 0
      blkB.raw.highValue = currentRdsStatus.resp.BLOCKBH;
      blkB.raw.lowValue = currentRdsStatus.resp.BLOCKBL;

      rdsTextAdress0A = blkB.group0.address;
      if (rdsTextAdress0A >= 0 && rdsTextAdress0A < 4) {
        SI4735_getNext2Block(&rds_buffer0A[rdsTextAdress0A * 2]);
        rds_buffer0A[8] = '\0';
        return rds_buffer0A;
      }
    }
  }
  return NULL;
}

/**
 * @ingroup group16 RDS status
 *
 * @brief Gets the Text processed for the 2A group
 *
 * @return char* string with the Text of the group A2
 */
char *SI4735_getRdsText2A(void) {
  si47x_rds_blockb blkB;

  // getRdsStatus();
  if (SI4735_getRdsReceived()) {
    if (SI4735_getRdsGroupType() == 2 /* && getRdsVersionCode() == 0 */) {
      // Process group 2A
      // Decode B block information
      blkB.raw.highValue = currentRdsStatus.resp.BLOCKBH;
      blkB.raw.lowValue = currentRdsStatus.resp.BLOCKBL;
      rdsTextAdress2A = blkB.group2.address;

      if (rdsTextAdress2A >= 0 && rdsTextAdress2A < 16) {
        SI4735_getNext4Block(&rds_buffer2A[rdsTextAdress2A * 4]);
        rds_buffer2A[63] = '\0';
        return rds_buffer2A;
      }
    }
  }
  return NULL;
}

/**
 * @ingroup group16 RDS status
 *
 * @brief Gets the Text processed for the 2B group
 *
 * @return char* string with the Text of the group AB
 */
char *SI4735_getRdsText2B(void) {
  si47x_rds_blockb blkB;

  // getRdsStatus();
  // if (getRdsReceived())
  // {
  // if (getRdsNewBlockB())
  // {
  if (SI4735_getRdsGroupType() == 2 /* && getRdsVersionCode() == 1 */) {
    // Process group 2B
    blkB.raw.highValue = currentRdsStatus.resp.BLOCKBH;
    blkB.raw.lowValue = currentRdsStatus.resp.BLOCKBL;
    rdsTextAdress2B = blkB.group2.address;
    if (rdsTextAdress2B >= 0 && rdsTextAdress2B < 16) {
      SI4735_getNext2Block(&rds_buffer2B[rdsTextAdress2B * 2]);
      rds_buffer2B[32] = '\0';
      return rds_buffer2B;
    }
  }
  //  }
  // }
  return NULL;
}

/**
 * @ingroup group16 RDS Time and Date
 *
 * @brief Gets the RDS time and date when the Group type is 4
 * @details Returns theUTC Time and offset (to convert it to local time)
 * @details return examples:
 * @details                 12:31 +03:00
 * @details                 21:59 -02:30
 *
 * @return  point to char array. Format:  +/-hh:mm (offset)
 */
char *SI4735_getRdsTime() {
  // Under Test and construction
  // Need to check the Group Type before.
  si47x_rds_date_time dt;

  uint16_t minute;
  uint16_t hour;

  if (SI4735_getRdsGroupType() == 4) {
    char offset_sign;
    int offset_h;
    int offset_m;

    // uint16_t y, m, d;

    dt.raw[4] = currentRdsStatus.resp.BLOCKBL;
    dt.raw[5] = currentRdsStatus.resp.BLOCKBH;
    dt.raw[2] = currentRdsStatus.resp.BLOCKCL;
    dt.raw[3] = currentRdsStatus.resp.BLOCKCH;
    dt.raw[0] = currentRdsStatus.resp.BLOCKDL;
    dt.raw[1] = currentRdsStatus.resp.BLOCKDH;

    // Unfortunately it was necessary dues to  the GCC compiler on 32-bit
    // platform. See si47x_rds_date_time (typedef union) and CGG “Crosses
    // boundary” issue/features. Now it is working on Atmega328, STM32, Arduino
    // DUE, ESP32 and more.
    minute = dt.refined.minute;
    hour = dt.refined.hour;

    offset_sign = (dt.refined.offset_sense == 1) ? '+' : '-';
    offset_h = (dt.refined.offset * 30) / 60;
    offset_m = (dt.refined.offset * 30) - (offset_h * 60);
    // sprintf(rds_time, "%02u:%02u %c%02u:%02u", dt.refined.hour,
    // dt.refined.minute, offset_sign, offset_h, offset_m); sprintf(rds_time,
    // "%02u:%02u %c%02u:%02u", hour, minute, offset_sign, offset_h, offset_m);

    // Using convertToChar instead sprintf to save space (about 1.2K on
    // ATmega328 compiler tools).

    if (offset_h > 12 || offset_m > 60 || hour > 24 || minute > 60)
      return NULL;

    SI4735_convertToChar(hour, rds_time, 2, 0, ' ', false);
    rds_time[2] = ':';
    SI4735_convertToChar(minute, &rds_time[3], 2, 0, ' ', false);
    rds_time[5] = ' ';
    rds_time[6] = offset_sign;
    SI4735_convertToChar(offset_h, &rds_time[7], 2, 0, ' ', false);
    rds_time[9] = ':';
    SI4735_convertToChar(offset_m, &rds_time[10], 2, 0, ' ', false);
    rds_time[12] = '\0';

    return rds_time;
  }

  return NULL;
}

/**
 * @ingroup group16 RDS
 * @brief Gets Station Name, Station Information, Program Information and
 * utcTime
 * @details This function populates four char pointer variable parameters with
 * Station Name, Station Information, Programa Information and UTC time.
 * @details You must call  setRDS(true), setRdsFifo(true) before calling
 * getRdsAllData(...)
 * @details ATTENTION: You don't need to call any additional function to obtain
 * the RDS information; simply follow the steps outlined below.
 * @details ATTENTION: If no data is found for the given parameter, it is
 * assigned a NULL value. Prior to using the pointers variable, make sure to
 * check if it is null.
 * @details the right way to call this function is shown below.
 * @code {.cpp}
 *
 * void setup() {
 *   rx.setup(RESET_PIN, FM_FUNCTION);
 *   rx.setFM(8400, 10800, currentFrequency, 10);
 *   delay(500);
 *   rx.setRdsConfig(3, 3, 3, 3, 3);
 *   rx.setFifoCount(1);
 * }
 *
 * char *utcTime;
 * char *stationName;
 * char *programInfo;
 * char *stationInfo;
 *
 * void showStationName() {
 *   if (stationName != NULL) {
 *     // do something
 *    }
 *  }
 *
 * void showStationInfo() {
 *   if (stationInfo != NULL) {
 *     // do something
 *     }
 *  }
 *
 * void showProgramaInfo() {
 *  if (programInfo != NULL) {
 *    // do something
 *  }
 * }
 *
 * void showUtcTime() {
 *   if (rdsTime != NULL) {
 *     // do something
 *   }
 * }
 *
 * void loop() {
 *   .
 *   .
 *   .
 *   if (rx.isCurrentTuneFM()) {
 *     // The char pointers above will be populate by the call below. So, the
 * char pointers need to be passed by reference (pointer to pointer). if
 * (rx.getRdsAllData(&stationName, &stationInfo , &programInfo, &rdsTime) ) {
 *         showProgramaInfo(programInfo); // you need check if programInfo is
 * null in showProgramaInfo showStationName(stationName); // you need check if
 * stationName is null in showStationName showStationInfo(stationInfo); // you
 * need check if stationInfo is null in showStationInfo showUtcTime(rdsTime); //
 * // you need check if rdsTime is null in showUtcTime
 *     }
 *   }
 *   .
 *   .
 *   .
 *   delay(5);
 * }
 * @endcode
 * @details ATTENTION: the parameters below are point to point to array of char.
 * @param stationName (reference)  - if NOT NULL,  point to Name of the Station
 * (char array -  9 bytes)
 * @param stationInformation (reference)  - if NOT NULL, point to Station
 * information (char array - 33 bytes)
 * @param programInformation (reference)  - if NOT NULL, point to program
 * information (char array - 65 nytes)
 * @param utcTime  (reference)  - if NOT NULL, point to char array containing
 * the current UTC time (format HH:MM:SS +HH:MM)
 * @return True if found at least one valid data
 * @see setRDS, setRdsFifo, getRdsAllData
 */
bool SI4735_getRdsAllData(char **stationName, char **stationInformation,
                          char **programInformation, char **utcTime) {
  SI4735_rdsBeginQuery();
  if (!SI4735_getRdsReceived())
    return false;
  if (!SI4735_getRdsSync() || SI4735_getNumRdsFifoUsed() == 0)
    return false;
  *stationName = SI4735_getRdsText0A();        // returns NULL if no information
  *stationInformation = SI4735_getRdsText2B(); // returns NULL if no information
  *programInformation = SI4735_getRdsText2A(); // returns NULL if no information
  *utcTime = SI4735_getRdsTime();              // returns NULL if no information

  return (bool)stationName | (bool)stationInformation |
         (bool)programInformation | (bool)utcTime;
}

/**
 * @ingroup group16 RDS Modified Julian Day Converter (MJD)
 * @brief Converts the MJD number to integers Year, month and day
 * @details Calculates day, month and year based on MJD
 * @details This MJD algorithm is an adaptation of the javascript code found at
 * http://www.csgnetwork.com/julianmodifdateconv.html
 * @param mjd   mjd number
 * @param year  year variable reference
 * @param month month variable reference
 * @param day day variable reference
 */
void SI4735_mjdConverter(uint32_t mjd, uint32_t *year, uint32_t *month,
                         uint32_t *day) {
  uint32_t jd, ljd, njd;
  jd = mjd + 2400001;
  ljd = jd + 68569;
  njd = (uint32_t)(4 * ljd / 146097);
  ljd = ljd - (uint32_t)((146097 * njd + 3) / 4);
  *year = (uint32_t)(4000 * (ljd + 1) / 1461001);
  ljd = ljd - (uint32_t)((1461 * (*year) / 4)) + 31;
  *month = (uint32_t)(80 * ljd / 2447);
  *day = ljd - (uint32_t)(2447 * (*month) / 80);
  ljd = (uint32_t)(*month / 11);
  *month = (uint32_t)(*month + 2 - 12 * ljd);
  *year = (uint32_t)(100 * (njd - 49) + (*year) + ljd);
}

/**
 * @ingroup group16 RDS Time and Date
 * @brief   Decodes the RDS time to LOCAL Julian Day and time
 * @details This method gets the RDS date time massage and converts it from MJD
 * to JD and UTC time to local time
 * @details The Date and Time service may not work correctly depending on the FM
 * station that provides the service.
 * @details I have noticed that some FM stations do not use the service properly
 * in my location.
 * @details Example:
 * @code
 *      uint16_t year, month, day, hour, minute;
 *      .
 *      .
 *      si4735.getRdsStatus();
 *      si4735.getRdsDateTime(&year, &month, &day, &hour, &minute);
 *      .
 *      .
 * @endcode
 * @param rYear  year variable reference
 * @param rMonth month variable reference
 * @param rDay day variable reference
 * @param rHour local hour variable reference
 * @param rMinute local minute variable reference
 * @return true, it the RDS Date and time were processed
 */
bool SI4735_getRdsDateTime(uint16_t *rYear, uint16_t *rMonth, uint16_t *rDay,
                           uint16_t *rHour, uint16_t *rMinute) {
  si47x_rds_date_time dt;

  int16_t local_minute;
  uint16_t minute;
  uint16_t hour;
  uint32_t mjd, day, month, year;

  if (SI4735_getRdsGroupType() == 4) {

    dt.raw[4] = currentRdsStatus.resp.BLOCKBL;
    dt.raw[5] = currentRdsStatus.resp.BLOCKBH;
    dt.raw[2] = currentRdsStatus.resp.BLOCKCL;
    dt.raw[3] = currentRdsStatus.resp.BLOCKCH;
    dt.raw[0] = currentRdsStatus.resp.BLOCKDL;
    dt.raw[1] = currentRdsStatus.resp.BLOCKDH;

    // Unfortunately the resource below was necessary dues to  the GCC compiler
    // on 32-bit platform. See si47x_rds_date_time (typedef union) and CGG
    // “Crosses boundary” issue/features. Now it is working on Atmega328, STM32,
    // Arduino DUE, ESP32 and more.

    mjd = dt.refined.mjd;

    minute = dt.refined.minute;
    hour = dt.refined.hour;

    // calculates the jd Year, Month and Day base on mjd number
    // mjdConverter(mjd, &year, &month, &day);

    // Converting UTC to local time
    local_minute =
        ((hour * 60) + minute) +
        ((dt.refined.offset * 30) * ((dt.refined.offset_sense == 1) ? -1 : 1));
    if (local_minute < 0) {
      local_minute += 1440;
      mjd--; // drecreases one day
    } else if (local_minute > 1440) {
      local_minute -= 1440;
      mjd++; // increases one day
    }

    // calculates the jd Year, Month and Day base on mjd number
    SI4735_mjdConverter(mjd, &year, &month, &day);

    hour = (uint16_t)local_minute / 60;
    minute = local_minute - (hour * 60);

    if (hour > 24 || minute > 60 || day > 31 || month > 12)
      return false;

    *rYear = (uint16_t)year;
    *rMonth = (uint16_t)month;
    *rDay = (uint16_t)day;
    *rHour = hour;
    *rMinute = minute;

    return true;
  }
  return false;
}

/**
 * @ingroup group16 RDS Time and Date
 * @brief Gets the RDS the Time and Date when the Group type is 4
 * @details Returns the Date, UTC Time and offset (to convert it to local time)
 * @details return examples:
 * @details                 2021-07-29 12:31 +03:00
 * @details                 1964-05-09 21:59 -02:30
 *
 * @return array of char yy-mm-dd hh:mm +/-hh:mm offset
 */
char *SI4735_getRdsDateTimeString() {
  si47x_rds_date_time dt;

  uint16_t minute;
  uint16_t hour;
  uint32_t mjd, day, month, year;

  if (SI4735_getRdsGroupType() == 4) {
    char offset_sign;
    int offset_h;
    int offset_m;

    dt.raw[4] = currentRdsStatus.resp.BLOCKBL;
    dt.raw[5] = currentRdsStatus.resp.BLOCKBH;
    dt.raw[2] = currentRdsStatus.resp.BLOCKCL;
    dt.raw[3] = currentRdsStatus.resp.BLOCKCH;
    dt.raw[0] = currentRdsStatus.resp.BLOCKDL;
    dt.raw[1] = currentRdsStatus.resp.BLOCKDH;

    // Unfortunately the resource below was necessary dues to  the GCC compiler
    // on 32-bit platform. See si47x_rds_date_time (typedef union) and CGG
    // “Crosses boundary” issue/features. Now it is working on Atmega328, STM32,
    // Arduino DUE, ESP32 and more.

    mjd |= dt.refined.mjd;

    minute = dt.refined.minute;
    hour = dt.refined.hour;

    // calculates the jd (Year, Month and Day) base on mjd number
    SI4735_mjdConverter(mjd, &year, &month, &day);

    // Calculating hour, minute and offset
    offset_sign = (dt.refined.offset_sense == 1) ? '+' : '-';
    offset_h = (dt.refined.offset * 30) / 60;
    offset_m = (dt.refined.offset * 30) - (offset_h * 60);

    // Converting the result to array char -
    // Using convertToChar instead sprintf to save space (about 1.2K on
    // ATmega328 compiler tools).

    if (offset_h > 12 || offset_m > 60 || hour > 24 || minute > 60 ||
        day > 31 || month > 12)
      return NULL;

    SI4735_convertToChar(year, rds_time, 4, 0, ' ', false);
    rds_time[4] = '-';
    SI4735_convertToChar(month, &rds_time[5], 2, 0, ' ', false);
    rds_time[7] = '-';
    SI4735_convertToChar(day, &rds_time[8], 2, 0, ' ', false);
    rds_time[10] = ' ';
    SI4735_convertToChar(hour, &rds_time[11], 2, 0, ' ', false);
    rds_time[13] = ':';
    SI4735_convertToChar(minute, &rds_time[14], 2, 0, ' ', false);
    rds_time[16] = ' ';
    rds_time[17] = offset_sign;
    SI4735_convertToChar(offset_h, &rds_time[18], 2, 0, ' ', false);
    rds_time[20] = ':';
    SI4735_convertToChar(offset_m, &rds_time[21], 2, 0, ' ', false);
    rds_time[23] = '\0';

    return rds_time;
  }

  return NULL;
}
