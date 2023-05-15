/* -----------------------------------------------------------------------------

Clock Configuration tab
-----------------------
  1. Configure HCLK Clock: HCLK (MHz) = <b>25 or higher</b>

Configuration tab
-----------------
  1. Under Connectivity open ETH Configuration:
     - GPIO Settings: review settings, no changes required
          Pin Name | Signal on Pin | GPIO mode | GPIO Pull-up/Pull..| Maximum out |
          :--------|:--------------|:----------|:-------------------|:------------|
          PA1      | ETH_REF_CLK   | Alternate | No pull-up and no..| High        |
          PA2      | ETH_MDIO      | Alternate | No pull-up and no..| High        |
          PA7      | ETH_CRS_DV    | Alternate | No pull-up and no..| High        |
          PC1      | ETH_MDC       | Alternate | No pull-up and no..| High        |
          PC4      | ETH_RXD0      | Alternate | No pull-up and no..| High        |
          PC5      | ETH_RXD1      | Alternate | No pull-up and no..| High        |
          PG11     | ETH_TX_EN     | Alternate | No pull-up and no..| High        |
          PG13     | ETH_TXD0      | Alternate | No pull-up and no..| High        |
          PG14     | ETH_TXD1      | Alternate | No pull-up and no..| High        |

     - NVIC Settings: enable interrupts
          Interrupt Table                      | Enable | Preemption Priority | Sub Priority
          :------------------------------------|:-------|:--------------------|:--------------
          Ethernet global interrupt            |   ON   | 0                   | 0

*/

#ifndef   __UNALIGNED_UINT32        /* deprecated */
  #define __UNALIGNED_UINT32(x)                  (*((__packed uint32_t *)(x)))
#endif
#ifndef   __UNALIGNED_UINT16_WRITE
  #define __UNALIGNED_UINT16_WRITE(addr, val)    ((*((__packed uint16_t *)(addr))) = (val))
#endif
#ifndef   __UNALIGNED_UINT16_READ
  #define __UNALIGNED_UINT16_READ(addr)          (*((const __packed uint16_t *)(addr)))
#endif
#ifndef   __UNALIGNED_UINT32_WRITE
  #define __UNALIGNED_UINT32_WRITE(addr, val)    ((*((__packed uint32_t *)(addr))) = (val))
#endif
#ifndef   __UNALIGNED_UINT32_READ
  #define __UNALIGNED_UINT32_READ(addr)          (*((const __packed uint32_t *)(addr)))
#endif



// to be evaluated at run time
// Pre-calculated value
//#define ETH_PTPSSIR_VALUE = (uint32_t)((1000000000ULL + SYS_HCLK_CLOCK_FREQUENCY - 1)/ SYS_HCLK_CLOCK_FREQUENCY)
//#define ETH_PTPTSAR_VALUE =(uint32_t)((1000000000ULL << 32) / (SYS_HCLK_CLOCK_FREQUENCY * ETH_PTPSSIR_VALUE))

/* Data Cache maintenance operations */
#ifndef EMAC_DCACHE_MAINTENANCE
#ifdef  EMAC_DCACHE
#define EMAC_DCACHE_MAINTENANCE       (EMAC_DCACHE)
#else
#define EMAC_DCACHE_MAINTENANCE       (1U)
#endif
#endif

/* Receive/transmit Checksum offload enable */
#ifndef EMAC_CHECKSUM_OFFLOAD
#define EMAC_CHECKSUM_OFFLOAD   1
#endif

/* IEEE 1588 time stamping enable (PTP) */
#ifndef EMAC_TIME_STAMP
#define EMAC_TIME_STAMP         0
#endif

/* Ethernet DMA Descriptor/Buffer memory address */
#ifdef  EMAC_DMA_MEMORY_ADDRESS
#define EMAC_DMA_MEMORY_ADDR    EMAC_DMA_MEMORY_ADDRESS
#endif

#include "EMAC_STM32F7xx.h"
#include "lib_digini.h"

#define ARM_ETH_MAC_DRV_VERSION ARM_DRIVER_VERSION_MAJOR_MINOR(1,0) /* driver version */

/* Timeouts */
#define PHY_TIMEOUT         2U          /* PHY Register access timeout in ms  */

/* ETH Memory Buffer configuration */
#define NUM_RX_BUF          4U          /* 0x1800 for Rx (4*1536=6K)          */
#define NUM_TX_BUF          2U          /* 0x0C00 for Tx (2*1536=3K)          */
#define ETH_BUF_SIZE        1536U       /* ETH Receive/Transmit buffer size   */

/* Interrupt Handler Prototype */
void ETH_IRQHandler (void);

/* Ethernet Pin definitions */
static const IO_ID_e ETH_Pins[] =
{
    IO_MX_ETH_MDC,      /* MII, RMII */
    IO_MX_ETH_MDIO,     /* MII, RMII */
    IO_MX_ETH_TXD0,     /* MII, RMII */
    IO_MX_ETH_TXD1,     /* MII, RMII */
    IO_MX_ETH_RXD0,     /* MII, RMII */
    IO_MX_ETH_RXD1,     /* MII, RMII */
    IO_MX_ETH_TX_EN,    /* MII, RMII */
#if (USE_ETH_RMII == DEF_DISABLED)
    IO_MX_ETH_TXD2,     /* MII, ---- */
    IO_MX_ETH_TXD3,     /* MII, ---- */
    IO_MX_ETH_RXD2,     /* MII, ---- */
    IO_MX_ETH_RXD3,     /* MII, ---- */
    IO_MX_ETH_TX_CLK,   /* MII, ---- */
    IO_MX_ETH_RX_CLK,   /* MII, ---- */
  //#ifdef MX_ETH_CRS_GPIOx
    IO_MX_ETH_CRS,      /* MII, ---- */
  //#endif
  //#ifdef MX_ETH_COL_GPIOx
    IO_MX_ETH_COL,      /* MII, ---- */
 //#endif
    IO_MX_ETH_RX_DV,    /* MII, ---- */
  //#ifdef MX_ETH_RX_ER_GPIOx
    IO_MX_ETH_RX_ER,    /* MII, ---- */
  //#endif
#else
    IO_MX_ETH_CRS_DV,    /* ---, RMII */
    IO_MX_ETH_REF_CLK,   /* ---, RMII */
#endif
};

/* Driver Version */
static const ARM_DRIVER_VERSION DriverVersion =
{
  ARM_ETH_MAC_API_VERSION,
  ARM_ETH_MAC_DRV_VERSION
};

/* Driver Capabilities */
static const ARM_ETH_MAC_CAPABILITIES DriverCapabilities =
{
    (EMAC_CHECKSUM_OFFLOAD != 0) ? 1U : 0U,   /* checksum_offload_rx_ip4  */
    (EMAC_CHECKSUM_OFFLOAD != 0) ? 1U : 0U,   /* checksum_offload_rx_ip6  */
    (EMAC_CHECKSUM_OFFLOAD != 0) ? 1U : 0U,   /* checksum_offload_rx_udp  */
    (EMAC_CHECKSUM_OFFLOAD != 0) ? 1U : 0U,   /* checksum_offload_rx_tcp  */
    (EMAC_CHECKSUM_OFFLOAD != 0) ? 1U : 0U,   /* checksum_offload_rx_icmp */
    (EMAC_CHECKSUM_OFFLOAD != 0) ? 1U : 0U,   /* checksum_offload_tx_ip4  */
    (EMAC_CHECKSUM_OFFLOAD != 0) ? 1U : 0U,   /* checksum_offload_tx_ip6  */
    (EMAC_CHECKSUM_OFFLOAD != 0) ? 1U : 0U,   /* checksum_offload_tx_udp  */
    (EMAC_CHECKSUM_OFFLOAD != 0) ? 1U : 0U,   /* checksum_offload_tx_tcp  */
    (EMAC_CHECKSUM_OFFLOAD != 0) ? 1U : 0U,   /* checksum_offload_tx_icmp */
    (USE_ETH_RMII == DEF_DISABLED) ?
    ARM_ETH_INTERFACE_MII :
    ARM_ETH_INTERFACE_RMII,                   /* media_interface          */
    0U,                                       /* mac_address              */
    1U,                                       /* event_rx_frame           */
    1U,                                       /* event_tx_frame           */
    1U,                                       /* event_wakeup             */
    (EMAC_TIME_STAMP != 0) ? 1U : 0U          /* precision_timer          */
  #if (defined(ARM_ETH_MAC_API_VERSION) && (ARM_ETH_MAC_API_VERSION >= 0x201U))
  , 0U                                        /* reserved bits            */
  #endif
};

/* Local variables */
static EMAC_CTRL Emac;

/* Ethernet DMA Descriptor/Buffer memory */
static struct
{
    RX_Desc  rx[NUM_RX_BUF];
    TX_Desc  tx[NUM_TX_BUF];
    uint32_t rx_buf [NUM_RX_BUF][ETH_BUF_SIZE >> 2];
    uint32_t tx_buf [NUM_TX_BUF][ETH_BUF_SIZE >> 2];
} Desc __MEMORY_AT(EMAC_DMA_MEMORY_ADDR);


/**
  \fn          void init_rx_desc (void)
  \brief       Initialize Rx DMA descriptors.
  \return      none.
*/
static void init_rx_desc(void)
{
    uint32_t i,next;

    for(i = 0; i < NUM_RX_BUF; i++)
    {
        Desc.rx[i].Stat = DMA_RX_OWN;
        Desc.rx[i].Ctrl = DMA_RX_RCH | ETH_BUF_SIZE;
        Desc.rx[i].Addr = (uint8_t *)&Desc.rx_buf[i];
        next = i + 1;
        if(next == NUM_RX_BUF)
        {
            next = 0;
        }
        Desc.rx[i].Next = &Desc.rx[next];
    }

  #if (EMAC_DCACHE_MAINTENANCE == 1)
    if((SCB->CCR & SCB_CCR_DC_Msk) != 0)
    {
        SCB_CleanDCache_by_Addr((uint32_t*)Desc.rx, sizeof(Desc.rx));
    }
  #endif

    ETH->DMARDLAR = (uint32_t)&Desc.rx[0];
    Emac.rx_index = 0;
}

/**
  \fn          void init_tx_desc (void)
  \brief       Initialize Tx DMA descriptors.
  \return      none.
*/
static void init_tx_desc(void)
{
    uint32_t i,next;

    for(i = 0; i < NUM_TX_BUF; i++)
    {
        Desc.tx[i].CtrlStat = DMA_TX_TCH | DMA_TX_LS | DMA_TX_FS;
        Desc.tx[i].Addr     = (uint8_t *)&Desc.tx_buf[i];
        next = i + 1U;

        if(next == NUM_TX_BUF)
        {
            next = 0;
        }

        Desc.tx[i].Next = &Desc.tx[next];
    }

  #if (EMAC_DCACHE_MAINTENANCE == 1)
    if((SCB->CCR & SCB_CCR_DC_Msk) != 0)
    {
        SCB_CleanDCache_by_Addr((uint32_t*)Desc.tx, sizeof(Desc.tx));
    }
  #endif

    ETH->DMATDLAR = (uint32_t)&Desc.tx[0];
    Emac.tx_index = 0;
}

/**
  \fn          void init_dma (void)
  \brief       Initialize DMA.
  \return      none.
*/
static void init_dma(void)
{

  if((Emac.flags & EMAC_FLAG_DMA_INIT) == 0)
  {
        Emac.flags |= EMAC_FLAG_DMA_INIT;

      #if ((EMAC_CHECKSUM_OFFLOAD != 0) || (EMAC_TIME_STAMP != 0))
        /* Enhanced descriptor enable */
        ETH->DMABMR |= ETH_DMABMR_EDE;
      #endif

        /* Initialize DMA Descriptors */
        init_rx_desc();
        init_tx_desc();

        /* Enable Rx interrupts */
        ETH->DMASR  = 0xFFFFFFFF;
        ETH->DMAIER = ETH_DMAIER_NISE | ETH_DMAIER_RIE | ETH_DMAIER_TIE;
    }
}


// TODO AR will use my own library instead of repeating stuff at the infinite
/**
  \fn          uint32_t crc32_8bit_rev (uint32_t crc32, uint8_t val)
  \brief       Calculate 32-bit CRC (Polynomial: 0x04C11DB7, data bit-reversed).
  \param[in]   crc32  CRC initial value
  \param[in]   val    Input value
  \return      Calculated CRC value
*/
static uint32_t crc32_8bit_rev (uint32_t crc32, uint8_t val)
{
    uint32_t n;

    crc32 ^= __RBIT (val);

    for (n = 8; n; n--)
    {
        if(crc32 & 0x80000000U)
        {
            crc32 <<= 1;
            crc32  ^= 0x04C11DB7U;
        }
        else
        {
            crc32 <<= 1;
        }
    }
    return(crc32);
}

/**
  \fn          uint32_t crc32_data (const uint8_t *data, uint32_t len)
  \brief       Calculate standard 32-bit Ethernet CRC.
  \param[in]   data  Pointer to buffer containing the data
  \param[in]   len   Data length in bytes
  \return      Calculated CRC value
*/
static uint32_t crc32_data (const uint8_t *data, uint32_t len) {
  uint32_t cnt, crc;

  crc = 0xFFFFFFFFU;
  for (cnt = len; cnt; cnt--) {
    crc = crc32_8bit_rev (crc, *data++);
  }
  return (crc ^ 0xFFFFFFFFU);
}

#if (ETH_SMI_SW != 0) /* Software MDIO */
/**
  \fn          void SW_MDIO_Dir (uint32_t dir)
  \brief       Turnaround MDO
  \param[in]   dir  MDIO direction: 0=input, 1=output
*/
static void SW_MDIO_Init (void) {
  GPIO_InitTypeDef GPIO_InitStruct;

  /* Set MDC as output */
  GPIO_InitStruct.Pin   = MX_Ethernet_MDC_GPIO_Pin;
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
  HAL_GPIO_Init(MX_Ethernet_MDC_GPIOx, &GPIO_InitStruct);

  HAL_GPIO_WritePin (MX_Ethernet_MDC_GPIOx, MX_Ethernet_MDC_GPIO_Pin, GPIO_PIN_RESET);
}

/**
  \fn          void SW_MDIO_Dir (uint32_t dir)
  \brief       Turnaround MDO
  \param[in]   dir  MDIO direction: 0=input, 1=output
*/
static void SW_MDIO_Dir (uint32_t dir) {
  GPIO_InitTypeDef GPIO_InitStruct;

  /* Set MDIO as input */
  GPIO_InitStruct.Pin   = MX_Ethernet_MDIO_GPIO_Pin;
  GPIO_InitStruct.Mode  = (dir) ? (GPIO_MODE_OUTPUT_PP) : (GPIO_MODE_INPUT);
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
  HAL_GPIO_Init(MX_Ethernet_MDIO_GPIOx, &GPIO_InitStruct);

  if (dir == 0) {
    HAL_GPIO_WritePin (MX_Ethernet_MDC_GPIOx, MX_Ethernet_MDC_GPIO_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin (MX_Ethernet_MDC_GPIOx, MX_Ethernet_MDC_GPIO_Pin, GPIO_PIN_RESET);
  }
}

/**
  \fn          void SW_MDIO_Write (uint32_t data, uint32_t len)
  \brief       Output a data to the MII PHY management interface.
  \param[in]   data  Data to write
  \param[in]   len   Data length in bits
*/
static void SW_MDIO_Write (uint32_t data, uint32_t len) {

  data <<= (32 - len);
  for (; len; len--) {
    /* Set data bit */
    HAL_GPIO_WritePin (MX_Ethernet_MDIO_GPIOx, MX_Ethernet_MDIO_GPIO_Pin, (data & 0x80000000) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    /* Clock it out */
    HAL_GPIO_WritePin (MX_Ethernet_MDC_GPIOx, MX_Ethernet_MDC_GPIO_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin (MX_Ethernet_MDC_GPIOx, MX_Ethernet_MDC_GPIO_Pin, GPIO_PIN_RESET);

    data <<= 1;
  }
}

/**
  \fn          uint32_t SW_MDIO_Read (void)
  \brief       Read a value from the MII PHY management interface.
  \return      Data read
*/
static uint32_t SW_MDIO_Read (void) {
  uint32_t i;
  uint32_t data = 0;

  for (i = 0; i < 16; i++) {
    data <<= 1;
    /* Clock in data bit */
    HAL_GPIO_WritePin (MX_Ethernet_MDC_GPIOx, MX_Ethernet_MDC_GPIO_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin (MX_Ethernet_MDC_GPIOx, MX_Ethernet_MDC_GPIO_Pin, GPIO_PIN_RESET);
    if (HAL_GPIO_ReadPin(MX_Ethernet_MDIO_GPIOx, MX_Ethernet_MDIO_GPIO_Pin) == GPIO_PIN_SET) {
      data |= 1;
    }
  }
  return (data);
}
#endif


/* Ethernet Driver functions */

/**
  \fn          ARM_DRIVER_VERSION GetVersion (void)
  \brief       Get driver version.
  \return      \ref ARM_DRIVER_VERSION
*/
static ARM_DRIVER_VERSION GetVersion(void)
{
    return DriverVersion;
}


/**
  \fn          ARM_ETH_MAC_CAPABILITIES GetCapabilities (void)
  \brief       Get driver capabilities.
  \return      \ref ARM_ETH_MAC_CAPABILITIES
*/
static ARM_ETH_MAC_CAPABILITIES GetCapabilities(void)
{
    return DriverCapabilities;
}


/**
  \fn          int32_t Initialize (ARM_ETH_MAC_SignalEvent_t cb_event)
  \brief       Initialize Ethernet MAC Device.
  \param[in]   cb_event  Pointer to \ref ARM_ETH_MAC_SignalEvent
  \return      \ref execution_status
*/
static int32_t Initialize(ARM_ETH_MAC_SignalEvent_t cb_event)
{
    /* Enable SYSCFG clock */
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

  #if (USE_ETH_RMII == DEF_ENABLED)
    SYSCFG->PMC |=  SYSCFG_PMC_MII_RMII_SEL;
  #else
    SYSCFG->PMC &= ~SYSCFG_PMC_MII_RMII_SEL;
  #endif

    for(size_t i = 0; i < sizeof(ETH_Pins) / sizeof(IO_ID_e); i++)
    {
        IO_PinInit(ETH_Pins[i]);
    }

  #if (ETH_SMI_SW != 0) /* Software MDIO */
    SW_MDIO_Init();
  #endif

    /* Clear control structure */
    memset((void *)&Emac, 0, sizeof (EMAC_CTRL));

    Emac.cb_event = cb_event;
    Emac.flags    = EMAC_FLAG_INIT;

    return ARM_DRIVER_OK;
}

/**
  \fn          int32_t Uninitialize (void)
  \brief       De-initialize Ethernet MAC Device.
  \return      \ref execution_status
*/
static int32_t Uninitialize(void)
{
    for(size_t i = 0; i < sizeof(ETH_Pins) / sizeof(IO_ID_e); i++)
    {
       IO_PinInitInput(ETH_Pins[i]);
    }
    Emac.flags &= ~EMAC_FLAG_INIT;

    return ARM_DRIVER_OK;
}

/**
  \fn          int32_t PowerControl (ARM_POWER_STATE state)
  \brief       Control Ethernet MAC Device Power.
  \param[in]   state  Power state
  \return      \ref execution_status
*/
static int32_t PowerControl(ARM_POWER_STATE state)
{
    uint32_t clkdiv;

    if((state != ARM_POWER_OFF) && (state != ARM_POWER_FULL) && (state != ARM_POWER_LOW))
    {
        return ARM_DRIVER_ERROR_UNSUPPORTED;
    }

    switch(state)
    {
        case ARM_POWER_OFF:
            /* Enable Ethernet clocks */
            RCC->AHB1ENR |= (RCC_AHB1ENR_ETHMACEN   |
                             RCC_AHB1ENR_ETHMACTXEN |
                             RCC_AHB1ENR_ETHMACRXEN
                           #if (EMAC_TIME_STAMP)
                           | RCC_AHB1ENR_ETHMACPTPEN
                           #endif
                             );

            /* Reset Ethernet MAC peripheral */
            RCC->AHB1RSTR |=  RCC_AHB1RSTR_ETHMACRST;
            __NOP(); __NOP(); __NOP(); __NOP();
            RCC->AHB1RSTR &= ~RCC_AHB1RSTR_ETHMACRST;

            /* Disable ethernet interrupts */
            NVIC_DisableIRQ(ETH_IRQn);
            ETH->DMAIER  = 0;
            ETH->PTPTSCR = 0;

            /* Disable Ethernet clocks */
            RCC->AHB1ENR &= ~(RCC_AHB1ENR_ETHMACEN   |
                              RCC_AHB1ENR_ETHMACTXEN |
                              RCC_AHB1ENR_ETHMACRXEN
          #if (EMAC_TIME_STAMP)
                            | RCC_AHB1ENR_ETHMACPTPEN
          #endif
                              );

            Emac.flags &= ~(EMAC_FLAG_POWER | EMAC_FLAG_DMA_INIT);
        break;

        case ARM_POWER_LOW:
            return ARM_DRIVER_ERROR_UNSUPPORTED;

        case ARM_POWER_FULL:
            if((Emac.flags & EMAC_FLAG_INIT) == 0)
            {
                /* Driver not initialized */
                return ARM_DRIVER_ERROR;
            }

            if((Emac.flags & EMAC_FLAG_POWER) != 0)
            {
                /* Driver already powered */
                break;
            }

            /* Enable Ethernet clocks */
            RCC->AHB1ENR |= (RCC_AHB1ENR_ETHMACEN   |
                             RCC_AHB1ENR_ETHMACTXEN |
                             RCC_AHB1ENR_ETHMACRXEN
                           #if (EMAC_TIME_STAMP)
                           | RCC_AHB1ENR_ETHMACPTPEN
                           #endif
                             );

            /* Reset Ethernet MAC peripheral */
            RCC->AHB1RSTR |=  RCC_AHB1RSTR_ETHMACRST;
            __NOP(); __NOP(); __NOP(); __NOP();
            RCC->AHB1RSTR &= ~RCC_AHB1RSTR_ETHMACRST;

            /* MDC clock range selection */
          #if   (SYS_HCLK_CLOCK_FREQUENCY >= 150000000)
            clkdiv = ETH_MACMIIAR_CR_Div102;
          #elif (SYS_HCLK_CLOCK_FREQUENCY >= 100000000)
            clkdiv = ETH_MACMIIAR_CR_Div62;
          #elif (SYS_HCLK_CLOCK_FREQUENCY >= 60000000)
            clkdiv = ETH_MACMIIAR_CR_Div42;
          #elif (SYS_HCLK_CLOCK_FREQUENCY >= 35000000)
            clkdiv = ETH_MACMIIAR_CR_Div26;
          #elif (SYS_HCLK_CLOCK_FREQUENCY >= 25000000)
            clkdiv = ETH_MACMIIAR_CR_Div16;
          #else
            #pragma message "XSTR(SYS_HCLK_CLOCK_FREQUENCY)"
            #error CPU Core HCLK is too slow for Ethernet!
          #endif

            ETH->MACMIIAR = clkdiv;

            /* Initialize MAC configuration */
            ETH->MACCR   = ETH_MACCR_ROD | 0x00008000U;

            /* Initialize Filter registers */
            ETH->MACFFR  = ETH_MACFFR_BFD;
            ETH->MACFCR  = ETH_MACFCR_ZQPD;

            /* Initialize Address registers */
            ETH->MACA0HR = 0; ETH->MACA0LR = 0;
            ETH->MACA1HR = 0; ETH->MACA1LR = 0;
            ETH->MACA2HR = 0; ETH->MACA2LR = 0;
            ETH->MACA3HR = 0; ETH->MACA3LR = 0;

            /* Mask timestamp interrupts */
            ETH->MACIMR |= ETH_MACIMR_TSTIM | ETH_MACIMR_PMTIM;

          #if (EMAC_TIME_STAMP)
            ETH->PTPTSCR = ETH_PTPTSCR_TSE;

// TODO AR use preprocessor
            ETH->PTPSSIR = (uint32_t)((1000000000ULL + SYS_HCLK_CLOCK_FREQUENCY - 1)/ SYS_HCLK_CLOCK_FREQUENCY);
            ETH->PTPTSAR = (uint32_t)((1000000000ULL << 32) / (SYS_HCLK_CLOCK_FREQUENCY * ETH->PTPSSIR));

            ETH->PTPTSCR = ETH_PTPTSSR_TSSIPV4FE | ETH_PTPTSSR_TSSIPV6FE | ETH_PTPTSSR_TSSSR |
                                                                           ETH_PTPTSCR_TSARU |
                                                                           ETH_PTPTSCR_TSSTI |
                                                                           ETH_PTPTSCR_TSFCU |
                                                                           ETH_PTPTSCR_TSE;
            Emac.tx_ts_index  = 0;
          #endif

            /* Disable MMC interrupts */
            ETH->MMCTIMR = ETH_MMCTIMR_TGFM  | ETH_MMCTIMR_TGFMSCM | ETH_MMCTIMR_TGFSCM;
            ETH->MMCRIMR = ETH_MMCRIMR_RGUFM | ETH_MMCRIMR_RFAEM   | ETH_MMCRIMR_RFCEM;

            NVIC_ClearPendingIRQ (ETH_IRQn);
            NVIC_EnableIRQ (ETH_IRQn);

            Emac.frame_end = NULL;
            Emac.flags    |= EMAC_FLAG_POWER;
        break;
    }

    return ARM_DRIVER_OK;
}

/**
  \fn          int32_t GetMacAddress (ARM_ETH_MAC_ADDR *ptr_addr)
  \brief       Get Ethernet MAC Address.
  \param[in]   ptr_addr  Pointer to address
  \return      \ref execution_status
*/
static int32_t GetMacAddress(ARM_ETH_MAC_ADDR *ptr_addr)
{
    uint32_t val;

    if(ptr_addr == NULL)
    {
        return ARM_DRIVER_ERROR_PARAMETER;
    }

    if((Emac.flags & EMAC_FLAG_POWER) == 0)
    {
        return ARM_DRIVER_ERROR;
    }

    val = ETH->MACA0HR;
    ptr_addr->b[5] = (uint8_t)(val >>  8);
    ptr_addr->b[4] = (uint8_t)(val);
    val = ETH->MACA0LR;
    ptr_addr->b[3] = (uint8_t)(val >> 24);
    ptr_addr->b[2] = (uint8_t)(val >> 16);
    ptr_addr->b[1] = (uint8_t)(val >>  8);
    ptr_addr->b[0] = (uint8_t)(val);

    return ARM_DRIVER_OK;
}

/**
  \fn          int32_t SetMacAddress (const ARM_ETH_MAC_ADDR *ptr_addr)
  \brief       Set Ethernet MAC Address.
  \param[in]   ptr_addr  Pointer to address
  \return      \ref execution_status
*/
static int32_t SetMacAddress(const ARM_ETH_MAC_ADDR *ptr_addr)
{
    if(ptr_addr == NULL)
    {
        return ARM_DRIVER_ERROR_PARAMETER;
    }

    if((Emac.flags & EMAC_FLAG_POWER) == 0)
    {
        return ARM_DRIVER_ERROR;
    }

    /* Set Ethernet MAC Address registers */
    ETH->MACA0HR = ((uint32_t)ptr_addr->b[5] <<  8) |  (uint32_t)ptr_addr->b[4];
    ETH->MACA0LR = ((uint32_t)ptr_addr->b[3] << 24) | ((uint32_t)ptr_addr->b[2] << 16) |
                   ((uint32_t)ptr_addr->b[1] <<  8) |  (uint32_t)ptr_addr->b[0];

    return ARM_DRIVER_OK;
}

/**
  \fn          int32_t SetAddressFilter (const ARM_ETH_MAC_ADDR *ptr_addr,
                                               uint32_t          num_addr)
  \brief       Configure Address Filter.
  \param[in]   ptr_addr  Pointer to addresses
  \param[in]   num_addr  Number of addresses to configure
  \return      \ref execution_status
*/
static int32_t SetAddressFilter(const ARM_ETH_MAC_ADDR *ptr_addr, uint32_t num_addr)
{
    uint32_t crc;

    if((ptr_addr == NULL) && (num_addr != 0))
    {
        return ARM_DRIVER_ERROR_PARAMETER;
    }

    if((Emac.flags & EMAC_FLAG_POWER) == 0)
    {
        return ARM_DRIVER_ERROR;
    }

    /* Use unicast address filtering for first 3 MAC addresses */
    ETH->MACFFR &= ~(ETH_MACFFR_HPF | ETH_MACFFR_HM);
    ETH->MACHTHR = 0U; ETH->MACHTLR = 0U;

    if(num_addr == 0U)
    {
        ETH->MACA1HR = 0U; ETH->MACA1LR = 0U;
        ETH->MACA2HR = 0U; ETH->MACA2LR = 0U;
        ETH->MACA3HR = 0U; ETH->MACA3LR = 0U;
        return ARM_DRIVER_OK;
    }

    ETH->MACA1HR = ((uint32_t)ptr_addr->b[5] <<  8) |  (uint32_t)ptr_addr->b[4] | ETH_MACA1HR_AE;
    ETH->MACA1LR = ((uint32_t)ptr_addr->b[3] << 24) | ((uint32_t)ptr_addr->b[2] << 16) |
                   ((uint32_t)ptr_addr->b[1] <<  8) |  (uint32_t)ptr_addr->b[0];
    num_addr--;

    if(num_addr == 0U)
    {
        ETH->MACA2HR = 0U; ETH->MACA2LR = 0U;
        ETH->MACA3HR = 0U; ETH->MACA3LR = 0U;
        return ARM_DRIVER_OK;
    }

  ptr_addr++;

    ETH->MACA2HR = ((uint32_t)ptr_addr->b[5] <<  8) |  (uint32_t)ptr_addr->b[4] | ETH_MACA2HR_AE;
    ETH->MACA2LR = ((uint32_t)ptr_addr->b[3] << 24) | ((uint32_t)ptr_addr->b[2] << 16) |
                   ((uint32_t)ptr_addr->b[1] <<  8) |  (uint32_t)ptr_addr->b[0];
    num_addr--;
    if(num_addr == 0U)
    {
        ETH->MACA3HR = 0U; ETH->MACA3LR = 0U;
        return ARM_DRIVER_OK;
    }

    ptr_addr++;

    ETH->MACA3HR = ((uint32_t)ptr_addr->b[5] <<  8) |  (uint32_t)ptr_addr->b[4] | ETH_MACA3HR_AE;
    ETH->MACA3LR = ((uint32_t)ptr_addr->b[3] << 24) | ((uint32_t)ptr_addr->b[2] << 16) |
                   ((uint32_t)ptr_addr->b[1] <<  8) |  (uint32_t)ptr_addr->b[0];
    num_addr--;

    if(num_addr == 0U)
    {
        return ARM_DRIVER_OK;
    }

    ptr_addr++;

    /* Calculate 64-bit Hash table for remaining MAC addresses */
    for(; num_addr; ptr_addr++, num_addr--)
    {
        crc = crc32_data (&ptr_addr->b[0], 6U) >> 26;

        if(crc & 0x20)
        {
            ETH->MACHTHR |= (1U << (crc & 0x1FU));
        }
        else
        {
            ETH->MACHTLR |= (1U << crc);
        }
    }

    /* Enable both, unicast and hash address filtering */
    ETH->MACFFR |= ETH_MACFFR_HPF | ETH_MACFFR_HM;

    return ARM_DRIVER_OK;
}

/**
  \fn          int32_t SendFrame (const uint8_t *frame, uint32_t len, uint32_t flags)
  \brief       Send Ethernet frame.
  \param[in]   frame  Pointer to frame buffer with data to send
  \param[in]   len    Frame buffer length in bytes
  \param[in]   flags  Frame transmit flags (see ARM_ETH_MAC_TX_FRAME_...)
  \return      \ref execution_status
*/
static int32_t SendFrame(const uint8_t *frame, uint32_t len, uint32_t flags)
{
    uint8_t *dst = Emac.frame_end;
    uint32_t ctrl;

    if((frame == NULL) || (len == 0U))
    {
        return ARM_DRIVER_ERROR_PARAMETER;
    }

    if((Emac.flags & EMAC_FLAG_POWER) == 0U)
    {
        return ARM_DRIVER_ERROR;
    }

    if(dst == NULL)
    {
        /* Start of a new transmit frame */
      #if(EMAC_DCACHE_MAINTENANCE == 1)
        if((SCB->CCR & SCB_CCR_DC_Msk) != 0)
        {
            SCB_InvalidateDCache_by_Addr ((uint32_t*)&Desc.tx[Emac.tx_index], sizeof(TX_Desc));
        }
      #endif

        if(Desc.tx[Emac.tx_index].CtrlStat & DMA_TX_OWN)
        {
            /* Transmitter is busy, wait */
            return ARM_DRIVER_ERROR_BUSY;
        }

        dst = Desc.tx[Emac.tx_index].Addr;
        Desc.tx[Emac.tx_index].Size = len;
    }
    else
    {
        /* Sending data fragments in progress */
        Desc.tx[Emac.tx_index].Size += len;
    }

    /* Fast-copy data fragments to ETH-DMA buffer */
    for(; len > 7; dst += 8, frame += 8, len -= 8)
    {
        __UNALIGNED_UINT32_WRITE(&dst[0], __UNALIGNED_UINT32_READ(&frame[0]));
        __UNALIGNED_UINT32_WRITE(&dst[4], __UNALIGNED_UINT32_READ(&frame[4]));
    }

    /* Copy remaining 7 bytes */
    for(; len > 1; dst += 2, frame += 2, len -= 2)
    {
        __UNALIGNED_UINT16_WRITE(&dst[0], __UNALIGNED_UINT16_READ(&frame[0]));
    }

    if(len > 0)
    {
        dst++[0] = frame++[0];
    }

    if(flags & ARM_ETH_MAC_TX_FRAME_FRAGMENT)
    {
        /* More data to come, remember current write position */
        Emac.frame_end = dst;
        return ARM_DRIVER_OK;
    }

    /* Frame is now ready, send it to DMA */
    ctrl = Desc.tx[Emac.tx_index].CtrlStat & ~DMA_TX_CIC;

  #if (EMAC_CHECKSUM_OFFLOAD != 0)
    if (Emac.tx_cks_offload)
    {
        /* The following is a workaround for EMAC silicon problem:       */
        /*   "Incorrect layer 3 (L3) checksum is inserted in the sent    */
        /*    IPv4 fragmented packets."                                  */
        /* Description:                                                  */
        /*   When automatic checksum insertion is enabled and the packet */
        /*   is IPv4 frame fragment, then the MAC may incorrectly insert */
        /*   checksum into the packet. This corrupts the payload data    */
        /*   and generates checksum errors at the receiver.              */
        uint16_t prot = __UNALIGNED_UINT16_READ(&Desc.tx[Emac.tx_index].Addr[12]);
        uint16_t frag = __UNALIGNED_UINT16_READ(&Desc.tx[Emac.tx_index].Addr[20]);

        if((prot == 0x0008) && (frag & 0xFF3F))
        {
            /* Insert only IP header checksum in fragmented frame */
            ctrl |= DMA_TX_CIC_IP;
        }
        else
        {
            /* Insert IP header and payload checksums (TCP,UDP,ICMP) */
            ctrl |= DMA_TX_CIC;
        }
    }
  #endif

    ctrl &= ~(DMA_TX_IC | DMA_TX_TTSE);

    if(flags & ARM_ETH_MAC_TX_FRAME_EVENT)
    {
        ctrl |= DMA_TX_IC;
    }

  #if (EMAC_TIME_STAMP != 0)
    if(flags & ARM_ETH_MAC_TX_FRAME_TIMESTAMP)
    {
        ctrl |= DMA_TX_TTSE;
    }

    Emac.tx_ts_index = Emac.tx_index;
  #endif

    __DMB();
    Desc.tx[Emac.tx_index].CtrlStat = ctrl | DMA_TX_OWN;
    __DMB();

  #if (EMAC_DCACHE_MAINTENANCE == 1)
    if((SCB->CCR & SCB_CCR_DC_Msk) != 0)
    {
        SCB_CleanDCache_by_Addr ((uint32_t *)Desc.tx[Emac.tx_index].Addr, ETH_BUF_SIZE);
        SCB_CleanDCache_by_Addr ((uint32_t *)&Desc.tx[Emac.tx_index], sizeof(TX_Desc));
    }
  #endif

    Emac.tx_index++;

    if(Emac.tx_index == NUM_TX_BUF)
    {
        Emac.tx_index = 0;
    }

    Emac.frame_end = NULL;

    if((ETH->DMASR & ETH_DMASR_TBUS) != 0)
    {
        /* Transmit buffer unavailable, resume DMA */
        ETH->DMASR   = ETH_DMASR_TBUS;
        ETH->DMATPDR = 0;
    }

    return ARM_DRIVER_OK;
}

/**
  \fn          int32_t ReadFrame (uint8_t *frame, uint32_t len)
  \brief       Read data of received Ethernet frame.
  \param[in]   frame  Pointer to frame buffer for data to read into
  \param[in]   len    Frame buffer length in bytes
  \return      number of data bytes read or execution status
                 - value >= 0: number of data bytes read
                 - value < 0: error occurred, value is execution status as defined with \ref execution_status
*/
static int32_t ReadFrame(uint8_t *frame, uint32_t len)
 {
    uint8_t const *src = Desc.rx[Emac.rx_index].Addr;
    int32_t cnt        = (int32_t)len;

    if((frame == NULL) && (len != 0))
    {
        return ARM_DRIVER_ERROR_PARAMETER;
    }

    if((Emac.flags & EMAC_FLAG_POWER) == 0)
    {
        return ARM_DRIVER_ERROR;
    }

  #if (EMAC_DCACHE_MAINTENANCE == 1)
    if((SCB->CCR & SCB_CCR_DC_Msk) != 0)
    {
        SCB_InvalidateDCache_by_Addr ((uint32_t*)(uint32_t)src, ETH_BUF_SIZE);
    }
  #endif

    /* Fast-copy data to frame buffer */
    for(; len > 7; frame += 8, src += 8, len -= 8)
    {
        __UNALIGNED_UINT32_WRITE(&frame[0], __UNALIGNED_UINT32_READ(&src[0]));
        __UNALIGNED_UINT32_WRITE(&frame[4], __UNALIGNED_UINT32_READ(&src[4]));
    }

    /* Copy remaining 7 bytes */
    for(; len > 1; frame += 2, src += 2, len -= 2)
    {
        __UNALIGNED_UINT16_WRITE(&frame[0], __UNALIGNED_UINT16_READ(&src[0]));
    }

    if(len > 0)
    {
        frame[0] = src[0];
    }

    /* Return this block back to ETH-DMA */
    __DMB();
    Desc.rx[Emac.rx_index].Stat = DMA_RX_OWN;
    __DMB();

  #if (EMAC_DCACHE_MAINTENANCE == 1U)
    if((SCB->CCR & SCB_CCR_DC_Msk) != 0)
    {
        SCB_CleanDCache_by_Addr ((uint32_t *)&Desc.rx[Emac.rx_index], sizeof(RX_Desc));
    }
  #endif

    Emac.rx_index++;

    if(Emac.rx_index == NUM_RX_BUF)
    {
        Emac.rx_index = 0;
    }

    if(ETH->DMASR & ETH_DMASR_RBUS)
    {
        /* Receive buffer unavailable, resume DMA */
        ETH->DMASR   = ETH_DMASR_RBUS;
        ETH->DMARPDR = 0;
    }

    return (cnt);
}

/**
  \fn          uint32_t GetRxFrameSize (void)
  \brief       Get size of received Ethernet frame.
  \return      number of bytes in received frame
*/
static uint32_t GetRxFrameSize(void)
{
    volatile uint32_t stat;

    if((Emac.flags & EMAC_FLAG_POWER) == 0)
    {
        return 0;
    }

  #if (EMAC_DCACHE_MAINTENANCE == 1)
    if((SCB->CCR & SCB_CCR_DC_Msk) != 0)
    {
        SCB_InvalidateDCache_by_Addr ((uint32_t*)&Desc.rx[Emac.rx_index], sizeof(RX_Desc));
    }
  #endif

    stat = Desc.rx[Emac.rx_index].Stat;

    if(stat & DMA_RX_OWN)
    {
        /* Owned by DMA */
        return 0;
    }

    if(((stat & DMA_RX_ES) != 0) || ((stat & DMA_RX_FS) == 0) || ((stat & DMA_RX_LS) == 0))
    {
        /* Error, this block is invalid */
        return (0xFFFFFFFF);
    }

    return (((stat & DMA_RX_FL) >> 16) - 4);
}

/**
  \fn          int32_t GetRxFrameTime (ARM_ETH_MAC_TIME *time)
  \brief       Get time of received Ethernet frame.
  \param[in]   time  Pointer to time structure for data to read into
  \return      \ref execution_status
*/
static int32_t GetRxFrameTime(ARM_ETH_MAC_TIME* time)
{
  #if (EMAC_TIME_STAMP)
    volatile RX_Desc *rxd = &Desc.rx[Emac.rx_index];

    if((Emac.flags & EMAC_FLAG_POWER) == 0)
    {
        return ARM_DRIVER_ERROR;
    }

   #if (EMAC_DCACHE_MAINTENANCE == 1)
    if((SCB->CCR & SCB_CCR_DC_Msk) != 0)
    {
        SCB_InvalidateDCache_by_Addr (&Desc.rx[Emac.rx_index], sizeof(RX_Desc));
    }
   #endif

    if(rxd->Stat & DMA_RX_OWN)
    {
        /* Owned by DMA */
        return ARM_DRIVER_ERROR_BUSY;
    }

    time->ns  = rxd->TimeLo;
    time->sec = rxd->TimeHi;

    return ARM_DRIVER_OK;
  #else
    (void)time;
    return ARM_DRIVER_ERROR;
  #endif
}

/**
  \fn          int32_t GetTxFrameTime (ARM_ETH_MAC_TIME *time)
  \brief       Get time of transmitted Ethernet frame.
  \param[in]   time  Pointer to time structure for data to read into
  \return      \ref execution_status
*/
static int32_t GetTxFrameTime (ARM_ETH_MAC_TIME *time)
{
  #if (EMAC_TIME_STAMP)
    volatile TX_Desc *txd = &Desc.tx[Emac.tx_ts_index];

    if((Emac.flags & EMAC_FLAG_POWER) == 0)
    {
        return ARM_DRIVER_ERROR;
    }

   #if (EMAC_DCACHE_MAINTENANCE == 1)
    if((SCB->CCR & SCB_CCR_DC_Msk) != 0)
    {
        SCB_InvalidateDCache_by_Addr (&Desc.tx[Emac.tx_ts_index], sizeof(TX_Desc);
    }
   #endif

    if(txd->CtrlStat & DMA_RX_OWN)
    {
        /* Owned by DMA */
        return ARM_DRIVER_ERROR_BUSY;
    }

    if((txd->CtrlStat & DMA_TX_TTSS) == 0)
    {
        /* No transmit time stamp available */
        return ARM_DRIVER_ERROR;
    }

    time->ns  = txd->TimeLo;
    time->sec = txd->TimeHi;
    return ARM_DRIVER_OK;
  #else
    (void)time;
    return ARM_DRIVER_ERROR;
  #endif
}

/**
  \fn          int32_t ControlTimer (uint32_t control, ARM_ETH_MAC_TIME *time)
  \brief       Control Precision Timer.
  \param[in]   control  operation
  \param[in]   time     Pointer to time structure
  \return      \ref execution_status
*/
static int32_t ControlTimer(uint32_t control, ARM_ETH_MAC_TIME* time)
{
  #if (EMAC_TIME_STAMP != 0)
    if((Emac.flags & EMAC_FLAG_POWER) == 0U)
    {
        return ARM_DRIVER_ERROR;
    }
    if((control != ARM_ETH_MAC_TIMER_GET_TIME)     &&
       (control != ARM_ETH_MAC_TIMER_SET_TIME)     &&
       (control != ARM_ETH_MAC_TIMER_INC_TIME)     &&
       (control != ARM_ETH_MAC_TIMER_DEC_TIME)     &&
       (control != ARM_ETH_MAC_TIMER_SET_ALARM)    &&
       (control != ARM_ETH_MAC_TIMER_ADJUST_CLOCK))
    {
        return ARM_DRIVER_ERROR_PARAMETER;
    }

    switch(control)
    {
        case ARM_ETH_MAC_TIMER_GET_TIME:
            /* Get current time */
            time->sec = ETH->PTPTSHR;
            time->ns  = ETH->PTPTSLR;
        break;

        case ARM_ETH_MAC_TIMER_SET_TIME:
            /* Set new time */
            ETH->PTPTSHUR = time->sec;
            ETH->PTPTSLUR = time->ns;
            /* Initialize TS time */
            ETH->PTPTSCR |= ETH_PTPTSCR_TSSTI;
        break;

        case ARM_ETH_MAC_TIMER_INC_TIME:
            /* Increment current time */
            ETH->PTPTSHUR = time->sec;
            ETH->PTPTSLUR = time->ns;

            /* Time stamp system time update */
            ETH->PTPTSCR |=  ETH_PTPTSCR_TSSTU;
        break;

        case ARM_ETH_MAC_TIMER_DEC_TIME:
            /* Decrement current time */
            ETH->PTPTSHUR = time->sec;
            ETH->PTPTSLUR = time->ns | 0x80000000U;

            /* Time stamp system time update */
            ETH->PTPTSCR |=  ETH_PTPTSCR_TSSTU;
        break;

        case ARM_ETH_MAC_TIMER_SET_ALARM:
            /* Set alarm time */
            ETH->PTPTTHR  = time->sec;
            ETH->PTPTTLR  = time->ns;

            /* Enable timestamp interrupt in PTP control */
            ETH->PTPTSCR |= ETH_PTPTSCR_TSITE;

            if(time->sec || time->ns)
            {
                /* Enable timestamp trigger interrupt */
                ETH->MACIMR &= ~ETH_MACIMR_TSTIM;
            }
            else
            {
                /* Disable timestamp trigger interrupt */
                ETH->MACIMR |= ETH_MACIMR_TSTIM;
            }
        break;

        case ARM_ETH_MAC_TIMER_ADJUST_CLOCK:
            /* Adjust current time, fine correction */
            /* Correction factor is Q31 (0x80000000 = 1.000000000) */
            ETH->PTPTSAR = (uint32_t)(((uint64_t)time->ns * ETH->PTPTSAR) >> 31);
            /* Fine TS clock correction */
            ETH->PTPTSCR |= ETH_PTPTSCR_TSARU;
        break;
    }

    return ARM_DRIVER_OK;
#else
    (void)control;
    (void)time;
    return ARM_DRIVER_ERROR;
#endif
}

/**
  \fn          int32_t Control (uint32_t control, uint32_t arg)
  \brief       Control Ethernet Interface.
  \param[in]   control  operation
  \param[in]   arg      argument of operation (optional)
  \return      \ref execution_status
*/
static int32_t Control(uint32_t control, uint32_t arg)
 {
    uint32_t maccr;
    uint32_t dmaomr;
    uint32_t macffr;

    if((Emac.flags & EMAC_FLAG_POWER) == 0)
    {
        return ARM_DRIVER_ERROR;
    }
    if((control != ARM_ETH_MAC_CONFIGURE)   &&
       (control != ARM_ETH_MAC_CONTROL_TX)  &&
       (control != ARM_ETH_MAC_CONTROL_RX)  &&
       (control != ARM_ETH_MAC_FLUSH)       &&
       (control != ARM_ETH_MAC_SLEEP)       &&
       (control != ARM_ETH_MAC_VLAN_FILTER))
    {
        return ARM_DRIVER_ERROR_PARAMETER;
    }

    switch(control)
    {
        case ARM_ETH_MAC_CONFIGURE:
            maccr = ETH->MACCR & ~(ETH_MACCR_FES | ETH_MACCR_DM | ETH_MACCR_LM | ETH_MACCR_IPCO);

            /* Configure 100MBit/10MBit mode */
            switch(arg & ARM_ETH_MAC_SPEED_Msk)
            {
                case ARM_ETH_MAC_SPEED_10M:
                  #if (USE_ETH_RMII == DEF_ENABLED)
                    /* RMII Half Duplex Collision detection does not work */
                    maccr |= ETH_MACCR_DM;
                  #endif
                break;

                case ARM_ETH_SPEED_100M:
                    maccr |= ETH_MACCR_FES;
                break;

                default:
                    return ARM_DRIVER_ERROR_UNSUPPORTED;
            }

            /* Confige Half/Full duplex mode */
            switch(arg & ARM_ETH_MAC_DUPLEX_Msk)
            {
                case ARM_ETH_MAC_DUPLEX_FULL:
                    maccr |= ETH_MACCR_DM;
                break;

                case ARM_ETH_MAC_DUPLEX_HALF:
                break;

                default:
                    return ARM_DRIVER_ERROR;
            }

            /* Configure loopback mode */
            if(arg & ARM_ETH_MAC_LOOPBACK)
            {
                maccr |= ETH_MACCR_LM;
            }

            dmaomr = ETH->DMAOMR & ~(ETH_DMAOMR_RSF| ETH_DMAOMR_TSF);

          #if (EMAC_CHECKSUM_OFFLOAD != 0)
            /* Enable rx checksum verification */
            if(arg & ARM_ETH_MAC_CHECKSUM_OFFLOAD_RX)
            {
                maccr  |= ETH_MACCR_IPCO;
                dmaomr |= ETH_DMAOMR_RSF;
            }

            /* Enable tx checksum generation */
            if(arg & ARM_ETH_MAC_CHECKSUM_OFFLOAD_TX)
            {
                dmaomr |= ETH_DMAOMR_TSF;
                Emac.tx_cks_offload = true;
            }
            else
            {
                Emac.tx_cks_offload = false;
            }

          #else

            if((arg & ARM_ETH_MAC_CHECKSUM_OFFLOAD_RX) || (arg & ARM_ETH_MAC_CHECKSUM_OFFLOAD_TX))
            {
                /* Checksum offload is disabled in the driver */
                return ARM_DRIVER_ERROR;
            }
          #endif

            ETH->DMAOMR = dmaomr;
            ETH->MACCR  = maccr;

            macffr = ETH->MACFFR & ~(ETH_MACFFR_PM | ETH_MACFFR_PAM | ETH_MACFFR_BFD);

            /* Enable broadcast frame receive */
            if((arg & ARM_ETH_MAC_ADDRESS_BROADCAST) == 0)
            {
                macffr |= ETH_MACFFR_BFD;
            }

            /* Enable all multicast frame receive */
            if(arg & ARM_ETH_MAC_ADDRESS_MULTICAST)
            {
                macffr |= ETH_MACFFR_PAM;
            }

            /* Enable promiscuous mode (no filtering) */
            if(arg & ARM_ETH_MAC_ADDRESS_ALL)
            {
                macffr |= ETH_MACFFR_PM;
            }

            ETH->MACFFR = macffr;
        break;

        case ARM_ETH_MAC_CONTROL_TX:
            /* Enable/disable MAC transmitter */
            maccr  = ETH->MACCR  & ~ETH_MACCR_TE;
            dmaomr = ETH->DMAOMR & ~ETH_DMAOMR_ST;

            if(arg != 0)
            {
                init_dma ();
                maccr  |= ETH_MACCR_TE;
                dmaomr |= ETH_DMAOMR_ST;
            }

            ETH->MACCR  = maccr;
            ETH->DMAOMR = dmaomr;
        break;

        case ARM_ETH_MAC_CONTROL_RX:
            /* Enable/disable MAC receiver */
            maccr  = ETH->MACCR  & ~ETH_MACCR_RE;
            dmaomr = ETH->DMAOMR & ~ETH_DMAOMR_SR;

            if(arg != 0)
            {
                init_dma ();
                maccr  |= ETH_MACCR_RE;
                dmaomr |= ETH_DMAOMR_SR;
            }

            ETH->MACCR  = maccr;
            ETH->DMAOMR = dmaomr;
        break;

        case ARM_ETH_MAC_FLUSH:
            /* Flush tx and rx buffers */
            if(arg & ARM_ETH_MAC_FLUSH_RX)
            {
            }

            if(arg & ARM_ETH_MAC_FLUSH_TX)
            {
                ETH->DMAOMR |= ETH_DMAOMR_FTF;
            }
        break;

        case ARM_ETH_MAC_VLAN_FILTER:
            /* Configure VLAN filter */
            ETH->MACVLANTR = arg;
        break;
    }

    return ARM_DRIVER_OK;
}


/**
  \fn          int32_t PHY_Read (uint8_t phy_addr, uint8_t reg_addr, uint16_t *data)
  \brief       Read Ethernet PHY Register through Management Interface.
  \param[in]   phy_addr  5-bit device address
  \param[in]   reg_addr  5-bit register address
  \param[out]  data      Pointer where the result is written to
  \return      \ref execution_status
*/
static int32_t PHY_Read(uint8_t phy_addr, uint8_t reg_addr, uint16_t* data)
{
  #if (ETH_SMI_SW == 0) /* Hardware MDIO */
    uint32_t     val;
    TickCount_t  TickStart;

    if((Emac.flags & EMAC_FLAG_POWER) == 0)
    {
        return ARM_DRIVER_ERROR;
    }

    val = ETH->MACMIIAR & ETH_MACMIIAR_CR;

    ETH->MACMIIAR = val | ETH_MACMIIAR_MB | ((uint32_t)phy_addr << 11) | ((uint32_t)reg_addr << 6);

    /* Wait until operation completed */
    TickStart = GetTick();

    do
    {
        if((ETH->MACMIIAR & ETH_MACMIIAR_MB) == 0)
        {
            break;
        }
    }
    while(TickHasTimeOut(TickStart, PHY_TIMEOUT) == false);

    if((ETH->MACMIIAR & ETH_MACMIIAR_MB) == 0)
    {
        *data = ETH->MACMIIDR & ETH_MACMIIDR_MD;
        return ARM_DRIVER_OK;
    }

    return ARM_DRIVER_ERROR_TIMEOUT;

  #else /* Software MDIO */
    if((Emac.flags & EMAC_FLAG_POWER) == 0)
    {
        return ARM_DRIVER_ERROR;
    }

    SW_MDIO_Dir (1); /* Dir: output */
    /* 32 consecutive ones on MDO to establish sync */
    SW_MDIO_Write (0xFFFFFFFF, 32);

    /* Start code (01), Read command (10) */
    SW_MDIO_Write (0x06, 4);

    /* Write PHY device address */
    SW_MDIO_Write (phy_addr, 5);

    /* Write PHY register address */
    SW_MDIO_Write (reg_addr, 5);

    /* Turnaround MDO is tristated */
    SW_MDIO_Dir (0); /* Dir: input */

    /* Read the data value */
    *data = (uint16_t)SW_MDIO_Read ();

    /* Turnaround MDIO is tristated */
    SW_MDIO_Dir (0); /* Dir: input */

    return ARM_DRIVER_OK;
  #endif
}


/**
  \fn          int32_t PHY_Write (uint8_t phy_addr, uint8_t reg_addr, uint16_t data)
  \brief       Write Ethernet PHY Register through Management Interface.
  \param[in]   phy_addr  5-bit device address
  \param[in]   reg_addr  5-bit register address
  \param[in]   data      16-bit data to write
  \return      \ref execution_status
*/
static int32_t PHY_Write(uint8_t phy_addr, uint8_t reg_addr, uint16_t data)
{
  #if (ETH_SMI_SW == 0) /* Hardware MDIO */
    uint32_t     val;
    TickCount_t  TickStart;

    if((Emac.flags & EMAC_FLAG_POWER) == 0)
    {
        return ARM_DRIVER_ERROR;
    }

    ETH->MACMIIDR = data;
    val = ETH->MACMIIAR & ETH_MACMIIAR_CR;
    ETH->MACMIIAR = val | ETH_MACMIIAR_MB | ETH_MACMIIAR_MW | ((uint32_t)phy_addr << 11) | ((uint32_t)reg_addr << 6);

    /* Wait until operation completed */
    TickStart = GetTick();

    do
    {
        if((ETH->MACMIIAR & ETH_MACMIIAR_MB) == 0)
        {
            break;
        }
    }
    while(TickHasTimeOut(TickStart, PHY_TIMEOUT) == false);

    if((ETH->MACMIIAR & ETH_MACMIIAR_MB) == 0)
    {
        return ARM_DRIVER_OK;
    }

    return ARM_DRIVER_ERROR_TIMEOUT;

  #else /* Software MDIO */
    if((Emac.flags & EMAC_FLAG_POWER) == 0)
    {
        return ARM_DRIVER_ERROR;
    }

    SW_MDIO_Dir(1); /* Dir: output */

    /* 32 consecutive ones on MDO to establish sync */
    SW_MDIO_Write(0xFFFFFFFF, 32);

    /* Start code (01), Write command (01) */
    SW_MDIO_Write(0x05, 4);

    /* Write PHY device address */
    SW_MDIO_Write(phy_addr, 5);

    /* Write PHY register address */
    SW_MDIO_Write(reg_addr, 5);

    /* Turnaround MDIO (1,0)*/
    SW_MDIO_Write(0x02, 2);

    /* Write the data value */
    SW_MDIO_Write(data, 16);

    /* Turnaround MDO is tristated */
    SW_MDIO_Dir (0); /* Dir: input */

    return ARM_DRIVER_OK;
  #endif
}


/* Ethernet IRQ Handler */
void ETH_IRQHandler(void)
{
    uint32_t dmasr, macsr, event = 0;

    dmasr = ETH->DMASR;
    ETH->DMASR = dmasr & (ETH_DMASR_NIS | ETH_DMASR_RS | ETH_DMASR_TS);

    if(dmasr & ETH_DMASR_TS)
    {
        /* Frame sent */
        event |= ARM_ETH_MAC_EVENT_TX_FRAME;
    }

    if(dmasr & ETH_DMASR_RS)
    {
        /* Frame received */
        event |= ARM_ETH_MAC_EVENT_RX_FRAME;
    }

    macsr = ETH->MACSR;

  #if (EMAC_TIME_STAMP != 0)
    if(macsr & ETH_MACSR_TSTS)
    {
        /* Timestamp interrupt */
        if(ETH->PTPTSSR & 2 /*ETH_PTPTSSR_TSTTR*/)
        {
            /* Time stamp target time reached */
            event |= ARM_ETH_MAC_EVENT_TIMER_ALARM;
        }
    }
  #endif
    if(macsr & ETH_MACSR_PMTS)
    {
        ETH->MACPMTCSR;
        event |= ARM_ETH_MAC_EVENT_WAKEUP;
    }

    /* Callback event notification */
    if(event && Emac.cb_event)
    {
        Emac.cb_event (event);
    }
}


/* MAC Driver Control Block */
ARM_DRIVER_ETH_MAC Driver_ETH_MAC0 =
{
    GetVersion,
    GetCapabilities,
    Initialize,
    Uninitialize,
    PowerControl,
    GetMacAddress,
    SetMacAddress,
    SetAddressFilter,
    SendFrame,
    ReadFrame,
    GetRxFrameSize,
    GetRxFrameTime,
    GetTxFrameTime,
    ControlTimer,
    Control,
    PHY_Read,
    PHY_Write
};
