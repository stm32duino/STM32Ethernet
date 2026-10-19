# STM32Ethernet

STM32Ethernet allows an Ethernet-compatible STM32 board, such as a NUCLEO or
DISCOVERY board, to connect to a local network or the Internet. It follows the
Arduino Ethernet API and provides client, server, DHCP, and DNS functionality.

The [hosted API documentation](https://stm32duino.github.io/STM32Ethernet/) is
available online. See the [examples](examples/) for complete sketches.

## Requirements

- An STM32 board with Ethernet hardware support, for example:
  - NUCLEO-F429ZI
  - STM32F746G-DISCOVERY
- The [STM32duino LwIP library](https://github.com/stm32duino/LwIP).

Both libraries are available through the Arduino Library Manager. In the
Arduino IDE, open **Sketch > Include Library > Manage Libraries...**, search for
`LwIP`, then install **LwIP** and **STM32Ethernet**.

STM32Ethernet is based on [LwIP](https://savannah.nongnu.org/projects/lwip/),
a lightweight TCP/IP stack, and is compatible with the
[Arduino Ethernet API](https://www.arduino.cc/reference/en/libraries/ethernet/).

## Quick start

Include both libraries, then start the connection with DHCP:

```cpp
#include <LwIP.h>
#include <STM32Ethernet.h>

void setup() {
  Serial.begin(9600);
  if (Ethernet.begin() == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
  }
}

void loop() {
}
```

`Ethernet.begin()` returns `1` when DHCP configuration succeeds and `0` when
it fails. When DHCP is unavailable, configure a static address instead:

```cpp
IPAddress ip(192, 168, 0, 177);
Ethernet.begin(ip);
```

The static-address overloads also allow the subnet mask, gateway, and DNS
server to be provided:

```cpp
Ethernet.begin(ip, subnet);
Ethernet.begin(ip, subnet, gateway);
Ethernet.begin(ip, subnet, gateway, dns);
```

See the [WebClient example](examples/WebClient/WebClient.ino) for a DHCP
connection with a static-IP fallback.

## MAC address

When no MAC address is supplied, the library retrieves the board's internal
MAC address. Read it after initializing Ethernet:

```cpp
uint8_t mac[6];
Ethernet.begin();
Ethernet.MACAddress(mac);
```

To use a custom MAC address, set it before initialization:

```cpp
uint8_t mac[] = {0x00, 0x80, 0xE1, 0x01, 0x01, 0x01};
Ethernet.setMACAddress(mac);
Ethernet.begin();
```

The library also provides `begin()` overloads that accept a MAC address
directly.

## LwIP configuration and scheduling

LwIP options are configured in `lwipopts.h`. The library provides
`lwipopts_default.h` as its default configuration. To customize options for a
sketch, provide a `STM32lwipopts.h` file, or add options to the default
configuration through `lwipopts_extra.h`.

The LwIP stack needs periodic processing for timers and packet reception. A
timer callback runs `stm32_eth_scheduler()` every 1 ms by default, using
`TIM14` (`DEFAULT_ETHERNET_TIMER`). A core variant can redefine
`DEFAULT_ETHERNET_TIMER`. Avoid disabling interrupts for extended periods.
Call `Ethernet.schedule()` to process the LwIP stack explicitly, for example
after a long-running loop.

DHCP lease renewal is handled by LwIP in the background, so calling
`Ethernet.maintain()` is not required for normal operation. The method remains
available for sketches that need to check or handle DHCP lease renewals.

## Examples

The repository includes sketches for DHCP address reporting, TCP clients and
servers, UDP, DNS/NTP, and FreeRTOS. Open them from the Arduino IDE's examples
menu or browse the [examples directory](examples/).

## Documentation and source

- [Latest released API documentation](https://stm32duino.github.io/STM32Ethernet/)
- [Development API documentation](https://stm32duino.github.io/STM32Ethernet/dev/)
- [STM32Ethernet source repository](https://github.com/stm32duino/STM32Ethernet)
- [LwIP Arduino library](https://github.com/stm32duino/LwIP)
