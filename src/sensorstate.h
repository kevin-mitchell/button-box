#include <stdlib.h>
#include <string.h>

using std::string;

class SensorState
{ 
    // Access specifier 
    public: 
        int latest_distance_mm = 8888;        
        int last_update_timestamp = 0;
        // Member Functions() 

        /**
        * is this a meaningful change that warrents updating the state elsewhere?
        **/
        bool has_sensor_changed(int new_distance) { 
            if(abs(new_distance - latest_distance_mm) >= RANGE_SENSITIVITY) {
                latest_distance_mm = new_distance;
                last_update_timestamp = (int)mg_time();
                return true;
            } else {
                return false;
            }
        }

        bool is_active() {
          return (
            latest_distance_mm < 2000 &&
            (int)mg_time() - last_update_timestamp < READING_TIMEOUT_SECONDS);
        }

        int get_latest_distance_mm() {
            return latest_distance_mm;
        }

        int get_latest_distance_cm() {
            return (int)((double)latest_distance_mm/10.0);
        }

   
    private:
        //the number of millimeters of range sensitivity we should have.
        static const int RANGE_SENSITIVITY = 150;
        static const int READING_TIMEOUT_SECONDS = 3600;        
}; 