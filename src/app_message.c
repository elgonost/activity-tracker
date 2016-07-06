#include <pebble.h>

#define BUFFER_SIZE 1300
// declaration of the byte arrays we send
// X,Y,Z are 4 digit numbers so we only need 2 concecutive bytes
// to represent each one
#define XYZ_BYTE_ARRAY_SIZE BUFFER_SIZE*2
// Timestamp is considered 9 digit number for convenience so we use 5 concecutive bytes
// to represent each timestamp value.
#define TIME_BYTE_ARRAY_SIZE 14
#define NUM_SAMPLES 25


static Window *s_window;

static int s_battery_level;
static TextLayer *s_battery_layer;

static TextLayer *s_time_layer;
static TextLayer *fall_layer;

AppTimer *appTimer;

DictionaryIterator *iter;
bool js_flag = false;
bool data_col = true;
bool activity_flag = false;
bool fall = false;

//Byte arrays
uint8_t XBytes[XYZ_BYTE_ARRAY_SIZE];
uint8_t YBytes[XYZ_BYTE_ARRAY_SIZE];
uint8_t ZBytes[XYZ_BYTE_ARRAY_SIZE];
uint8_t TimeBytes[TIME_BYTE_ARRAY_SIZE];	

int counter = 0;
int timeCounter = 0;
int dummy = 10;

int act_index = 0;
int select_counter = 0;
int number_of_activities = 0;
char* adl = "None";
char* duration = "None";
char* status = "Disabled";

// Keys for AppMessage Dictionary
// These should correspond to the values 
//you defined in appinfo.json/Settings
enum {
	STATUS_KEY = 0,	
	MESSAGE_KEY = 1,
  X_KEY = 2,
  Y_KEY = 3,
  Z_KEY = 4,
  TIME_KEY = 5,
  TIME_VALUE_START = 6,
  TIME_VALUE_END = 7,
  ACTIVITY = 8,
  ACTIVITY_NUMBER = 9,
  FALL_KEY = 10
};

struct byteForm{
  uint8_t first_half;
  uint8_t second_half;
};

struct timeByteForm{
  uint8_t byte_0;
  uint8_t byte_1;
  uint8_t byte_2;
  uint8_t byte_3;
  uint8_t byte_4;
  
  uint8_t byte_5;
  uint8_t byte_6;
};

//Converts the accelerometer values to bytes
struct byteForm convert_xyz_to_bytes(int value){
  struct byteForm split;
  
  //OR operation with 128 is equal to making MSB = 1
  //which represents the negative sign
  if(value<0)
    split.first_half = (abs(value)/100) | 128;
  else
    split.first_half = abs(value)/100;
  split.second_half = abs(value)%100;
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "Test value: %d. Converted to %d%d",value,split.first_half,split.second_half);
  return split;
}

struct timeByteForm convert_time_to_bytes(int value1,int value2){
  struct timeByteForm split;
  value2 = abs(value2);
  value1 = abs(value1);
  split.byte_0 = value2 % 100;
  split.byte_1 = (value2/100) % 100;
  split.byte_2 = (value2/10000) % 100;
  split.byte_3 = (value2/1000000);
  split.byte_4 = value1 % 100;
  split.byte_5 = (value1/100) % 100;
  split.byte_6 = (value1/10000);
  return split;
}

struct byteForm temp;
struct timeByteForm temp2,referenceTimestamp,choiceTimestamp;

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
  
}

static void battery_callback(BatteryChargeState state) {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"battery level is %d ",s_battery_level);
  // Record the new battery level
  s_battery_level = state.charge_percent;
  // Update meter
  //layer_mark_dirty(s_battery_layer);
  static char s_buffer[128];
   
  // Compose string of all data
  snprintf(s_buffer, sizeof(s_buffer),"Battery: %d%%",s_battery_level);
  text_layer_set_text(s_battery_layer, s_buffer);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  // A single click has just occured
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Select key pressed");
  app_timer_cancel(appTimer);
  text_layer_set_text(fall_layer, " ");
  APP_LOG(APP_LOG_LEVEL_DEBUG,"AppTimer was Canceled");
}

static void click_config_provider(void *context) {
  // Subcribe to button click events here
  ButtonId id = BUTTON_ID_SELECT;  // The Select button
  window_single_click_subscribe(id, select_click_handler);
}




static void data_handler(AccelData *data, uint32_t num_samples) {
    
  if((js_flag)&&(data_col)){
    //javascript is ready and we can start gathering data
    if(counter<BUFFER_SIZE){
      //keep gathering data
      for(int i=0;i<NUM_SAMPLES;i++){
        temp = convert_xyz_to_bytes(data[i].x);
        XBytes[2*counter] = temp.first_half;  
        XBytes[2*counter+1] = temp.second_half;
        
        temp = convert_xyz_to_bytes(data[i].y);
        YBytes[2*counter] = temp.first_half;  
        YBytes[2*counter+1] = temp.second_half;
        
        temp = convert_xyz_to_bytes(data[i].z);
        ZBytes[2*counter] = temp.first_half;  
        ZBytes[2*counter+1] = temp.second_half;
        
        if( ( (i==0) && counter==0 ) || ( ( i==(NUM_SAMPLES-1)) && (counter==(BUFFER_SIZE-1)) )) {
          APP_LOG(APP_LOG_LEVEL_DEBUG,"writing timestamp");
          APP_LOG(APP_LOG_LEVEL_DEBUG,"Counter=%d and i=%d",counter,i);
          temp2 = convert_time_to_bytes(data[i].timestamp / 100000000,data[i].timestamp % 100000000);
          
          TimeBytes[timeCounter] = temp2.byte_6;
          TimeBytes[timeCounter+1] = temp2.byte_5;
          TimeBytes[timeCounter+2] = temp2.byte_4;
          TimeBytes[timeCounter+3] = temp2.byte_3;
          TimeBytes[timeCounter+4] = temp2.byte_2;
          TimeBytes[timeCounter+5] = temp2.byte_1;
          TimeBytes[timeCounter+6] = temp2.byte_0;
          
          timeCounter+=7;
        }
        counter++;
      }
      //for debugging
      if(counter==325)
        APP_LOG(APP_LOG_LEVEL_DEBUG,"Counter is 325");
      if(counter==650)
        APP_LOG(APP_LOG_LEVEL_DEBUG,"Counter is 650");
      if(counter==975)
        APP_LOG(APP_LOG_LEVEL_DEBUG,"Counter is 975");
      if(counter==1300)
        APP_LOG(APP_LOG_LEVEL_DEBUG,"Counter is 1300 . . . sending");
      
    }
    else{
      //When we get here our Buffers are full. We need to send data
      //to PebbleKitJS.
      app_message_outbox_begin(&iter);
      dict_write_data(iter,X_KEY,&XBytes[0],sizeof(XBytes));
      dict_write_data(iter,Y_KEY,&YBytes[0],sizeof(YBytes));
      dict_write_data(iter,Z_KEY,&ZBytes[0],sizeof(ZBytes));
      dict_write_data(iter,TIME_KEY,&TimeBytes[0],sizeof(TimeBytes));
      dict_write_end(iter);
      app_message_outbox_send();      
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Message was sent to PebbleKitJS"); 
      counter = 0;
      timeCounter = 0;
    }
  }
}


void appTimerCallback(void *data){
  APP_LOG(APP_LOG_LEVEL_DEBUG, "AppTimer not cancelled. Sending email.");
  vibes_short_pulse();
  text_layer_set_text(fall_layer, " ");
  fall = true;
  //send email
}



// Called when a message is received from PebbleKitJS
static void in_received_handler(DictionaryIterator *received, void *context) {
	Tuple *tuple;
	
	tuple = dict_find(received, STATUS_KEY);
	if(tuple) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Status: %d", (int)tuple->value->uint32); 
	}
  
	if((int)tuple->value->uint32==0){
    APP_LOG(APP_LOG_LEVEL_DEBUG, "INITIALIZATION"); 
    js_flag = true;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "JS_FLAG is set true");
  }
  if((int)tuple->value->uint32==1){
    APP_LOG(APP_LOG_LEVEL_DEBUG, "FALL DETECTED"); 
    //alarm with vibration
    vibes_short_pulse();
    text_layer_set_text(fall_layer, "Are you alright?");
    appTimer = app_timer_register(30000,appTimerCallback,NULL);    
  }
  
	tuple = dict_find(received, MESSAGE_KEY);
	if(tuple) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Message: %s", tuple->value->cstring);
	}
  
  
}

// Called when an incoming message from PebbleKitJS is dropped
static void in_dropped_handler(AppMessageResult reason, void *context) {	
}

// Called when PebbleKitJS does not acknowledge receipt of a message
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
}

// Called when PebbleKitJS acknowledges receipt of a message
static void out_sent_handler(DictionaryIterator *iter, void *context) {
  if(fall){
    app_message_outbox_begin(&iter);
    dict_write_int(iter,FALL_KEY,&dummy,sizeof(int),true);
    dict_write_end(iter);
    app_message_outbox_send();
    fall=false;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "OUT_SENT_HANDLER HERE");     
  }
  
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);

  // Create output TextLayer
  s_time_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(58, 45), window_bounds.size.w, 50));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  
 
  s_battery_layer = text_layer_create(GRect(0, 90, window_bounds.size.w - 10, window_bounds.size.h-140));
  text_layer_set_font(s_battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text(s_battery_layer, "No data yet.");
  text_layer_set_overflow_mode(s_battery_layer, GTextOverflowModeWordWrap);
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_battery_layer));
  
  fall_layer = text_layer_create(GRect(0, 110, window_bounds.size.w - 10, window_bounds.size.h));
  text_layer_set_font(fall_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  //text_layer_set_text(fall_layer, "Are you alright?");
  text_layer_set_overflow_mode(fall_layer, GTextOverflowModeWordWrap);
  text_layer_set_text_alignment(fall_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(fall_layer));
    
}

static void main_window_unload(Window *window) {
  // Destroy output TextLayer
  text_layer_destroy(s_time_layer);
}

static void init(void) {
	s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });  
  window_set_click_config_provider(s_window, click_config_provider);
	window_stack_push(s_window, true);    
  update_time();
  battery_state_service_subscribe(battery_callback);
  battery_callback(battery_state_service_peek());
	
	// Register AppMessage handlers
	app_message_register_inbox_received(in_received_handler); 
	app_message_register_inbox_dropped(in_dropped_handler); 
	app_message_register_outbox_failed(out_failed_handler);
  app_message_register_outbox_sent(out_sent_handler);

  accel_data_service_subscribe(NUM_SAMPLES, data_handler);
  accel_service_set_sampling_rate(ACCEL_SAMPLING_25HZ);
  
  // Initialize AppMessage inbox and outbox buffers with a suitable size
  const int inbox_size = 128;
  const int outbox_size = 8000;
  app_message_open(inbox_size, outbox_size);
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

}

static void deinit(void) {
	app_message_deregister_callbacks();
	window_destroy(s_window);
}

int main( void ) {
	init();
	app_event_loop();
	deinit();
}