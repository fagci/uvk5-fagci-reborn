#ifndef RDS2_H
#define RDS2_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @ingroup group01
 *
 * @brief Data type for RDS Status command and response information
 *
 * @see Si47XX PROGRAMMING GUIDE; AN332 (REV 1.0); pages 77 and 78
 * @see Also https://en.wikipedia.org/wiki/Radio_Data_System
 */
typedef union {
  struct {
    uint8_t INTACK : 1; // Interrupt Acknowledge; 0 = RDSINT status preserved; 1
                        // = Clears RDSINT.
    uint8_t MTFIFO : 1; // Empty FIFO; 0 = If FIFO not empty; 1 = Clear RDS
                        // Receive FIFO.
    uint8_t STATUSONLY : 1; // Determines if data should be removed from the RDS
                            // FIFO.
    uint8_t dummy : 5;
  } arg;
  uint8_t raw;
} si47x_rds_command;

/**
 * @ingroup group01
 *
 * @brief Response data type for current channel and reads an entry from the RDS
 * FIFO.
 *
 * @see Si47XX PROGRAMMING GUIDE; AN332 (REV 1.0); pages 77 and 78
 */
typedef union {
  struct {
    // status ("RESP0")
    uint8_t STCINT : 1;
    uint8_t DUMMY1 : 1;
    uint8_t RDSINT : 1;
    uint8_t RSQINT : 1;
    uint8_t DUMMY2 : 2;
    uint8_t ERR : 1;
    uint8_t CTS : 1;
    // RESP1
    uint8_t RDSRECV : 1; //!<  RDS Received; 1 = FIFO filled to minimum number
                         //!<  of groups set by RDSFIFOCNT.
    uint8_t RDSSYNCLOST : 1; //!<  RDS Sync Lost; 1 = Lost RDS synchronization.
    uint8_t
        RDSSYNCFOUND : 1; //!<  RDS Sync Found; 1 = Found RDS synchronization.
    uint8_t DUMMY3 : 1;
    uint8_t RDSNEWBLOCKA : 1; //!<  RDS New Block A; 1 = Valid Block A data has
                              //!<  been received.
    uint8_t RDSNEWBLOCKB : 1; //!<  RDS New Block B; 1 = Valid Block B data has
                              //!<  been received.
    uint8_t DUMMY4 : 2;
    // RESP2
    uint8_t RDSSYNC : 1; //!<  RDS Sync; 1 = RDS currently synchronized.
    uint8_t DUMMY5 : 1;
    uint8_t GRPLOST : 1; //!<  Group Lost; 1 = One or more RDS groups discarded
                         //!<  due to FIFO overrun.
    uint8_t DUMMY6 : 5;
    // RESP3 to RESP11
    uint8_t RDSFIFOUSED; //!<  RESP3 - RDS FIFO Used; Number of groups remaining
                         //!<  in the RDS FIFO (0 if empty).
    uint8_t BLOCKAH;     //!<  RESP4 - RDS Block A; HIGH byte
    uint8_t BLOCKAL;     //!<  RESP5 - RDS Block A; LOW byte
    uint8_t BLOCKBH;     //!<  RESP6 - RDS Block B; HIGH byte
    uint8_t BLOCKBL;     //!<  RESP7 - RDS Block B; LOW byte
    uint8_t BLOCKCH;     //!<  RESP8 - RDS Block C; HIGH byte
    uint8_t BLOCKCL;     //!<  RESP9 - RDS Block C; LOW byte
    uint8_t BLOCKDH;     //!<  RESP10 - RDS Block D; HIGH byte
    uint8_t BLOCKDL;     //!<  RESP11 - RDS Block D; LOW byte
    // RESP12 - Blocks A to D Corrected Errors.
    // 0 = No errors;
    // 1 = 1–2 bit errors detected and corrected;
    // 2 = 3–5 bit errors detected and corrected.
    // 3 = Uncorrectable.
    uint8_t BLED : 2;
    uint8_t BLEC : 2;
    uint8_t BLEB : 2;
    uint8_t BLEA : 2;
  } resp;
  uint8_t raw[13];
} si47x_rds_status;

/**
 * @ingroup group01
 *
 * @brief FM_RDS_INT_SOURCE property data type
 *
 * @see Si47XX PROGRAMMING GUIDE; AN332 (REV 1.0); page 103
 * @see also https://en.wikipedia.org/wiki/Radio_Data_System
 */
typedef union {
  struct {
    uint8_t RDSRECV : 1;      //!<  If set, generate RDSINT when RDS FIFO has at
                              //!<  least FM_RDS_INT_FIFO_COUNT entries.
    uint8_t RDSSYNCLOST : 1;  //!<  If set, generate RDSINT when RDS loses
                              //!<  synchronization.
    uint8_t RDSSYNCFOUND : 1; //!<  f set, generate RDSINT when RDS gains
                              //!<  synchronization.
    uint8_t DUMMY1 : 1;       //!<  Always write to 0.
    uint8_t RDSNEWBLOCKA : 1; //!<  If set, generate an interrupt when Block A
                              //!<  data is found or subsequently changed
    uint8_t RDSNEWBLOCKB : 1; //!<  If set, generate an interrupt when Block B
                              //!<  data is found or subsequently changed
    uint8_t DUMMY2 : 5;       //!<  Reserved - Always write to 0.
    uint8_t DUMMY3 : 5;       //!<  Reserved - Always write to 0.
  } refined;
  uint8_t raw[2];
} si47x_rds_int_source;

/**
 * @ingroup group01
 *
 * @brief Data type for FM_RDS_CONFIG Property
 *
 * IMPORTANT: all block errors must be less than or equal the associated block
 * error threshold for the group to be stored in the RDS FIFO. 0 = No errors; 1
 * = 1–2 bit errors detected and corrected; 2 = 3–5 bit errors detected and
 * corrected; 3 = Uncorrectable. Recommended Block Error Threshold options:
 *  2,2,2,2 = No group stored if any errors are uncorrected.
 *  3,3,3,3 = Group stored regardless of errors.
 *  0,0,0,0 = No group stored containing corrected or uncorrected errors.
 *  3,2,3,3 = Group stored with corrected errors on B, regardless of errors on
 * A, C, or D.
 *
 * @see Si47XX PROGRAMMING GUIDE; AN332 (REV 1.0); pages 58 and 104
 */
typedef union {
  struct {
    uint8_t RDSEN : 1; //!<  1 = RDS Processing Enable.
    uint8_t DUMMY1 : 7;
    uint8_t BLETHD : 2; //!<  Block Error Threshold BLOCKD
    uint8_t BLETHC : 2; //!<  Block Error Threshold BLOCKC.
    uint8_t BLETHB : 2; //!<  Block Error Threshold BLOCKB.
    uint8_t BLETHA : 2; //!<  Block Error Threshold BLOCKA.
  } arg;
  uint8_t raw[2];
} si47x_rds_config;

/**
 * @ingroup group01
 *
 * @brief Block A data type
 */
typedef union {
  struct {
    uint16_t pi;
  } refined;
  struct {
    uint8_t highValue; // Most Significant uint8_t first
    uint8_t lowValue;
  } raw;
} si47x_rds_blocka;

/**
 * @ingroup group01
 *
 * @brief Block B data type
 *
 * @details For GCC on System-V ABI on 386-compatible (32-bit processors), the
 * following stands:
 *
 * 1) Bit-fields are allocated from right to left (least to most significant).
 * 2) A bit-field must entirely reside in a storage unit appropriate for its
 * declared type. Thus a bit-field never crosses its unit boundary. 3)
 * Bit-fields may share a storage unit with other struct/union members,
 * including members that are not bit-fields. Of course, struct members occupy
 * different parts of the storage unit. 4) Unnamed bit-fields' types do not
 * affect the alignment of a structure or union, although individual bit-fields'
 * member offsets obey the alignment constraints.
 *
 * @see also Si47XX PROGRAMMING GUIDE; AN332 (REV 1.0); pages 78 and 79
 * @see also https://en.wikipedia.org/wiki/Radio_Data_System
 */
typedef union {
  struct {
    uint16_t address : 2; // Depends on Group Type and Version codes. If 0A or
                          // 0B it is the Text Segment Address.
    uint16_t DI : 1;      // Decoder Controll bit
    uint16_t MS : 1;      // Music/Speech
    uint16_t TA : 1;      // Traffic Announcement
    uint16_t programType : 5;        // PTY (Program Type) code
    uint16_t trafficProgramCode : 1; // (TP) => 0 = No Traffic Alerts; 1 =
                                     // Station gives Traffic Alerts
    uint16_t versionCode : 1;        // (B0) => 0=A; 1=B
    uint16_t groupType : 4;          // Group Type code.
  } group0;
  struct {
    uint16_t address : 4; // Depends on Group Type and Version codes. If 2A or
                          // 2B it is the Text Segment Address.
    uint16_t textABFlag : 1;  // Do something if it chanhes from binary "0" to
                              // binary "1" or vice-versa
    uint16_t programType : 5; // PTY (Program Type) code
    uint16_t trafficProgramCode : 1; // (TP) => 0 = No Traffic Alerts; 1 =
                                     // Station gives Traffic Alerts
    uint16_t versionCode : 1;        // (B0) => 0=A; 1=B
    uint16_t groupType : 4;          // Group Type code.
  } group2;
  struct {
    uint16_t content : 4;     // Depends on Group Type and Version codes.
    uint16_t textABFlag : 1;  // Do something if it chanhes from binary "0" to
                              // binary "1" or vice-versa
    uint16_t programType : 5; // PTY (Program Type) code
    uint16_t trafficProgramCode : 1; // (TP) => 0 = No Traffic Alerts; 1 =
                                     // Station gives Traffic Alerts
    uint16_t versionCode : 1;        // (B0) => 0=A; 1=B
    uint16_t groupType : 4;          // Group Type code.
  } refined;
  struct {
    uint8_t lowValue;
    uint8_t highValue; // Most Significant byte first
  } raw;
} si47x_rds_blockb;

/*
 *
 *
 * Group type 4A ( RDS Date and Time)
 * When group type 4A is used by the station, it shall be transmitted every
 * minute according to EN 50067. This Structure uses blocks 2,3 and 5 (B,C,D)
 *
 * Commented due to “Crosses boundary” on GCC 32-bit plataform.
 */
/*
typedef union {
    struct
    {
        uint32_t offset : 5;       // Local Time Offset
        uint32_t offset_sense : 1; // Local Offset Sign ( 0 = + , 1 = - )
        uint32_t minute : 6;       // UTC Minutes
        uint32_t hour : 5;         // UTC Hours
        uint32_t mjd : 17;        // Modified Julian Day Code
    } refined;
    uint8_t raw[6];
} si47x_rds_date_time;
*/

/**
 * @ingroup group01
 *
 * Group type 4A ( RDS Date and Time)
 * When group type 4A is used by the station, it shall be transmitted every
 * minute according to EN 50067. This Structure uses blocks 2,3 and 5 (B,C,D)
 *
 * ATTENTION:
 * To make it compatible with 8, 16 and 32 bits platforms and avoid Crosses
 * boundary, it was necessary to split minute and hour representation.
 */
/*
typedef union
{
    struct
    {
        uint8_t offset : 5;       // Local Time Offset
        uint8_t offset_sense : 1; // Local Offset Sign ( 0 = + , 1 = - )
        uint8_t minute1 : 2;      // UTC Minutes - 2 bits less significant (void
“Crosses boundary”). uint8_t minute2 : 4;      // UTC Minutes - 4 bits  more
significant  (void “Crosses boundary”) uint8_t hour1 : 4;        // UTC Hours -
4 bits less significant (void “Crosses boundary”) uint8_t hour2 : 1;        //
UTC Hours - 4 bits more significant (void “Crosses boundary”) uint16_t mjd1 :
15;        // Modified Julian Day Code - 15  bits less significant (void
“Crosses boundary”) uint16_t mjd2 : 2;         // Modified Julian Day Code - 2
bits more significant (void “Crosses boundary”) } refined; uint8_t raw[6]; }
si47x_rds_date_time;
*/
typedef union {
  struct {
    uint32_t offset : 5;       // Local Time Offset
    uint32_t offset_sense : 1; // Local Offset Sign ( 0 = + , 1 = - )
    uint32_t minute : 6;       // UTC Minutes
    uint32_t hour : 5;         // UTC Hours
    uint32_t mjd : 17;         // Modified Julian Day Code
  } refined;
  uint8_t raw[6];
} si47x_rds_date_time;

#endif /* end of include guard: RDS2_H */
