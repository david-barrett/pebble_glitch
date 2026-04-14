#include <pebble.h>
// https://www.nindestruct.com/fonts.html

#if !defined(PBL_COLOR)
  #define RESOURCE_ID_GLITCH_COLOUR RESOURCE_ID_GLITCH_BW
#endif

// Define the bottom margin once. 45px for Round, 25px for Square.
#define DATE_MARGIN PBL_IF_ROUND_ELSE(45, 25)
#define BATTERY_Y_OFFSET PBL_IF_ROUND_ELSE(35, 25) // Optional: adjust top margin too

static Window *s_main_window;
static TextLayer *s_time_layer, *s_date_layer;
static GFont s_nin_font;
static GBitmap *s_glitch_bitmap;
static BitmapLayer *s_glitch_layer;
static Layer *s_battery_layer;
static bool s_is_glitching = false;
static bool s_show_error = false;
static int s_battery_level;
static char s_date_buffer[32];

static void update_time() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  static char s_time_buffer[8];
  strftime(s_time_buffer, sizeof(s_time_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
  
  if(s_time_layer) text_layer_set_text(s_time_layer, s_time_buffer);

  strftime(s_date_buffer, sizeof(s_date_buffer), "%Y.%m.%d // LOG_DECAY", tick_time);
  if(s_date_layer) text_layer_set_text(s_date_layer, s_date_buffer);
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_text_color(ctx, GColorCyan);

  if (s_is_glitching && s_show_error) {
    graphics_draw_text(ctx, "SIGNAL LOSS", fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), 
                       bounds, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
    return; 
  }
    
  char s_batt_buffer[32];
  snprintf(s_batt_buffer, sizeof(s_batt_buffer), "BATTERY [ ");
  graphics_draw_text(ctx, s_batt_buffer, fonts_get_system_font(FONT_KEY_GOTHIC_14), 
                     GRect(0, 0, 65, 20), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);

  int x_offset = 62;
  for (int i = 0; i < 5; i++) {
    graphics_context_set_fill_color(ctx, (s_battery_level >= (i + 1) * 20) ? GColorCyan : GColorDarkCandyAppleRed);
    graphics_fill_rect(ctx, GRect(x_offset + (i * 5), 4, 3, 10), 0, GCornerNone);
  }

  snprintf(s_batt_buffer, sizeof(s_batt_buffer), " ] %d%%", s_battery_level);
  graphics_draw_text(ctx, s_batt_buffer, fonts_get_system_font(FONT_KEY_GOTHIC_14), 
                     GRect(x_offset + 25, 0, 50, 20), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
}

static void battery_callback(BatteryChargeState state) {
  s_battery_level = state.charge_percent;
  if(s_battery_layer) layer_mark_dirty(s_battery_layer);
}

static void glitch_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  if(s_glitch_bitmap) graphics_draw_bitmap_in_rect(ctx, s_glitch_bitmap, bounds);

  if (s_is_glitching) {
    graphics_context_set_fill_color(ctx, GColorWhite);
    for (int i = 0; i < 12; i++) {
      int y = rand() % bounds.size.h;
      int height = 2 + (rand() % 8);
      graphics_fill_rect(ctx, GRect(0, y, bounds.size.w, height), 0, GCornerNone);
    }
  }
}

static void glitch_timer_callback(void *data) {
  static int count = 0;
  if (!s_main_window || !window_stack_contains_window(s_main_window)) {
    count = 0; return;
  }

  Layer *window_layer = window_get_root_layer(s_main_window);
  GRect bounds = layer_get_bounds(window_layer);
  int font_h = 90;
  
  // Dynamic margin based on screen shape
  int date_margin = PBL_IF_ROUND_ELSE(35, 25);

  if (count < 15) { 
    s_is_glitching = !s_is_glitching; 
    if (count % 3 == 0) s_show_error = !s_show_error;

    int j_x = (rand() % 21) - 10;
    int j_y = (rand() % 21) - 10;

    if(s_time_layer) layer_set_frame(text_layer_get_layer(s_time_layer), GRect(j_x, (bounds.size.h/2)-(font_h/2)+j_y, bounds.size.w, font_h));
    if(s_battery_layer) layer_set_frame(s_battery_layer, GRect((bounds.size.w/2)-60+(j_x*2), BATTERY_Y_OFFSET+(j_y/2), 120, BATTERY_Y_OFFSET));
    
    // Updated Jitter for Date
    if(s_date_layer) {
      layer_set_frame(text_layer_get_layer(s_date_layer), 
                      GRect(-j_x, (bounds.size.h - DATE_MARGIN) - (j_y / 2), bounds.size.w, 20));
    }
    
    app_timer_register(50, glitch_timer_callback, NULL); 
    count++;
  } else {
    s_is_glitching = false;
    s_show_error = false;
    count = 0;

    if(s_time_layer) layer_set_frame(text_layer_get_layer(s_time_layer), GRect(0, (bounds.size.h/2)-(font_h/2), bounds.size.w, font_h));
    if(s_battery_layer) layer_set_frame(s_battery_layer, GRect((bounds.size.w/2)-60, 25, 120, 25));
    
    // Updated Reset for Date
    if(s_date_layer) {
      layer_set_frame(text_layer_get_layer(s_date_layer), 
                      GRect(0, bounds.size.h - DATE_MARGIN, bounds.size.w, 20));
    }
  }
  layer_mark_dirty(window_layer);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // 1. Bitmaps
  s_glitch_bitmap = gbitmap_create_with_resource(RESOURCE_ID_GLITCH_COLOUR);
  s_glitch_layer = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(s_glitch_layer, s_glitch_bitmap);
  bitmap_layer_set_alignment(s_glitch_layer, GAlignCenter);
  layer_set_update_proc(bitmap_layer_get_layer(s_glitch_layer), glitch_update_proc);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_glitch_layer));

  // 2. Font & Time
  s_nin_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_NIN_42));
  s_time_layer = text_layer_create(GRect(0, (bounds.size.h/2)-45, bounds.size.w, 90));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_font(s_time_layer, s_nin_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  // 3. Battery
  s_battery_layer = layer_create(GRect((bounds.size.w/2)-60, BATTERY_Y_OFFSET, 120, BATTERY_Y_OFFSET));
  layer_set_update_proc(s_battery_layer, battery_update_proc);
  layer_add_child(window_layer, s_battery_layer);

  // 4. Date
  s_date_layer = text_layer_create(GRect(0, bounds.size.h - DATE_MARGIN, bounds.size.w, 20));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorCyan);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

  // 5. Init Services
  battery_state_service_subscribe(battery_callback);
  battery_callback(battery_state_service_peek());
  update_time();
}

static void main_window_unload(Window *window) {
  battery_state_service_unsubscribe();
  text_layer_destroy(s_time_layer); s_time_layer = NULL;
  text_layer_destroy(s_date_layer); s_date_layer = NULL;
  layer_destroy(s_battery_layer); s_battery_layer = NULL;
  bitmap_layer_destroy(s_glitch_layer);
  gbitmap_destroy(s_glitch_bitmap);
  fonts_unload_custom_font(s_nin_font);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  app_timer_register(10, glitch_timer_callback, NULL);
}

static void init() {
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load, .unload = main_window_unload
  });
  window_stack_push(s_main_window, true);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}