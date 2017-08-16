# STM32Ethernet

## Ethernet Library for Arduino

With an STM32 board with Ethernet compatibility, this library allows a STM32
board (NUCLEO, DISCOVERY, ...) to connect to the internet.  

For more information about this library please visit us at
http://www.arduino.cc/en/Reference/Ethernet

## Note

The library is based on LwIP, a Lightweight TCP/IP stack.  
http://git.savannah.gnu.org/cgit/lwip.git
The LwIP has been ported as Arduino library. The STM32Ethernet library depends
on it.

EthernetClass::maintain() in no more required to renew IP address from DHCP.  
It is done automatically by the LwIP stack in a background task.  

An Idle task is required by the LwIP stack to handle timer and data reception.
This idle task is called inside the main loop in background by the function
stm32_eth_scheduler(). Be careful to not lock the system inside the function
loop() where LwIP could never be updated. Call EthernetUDP::parsePacket() or
EthernetClient::available() performs an update of the LwIP stack.

## Wiki

You can find information at https://github.com/stm32duino/wiki/wiki/STM32Ethernet
