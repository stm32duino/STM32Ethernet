/*
 * @file    lwipopts.h
 * @author  Frederic Pillon <frederic.pillon@st.com> for STMicroelectronics.
 * @brief   Include header file to match Arduino library format
 *
 * @details Includes STM32lwipopts.h when it is present. Otherwise it includes
 *          lwipopts_extra.h (when present) followed by lwipopts_default.h.
 */

#ifndef _ARDUINO_LWIPOPTS_H
#define _ARDUINO_LWIPOPTS_H

/* LwIP specific configuration options. */
#if __has_include("STM32lwipopts.h")
  #include "STM32lwipopts.h"
#else
  #if __has_include("lwipopts_extra.h")
    #include "lwipopts_extra.h"
  #endif
  #include "lwipopts_default.h"
#endif

#endif /* _ARDUINO_LWIPOPTS_H */

