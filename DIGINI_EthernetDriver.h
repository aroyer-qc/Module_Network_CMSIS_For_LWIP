// Digini_EthernetDriver.h

#pragma once

// TODO AR put all phy here, let config (ethernet_cfg.h) )choose the right MAC
#include "EMAC_STM32f7xx.h"

// TODO AR put all phy here, let config (ethernet_cfg.h) )choose the right PHY
#include "PHY_LAN8742A.h"



#define ETH_PHY_NUM 0
#define ETH_MAC_NUM 0


// temporary do not use it here

/* ################## Ethernet peripheral configuration ##################### */

/* Section 1 : Ethernet peripheral configuration */

/* MAC ADDRESS: MAC_ADDR0:MAC_ADDR1:MAC_ADDR2:MAC_ADDR3:MAC_ADDR4:MAC_ADDR5 */
#define MAC_ADDR0   2
#define MAC_ADDR1   0
#define MAC_ADDR2   0
#define MAC_ADDR3   0
#define MAC_ADDR4   0
#define MAC_ADDR5   0

/* Definition of the Ethernet driver buffers size and count */
#define ETH_RX_BUF_SIZE                ETH_MAX_PACKET_SIZE /* buffer size for receive               */
#define ETH_TX_BUF_SIZE                ETH_MAX_PACKET_SIZE /* buffer size for transmit              */
#define ETH_RXBUFNB                    ((uint32_t)4)       /* 4 Rx buffers of size ETH_RX_BUF_SIZE  */
#define ETH_TXBUFNB                    ((uint32_t)4)       /* 4 Tx buffers of size ETH_TX_BUF_SIZE  */

/* Section 2: PHY configuration section */


/* PHY Reset delay these values are based on a 1 ms Systick interrupt*/
#define PHY_RESET_DELAY                 ((uint32_t)0x000000FF)
/* PHY Configuration delay */
#define PHY_CONFIG_DELAY                ((uint32_t)0x00000FFF)

#define PHY_READ_TO                     ((uint32_t)0x0000FFFF)
#define PHY_WRITE_TO                    ((uint32_t)0x0000FFFF)

/* Section 3: Common PHY Registers */

#define PHY_BCR                         ((uint16_t)0x00)    /*!< Transceiver Basic Control Register   */
#define PHY_BSR                         ((uint16_t)0x01)    /*!< Transceiver Basic Status Register    */

#define PHY_RESET                       ((uint16_t)0x8000)  /*!< PHY Reset */
#define PHY_LOOPBACK                    ((uint16_t)0x4000)  /*!< Select loop-back mode */
#define PHY_FULLDUPLEX_100M             ((uint16_t)0x2100)  /*!< Set the full-duplex mode at 100 Mb/s */
#define PHY_HALFDUPLEX_100M             ((uint16_t)0x2000)  /*!< Set the half-duplex mode at 100 Mb/s */
#define PHY_FULLDUPLEX_10M              ((uint16_t)0x0100)  /*!< Set the full-duplex mode at 10 Mb/s  */
#define PHY_HALFDUPLEX_10M              ((uint16_t)0x0000)  /*!< Set the half-duplex mode at 10 Mb/s  */
#define PHY_AUTONEGOTIATION             ((uint16_t)0x1000)  /*!< Enable auto-negotiation function     */
#define PHY_RESTART_AUTONEGOTIATION     ((uint16_t)0x0200)  /*!< Restart auto-negotiation function    */
#define PHY_POWERDOWN                   ((uint16_t)0x0800)  /*!< Select the power down mode           */
#define PHY_ISOLATE                     ((uint16_t)0x0400)  /*!< Isolate PHY from MII                 */

#define PHY_AUTONEGO_COMPLETE           ((uint16_t)0x0020)  /*!< Auto-Negotiation process completed   */
#define PHY_LINKED_STATUS               ((uint16_t)0x0004)  /*!< Valid link established               */
#define PHY_JABBER_DETECTION            ((uint16_t)0x0002)  /*!< Jabber condition detected            */

/* Section 4: Extended PHY Registers */

#define PHY_SR                          ((uint16_t)0x10)    /*!< PHY status register Offset                      */
#define PHY_MICR                        ((uint16_t)0x11)    /*!< MII Interrupt Control Register                  */
#define PHY_MISR                        ((uint16_t)0x12)    /*!< MII Interrupt Status and Misc. Control Register */

#define PHY_LINK_STATUS                 ((uint16_t)0x0001)  /*!< PHY Link mask                                   */
#define PHY_SPEED_STATUS                ((uint16_t)0x0002)  /*!< PHY Speed mask                                  */
#define PHY_DUPLEX_STATUS               ((uint16_t)0x0004)  /*!< PHY Duplex mask                                 */

#define PHY_MICR_INT_EN                 ((uint16_t)0x0002)  /*!< PHY Enable interrupts                           */
#define PHY_MICR_INT_OE                 ((uint16_t)0x0001)  /*!< PHY Enable output interrupt events              */

#define PHY_MISR_LINK_INT_EN            ((uint16_t)0x0020)  /*!< Enable Interrupt on change of link status       */
#define PHY_LINK_INTERRUPT              ((uint16_t)0x2000)  /*!< PHY link status interrupt mask                  */

