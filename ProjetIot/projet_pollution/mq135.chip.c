#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  pin_t pin;
  int ppm;
} chip_data_t;


void chip_timer_callback(void *data) {
  chip_data_t *chip_data = (chip_data_t*)data;
  chip_data->ppm = attr_read(chip_data->ppm);
  printf("%d\n", chip_data->ppm);
  
  // Simulate converting TDS value to voltage
  float volts = chip_data->ppm *5.0/ 1000;
  
  printf("%d\n", chip_data->ppm);
  printf("%f\n", volts);
  // Send the correct voltage on the out pin
  pin_dac_write(chip_data->pin, volts);
}

void chip_init() {
  chip_data_t *chip_data = (chip_data_t*)malloc(sizeof(chip_data_t));
  chip_data->ppm = attr_init("ppm", 400); 
  chip_data->pin = pin_init("A0", ANALOG);
  
  const timer_config_t config = {
    .callback = chip_timer_callback,
    .user_data = chip_data,
  };

  timer_t timer_id = timer_init(&config);
  timer_start(timer_id, 1000, true); 

  printf("Hello from mq-135 chip!\n");
}
