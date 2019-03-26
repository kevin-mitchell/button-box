#include "mgos.h"
#include "mgos_aws_shadow.h"
#include "mgos_mqtt.h"
#include "mgos_timers.h"
#include "mgos_arduino_pololu_VL53L0X.h"
#include "sensorstate.h"
#include "bb_parser.h"

//Display for device can hold up to 10 display values
const int DISPLAY_SIZE = 10;

//The configuration settings for the device
device_configuration config;

//The maximum size of the JSON we will be generating
// #define MAXIMUM_JSON_OUTPUT_LENGTH 1500
//this is where we'll keep the JSON that we generate
const size_t BUFFER_SIZE = 1500;
char jsonOutput[BUFFER_SIZE];

//This will contain the state of all of the displays
display_element displayStates[DISPLAY_SIZE];

SensorState* sensorState = new SensorState();
VL53L0X* vl53 = mgos_VL53L0X_create();

/** 
 *
 * Send the state of the device to update the IoT device shadow. 
 *
 */
void report_state(void)
{
  LOG(LL_INFO, ("report state..."));
  build_json(
    jsonOutput, 
    BUFFER_SIZE, 
    config, 
    displayStates, 
    DISPLAY_SIZE, 
    sensorState->get_latest_distance_cm(), 
    sensorState->is_active());
  LOG(LL_INFO, ("state being sent to AWS %s", jsonOutput));
  //Note we're using mgos_aws_shadow_update_simple instaed of mgos_aws_shadow_updatef because it's just a string
  //@see: https://github.com/mongoose-os-libs/aws/blob/master/src/mgos_aws_shadow.c#L510
  mgos_aws_shadow_update_simple(0, jsonOutput);
}

/**
  * Update the state of the local device. This is where I would handle turning on LEDs or (in theory) playing a sound, etc.
  */
void update_state(void)
{
  LOG(LL_INFO, ("update state..."));
}

/*
 * Main AWS Device Shadow state callback handler. Will get invoked when
 * connection is established or when new versions of the state arrive via one of the topics.
 *
 * CONNECTED event comes with no state.
 *
 * For DELTA events, state is passed as "desired", reported is not set.
 *
 */
static void aws_shadow_state_handler(void *arg, enum mgos_aws_shadow_event ev,
                                     uint64_t version,
                                     const struct mg_str reported,
                                     const struct mg_str desired,
                                     const struct mg_str reported_md,
                                     const struct mg_str desired_md)
{

  LOG(LL_INFO, ("== Event: %d (%s), version: %llu", ev,
                mgos_aws_shadow_event_name(ev), version));

  if (ev == MGOS_AWS_SHADOW_CONNECTED)
  {
    report_state();
    return;
  }
  if (ev != MGOS_AWS_SHADOW_GET_ACCEPTED &&
      ev != MGOS_AWS_SHADOW_UPDATE_DELTA)
  {
    return;
  }

  extract_configuration(&config, desired.p);
  extract_thing_data(displayStates, DISPLAY_SIZE, desired.p);

  update_state();

  if (ev == MGOS_AWS_SHADOW_UPDATE_DELTA)
  {
    report_state();
  }
  (void)arg;
}

static void distance_sensor_reading_cb(void *arg)
{
  int reading = mgos_VL53L0X_readRangeSingleMillimeters(vl53);
  // LOG(LL_INFO, ("VL52L0X reading read: %d", sensorState.current_reading));
  // if (has_distance_range_changed(sensorState));
  if (sensorState->has_sensor_changed(reading)) {
    report_state();
  }
  (void) arg;
}

enum mgos_app_init_result mgos_app_init(void)
{
  LOG(LL_INFO, ("setup started."));


  // Initialize Pololu VL53L0X library
  
  mgos_VL53L0X_begin(vl53);
  mgos_VL53L0X_init_2v8(vl53);
  mgos_VL53L0X_setMeasurementTimingBudget(vl53, 20000);
  // int reading = mgos_VL53L0X_readRangeSingleMillimeters(vl53);

  //todo: probably should be a constant, or even part of the configuration from the Cloud
  //This is the time delay between sensor readings
  int sensorReadingPeriod = 500;
  mgos_set_timer(sensorReadingPeriod, MGOS_TIMER_REPEAT, distance_sensor_reading_cb, NULL);

  LOG(LL_INFO, ("VL52L0X setup complete."));


  /* Register AWS shadow callback handler	*/
  mgos_aws_shadow_set_state_handler(aws_shadow_state_handler, NULL);

  /* setup output */
  int led_pin = 25;
  mgos_gpio_set_mode(led_pin, MGOS_GPIO_MODE_OUTPUT);

  /* setup button */
  // int btn_pin = 14;
  // enum mgos_gpio_pull_type btn_pull;
  // enum mgos_gpio_int_mode btn_int_edge;
  LOG(LL_INFO, ("some button info here"));
  // mgos_gpio_set_button_handler(btn_pin, MGOS_GPIO_PULL_DOWN, MGOS_GPIO_INT_EDGE_POS, 20, button_cb,
  //                              NULL);

  return MGOS_APP_INIT_SUCCESS;
}
