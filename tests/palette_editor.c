#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#define MATRIX_ROWS 8
#define MATRIX_COLS 5
#define RGB_MATRIX_MAXIMUM_BRIGHTNESS 190
#define QK_KB_0 1000
#define USB_DEVICE_STATE_CONFIGURED 2
#define XTREEMZE_RGB_PROFILE_UNASSIGNED 255
#define ENCODER_CW_EVENT 2
#define IS_ENCODEREVENT(e) ((e).type >= 2)
typedef uint32_t layer_state_t;
typedef struct { struct { bool pressed; struct { uint8_t row,col; } key; uint8_t type; } event; } keyrecord_t;
typedef struct { uint8_t mode,h,s,v,speed; } xtreemze_rgb_profile_t;
typedef struct { xtreemze_rgb_profile_t profiles[50]; } xtreemze_rgb_profile_store_t;
static xtreemze_rgb_profile_store_t rgb_store;
static bool rgb_store_dirty, preview_active, combo_override_active, legacy_passthrough_active;
static xtreemze_rgb_profile_t preview_profile;
static uint32_t preview_started, now;
static layer_state_t layer_state = 12, default_layer_state = 1;
static bool master = true, transport = true, combos = true;
static int saves, settings_saves;
static uint32_t matrix_rows[MATRIX_ROWS];
static uint32_t timer_read32(void) { return now; }
static uint32_t timer_elapsed32(uint32_t n) { return now-n; }
static bool is_keyboard_master(void) { return master; }
static bool is_transport_connected(void) { return transport; }
static int usb_device_state_get_configure_state(void) { return 2; }
static void clear_keyboard(void) {}
static void layer_clear(void) { layer_state = 0; }
static void layer_state_set(layer_state_t s) { layer_state=s; }
static uint8_t get_highest_layer(layer_state_t s) { uint8_t i=0; while(s>>=1) i++; return i; }
static int matrix_get_row(int row) { return matrix_rows[row]; }
static bool is_combo_enabled(void) { return combos; }
static void combo_disable(void) { combos=false; }
static void combo_enable(void) { combos=true; }
static void rgb_store_ensure_loaded(void) {}
static void rgb_profile_invalidate_runtime(void) {}
static void rgb_store_save(void) { saves++; rgb_store_dirty=false; }
static xtreemze_rgb_profile_t *rgb_profile_for_scope(uint8_t s,uint8_t i) { return &rgb_store.profiles[(s==0?0:s==1?1:s==2?14:18)+i]; }
static void xtreemze_palette_settings_begin(void) {}
static void xtreemze_palette_settings_end(bool save) { settings_saves += save; }
static void xtreemze_palette_settings_color(uint8_t s,uint8_t i,uint8_t h,uint8_t sat,uint8_t v) { (void)s;(void)i;(void)h;(void)sat;(void)v; }
static void xtreemze_palette_settings_preview(uint8_t s,uint8_t i,uint8_t p,uint8_t h,uint8_t sat) { (void)s;(void)i;(void)p;(void)h;(void)sat; }
#include "../keyboards/splitkb/halcyon/ferris/keymaps/xtreemze_final/palette_editor_impl.h"
static void key(int row,int col) { keyrecord_t r={.event={.pressed=true,.key={row,col}}}; assert(!xtreemze_palette_process(0,&r)); }
int main(void) {
  memset(&rgb_store,255,sizeof(rgb_store));
  xtreemze_palette_enter();
  assert(palette_active && layer_state==0 && !combos);
  palette_wait_release=false;
  key(1,1);
  assert(saves==0 && settings_saves==0);
  key(0,0); xtreemze_palette_housekeeping();
  assert(!palette_active && layer_state==12 && combos && saves==0);
  assert(rgb_store.profiles[1].mode==255);
  xtreemze_palette_enter(); palette_wait_release=false;
  key(1,1); key(0,1); xtreemze_palette_housekeeping();
  assert(!palette_active && saves==1 && settings_saves==1);
  xtreemze_palette_enter(); palette_wait_release=false;
  now += 60001; xtreemze_palette_housekeeping(); xtreemze_palette_housekeeping();
  assert(!palette_active && saves==1);
  xtreemze_palette_enter(); palette_wait_release=false; master=false;
  xtreemze_palette_housekeeping(); xtreemze_palette_housekeeping();
  assert(!palette_active && saves==1);
  master=true;
  /* Hold threshold, entry release drain and exit release drain are observable. */
  keyrecord_t enter={.event={.pressed=true}};
  assert(!xtreemze_palette_process(XTREEMZE_PALETTE_KEYCODE,&enter));
  now += 699; xtreemze_palette_housekeeping(); assert(!palette_active);
  matrix_rows[0]=1;
  now++; xtreemze_palette_housekeeping(); assert(palette_active && palette_wait_release);
  const int prior_saves=saves;
  key(0,1); assert(!palette_exiting && saves==prior_saves);
  matrix_rows[0]=0; xtreemze_palette_housekeeping(); assert(!palette_wait_release);
  for(int i=0;i<100;i++) key(1,1);
  assert(saves==prior_saves);
  matrix_rows[0]=1; key(0,0); xtreemze_palette_housekeeping();
  assert(palette_active && palette_exiting);
  matrix_rows[0]=0; xtreemze_palette_housekeeping();
  assert(!palette_active && layer_state==12);
  for(int i=0;i<20;i++) {
    xtreemze_palette_enter(); palette_wait_release=false;
    for(int j=0;j<80;j++) key(0,3);
    assert(palette_index < palette_count());
    transport=false; xtreemze_palette_housekeeping();
    xtreemze_palette_housekeeping(); transport=true;
    assert(!palette_active && saves==prior_saves);
  }
  return 0;
}
