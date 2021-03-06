/*
address location 

  name.devices  hex  ============ dec
  lcd i2c  0x27 ============ 39
  ds3231   0x68 ============ 104




*/

#define addressEeprom 80
#define addressRTC 104

#define delayScan 500
#define DELAYVAL 1000
#define NUMPIXELS 1

#define baudrate 115200

// ======== define pinout ============//

#define ledBoard PB9
#define buttonBoard PB8

#define rl1 PB12
#define rl2 PB13
#define rl3 PB14
#define rl4 PB15
#define rl5 PA8
#define rl6 PA9
#define rl7 PA10

// ======== define pinout ============//

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_NeoPixel.h>
#include <SimpleTimer.h>

SimpleTimer timerSerial, timerButton;

LiquidCrystal_I2C lcd(0x27, 20, 4);
Adafruit_NeoPixel pixels(NUMPIXELS, ledBoard, NEO_GRB + NEO_KHZ800);

HardwareSerial Serial2(PA3, PA2);
HardwareSerial Serial3(PB11, PB10);

bool rtcLive, eepromLive, buttonLive, canbusLive, modbusLive;
int counterButton;

String toolName = "EHUBV3 CHECKER";

String boardSerialSend = "EHUBV3 Send";
String boardSerialReceive = "EHUBV3 Receive";

bool boardSendStatus, boardReceiveStatus;

#define DEBUG 0

typedef enum
{
  STANDARD_FORMAT = 0,
  EXTENDED_FORMAT
} CAN_FORMAT;

/* Symbolic names for type of CAN message                                    */
typedef enum
{
  DATA_FRAME = 0,
  REMOTE_FRAME
} CAN_FRAME;

/* Symbolic names for bit rate of CAN message                                */
//typedef enum {CAN_50KBPS, CAN_100KBPS, CAN_125KBPS, CAN_250KBPS, CAN_500KBPS, CAN_1000KBPS} BITRATE;
typedef enum
{
  CAN_50KBPS,
  CAN_100KBPS,
  CAN_125KBPS,
  CAN_250KBPS
} BITRATE;

/* Symbolic names for formats of CAN message                                 */

typedef struct
{
  uint32_t id;     /* 29 bit identifier                               */
  uint8_t data[8]; /* Data field                                      */
  uint8_t len;     /* Length of data field in bytes                   */
  uint8_t ch;      /* Object channel(Not use)                         */
  uint8_t format;  /* 0 - STANDARD, 1- EXTENDED IDENTIFIER            */
  uint8_t type;    /* 0 - DATA FRAME, 1 - REMOTE FRAME                */
} CAN_msg_t;

typedef const struct
{
  uint8_t TS2;
  uint8_t TS1;
  uint8_t BRP;
} CAN_bit_timing_config_t;

CAN_bit_timing_config_t can_configs[6] = {{2, 13, 45}, {2, 15, 20}, {2, 13, 18}, {2, 13, 9}, {2, 15, 4}, {2, 15, 2}};

/**
 * Print registers.
*/
void printRegister(char *buf, uint32_t reg)
{
  if (DEBUG == 0)
    return;
  Serial3.print(buf);
  Serial3.print(reg, HEX);
  Serial3.println();
}

/**
 * Initializes the CAN filter registers.
 *
 * @preconditions   - This register can be written only when the filter initialization mode is set (FINIT=1) in the CAN_FMR register.
 * @params: index   - Specified filter index. index 27:14 are available in connectivity line devices only.
 * @params: scale   - Select filter scale.
 *                    0: Dual 16-bit scale configuration
 *                    1: Single 32-bit scale configuration
 * @params: mode    - Select filter mode.
 *                    0: Two 32-bit registers of filter bank x are in Identifier Mask mode
 *                    1: Two 32-bit registers of filter bank x are in Identifier List mode
 * @params: fifo    - Select filter assigned.
 *                    0: Filter assigned to FIFO 0
 *                    1: Filter assigned to FIFO 1
 * @params: bank1   - Filter bank register 1
 * @params: bank2   - Filter bank register 2
 *
 */
void CANSetFilter(uint8_t index, uint8_t scale, uint8_t mode, uint8_t fifo, uint32_t bank1, uint32_t bank2)
{
  if (index > 27)
    return;

  CAN1->FA1R &= ~(0x1UL << index); // Deactivate filter

  if (scale == 0)
  {
    CAN1->FS1R &= ~(0x1UL << index); // Set filter to Dual 16-bit scale configuration
  }
  else
  {
    CAN1->FS1R |= (0x1UL << index); // Set filter to single 32 bit configuration
  }
  if (mode == 0)
  {
    CAN1->FM1R &= ~(0x1UL << index); // Set filter to Mask mode
  }
  else
  {
    CAN1->FM1R |= (0x1UL << index); // Set filter to List mode
  }

  if (fifo == 0)
  {
    CAN1->FFA1R &= ~(0x1UL << index); // Set filter assigned to FIFO 0
  }
  else
  {
    CAN1->FFA1R |= (0x1UL << index); // Set filter assigned to FIFO 1
  }

  CAN1->sFilterRegister[index].FR1 = bank1; // Set filter bank registers1
  CAN1->sFilterRegister[index].FR2 = bank2; // Set filter bank registers2

  CAN1->FA1R |= (0x1UL << index); // Activate filter
}

/**
 * Initializes the CAN controller with specified bit rate.
 *
 * @params: bitrate - Specified bitrate. If this value is not one of the defined constants, bit rate will be defaulted to 125KBS
 * @params: remap   - Select CAN port. 
 *                    =0:CAN_RX mapped to PA11, CAN_TX mapped to PA12
 *                    =1:Not used
 *                    =2:CAN_RX mapped to PB8, CAN_TX mapped to PB9 (not available on 36-pin package)
 *                    =3:CAN_RX mapped to PD0, CAN_TX mapped to PD1 (available on 100-pin and 144-pin package)
 *
 */
bool CANInit(BITRATE bitrate, int remap)
{

  // Reference manual
  // https://www.st.com/content/ccc/resource/technical/document/reference_manual/59/b9/ba/7f/11/af/43/d5/CD00171190.pdf/files/CD00171190.pdf/jcr:content/translations/en.CD00171190.pdf

  RCC->APB1ENR |= 0x2000000UL; // Enable CAN clock
  RCC->APB2ENR |= 0x1UL;       // Enable AFIO clock
  AFIO->MAPR &= 0xFFFF9FFF;    // reset CAN remap
                               // CAN_RX mapped to PA11, CAN_TX mapped to PA12

  if (remap == 0)
  {
    RCC->APB2ENR |= 0x4UL;      // Enable GPIOA clock
    GPIOA->CRH &= ~(0xFF000UL); // Configure PA12(0b0000) and PA11(0b0000)
                                // 0b0000
                                //   MODE=00(Input mode)
                                //   CNF=00(Analog mode)

    GPIOA->CRH |= 0xB8FFFUL; // Configure PA12(0b1011) and PA11(0b1000)
                             // 0b1011
                             //   MODE=11(Output mode, max speed 50 MHz)
                             //   CNF=10(Alternate function output Push-pull
                             // 0b1000
                             //   MODE=00(Input mode)
                             //   CNF=10(Input with pull-up / pull-down)

    GPIOA->ODR |= 0x1UL << 12; // PA12 Upll-up
  }

  if (remap == 2)
  {
    AFIO->MAPR |= 0x00004000; // set CAN remap
                              // CAN_RX mapped to PB8, CAN_TX mapped to PB9 (not available on 36-pin package)

    RCC->APB2ENR |= 0x8UL;   // Enable GPIOB clock
    GPIOB->CRH &= ~(0xFFUL); // Configure PB9(0b0000) and PB8(0b0000)
                             // 0b0000
                             //   MODE=00(Input mode)
                             //   CNF=00(Analog mode)

    GPIOB->CRH |= 0xB8UL; // Configure PB9(0b1011) and PB8(0b1000)
                          // 0b1011
                          //   MODE=11(Output mode, max speed 50 MHz)
                          //   CNF=10(Alternate function output Push-pull
                          // 0b1000
                          //   MODE=00(Input mode)
                          //   CNF=10(Input with pull-up / pull-down)

    GPIOB->ODR |= 0x1UL << 8; // PB8 Upll-up
  }

  if (remap == 3)
  {
    AFIO->MAPR |= 0x00005000; // set CAN remap
                              // CAN_RX mapped to PD0, CAN_TX mapped to PD1 (available on 100-pin and 144-pin package)

    RCC->APB2ENR |= 0x20UL;  // Enable GPIOD clock
    GPIOD->CRL &= ~(0xFFUL); // Configure PD1(0b0000) and PD0(0b0000)
                             // 0b0000
                             //   MODE=00(Input mode)
                             //   CNF=00(Analog mode)

    GPIOD->CRH |= 0xB8UL; // Configure PD1(0b1011) and PD0(0b1000)
                          // 0b1000
                          //   MODE=00(Input mode)
                          //   CNF=10(Input with pull-up / pull-down)
                          // 0b1011
                          //   MODE=11(Output mode, max speed 50 MHz)
                          //   CNF=10(Alternate function output Push-pull

    GPIOD->ODR |= 0x1UL << 0; // PD0 Upll-up
  }

  CAN1->MCR |= 0x1UL; // Require CAN1 to Initialization mode
  while (!(CAN1->MSR & 0x1UL))
    ; // Wait for Initialization mode

  //CAN1->MCR = 0x51UL;                 // Hardware initialization(No automatic retransmission)
  CAN1->MCR = 0x41UL; // Hardware initialization(With automatic retransmission)

  // Set bit rates
  CAN1->BTR &= ~(((0x03) << 24) | ((0x07) << 20) | ((0x0F) << 16) | (0x1FF));
  CAN1->BTR |= (((can_configs[bitrate].TS2 - 1) & 0x07) << 20) | (((can_configs[bitrate].TS1 - 1) & 0x0F) << 16) | ((can_configs[bitrate].BRP - 1) & 0x1FF);

  // Configure Filters to default values
  CAN1->FMR |= 0x1UL;      // Set to filter initialization mode
  CAN1->FMR &= 0xFFFFC0FF; // Clear CAN2 start bank

  // bxCAN has 28 filters.
  // These filters are used for both CAN1 and CAN2.
  // STM32F103 has only CAN1, so all 28 are used for CAN1
  CAN1->FMR |= 0x1C << 8; // Assign all filters to CAN1

  // Set fileter 0
  // Single 32-bit scale configuration
  // Two 32-bit registers of filter bank x are in Identifier Mask mode
  // Filter assigned to FIFO 0
  // Filter bank register to all 0
  CANSetFilter(0, 1, 0, 0, 0x0UL, 0x0UL);

  CAN1->FMR &= ~(0x1UL); // Deactivate initialization mode

  uint16_t TimeoutMilliseconds = 1000;
  bool can1 = false;
  CAN1->MCR &= ~(0x1UL); // Require CAN1 to normal mode

  // Wait for normal mode
  // If the connection is not correct, it will not return to normal mode.
  for (uint16_t wait_ack = 0; wait_ack < TimeoutMilliseconds; wait_ack++)
  {
    if ((CAN1->MSR & 0x1UL) == 0)
    {
      can1 = true;
      break;
    }
    delayMicroseconds(1000);
  }
  //Serial3.print("can1=");
  //Serial3.println(can1);
  if (can1)
  {
    Serial3.println("CAN1 initialize ok");
  }
  else
  {
    Serial3.println("CAN1 initialize fail!!");
    return false;
  }
  return true;
}

#define STM32_CAN_TIR_TXRQ (1U << 0U) // Bit 0: Transmit Mailbox Request
#define STM32_CAN_RIR_RTR (1U << 1U)  // Bit 1: Remote Transmission Request
#define STM32_CAN_RIR_IDE (1U << 2U)  // Bit 2: Identifier Extension
#define STM32_CAN_TIR_RTR (1U << 1U)  // Bit 1: Remote Transmission Request
#define STM32_CAN_TIR_IDE (1U << 2U)  // Bit 2: Identifier Extension

#define CAN_EXT_ID_MASK 0x1FFFFFFFU
#define CAN_STD_ID_MASK 0x000007FFU

/**
 * Decodes CAN messages from the data registers and populates a 
 * CAN message struct with the data fields.
 * 
 * @preconditions A valid CAN message is received
 * @params CAN_rx_msg - CAN message structure for reception
 * 
 */
void CANReceive(CAN_msg_t *CAN_rx_msg)
{
  uint32_t id = CAN1->sFIFOMailBox[0].RIR;
  if ((id & STM32_CAN_RIR_IDE) == 0)
  { // Standard frame format
    CAN_rx_msg->format = STANDARD_FORMAT;
    ;
    CAN_rx_msg->id = (CAN_STD_ID_MASK & (id >> 21U));
  }
  else
  { // Extended frame format
    CAN_rx_msg->format = EXTENDED_FORMAT;
    ;
    CAN_rx_msg->id = (CAN_EXT_ID_MASK & (id >> 3U));
  }

  if ((id & STM32_CAN_RIR_RTR) == 0)
  { // Data frame
    CAN_rx_msg->type = DATA_FRAME;
  }
  else
  { // Remote frame
    CAN_rx_msg->type = REMOTE_FRAME;
  }

  CAN_rx_msg->len = (CAN1->sFIFOMailBox[0].RDTR) & 0xFUL;

  CAN_rx_msg->data[0] = 0xFFUL & CAN1->sFIFOMailBox[0].RDLR;
  CAN_rx_msg->data[1] = 0xFFUL & (CAN1->sFIFOMailBox[0].RDLR >> 8);
  CAN_rx_msg->data[2] = 0xFFUL & (CAN1->sFIFOMailBox[0].RDLR >> 16);
  CAN_rx_msg->data[3] = 0xFFUL & (CAN1->sFIFOMailBox[0].RDLR >> 24);
  CAN_rx_msg->data[4] = 0xFFUL & CAN1->sFIFOMailBox[0].RDHR;
  CAN_rx_msg->data[5] = 0xFFUL & (CAN1->sFIFOMailBox[0].RDHR >> 8);
  CAN_rx_msg->data[6] = 0xFFUL & (CAN1->sFIFOMailBox[0].RDHR >> 16);
  CAN_rx_msg->data[7] = 0xFFUL & (CAN1->sFIFOMailBox[0].RDHR >> 24);

  // Release FIFO 0 output mailbox.
  // Make the next incoming message available.
  CAN1->RF0R |= 0x20UL;
}

/**
 * Encodes CAN messages using the CAN message struct and populates the 
 * data registers with the sent.
 * 
 * @params CAN_tx_msg - CAN message structure for transmission
 * 
 */
void CANSend(CAN_msg_t *CAN_tx_msg)
{
  volatile int count = 0;

  uint32_t out = 0;
  if (CAN_tx_msg->format == EXTENDED_FORMAT)
  { // Extended frame format
    out = ((CAN_tx_msg->id & CAN_EXT_ID_MASK) << 3U) | STM32_CAN_TIR_IDE;
  }
  else
  { // Standard frame format
    out = ((CAN_tx_msg->id & CAN_STD_ID_MASK) << 21U);
  }

  // Remote frame
  if (CAN_tx_msg->type == REMOTE_FRAME)
  {
    out |= STM32_CAN_TIR_RTR;
  }

  CAN1->sTxMailBox[0].TDTR &= ~(0xF);
  CAN1->sTxMailBox[0].TDTR |= CAN_tx_msg->len & 0xFUL;

  CAN1->sTxMailBox[0].TDLR = (((uint32_t)CAN_tx_msg->data[3] << 24) |
                              ((uint32_t)CAN_tx_msg->data[2] << 16) |
                              ((uint32_t)CAN_tx_msg->data[1] << 8) |
                              ((uint32_t)CAN_tx_msg->data[0]));
  CAN1->sTxMailBox[0].TDHR = (((uint32_t)CAN_tx_msg->data[7] << 24) |
                              ((uint32_t)CAN_tx_msg->data[6] << 16) |
                              ((uint32_t)CAN_tx_msg->data[5] << 8) |
                              ((uint32_t)CAN_tx_msg->data[4]));

  // Send Go
  CAN1->sTxMailBox[0].TIR = out | STM32_CAN_TIR_TXRQ;

  // Wait until the mailbox is empty
  while (CAN1->sTxMailBox[0].TIR & 0x1UL && count++ < 1000000)
    ;

  // The mailbox don't becomes empty while loop
  if (CAN1->sTxMailBox[0].TIR & 0x1UL)
  {
    Serial3.println("Send Fail");
    Serial3.println(CAN1->ESR);
    Serial3.println(CAN1->MSR);
    Serial3.println(CAN1->TSR);
  }
}

/**
 * Returns whether there are CAN messages available.
 *
 * @returns If pending CAN messages are in the CAN controller
 *
 */
uint8_t CANMsgAvail(void)
{
  // Check for pending FIFO 0 messages
  return CAN1->RF0R & 0x3UL;
}

uint8_t counter = 0;
uint8_t frameLength = 0;
unsigned long previousMillis = 0; // stores last time output was updated
const long interval = 1000;       // transmission interval (milliseconds)

void setuPin()
{
  Serial2.begin(baudrate);
  Serial3.begin(baudrate);
  pinMode(buttonBoard, INPUT);
  pinMode(rl1, OUTPUT);
  pinMode(rl2, OUTPUT);
  pinMode(rl3, OUTPUT);
  pinMode(rl4, OUTPUT);
  pinMode(rl5, OUTPUT);
  pinMode(rl6, OUTPUT);
  pinMode(rl7, OUTPUT);
}

void setupCanbus()
{
  // bool ret = CANInit(CAN_250KBPS, 2);  // CAN_RX mapped to PB8, CAN_TX mapped to PB9
  bool ret = CANInit(CAN_250KBPS, 0); // CAN_RX mapped to PA11, CAN_TX mapped to PA12
  //bool ret = CANInit(CAN_1000KBPS, 2);  // CAN_RX mapped to PB8, CAN_TX mapped to PB9
  //bool ret = CANInit(CAN_1000KBPS, 3);  // CAN_RX mapped to PD0, CAN_TX mapped to PD1
  Serial3.println("status : " + String(ret));
  if (!ret)
    while (true)
      ;
}

#include "mcp_can.h"
#include <SPI.h>
const int SPI_CS_PIN = PA4;
MCP_CAN CAN(SPI_CS_PIN);

void setupMCP2515()
{
  //  SPI.setClockDivider(SPI_CLOCK_DIV16);
  Serial3.println("Setup MCP");
  while (CAN_OK != CAN.begin(CAN_250KBPS, MCP_8MHz)) // init can bus : baudrate = 500k
  {
    Serial3.println("CAN BUS Shield init fail");
    Serial3.println(" Init CAN BUS Shield again");
    delay(100);
  }

  Serial3.println("CAN OK : " + String(CAN_OK));
  canbusLive = true;
}

void setup()
{
  setuPin();
  setupCanbus();
  setupMCP2515();
  timerSerial.setInterval(1000, boardSerialSender);
  timerButton.setInterval(100, checkButton);
  lcd.init();
  pixels.begin();
  lcd.backlight();
  lcd.setCursor(3, 0);
  lcd.print(toolName);
  i2cScanner();
  checkLed();

  while (!buttonLive)
  {
    checkButton();
  }
}

void loopCanbus()
{
  CAN_msg_t CAN_TX_msg;
  CAN_msg_t CAN_RX_msg;

  CAN_TX_msg.data[0] = 0x00;
  CAN_TX_msg.data[1] = 0x01;
  CAN_TX_msg.data[2] = 0x02;
  CAN_TX_msg.data[3] = 0x03;
  CAN_TX_msg.data[4] = 0x04;
  CAN_TX_msg.data[5] = 0x05;
  CAN_TX_msg.data[6] = 0x06;
  CAN_TX_msg.data[7] = 0x07;
  CAN_TX_msg.len = frameLength;

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;
    if ((counter % 2) == 0)
    {
      CAN_TX_msg.type = DATA_FRAME;
      if (CAN_TX_msg.len == 0)
        CAN_TX_msg.type = REMOTE_FRAME;
      CAN_TX_msg.format = EXTENDED_FORMAT;
      CAN_TX_msg.id = 0x32F103;
    }
    else
    {
      CAN_TX_msg.type = DATA_FRAME;
      if (CAN_TX_msg.len == 0)
        CAN_TX_msg.type = REMOTE_FRAME;
      CAN_TX_msg.format = STANDARD_FORMAT;
      CAN_TX_msg.id = 0x103;
    }
    CANSend(&CAN_TX_msg);
    frameLength++;
    if (frameLength == 9)
      frameLength = 0;
    counter++;
  }

  if (CANMsgAvail())
  {
    CANReceive(&CAN_RX_msg);

    if (CAN_RX_msg.format == EXTENDED_FORMAT)
    {
      Serial3.print("Extended ID: 0x");
      if (CAN_RX_msg.id < 0x10000000)
        Serial3.print("0");
      if (CAN_RX_msg.id < 0x1000000)
        Serial3.print("00");
      if (CAN_RX_msg.id < 0x100000)
        Serial3.print("000");
      if (CAN_RX_msg.id < 0x10000)
        Serial3.print("0000");
      Serial3.print(CAN_RX_msg.id, HEX);
    }
    else
    {
      Serial3.print("Standard ID: 0x");
      if (CAN_RX_msg.id < 0x100)
        Serial3.print("0");
      if (CAN_RX_msg.id < 0x10)
        Serial3.print("00");
      Serial3.print(CAN_RX_msg.id, HEX);
      Serial3.print("     ");
    }

    Serial3.print(" DLC: ");
    Serial3.print(CAN_RX_msg.len);
    if (CAN_RX_msg.type == DATA_FRAME)
    {
      Serial3.print(" Data: ");
      for (int i = 0; i < CAN_RX_msg.len; i++)
      {
        Serial3.print("0x");
        Serial3.print(CAN_RX_msg.data[i], HEX);
        if (i != (CAN_RX_msg.len - 1))
          Serial3.print(" ");
      }
      Serial3.println();
    }
    else
    {
      Serial3.println(" Data: REMOTE REQUEST FRAME");
    }
  }
}

void loopMCP2515()
{
  unsigned char stmp[8] = {1, 2, 3, 4, 5, 6, 7, 8};

  CAN.sendMsgBuf(0x00, CAN_EXTID, 0, 8, stmp);
  delay(100); // send data per 100ms
}

void i2cScanner()
{

  byte error, address;
  int nDevices;
  nDevices = 0;

  Wire.beginTransmission(addressRTC);
  error = Wire.endTransmission();
  if (error == 0)
  {
    rtcLive = true;
  }

  delay(delayScan);

  Wire.beginTransmission(addressEeprom);
  error = Wire.endTransmission();
  if (error == 0)
  {
    eepromLive = true;
  }

  lcd.setCursor(0, 1);
  String rtcStatus = " NG", eepromStatus = " NG";
  if (rtcLive)
  {
    rtcStatus = " OK";
  }

  if (eepromLive)
  {
    eepromStatus = " OK";
  }

  lcd.setCursor(0, 1);
  lcd.print("RTC :" + rtcStatus + " EEPROM :" + eepromStatus);
  lcd.setCursor(0, 2);

  if (canbusLive)
  {
    lcd.print("CANBUS Init: OK");
  }
  else if (canbusLive)
  {
    lcd.print("CANBUS Init: NG");
  }

  delay(delayScan);
}

void checkLed()
{
  pixels.clear();
  pixels.setPixelColor(0, pixels.Color(150, 0, 0));
  pixels.show();
  delay(DELAYVAL);

  pixels.clear();
  pixels.setPixelColor(0, pixels.Color(0, 150, 0));
  pixels.show();
  delay(DELAYVAL);

  pixels.clear();
  pixels.setPixelColor(0, pixels.Color(0, 0, 150));
  pixels.show();
  delay(DELAYVAL);
  pixels.clear();
  pixels.setPixelColor(0, pixels.Color(0, 0, 0));
  pixels.show();
}

void checkButton()
{
  if (digitalRead(buttonBoard) == HIGH)
  {
    lcd.setCursor(0, 3);

    pixels.clear();
    delay(1000);
    if (counterButton == 0)
    {
      lcd.print("Tbl: OK, LED Hijau");
      pixels.setPixelColor(0, pixels.Color(0, 150, 0)); // hijau
      counterButton = 1;
    }

    else if (counterButton == 1)
    {
      lcd.print("Tbl: OK, LED Merah");
      pixels.setPixelColor(0, pixels.Color(150, 0, 0)); // hijau
      counterButton = 2;
    }

    else if (counterButton == 2)
    {
      lcd.print("Tbl: OK, LED Biru ");
      pixels.setPixelColor(0, pixels.Color(0, 0, 150)); // hijau
      pixels.show();
      counterButton = 0;
      delay(2000);
      pixels.clear();
      pixels.setPixelColor(0, pixels.Color(0, 0, 0));
      buttonLive = true;
      lcd.clear();
    }

    pixels.show();
  }
}

// void boardCheckStatus()
// {

//   if (!boardReceiveStatus)
//   {
//     boardSerialSender();
//   }

// }

void boardSerialSender()
{
  if (!boardReceiveStatus)
  {
    Serial3.println(boardSerialSend);
    boardSendStatus = true;
  }
}

void boardSerialReceiver()
{
  if (Serial3.available() > 0)
  {
    Serial3.println("fahmi");
    

    // boardReceiveStatus = true;
  }
}

void switchRelay()
{
  digitalWrite(rl1, HIGH);
  digitalWrite(rl2, HIGH);
  digitalWrite(rl3, HIGH);
  digitalWrite(rl4, HIGH);
  digitalWrite(rl5, HIGH);
  digitalWrite(rl6, HIGH);
  digitalWrite(rl7, HIGH);
  delay(3000);
  digitalWrite(rl1, LOW);
  digitalWrite(rl2, LOW);
  digitalWrite(rl3, LOW);
  digitalWrite(rl4, LOW);
  digitalWrite(rl5, LOW);
  digitalWrite(rl6, LOW);
  digitalWrite(rl7, LOW);
  delay(3000);
}

void printSerial()
{
  Serial3.println("Sundaya2");
  delay(1000);
  Serial2.println("Sundaya3");
  delay(1000);
}

void loop()
{
  printSerial();
  
  // boardSerialSender();
  // timerSerial.run();
  // boardSerialReceiver();
  // loopCanbus();
  // loopMCP2515();
  // delay(1000);
  // timerButton.run();
  // checkButton();
  // switchRelay();

  // delay(1000);
}
