#include <pebble.h>

#if !defined(PBL_COLOR)
  #define RESOURCE_ID_GLITCH_COLOUR RESOURCE_ID_GLITCH_BW
#endif

static Window *s_main_window;
static TextLayer *s_time_layer;
static GFont s_nin_font;
static GBitmap *s_glitch_bitmap;
static BitmapLayer *s_glitch_layer;
static bool s_is_glitching = false;

static void update_time() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
  text_layer_set_text(s_time_layer, s_buffer);
}

static void glitch_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_draw_bitmap_in_rect(ctx, s_glitch_bitmap, bounds);

  if (s_is_glitching) {
    graphics_context_set_fill_color(ctx, GColorWhite);
    
    // Industrial "Strobe" - 12 solid bars
    for (int i = 0; i < 12; i++) {
      // Use bounds.size.h so it works on Round (180) and Square (168)
      int y = rand() % bounds.size.h;
      int height = 2 + (rand() % 8); // Minimum 2 pixels thick so it's never "blank"
      
      // Ensure the width covers the whole screen
      graphics_fill_rect(ctx, GRect(0, y, bounds.size.w, height), 0, GCornerNone);
    }
  }
}

static void glitch_timer_callback(void *data) {
  static int count = 0;
  if (count < 10) { // Slightly more flickers for impact
    s_is_glitching = !s_is_glitching; 
    layer_mark_dirty(bitmap_layer_get_layer(s_glitch_layer));
    app_timer_register(50, glitch_timer_callback, NULL); 
    count++;
  } else {
    s_is_glitching = false;
    count = 0;
    layer_mark_dirty(bitmap_layer_get_layer(s_glitch_layer)); 
  }
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  #if defined(PBL_COLOR)
    s_glitch_bitmap = gbitmap_create_with_resource(RESOURCE_ID_GLITCH_COLOUR);
  #else
    s_glitch_bitmap = gbitmap_create_with_resource(RESOURCE_ID_GLITCH_BW);
  #endif

  s_glitch_layer = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(s_glitch_layer, s_glitch_bitmap);
  bitmap_layer_set_alignment(s_glitch_layer, GAlignCenter);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_glitch_layer));

  layer_set_update_proc(bitmap_layer_get_layer(s_glitch_layer), glitch_update_proc);

  s_nin_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_NIN_42));
  
  // MARGIN FIX: Increase width to full bounds.size.w
  // For Pebble Round, we increase height to 90 to ensure "22:55" doesn't clip
  int font_height = 90; 
  s_time_layer = text_layer_create(GRect(0, (bounds.size.h / 2) - (font_height / 2), bounds.size.w, font_height));
  
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_font(s_time_layer, s_nin_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  update_time();
}

static void main_window_unload(Window *window) {
  fonts_unload_custom_font(s_nin_font);
  text_layer_destroy(s_time_layer);
  gbitmap_destroy(s_glitch_bitmap);
  bitmap_layer_destroy(s_glitch_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  app_timer_register(10, glitch_timer_callback, NULL);
  
//   // Glitch every 10 seconds for debug
//   if (tick_time->tm_sec % 10 == 0) {
//     app_timer_register(10, glitch_timer_callback, NULL);
//   }
}

static void init() {
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_stack_push(s_main_window, true);
//   tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
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