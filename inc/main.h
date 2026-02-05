#include <Arduino.h>




// print debug information

#ifndef DEBUG_MODE
#define DEBUG_MODE 1
#endif



// This struct is directly copied from Teensy FlexCAN library to retain compatibility with it.
// Source: https://github.com/tonton81/FlexCAN_T4/

 struct CAN_message_t {
  uint32_t id =0;         // can identifier
  uint16_t timestamp = 0;  // time when message arrived
  uint8_t idhit = 0;       // filter that id came from
  struct {
    bool extended = 0;     // identifier is extended (29-bit)
    bool remote = 0;       // remote transmission request packet type
    bool overrun = 0;      // message overrun
    bool reserved = 0;
  } flags;
  uint8_t len = 8;         // length of data
  uint8_t buf[8] = { 0 };  // data
  int8_t mb = 0;           // used to identify mailbox reception
  uint8_t bus = 1;         // used to identify where the message came (CAN1, CAN2 or CAN3)
  bool seq = 0;            // sequential frames
} ;


// Struct to hold three arrays for the VIN in Fiat/Dodge/RAM format
struct VinToHexArrayResult
{
  uint8_t A[8];
  uint8_t B[8];
  uint8_t C[8];
};