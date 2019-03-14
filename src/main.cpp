#include "mgos.h"
#include "mgos_aws_shadow.h"
#include "mgos_mqtt.h"
#include "mgos_timers.h"
#include "mgos_arduino_pololu_VL53L0X.h"
#include "sensorstate.h"
#include "bb_parser.h"

#define JSON_BUTTON_LED "{distanceCentimeters: %d, active: %B, ledOn: %B}"

device_configuration config;
int* thing_data = [10];
SensorState* sensorState = new SensorState();
VL53L0X* vl53 = mgos_VL53L0X_create();

void report_state(void)
{
  mgos_aws_shadow_updatef(0, JSON_BUTTON_LED, sensorState->get_latest_distance_cm(), sensorState->is_active(), led_on);
}

void update_state(void)
{
  LOG(LL_INFO, ("update state... led is %d", led_on));
  set_status_led(led_on);
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
  LOG(LL_INFO, ("Reported state: %.*s\n", (int)reported.len, reported.p));
  LOG(LL_INFO, ("Desired state : %.*s\n", (int)desired.len, desired.p));
  LOG(LL_INFO, ("Reported metadata: %.*s\n", (int)reported_md.len, reported_md.p));
  LOG(LL_INFO, ("Desired metadata : %.*s\n", (int)desired_md.len, desired_md.p));

  // json_scanf(reported.p, reported.len, JSON_BUTTON_LED, &button_pressed, &led_on);

  // json_scanf(desired.p, desired.len, JSON_BUTTON_LED, &led_on);

  // update_state();

  // mgos_mqtt_pub("button", "pressed", 7, 1, false);

  config = extract_configuration(desired.p);
  thing_data = extract_thing_data(desired.p);

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

  mgos_set_timer(500, MGOS_TIMER_REPEAT, distance_sensor_reading_cb, NULL);

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
