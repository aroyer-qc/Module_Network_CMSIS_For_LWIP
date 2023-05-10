#ifndef __ETHERNETIF_H__
#define __ETHERNETIF_H__

#include "lwip/netif.h"
#include "lwip/err.h"

#ifdef __cplusplus
 extern "C" {
#endif

err_t    ethernetif_init(struct netif *netif);
void     ethernetif_poll(struct netif *netif);          // external driver 
void     ethernetif_check_link (struct netif *netif);   // external driver

void     ethernetif_input( void *s_pxNetIf );           // from old driver
//uint16_t readPHYRegister(uint16_t phyAddress);          // from old driver


#ifdef __cplusplus
 }
#endif

#endif
