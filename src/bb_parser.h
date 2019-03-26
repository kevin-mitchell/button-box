
//       UNCOMMENT FOR TESTING
// #include "../docs/frozen.c"  
// #include <float.h>
// #include <math.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>

//       COMMENT FOR TESTING
#include "frozen.c"



#define CONFIG_JSON "{onTime: %d, offTime: %d}"
#define ROOT_CONFIG_JSON "{ configuration: %M, group: "" }"
#define ROOT_CONFIG_OUTPUT_JSON "{ configuration: %s, group: %s, device:%s}"
#define THING_DATA_JSON "{position: %d, strength: %d}"
//This will be used to report the state of the device
#define DEVICE_OUTPUT_STATE_JSON "{distanceCentimeters: %d, active: %B}"
#define POTENTIAL_OUTPUTS 10

typedef struct
{
   long on_time;
   long off_time;
} device_configuration;
device_configuration _config;

typedef struct
{
   int position;
   int strength;
} display_element;

static void scan_configuration(const char *str, int len, void * user_data)
{
   json_scanf(str, len, CONFIG_JSON, &_config.on_time, &_config.off_time);      
}

void extract_thing_data(display_element* display_elements_buffer, int display_size, const char* message_input)
{
   int i, len = strlen(message_input), position, strength;
   struct json_token t;
   
   for(i = 0; i < display_size; i++) {
      display_elements_buffer[i].strength = -1;
      display_elements_buffer[i].position = -1;
   }
   
   for (i = 0; json_scanf_array_elem(message_input, len, ".group", i, &t) > 0; i++) {
      json_scanf(t.ptr, t.len, THING_DATA_JSON, &position, &strength);
      //If we have a posiiton that's out of bounds of the display array size, then we can't display this value
      //and trying to set it in the array would cause a out of bounds exception
      if(position > (display_size - 1)) {
         continue;
      }
      display_elements_buffer[i].strength = strength;
      display_elements_buffer[i].position = position;
   }
}

void extract_configuration(device_configuration* config_buffer, const char* message_input)
{  
   json_scanf(message_input, strlen(message_input), ROOT_CONFIG_JSON,
      scan_configuration, config_buffer);
   // printf("%s", message_input);
   config_buffer->on_time = _config.on_time;
   config_buffer->off_time = _config.off_time;
}

/**
 * Build up an array of Thing data in a single string
 */
void build_thing_value_json(char* output_buffer, size_t buffer_size, display_element* thing_data, int display_size) {   
   
   struct json_out out = JSON_OUT_BUF(output_buffer, buffer_size);
   json_printf(&out, "[");
   for(int i = 0; i < display_size; i++) {
      if(thing_data[i].position == -1) {
         break;
      }
      json_printf(&out, THING_DATA_JSON, thing_data[i].position, thing_data[i].strength);
      if(i+1 < display_size && thing_data[i+1].position != -1) {
         json_printf(&out, ",");   
      }
   }

   json_printf(&out, "]");

}

void build_configuration_json(char* output_buffer, size_t buffer_size, device_configuration config) {
 
   struct json_out out = JSON_OUT_BUF(output_buffer, buffer_size);
   json_printf(&out, CONFIG_JSON, config.on_time, config.off_time);
   
}

void build_local_device_state_json(char* output_buffer, size_t buffer_size, int distance_centimeters, bool active) {
 
   struct json_out out = JSON_OUT_BUF(output_buffer, buffer_size);
   json_printf(&out, DEVICE_OUTPUT_STATE_JSON, distance_centimeters, active);
   
}

void build_json(
   char* output_buffer, //output buffer, aka where the JSON goes
   size_t buffer_size, //size of output buffer because I guess this is C
   device_configuration config,  //the device configuration
   display_element* thing_data,  //array of thing_data
   int display_size,  //the display size
   int distance_centimeters,  //distance reading from the device
   bool active //is the device active (meaning it's not timed out and somebody is in the device)
   ) {
      struct json_out out = JSON_OUT_BUF(output_buffer, buffer_size);
      
      size_t small_buffer_size = 500;
      size_t large_buffer_size = 1200;

      char config_json[small_buffer_size];
      build_configuration_json(config_json, small_buffer_size, config);

      char thing_json[large_buffer_size];
      build_thing_value_json(thing_json, large_buffer_size, thing_data, display_size);

      char local_device_json[small_buffer_size];
      build_local_device_state_json(local_device_json, small_buffer_size, distance_centimeters, active);

      json_printf(&out, ROOT_CONFIG_OUTPUT_JSON, config_json, thing_json, local_device_json);
}



//       UNCOMMENT FOR TESTING


// int main() {
   
//    const char* testJson = "{ \"group\": [ { \"position\": 1, \"strength\": 3 }, { \"position\": 2, \"strength\": 7 }, { \"position\": 3, \"strength\": 0 } ], \"configuration\": { \"offTime\":9999999, \"onTime\":42424242 } }";
//    device_configuration config;
//    extract_configuration(&config, testJson);
//    printf("\n\nExtracting config...\n");
//    printf("\non_time: %ld\noff_tim: %ld\n", config.on_time, config.off_time);
   
//    size_t buffer_size = 1500;
//    char output[buffer_size];
//    build_configuration_json(output, buffer_size, config);
//    printf("\nBuilding config JSON...\n%s\n", output);

   
//    int display_size = 10;
//    display_element thing_data[display_size];
//    extract_thing_data(thing_data, display_size, testJson);
//    printf("\n%d", thing_data[0].strength);
   
   
//    printf("\n\n\nThing JSON\n\n");
   
//    build_thing_value_json(output, buffer_size, thing_data, display_size);
   
//    printf("\n%s", output);
   
//    printf("\n\nfinal output:\n");
//    build_json(output, buffer_size, config, thing_data, display_size, 55, true);
//    printf("%s", output);
// }


