// Based on
// Matt Gaidica, PhD
// Neurotech Hub, Washington University in St. Louis
// https://neurotechhub.wustl.edu/
// https://neurotechhub.wustl.edu/using-the-esp32-s3-ulp-coprocessor-with-the-arduino-ide/
// Modifications include:
//    checking if ULP is initialised and not reloading,
//    getting the count rate.
//    using different versions of esp than esp32s3



// ULPHandler.cpp
#include "ULPHandler.h"
#include "Configuration.h"


//#include "esp32s3/ulp.h"
#include "esp32/ulp.h"
//esp32, esp32s2, esp32s3 have the ULP library, esp32c6 doesn't seem to have the ULP libary
#include "driver/rtc_io.h"
#include "soc/rtc_io_reg.h"
#include "soc/rtc.h"
extern "C" {
  #include <esp_private/esp_clk.h>
}

//RTC_DATA_ATTR bool ulpInitialized = false;
//RTC_DATA_ATTR uint32_t ulpCount = 0;
//RTC_DATA_ATTR uint32_t lastWakeTime = 0;

enum {
  EDGE_COUNT,
  PREV_EDGE_COUNT,
  PREV_TICKS,
  INIT_FLAG,
  SLOW_PROG_ADDR  // Program start address
};


// counts all state changes (LOW->HIGH, HIGH->LOW)
// divide by 2 for single transition type
const ulp_insn_t ulp_program[] = {
  // Initialize transition counter and previous state
  I_MOVI(R3, 0),  // R3 <- 0 (reset the transition counter)
  I_MOVI(R2, 1),  // R2 <- 0 (previous state, assume LOW initially)

  // Main loop
  M_LABEL(1),

  // Read RTC_GPIO_INDEX with RTC offset
  I_RD_REG(RTC_GPIO_IN_REG, RTC_GPIO_INDEX + RTC_GPIO_IN_NEXT_S, RTC_GPIO_INDEX + RTC_GPIO_IN_NEXT_S),

  // Save the current state in a temporary register (R1)
  I_MOVR(R1, R0),  // R1 <- R0 (store current GPIO state temporarily)

  // Compare current state (R1) with previous state (R2)
  I_SUBR(R0, R1, R2),  // R0 = current state (R1) - previous state (R2)
  I_BL(5, 1),          // If R0 == 0 (no state change), skip instructions
  I_ADDI(R3, R3, 1),   // Increment R3 by 1 (transition detected)
  I_MOVR(R2, R1),      // R2 <- R1 (store the current state for the next iteration)

  // Store the transition counter
  I_MOVI(R1, EDGE_COUNT),  // Set R1 to address RTC_SLOW_MEM[1]
  I_ST(R3, R1, 0),         // Store it in RTC_SLOW_MEM

  // RTC clock on the ESP32-S3 is 17.5MHz, delay 0xFFFF = 3.74 ms
  I_DELAY(0xFFFF),  // debounce
  I_DELAY(0xFFFF),  // debounce
  I_DELAY(0xFFFF),  // debounce
  I_DELAY(0xFFFF),  // debounce
  I_DELAY(0xFFFF),  // debounce
  I_DELAY(0xFFFF),  // debounce

  M_BX(1),  // Loop back to label 1
};



void ULPHandler::setupULP() {

  Serial.println("Initialising ULP program.");
  
  memset(RTC_SLOW_MEM, 0, CONFIG_ULP_COPROC_RESERVE_MEM);  // set to zeros, optional
  size_t size = sizeof(ulp_program) / sizeof(ulp_insn_t);
  esp_err_t err = ulp_process_macros_and_load(SLOW_PROG_ADDR, ulp_program, &size);  // offset by PROG_ADDR

  if (err == ESP_OK) {
    // Serial.println("ULP program loaded successfully."); // optional
  } else if (err == ESP_ERR_NO_MEM) {
    Serial.println("Error: Not enough memory to load ULP program.");
  } else {
    Serial.printf("Error: ULP program load returned unexpected error %d\n", err);
  }

  // init GPIO for ULP to monitor
  rtc_gpio_init(GPIO_SENSOR_PIN);
  rtc_gpio_set_direction(GPIO_SENSOR_PIN, RTC_GPIO_MODE_INPUT_ONLY);
  if (ULP_COUNT_PIN_PULLUP_EN) {
      rtc_gpio_pullup_en(GPIO_SENSOR_PIN);     // enable the pull-up resistor
  }
  else {
      rtc_gpio_pullup_dis(GPIO_SENSOR_PIN);     // disable the pull-up resistor
  }
  rtc_gpio_pulldown_dis(GPIO_SENSOR_PIN);  // disable the pull-down resistor
  rtc_gpio_hold_en(GPIO_SENSOR_PIN);       // required to maintain pull-up

  ulp_run(SLOW_PROG_ADDR);  // Start the ULP program with offset

  // Set the marker to indicate initialisation is complete
  //RTC_SLOW_MEM[ULP_INIT_MARKER_ADDR] = ULP_INIT_MARKER;
  RTC_SLOW_MEM[INIT_FLAG] = ULP_INIT_MARKER;

  RTC_SLOW_MEM[PREV_TICKS] = 0;
  RTC_SLOW_MEM[PREV_EDGE_COUNT] = 0;

  Serial.println("Initialised ULP program.");
}

bool ULPHandler::isULPInitialized() {
    return (RTC_SLOW_MEM[INIT_FLAG] == ULP_INIT_MARKER);
}

uint32_t ULPHandler::getCount() {
  // The ULP coprocessor effectively operates on 16-bit logic.
  // Therefore, you should read values from RTC_SLOW_MEM with a 0xFFFF mask to isolate the lower 16-bits
  uint32_t count = RTC_SLOW_MEM[EDGE_COUNT] & 0xFFFF;
  return count;
}

float ULPHandler::getRate() {

  // The ULP coprocessor effectively operates on 16-bit logic.
  // Therefore, you should read values from RTC_SLOW_MEM with a 0xFFFF mask to isolate the lower 16-bits
  // countDiff should be 16 bit to handle overflow
  uint16_t count, prevCount, countDiff;
  count = RTC_SLOW_MEM[EDGE_COUNT] & 0xFFFF;
  prevCount = RTC_SLOW_MEM[PREV_EDGE_COUNT] & 0xFFFF;
  countDiff = count - prevCount;
  RTC_SLOW_MEM[PREV_EDGE_COUNT] = count;
  
  uint32_t ticksNow, prevTicks, ticksDiff, timeDiff;
  ticksNow = rtc_time_get() & 0xFFFFFFFF; // get lower 32-bits of current RTC clock ticks (48-bit)
  prevTicks = RTC_SLOW_MEM[PREV_TICKS]; // get previous lower 32-bits of RTC clock ticks (RTC memory is 32-bit)
  ticksDiff = ticksNow - prevTicks; // calc differnt in tics. this should handle overflow because the variables are 32-bit unsigned in.
  timeDiff = rtc_time_slowclk_to_us(ticksDiff, esp_clk_slowclk_cal_get()); //note there is some variance in the esp_clk_slowclk_cal_get value. If the timeDiff is from timeNow minus prevTime, then this can lead to issues with different calibration values used for timeNow and prevTime. So that's why we store the ticks, and perform the conversion using the ticksDiff.
  RTC_SLOW_MEM[PREV_TICKS] = ticksNow;

  float rate = 0;
  if (timeDiff != 0) {
    rate = (float)countDiff / ((float)timeDiff / 1000000);
  }
  
  return rate;
}


void ULPHandler::enter_deep_sleep() {

  delay(10);
  Serial.print("Sleeping for ");
  Serial.println(ULP_SLEEP_TIME);

  esp_sleep_enable_timer_wakeup(ULP_SLEEP_TIME * 1000000);  // Wake up every SLEEP_TIME seconds
  esp_deep_sleep_start();  // Enter deep sleep

}
