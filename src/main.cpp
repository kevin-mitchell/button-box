#include "mgos.h"
#include "mgos_aws_shadow.h"
#include "mgos_mqtt.h"
#include "mgos_timers.h"
#include "mgos_arduino_pololu_VL53L0X.h"

#define JSON_BUTTON_LED "{buttonPressed: %B, ledOn: %B}"

static mgos_timer_id press_timer = MGOS_INVALID_TIMER_ID;
bool button_pressed = false;
bool led_on = false;
VL53L0X* vl53 = mgos_VL53L0X_create();


static void set_status_led(bool turn_on)
{
  int led_pin = 25;

  mgos_gpio_write(led_pin, turn_on);
}

void report_state(void)
{
  // struct mbuf fb;
  // struct json_out fout = JSON_OUT_MBUF(&fb);
  // mbuf_init(&fb, 256);

  // json_printf(&fout, "{welcome:61234321}");

  // mbuf_trim(&fb);

  // LOG(LL_INFO, ("== Reporting state: %s", fb.buf));
  mgos_aws_shadow_updatef(0, JSON_BUTTON_LED, button_pressed, led_on);

  // mbuf_free(&fb);
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

  json_scanf(reported.p, reported.len, JSON_BUTTON_LED, &button_pressed, &led_on);

  json_scanf(desired.p, desired.len, JSON_BUTTON_LED, &button_pressed, &led_on);

  update_state();

  // mgos_mqtt_pub("button", "pressed", 7, 1, false);

  if (ev == MGOS_AWS_SHADOW_UPDATE_DELTA)
  {
    report_state();
  }
  (void)arg;
}

static void button_timer_cb(void *arg)
{
  int pin = 14;
  int n = 0; /* Number of times the button is reported down */
  for (int i = 0; i < 10; i++)
  {
    int level = mgos_gpio_read(pin);
    if (level > 0)
      n++;
    mgos_msleep(1);
  }

  if (n < 9)
  {
    button_pressed = false;
    LOG(LL_INFO, ("button no longer pressed."));
    report_state();
    mgos_clear_timer(press_timer);
  }

  (void)arg;
}

static void button_cb(int pin, void *arg)
{

  mgos_clear_timer(press_timer);
  // set_status_led(mgos_gpio_read(led_pin));
  // LOG(LL_INFO, ("LED pin %d", mgos_sys_config_get_provision_btn_hold_ms()));
  button_pressed = true;
  report_state();
  LOG(LL_INFO, ("button pressed.."));
  press_timer = mgos_set_timer(100, MGOS_TIMER_REPEAT, button_timer_cb, arg);
  (void)arg;
}

// static void my_blah_timer_cb(void *arg) {  
//   LOG(LL_INFO, ("uptime: %.2lf", mgos_uptime());
//   (void) arg;
// }

static void my_blah_timer_cb(void *arg)
{
  int reading = mgos_VL53L0X_readRangeSingleMillimeters(vl53);
  LOG(LL_INFO, ("VL52L0X reading read: %d", reading));
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

  mgos_set_timer(1000, MGOS_TIMER_REPEAT, my_blah_timer_cb, NULL);

  LOG(LL_INFO, ("VL52L0X setup complete."));


  /* Register AWS shadow callback handler	*/
  mgos_aws_shadow_set_state_handler(aws_shadow_state_handler, NULL);

  /* setup output */
  int led_pin = 25;
  mgos_gpio_set_mode(led_pin, MGOS_GPIO_MODE_OUTPUT);

  /* setup button */
  int btn_pin = 14;
  // enum mgos_gpio_pull_type btn_pull;
  // enum mgos_gpio_int_mode btn_int_edge;
  LOG(LL_INFO, ("some button info here"));
  mgos_gpio_set_button_handler(btn_pin, MGOS_GPIO_PULL_DOWN, MGOS_GPIO_INT_EDGE_POS, 20, button_cb,
                               NULL);

  return MGOS_APP_INIT_SUCCESS;
}
