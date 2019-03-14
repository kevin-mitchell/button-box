#include "../docs/frozen.c"
  
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONFIG_JSON "{distanceCentimeters: %d, active: %B, ledOn: %B}"
#define ROOT_CONFIG_JSON "{ configuration: %M }"
#define THING_DATA_JSON "{position: %d, strength: %d}"

typedef struct
{
   long on_time;
   long off_time;
} device_configuration;
device_configuration _config;

static void scan_configuration(const char *str, int len, void * user_data)
{
   json_scanf(str, len, CONFIG_JSON, &_config.on_time, &_config.off_time);      
}

int* extract_thing_data(char* message_input)
{
   static int thing_data[10] = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
   int i, len = strlen(message_input), position, strength;
   //this is the size of the display array;
   int display_size = sizeof(thing_data)/sizeof(thing_data[0]);
   struct json_token t;

   for (i = 0; json_scanf_array_elem(message_input, len, ".group", i, &t) > 0; i++) {
      json_scanf(t.ptr, t.len, THING_DATA_JSON, &position, &strength);
      //If we have a posiiton that's out of bounds of the display array size, then we can't display this value
      //and trying to set it in the array would cause a out of bounds exception
      if(position > (display_size-1)) {
         continue;
      }
      thing_data[position] = strength;
   }
   
   return thing_data;

}

device_configuration extract_configuration(char *message_input)
{  
   device_configuration config;    
   json_scanf(message_input, strlen(message_input), ROOT_CONFIG_JSON,
      scan_configuration, config);
   config.on_time = _config.on_time;
   config.off_time = _config.off_time;
   return config;
}

/**
 * Build up an array of thing data in a single string
 */
char* build_thing_value_json(int* thing_data) {   
   int display_size = sizeof(thing_data)/sizeof(thing_data[0]);
   const char *individual_values[display_size];
   for(int i = 0; i <= display_size; i++) {
      printf("%d", thing_data[i]);
   }

   { \"position\": 1, \"strength\": 3 }

   // char buf[200] = "";
   // struct json_out out = JSON_OUT_BUF(buf, sizeof(buf));
   // int test_data[3] = {2, 4, 5};
   // json_printf(&out, "%M", json_printf_array, test_data, sizeof(test_data), sizeof(test_data[0]),
   //              "%d");
   // printf("%s\n", buf);

   return "ok";
}

int main() {
   char *testJson = "{ \"group\": [ { \"position\": 1, \"strength\": 3 }, { \"position\": 2, \"strength\": 7 }, { \"position\": 3, \"strength\": 0 } ], \"configuration\": { \"offTime\":\"9999999\", \"onTime\":\"42424242\" } }";
   device_configuration config = extract_configuration(testJson);
   printf("\non_time: %ld\noff_tim: %ld\n", config.on_time, config.off_time);
   int* thing_data = extract_thing_data(testJson);
   
   build_thing_value_json(thing_data);

}


