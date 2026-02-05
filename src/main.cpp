#include "main.h"
#include <Arduino.h>
#include <stdio.h>
#include <mcp_can.h>
#include <SPI.h>

const char carVIN[] = "1D6RR6LM2GS888489"; // change to match the VIN in the radio or you will get a screen saying "Code Error, Please restart"
/*
  this will get translated to the following CAN messages:
  x0 31 44 36 52 52 36 4c
  x1 4d 32 47 53 38 38 38
  x2 34 38 39 00 00 00 00 
*/
const uint CS_PORT=PIN_CAN_CS;
// SPIClass mSPI(PA7,PA6,PA5,PA4);
// SPIClass(uint32_t mosi, uint32_t miso, uint32_t sclk, uint32_t ssel = PNUM_NOT_DEFINED);
// MCP_CAN CAN0(&mSPI, CS_PORT); // Set CS pin to PA3 (this goes to "INT" on mcp2515 board)
MCP_CAN CAN0(CS_PORT); // Set CS pin to PA3 (this goes to "INT" on mcp2515 board)

uint32_t last = 0;
int cMSG = 0;
uint32_t txDly = 100; // 100 mSec

/*
  https://mhhauto.com/Thread-SOLVED-UCONNECT-8-4-on-bench

  11-03-2020, 05:51 AM (This post was last modified: 11-03-2020, 05:52 AM by Gpgarage.)
  ID 122, DLC 4, 04 02 00 00  ,100ms
  ID 2FA, DLC 8, 00 01 00 48 00 00 00 00  ,100ms
  ID 3D3, DLC 8, 55 55 55 55 55 55 55 55  ,100ms
  ID 3E8, DLC 8, 00 XX XX XX XX XX XX XX  ,100ms
  ID 3E8, DLC 8, 01 XX XX XX XX XX XX XX  ,100ms
  ID 3E8, DLC 8, 02 XX XX XX 00 00 00 00  ,100ms
  //XX - car vin
  ..... can ihs 125kbit/s
*/
//  ^^^ from above
//    {.id = {0x3D3}, .len = {8}, .buf = {0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55}},
CAN_message_t can_messages[] = {
    {.id = {0x122}, .len = {4}, .buf = {0x04, 0x02, 0x00, 0x00}},
    {.id = {0x2FA}, .len = {8}, .buf = {0x00, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00}},
    {.id = {0x3E0}, .len = {8}, .buf = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {.id = {0x3E0}, .len = {8}, .buf = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {.id = {0x3E0}, .len = {8}, .buf = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
    {.id = {0x3E0}, .len = {8}, .buf = {0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}};


// Function to convert a 17-character VIN string to uint8_t arrays with hex representation
VinToHexArrayResult convertVINtoHex(const char *inputString)
{
  VinToHexArrayResult result; // Create a result struct
  // Ensure the input string is exactly 17 characters long
  if (strlen(inputString) != 17)
  {
    // Optionally handle error: for now, return arrays with 0x00 if string is invalid
    memset(&result, 0x00, sizeof(result));
    return result;
  }

  // Initialize the first element of each array (weird Dodge / Fiat way to paginate the msg)
  result.A[0] = 0x00; // A starts with 0x00
  result.B[0] = 0x01; // B starts with 0x01
  result.C[0] = 0x02; // C starts with 0x02

  // Copy 7 characters as hex to the remaining 7 elements of A
  for (int i = 0; i < 7; i++)
  {
    result.A[i + 1] = (uint8_t)inputString[i]; // Convert char to its ASCII hex value
  }

  // Copy the next 7 characters as hex to the remaining 7 elements of B
  for (int i = 0; i < 7; i++)
  {
    result.B[i + 1] = (uint8_t)inputString[i + 7]; // Convert char to its ASCII hex value
  }

  // Copy the final 3 characters as hex to the remaining 3 elements of C
  for (int i = 0; i < 3; i++)
  {
    result.C[i + 1] = (uint8_t)inputString[i + 14]; // Convert char to its ASCII hex value
  }

  // Fill the remaining elements of C with 0x00
  for (int i = 4; i < 8; i++)
  {
    result.C[i] = 0x00;
  }

  return result; // Return the struct containing the arrays
}

// Function to replace existing struct array data with returned values from convertVINtoHex
void updateExistingStructArray(CAN_message_t mcan_messages[], const char *inputString)
{
  // Call the convertVINtoHex function to get the new data
  VinToHexArrayResult newData = convertVINtoHex(inputString);
  // Replace the existing array data with the new vin data
  memcpy(mcan_messages[3].buf, newData.A, 8 * sizeof(uint8_t)); // Replace array pos 3
  memcpy(mcan_messages[4].buf, newData.B, 8 * sizeof(uint8_t)); // Replace array pos 4
  memcpy(mcan_messages[5].buf, newData.C, 8 * sizeof(uint8_t)); // Replace array pos 5
}

/*
  Transmit the CAN message, capture and display an
 * error core in case of failure.
 */
void loop()
{

  if (millis() / txDly != last) // executes every txDly ms
  {
    last = millis() / txDly;
//sndStat 6
//CAN0.getError() 11

    byte sndStat = CAN0.sendMsgBuf(can_messages[cMSG].id, 0, can_messages[cMSG].len, can_messages[cMSG].buf);
    if (sndStat == CAN_OK)
    {
      Serial.println("Message Sent Successfully!");
    }
    else
    {
      Serial.println("Error Sending Message...");
      Serial.println(sndStat);
        Serial.println(CAN0.getError()); 
    }

#if DEBUG_MODE
    // debug msges
    Serial.print("sending: id:");
    Serial.print(" x");
    Serial.print(can_messages[cMSG].id, HEX);
    Serial.print("  len: ");
    Serial.print(can_messages[cMSG].len);
    Serial.print("  msg:");
    for (int i = 0; i < can_messages[cMSG].len; i++)
    {
      Serial.print(" x");
      Serial.print((char)can_messages[cMSG].buf[i], HEX);
    }
    Serial.println("");
// end debug msges
#endif
    if (cMSG < (sizeof(can_messages) / sizeof(can_messages[0]) - 1))
    {
      cMSG++;
    }
    else
    {
      cMSG = 0;
    }
    digitalWrite(PIN_LED, !digitalRead(PIN_LED)); // blink the LED
  }
}

void setup()
{
  pinMode(PIN_LED, OUTPUT);
  Serial.begin(115200);
  delay(6000); // wait for 6 second

  if (CAN0.begin(MCP_ANY, CAN_125KBPS, MCP_16MHZ) == CAN_OK)
    Serial.println("MCP2515 Initialized Successfully!");
  else
    Serial.println("Error Initializing MCP2515...");
  CAN0.setMode(MCP_NORMAL); // Change to normal mode to allow messages to be transmitted

  updateExistingStructArray(can_messages, carVIN);

}
