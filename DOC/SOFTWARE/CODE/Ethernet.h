/* 
   * ETH_CLOCK_GPIO0_IN   - default: external clock from crystal oscillator
   * ETH_CLOCK_GPIO0_OUT  - 50MHz clock from internal APLL output on GPIO0 - possibly an inverter is needed for LAN8720
   * ETH_CLOCK_GPIO16_OUT - 50MHz clock from internal APLL output on GPIO16 - possibly an inverter is needed for LAN8720
   * ETH_CLOCK_GPIO17_OUT - 50MHz clock from internal APLL inverted output on GPIO17 - tested with LAN8720
*/
#ifndef __ETHERNET_H
#define __ETHERNET_H

#include <ETH.h>

#define SDCARD_LOG

#ifdef SDCARD_LOG
#define EthernetLog(...) Serial.print(__VA_ARGS__)
#define EthernetLogLn(...) Serial.println(__VA_ARGS__)
#define EthernetLogF(...) Serial.printf(__VA_ARGS__)
#else
#define EthernetLog(...)
#define EthernetLogLn(...)
#define EthernetLogF(...)
#endif

/** Pin# of the enable signal for the external crystal oscillator (-1 to disable for internal APLL source)*/
//#define ETH_PHY_POWER 12

/** Type of the Ethernet PHY (LAN8720 or TLK110)*/
#define ETH_TYPE ETH_PHY_LAN8720

/** I2C-address of Ethernet PHY (0 or 1 for LAN8720, 31 for TLK110)*/
#define ETH_ADDR 1
#define ETH_PHY_ADDR 1

/** Pin# of the I2C clock signal for the Ethernet PHY*/
#define ETH_MDC_PIN 23

/** Pin# of the I2C IO signal for the Ethernet PHY*/
#define ETH_MDIO_PIN 18

#define ETH_CLK_MODE ETH_CLOCK_GPIO17_OUT

#define ETH_POWER_PIN   -1

extern bool eth_connected;

void initEthernet();


#endif
