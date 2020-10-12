# 1 "d:\\Sundaya\\File JS Pro\\Ehub V3\\code\\EHUB V3 tes board\\tesBoard\\tesBoard.ino"
# 1 "d:\\Sundaya\\File JS Pro\\Ehub V3\\code\\EHUB V3 tes board\\tesBoard\\tesBoard.ino"
/*

address location 



  name.devices  hex  ============ dec

  lcd i2c  0x27 ============ 39

  ds3231   0x68 ============ 104









*/
// ======== define pinout ============//
# 35 "d:\\Sundaya\\File JS Pro\\Ehub V3\\code\\EHUB V3 tes board\\tesBoard\\tesBoard.ino"
// ======== define pinout ============//

# 38 "d:\\Sundaya\\File JS Pro\\Ehub V3\\code\\EHUB V3 tes board\\tesBoard\\tesBoard.ino" 2
# 39 "d:\\Sundaya\\File JS Pro\\Ehub V3\\code\\EHUB V3 tes board\\tesBoard\\tesBoard.ino" 2
# 40 "d:\\Sundaya\\File JS Pro\\Ehub V3\\code\\EHUB V3 tes board\\tesBoard\\tesBoard.ino" 2
# 41 "d:\\Sundaya\\File JS Pro\\Ehub V3\\code\\EHUB V3 tes board\\tesBoard\\tesBoard.ino" 2

SimpleTimer timerSerial, timerButton;

LiquidCrystal_I2C lcd(0x27, 20, 4);
Adafruit_NeoPixel pixels(1, 0, ((1<<6) | (1<<4) | (0<<2) | (2)) /*|< Transmit as G,R,B*/ + 0x0000 /*|< 800 KHz data transmission*/);

HardwareSerial Serial2(23 /* A3*/, 22 /* A2*/);
HardwareSerial Serial3(31, 30);

bool rtcLive, eepromLive, buttonLive, canbusLive, modbusLive;
int counterButton;

String toolName = "EHUBV3 CHECKER";

String boardSerialSend = "EHUBV3 Send";
String boardSerialReceive = "EHUBV3 Receive";

bool boardSendStatus, boardReceiveStatus;



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
  uint32_t id; /* 29 bit identifier                               */
  uint8_t data[8]; /* Data field                                      */
  uint8_t len; /* Length of data field in bytes                   */
  uint8_t ch; /* Object channel(Not use)                         */
  uint8_t format; /* 0 - STANDARD, 1- EXTENDED IDENTIFIER            */
  uint8_t type; /* 0 - DATA FRAME, 1 - REMOTE FRAME                */
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
# 109 "d:\\Sundaya\\File JS Pro\\Ehub V3\\code\\EHUB V3 tes board\\tesBoard\\tesBoard.ino"
void printRegister(char *buf, uint32_t reg)
{
  if (0 == 0)
    return;
  Serial3.print(buf);
  Serial3.print(reg, 16);
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
# 136 "d:\\Sundaya\\File JS Pro\\Ehub V3\\code\\EHUB V3 tes board\\tesBoard\\tesBoard.ino"
void CANSetFilter(uint8_t index, uint8_t scale, uint8_t mode, uint8_t fifo, uint32_t bank1, uint32_t bank2)
{
  if (index > 27)
    return;

  ((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->FA1R &= ~(0x1UL << index); // Deactivate filter

  if (scale == 0)
  {
    ((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->FS1R &= ~(0x1UL << index); // Set filter to Dual 16-bit scale configuration
  }
  else
  {
    ((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->FS1R |= (0x1UL << index); // Set filter to single 32 bit configuration
  }
  if (mode == 0)
  {
    ((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->FM1R &= ~(0x1UL << index); // Set filter to Mask mode
  }
  else
  {
    ((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->FM1R |= (0x1UL << index); // Set filter to List mode
  }

  if (fifo == 0)
  {
    ((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->FFA1R &= ~(0x1UL << index); // Set filter assigned to FIFO 0
  }
  else
  {
    ((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->FFA1R |= (0x1UL << index); // Set filter assigned to FIFO 1
  }

  ((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->sFilterRegister[index].FR1 = bank1; // Set filter bank registers1
  ((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->sFilterRegister[index].FR2 = bank2; // Set filter bank registers2

  ((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->FA1R |= (0x1UL << index); // Activate filter
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
# 186 "d:\\Sundaya\\File JS Pro\\Ehub V3\\code\\EHUB V3 tes board\\tesBoard\\tesBoard.ino"
bool CANInit(BITRATE bitrate, int remap)
{

  // Reference manual
  // https://www.st.com/content/ccc/resource/technical/document/reference_manual/59/b9/ba/7f/11/af/43/d5/CD00171190.pdf/files/CD00171190.pdf/jcr:content/translations/en.CD00171190.pdf

  ((RCC_TypeDef *)((0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00020000UL) + 0x00001000UL))->APB1ENR |= 0x2000000UL; // Enable CAN clock
  ((RCC_TypeDef *)((0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00020000UL) + 0x00001000UL))->APB2ENR |= 0x1UL; // Enable AFIO clock
  ((AFIO_TypeDef *)((0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00010000UL) + 0x00000000UL))->MAPR &= 0xFFFF9FFF; // reset CAN remap
                               // CAN_RX mapped to PA11, CAN_TX mapped to PA12

  if (remap == 0)
  {
    ((RCC_TypeDef *)((0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00020000UL) + 0x00001000UL))->APB2ENR |= 0x4UL; // Enable GPIOA clock
    ((GPIO_TypeDef *)((0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00010000UL) + 0x00000800UL))->CRH &= ~(0xFF000UL); // Configure PA12(0b0000) and PA11(0b0000)
                                // 0b0000
                                //   MODE=00(Input mode)
                                //   CNF=00(Analog mode)

    ((GPIO_TypeDef *)((0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00010000UL) + 0x00000800UL))->CRH |= 0xB8FFFUL; // Configure PA12(0b1011) and PA11(0b1000)
                             // 0b1011
                             //   MODE=11(Output mode, max speed 50 MHz)
                             //   CNF=10(Alternate function output Push-pull
                             // 0b1000
                             //   MODE=00(Input mode)
                             //   CNF=10(Input with pull-up / pull-down)

    ((GPIO_TypeDef *)((0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00010000UL) + 0x00000800UL))->ODR |= 0x1UL << 12; // PA12 Upll-up
  }

  if (remap == 2)
  {
    ((AFIO_TypeDef *)((0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00010000UL) + 0x00000000UL))->MAPR |= 0x00004000; // set CAN remap
                              // CAN_RX mapped to PB8, CAN_TX mapped to PB9 (not available on 36-pin package)

    ((RCC_TypeDef *)((0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00020000UL) + 0x00001000UL))->APB2ENR |= 0x8UL; // Enable GPIOB clock
    ((GPIO_TypeDef *)((0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00010000UL) + 0x00000C00UL))->CRH &= ~(0xFFUL); // Configure PB9(0b0000) and PB8(0b0000)
                             // 0b0000
                             //   MODE=00(Input mode)
                             //   CNF=00(Analog mode)

    ((GPIO_TypeDef *)((0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00010000UL) + 0x00000C00UL))->CRH |= 0xB8UL; // Configure PB9(0b1011) and PB8(0b1000)
                          // 0b1011
                          //   MODE=11(Output mode, max speed 50 MHz)
                          //   CNF=10(Alternate function output Push-pull
                          // 0b1000
                          //   MODE=00(Input mode)
                          //   CNF=10(Input with pull-up / pull-down)

    ((GPIO_TypeDef *)((0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00010000UL) + 0x00000C00UL))->ODR |= 0x1UL << 8; // PB8 Upll-up
  }

  if (remap == 3)
  {
    ((AFIO_TypeDef *)((0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00010000UL) + 0x00000000UL))->MAPR |= 0x00005000; // set CAN remap
                              // CAN_RX mapped to PD0, CAN_TX mapped to PD1 (available on 100-pin and 144-pin package)

    ((RCC_TypeDef *)((0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00020000UL) + 0x00001000UL))->APB2ENR |= 0x20UL; // Enable GPIOD clock
    ((GPIO_TypeDef *)((0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00010000UL) + 0x00001400UL))->CRL &= ~(0xFFUL); // Configure PD1(0b0000) and PD0(0b0000)
                             // 0b0000
                             //   MODE=00(Input mode)
                             //   CNF=00(Analog mode)

    ((GPIO_TypeDef *)((0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00010000UL) + 0x00001400UL))->CRH |= 0xB8UL; // Configure PD1(0b1011) and PD0(0b1000)
                          // 0b1000
                          //   MODE=00(Input mode)
                          //   CNF=10(Input with pull-up / pull-down)
                          // 0b1011
                          //   MODE=11(Output mode, max speed 50 MHz)
                          //   CNF=10(Alternate function output Push-pull

    ((GPIO_TypeDef *)((0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00010000UL) + 0x00001400UL))->ODR |= 0x1UL << 0; // PD0 Upll-up
  }

  ((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->MCR |= 0x1UL; // Require CAN1 to Initialization mode
  while (!(((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->MSR & 0x1UL))
    ; // Wait for Initialization mode

  //CAN1->MCR = 0x51UL;                 // Hardware initialization(No automatic retransmission)
  ((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->MCR = 0x41UL; // Hardware initialization(With automatic retransmission)

  // Set bit rates
  ((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->BTR &= ~(((0x03) << 24) | ((0x07) << 20) | ((0x0F) << 16) | (0x1FF));
  ((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->BTR |= (((can_configs[bitrate].TS2 - 1) & 0x07) << 20) | (((can_configs[bitrate].TS1 - 1) & 0x0F) << 16) | ((can_configs[bitrate].BRP - 1) & 0x1FF);

  // Configure Filters to default values
  ((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->FMR |= 0x1UL; // Set to filter initialization mode
  ((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->FMR &= 0xFFFFC0FF; // Clear CAN2 start bank

  // bxCAN has 28 filters.
  // These filters are used for both CAN1 and CAN2.
  // STM32F103 has only CAN1, so all 28 are used for CAN1
  ((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->FMR |= 0x1C << 8; // Assign all filters to CAN1

  // Set fileter 0
  // Single 32-bit scale configuration
  // Two 32-bit registers of filter bank x are in Identifier Mask mode
  // Filter assigned to FIFO 0
  // Filter bank register to all 0
  CANSetFilter(0, 1, 0, 0, 0x0UL, 0x0UL);

  ((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->FMR &= ~(0x1UL); // Deactivate initialization mode

  uint16_t TimeoutMilliseconds = 1000;
  bool can1 = false;
  ((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->MCR &= ~(0x1UL); // Require CAN1 to normal mode

  // Wait for normal mode
  // If the connection is not correct, it will not return to normal mode.
  for (uint16_t wait_ack = 0; wait_ack < TimeoutMilliseconds; wait_ack++)
  {
    if ((((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->MSR & 0x1UL) == 0)
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
# 327 "d:\\Sundaya\\File JS Pro\\Ehub V3\\code\\EHUB V3 tes board\\tesBoard\\tesBoard.ino"
/**

 * Decodes CAN messages from the data registers and populates a 

 * CAN message struct with the data fields.

 * 

 * @preconditions A valid CAN message is received

 * @params CAN_rx_msg - CAN message structure for reception

 * 

 */
# 335 "d:\\Sundaya\\File JS Pro\\Ehub V3\\code\\EHUB V3 tes board\\tesBoard\\tesBoard.ino"
void CANReceive(CAN_msg_t *CAN_rx_msg)
{
  uint32_t id = ((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->sFIFOMailBox[0].RIR;
  if ((id & (1U << 2U) /* Bit 2: Identifier Extension*/) == 0)
  { // Standard frame format
    CAN_rx_msg->format = STANDARD_FORMAT;
    ;
    CAN_rx_msg->id = (0x000007FFU & (id >> 21U));
  }
  else
  { // Extended frame format
    CAN_rx_msg->format = EXTENDED_FORMAT;
    ;
    CAN_rx_msg->id = (0x1FFFFFFFU & (id >> 3U));
  }

  if ((id & (1U << 1U) /* Bit 1: Remote Transmission Request*/) == 0)
  { // Data frame
    CAN_rx_msg->type = DATA_FRAME;
  }
  else
  { // Remote frame
    CAN_rx_msg->type = REMOTE_FRAME;
  }

  CAN_rx_msg->len = (((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->sFIFOMailBox[0].RDTR) & 0xFUL;

  CAN_rx_msg->data[0] = 0xFFUL & ((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->sFIFOMailBox[0].RDLR;
  CAN_rx_msg->data[1] = 0xFFUL & (((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->sFIFOMailBox[0].RDLR >> 8);
  CAN_rx_msg->data[2] = 0xFFUL & (((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->sFIFOMailBox[0].RDLR >> 16);
  CAN_rx_msg->data[3] = 0xFFUL & (((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->sFIFOMailBox[0].RDLR >> 24);
  CAN_rx_msg->data[4] = 0xFFUL & ((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->sFIFOMailBox[0].RDHR;
  CAN_rx_msg->data[5] = 0xFFUL & (((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->sFIFOMailBox[0].RDHR >> 8);
  CAN_rx_msg->data[6] = 0xFFUL & (((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->sFIFOMailBox[0].RDHR >> 16);
  CAN_rx_msg->data[7] = 0xFFUL & (((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->sFIFOMailBox[0].RDHR >> 24);

  // Release FIFO 0 output mailbox.
  // Make the next incoming message available.
  ((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->RF0R |= 0x20UL;
}

/**

 * Encodes CAN messages using the CAN message struct and populates the 

 * data registers with the sent.

 * 

 * @params CAN_tx_msg - CAN message structure for transmission

 * 

 */
# 383 "d:\\Sundaya\\File JS Pro\\Ehub V3\\code\\EHUB V3 tes board\\tesBoard\\tesBoard.ino"
void CANSend(CAN_msg_t *CAN_tx_msg)
{
  volatile int count = 0;

  uint32_t out = 0;
  if (CAN_tx_msg->format == EXTENDED_FORMAT)
  { // Extended frame format
    out = ((CAN_tx_msg->id & 0x1FFFFFFFU) << 3U) | (1U << 2U) /* Bit 2: Identifier Extension*/;
  }
  else
  { // Standard frame format
    out = ((CAN_tx_msg->id & 0x000007FFU) << 21U);
  }

  // Remote frame
  if (CAN_tx_msg->type == REMOTE_FRAME)
  {
    out |= (1U << 1U) /* Bit 1: Remote Transmission Request*/;
  }

  ((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->sTxMailBox[0].TDTR &= ~(0xF);
  ((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->sTxMailBox[0].TDTR |= CAN_tx_msg->len & 0xFUL;

  ((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->sTxMailBox[0].TDLR = (((uint32_t)CAN_tx_msg->data[3] << 24) |
                              ((uint32_t)CAN_tx_msg->data[2] << 16) |
                              ((uint32_t)CAN_tx_msg->data[1] << 8) |
                              ((uint32_t)CAN_tx_msg->data[0]));
  ((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->sTxMailBox[0].TDHR = (((uint32_t)CAN_tx_msg->data[7] << 24) |
                              ((uint32_t)CAN_tx_msg->data[6] << 16) |
                              ((uint32_t)CAN_tx_msg->data[5] << 8) |
                              ((uint32_t)CAN_tx_msg->data[4]));

  // Send Go
  ((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->sTxMailBox[0].TIR = out | (1U << 0U) /* Bit 0: Transmit Mailbox Request*/;

  // Wait until the mailbox is empty
  while (((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->sTxMailBox[0].TIR & 0x1UL && count++ < 1000000)
    ;

  // The mailbox don't becomes empty while loop
  if (((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->sTxMailBox[0].TIR & 0x1UL)
  {
    Serial3.println("Send Fail");
    Serial3.println(((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->ESR);
    Serial3.println(((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->MSR);
    Serial3.println(((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->TSR);
  }
}

/**

 * Returns whether there are CAN messages available.

 *

 * @returns If pending CAN messages are in the CAN controller

 *

 */
# 438 "d:\\Sundaya\\File JS Pro\\Ehub V3\\code\\EHUB V3 tes board\\tesBoard\\tesBoard.ino"
uint8_t CANMsgAvail(void)
{
  // Check for pending FIFO 0 messages
  return ((CAN_TypeDef *)(0x40000000UL /*!< Peripheral base address in the alias region */ + 0x00006400UL))->RF0R & 0x3UL;
}

uint8_t counter = 0;
uint8_t frameLength = 0;
unsigned long previousMillis = 0; // stores last time output was updated
const long interval = 1000; // transmission interval (milliseconds)

void setuPin()
{
  Serial2.begin(115200);
  Serial3.begin(115200);
  pinMode(1, 0x0);
  pinMode(16 /* LED Blackpill*/, 0x1);
  pinMode(15, 0x1);
  pinMode(14, 0x1);
  pinMode(13, 0x1);
  pinMode(12, 0x1);
  pinMode(11, 0x1);
  pinMode(10, 0x1);
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

# 476 "d:\\Sundaya\\File JS Pro\\Ehub V3\\code\\EHUB V3 tes board\\tesBoard\\tesBoard.ino" 2
# 477 "d:\\Sundaya\\File JS Pro\\Ehub V3\\code\\EHUB V3 tes board\\tesBoard\\tesBoard.ino" 2
const int SPI_CS_PIN = 24 /* A4*/;
MCP_CAN CAN(SPI_CS_PIN);

void setupMCP2515()
{
  //  SPI.setClockDivider(SPI_CLOCK_DIV16);
  Serial3.println("Setup MCP");
  while ((0) != CAN.begin(15, 2)) // init can bus : baudrate = 500k
  {
    Serial3.println("CAN BUS Shield init fail");
    Serial3.println(" Init CAN BUS Shield again");
    delay(100);
  }

  Serial3.println("CAN OK : " + String((0)));
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
      Serial3.print(CAN_RX_msg.id, 16);
    }
    else
    {
      Serial3.print("Standard ID: 0x");
      if (CAN_RX_msg.id < 0x100)
        Serial3.print("0");
      if (CAN_RX_msg.id < 0x10)
        Serial3.print("00");
      Serial3.print(CAN_RX_msg.id, 16);
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
        Serial3.print(CAN_RX_msg.data[i], 16);
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

  CAN.sendMsgBuf(0x00, (1), 0, 8, stmp);
  delay(100); // send data per 100ms
}

void i2cScanner()
{

  byte error, address;
  int nDevices;
  nDevices = 0;

  Wire.beginTransmission(104);
  error = Wire.endTransmission();
  if (error == 0)
  {
    rtcLive = true;
  }

  delay(500);

  Wire.beginTransmission(80);
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

  delay(500);
}

void checkLed()
{
  pixels.clear();
  pixels.setPixelColor(0, pixels.Color(150, 0, 0));
  pixels.show();
  delay(1000);

  pixels.clear();
  pixels.setPixelColor(0, pixels.Color(0, 150, 0));
  pixels.show();
  delay(1000);

  pixels.clear();
  pixels.setPixelColor(0, pixels.Color(0, 0, 150));
  pixels.show();
  delay(1000);
  pixels.clear();
  pixels.setPixelColor(0, pixels.Color(0, 0, 0));
  pixels.show();
}

void checkButton()
{
  if (digitalRead(1) == 0x1)
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
  digitalWrite(16 /* LED Blackpill*/, 0x1);
  digitalWrite(15, 0x1);
  digitalWrite(14, 0x1);
  digitalWrite(13, 0x1);
  digitalWrite(12, 0x1);
  digitalWrite(11, 0x1);
  digitalWrite(10, 0x1);
  delay(3000);
  digitalWrite(16 /* LED Blackpill*/, 0x0);
  digitalWrite(15, 0x0);
  digitalWrite(14, 0x0);
  digitalWrite(13, 0x0);
  digitalWrite(12, 0x0);
  digitalWrite(11, 0x0);
  digitalWrite(10, 0x0);
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
