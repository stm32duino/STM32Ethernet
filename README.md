# STM32Ethernet

## Ethernet Library for Arduino

With an STM32 board with Ethernet compatibility, this library allows a STM32
board (NUCLEO, DISCOVERY, ...) to connect to the internet.  

This library follows the Ethernet API from Arduino.<br>
For more information about it please visit: http://www.arduino.cc/en/Reference/Ethernet

## Note

The library is based on LwIP, a Lightweight TCP/IP stack, available here:

http://git.savannah.gnu.org/cgit/lwip.git

The LwIP has been ported as Arduino library. The STM32Ethernet library depends
on it.

`EthernetClass::maintain()` in no more required to renew IP address from DHCP.<br>
It is done automatically by the LwIP stack in a background task.  

An Idle task is required by the LwIP stack to handle timer and data reception.<br>
This idle task is called inside a timer callback each 1 ms by the
function `stm32_eth_scheduler()`.<br>
A `DEFAULT_ETHERNET_TIMER` is set in the library to `TIM14`.<br>
`DEFAULT_ETHERNET_TIMER` can be redefined in the core variant.<br>
Be careful to not lock the system in a function which disabling IRQ.<br>
Call `Ethernet::schedule()` performs an update of the LwIP stack.<br>

## Wiki

You can find information at https://github.com/stm32duino/wiki/wiki/STM32Ethernet
