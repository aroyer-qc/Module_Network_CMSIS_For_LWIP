#pragma once

// TODO AR put all phy here, let config (ethernet_cfg.h) )choose the right MAC
#include "EMAC_STM32F4xx.h"

// TODO AR put all phy here, let config (ethernet_cfg.h) )choose the right PHY
#include "PHY_LAN8742A.h"

/* Ethernet MAC & PHY Driver */
extern ARM_DRIVER_ETH_MAC ARM_Driver_ETH_MAC_(ETH_DRV_NUM);
extern ARM_DRIVER_ETH_PHY ARM_Driver_ETH_PHY_(ETH_DRV_NUM);
