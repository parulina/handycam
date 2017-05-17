#include "icon.h"
#include "handycam.h"

#include <stdio.h>
#include <math.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_native_dialog.h>
#include <allegro5/allegro_memfile.h>
#include <allegro5/allegro_primitives.h>


#define FILE_LOAD_EVENT ALLEGRO_GET_EVENT_TYPE('f', 'l', 'l', 'e')

static void setup_file_load_event(ALLEGRO_EVENT * ev, size_t size = 1024)
{
	ev->type = FILE_LOAD_EVENT;
	ev->user.data1 = (intptr_t) new char[size];
	ev->user.data2 = size;
	
	memset((char*)ev->user.data1, '\0', sizeof(char)*size);
}

static void dtor_file_load_event(ALLEGRO_USER_EVENT * event)
{
	delete [] (char*)event->data1;
}

void send_file_load_event(ALLEGRO_EVENT_SOURCE * source, string filepath)
{
	ALLEGRO_EVENT ev;
	setup_file_load_event(&ev, filepath.size()+1);
	filepath.copy((char*)ev.user.data1, ev.user.data2);
	al_emit_user_event(source, &ev, dtor_file_load_event);
}

#ifdef ALLEGRO_WINDOWS
#include <windows.h>
#include <allegro5/allegro_windows.h>

bool drop_callback(ALLEGRO_DISPLAY * display, UINT message, WPARAM wparam, LPARAM lparam, LRESULT * result, void * userdata)
{
	if(message == WM_DROPFILES){
		
		HDROP drop_info = (HDROP) wparam;
		UINT drop_count = DragQueryFile(drop_info, -1, NULL, 0);
		ALLEGRO_EVENT_SOURCE * file_load_source = (ALLEGRO_EVENT_SOURCE*)userdata;
		if(file_load_source && drop_count == 1){
			
			ALLEGRO_EVENT event;
			setup_file_load_event(&event, MAX_PATH+1);
			DragQueryFile(drop_info, 0, (char*)event.user.data1, MAX_PATH);
			al_emit_user_event(file_load_source, &event, dtor_file_load_event);
		}
		return true;
	}
	return false;
}
#endif

enum menuActions {
	menuRecord = 1,
	menuNightshot,
	menuLight
};

int main(int argc, char **argv)
{
	al_init();
	al_install_keyboard();
	al_install_mouse();
	al_init_font_addon();
	al_init_image_addon();
	al_init_primitives_addon();
	al_init_native_dialog_addon();
	
	int pal_width = 720, pal_height = 576;
	ALLEGRO_DISPLAY * display = al_create_display(pal_width, pal_height);
	ALLEGRO_TIMER * timer = al_create_timer(1.0 / 50);

	ALLEGRO_BITMAP * subject_bitmap = NULL;
	ALLEGRO_BITMAP * noise_bitmap = al_create_bitmap(pal_width, pal_height);
	
	ALLEGRO_MENU * menu = al_create_menu();
	if(menu){
		al_insert_menu_item(menu, 0, "Record", 		menuRecord, 0, 0, 0);
		al_insert_menu_item(menu, 0, "Nightshot", 	menuNightshot, 0, 0, 0);
		al_insert_menu_item(menu, 0, "Light", 		menuLight, 0, 0, 0);
		
	}
	
	ALLEGRO_EVENT_QUEUE *queue = al_create_event_queue();
	ALLEGRO_EVENT_SOURCE file_load_source;
	al_init_user_event_source(&file_load_source);
	
	if(queue){
		al_register_event_source(queue, al_get_display_event_source(display));
		al_register_event_source(queue, al_get_timer_event_source(timer));
		al_register_event_source(queue, al_get_keyboard_event_source());
		al_register_event_source(queue, al_get_mouse_event_source());
		al_register_event_source(queue, al_get_default_menu_event_source());
		al_register_event_source(queue, &file_load_source);
	}
	
	ALLEGRO_FILE * iconfile = al_open_memfile(icon_png, icon_png_len, "r");
	if(iconfile){
		ALLEGRO_BITMAP * icon = al_load_bitmap_f(iconfile, ".png");
		if(icon){
			al_set_display_icon(display, icon);
		}
	}
	
	
	al_start_timer(timer);
	
	al_set_window_title(display, "SONY HANDYCAM VISION");
	
	Handycam handycam;
	// according to original
	handycam.setHorizontalPadding(al_get_display_width(display)/9);
	handycam.setVerticalPadding(al_get_display_height(display)/8);
	
	handycam.registerEventSources(queue);
	
	
	al_clear_to_color(al_map_rgb(0, 0, 255));
	al_flip_display();
	
	int intro_countdown = 80;
	while(intro_countdown > 0){
		ALLEGRO_EVENT ev;
		al_wait_for_event(queue, &ev);
		if(ev.type == ALLEGRO_EVENT_TIMER){
			if(intro_countdown == 60){
				handycam.drawSystemMessage("SONY\n\"INFOLITHIUM\"\nSYSTEM", 
											0, 0, al_get_display_width(display), al_get_display_height(display));
				al_flip_display();
			}
			if(intro_countdown == 10){
				al_clear_to_color(al_map_rgb(0, 0, 255));
				al_flip_display();
			}
			
			intro_countdown--;
		}
	}
	
	// second init
	al_set_display_menu(display, menu);
	#ifdef ALLEGRO_WINDOWS
		HWND wnd = al_get_win_window_handle(display);
		SetWindowLongPtr(wnd, GWL_EXSTYLE, WS_EX_ACCEPTFILES);
		
		al_win_add_window_callback(display, drop_callback, &file_load_source);
	#endif
	
	if(argc == 2){
		send_file_load_event(&file_load_source, string(argv[1]));
	}
	
	char letters[21] = {0};
	int menu_height = 0;
	int fps = 0;
	int lens_countdown = 30;
	bool running = true, draw = false;
	double subject_scale = 1.0;
	
	while(running){
		ALLEGRO_EVENT ev;
		al_wait_for_event(queue, &ev);
		
		handycam.event(&ev);
		
		if(ev.type == FILE_LOAD_EVENT){
			string filepath((const char*)ev.user.data1, ev.user.data2);
			printf("... [%s]\n", filepath.c_str());
			
			ALLEGRO_BITMAP * file_bitmap = al_load_bitmap(filepath.c_str());
			if(file_bitmap){
				if(subject_bitmap) al_destroy_bitmap(subject_bitmap);
				subject_bitmap = file_bitmap;
			}
			
			al_unref_user_event(&ev.user);
		}
		if(ev.type == ALLEGRO_EVENT_TIMER){
			if(ev.timer.source == timer){
				draw = true;
				fps++;
				if(fps > 60) fps = 0;
				if(lens_countdown > 0) lens_countdown--;
			}
		}
		if(ev.type == ALLEGRO_EVENT_MENU_CLICK){
			if(ev.user.data1 == menuRecord){
				handycam.toggleRecording();
			}
			if(ev.user.data1 == menuNightshot){
				handycam.toggleNightshot();
			}
			if(ev.user.data1 == menuLight){
				handycam.cycleLight();
			}
		}
		if(ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE){
			running = false;
		}
		if(ev.type == ALLEGRO_EVENT_DISPLAY_RESIZE){
			al_acknowledge_resize(ev.display.source);
			if(!menu_height){
				menu_height = pal_height - ev.display.height;
				al_resize_display(display, pal_width, pal_height+menu_height);
			}
		}
		if(ev.type == ALLEGRO_EVENT_KEY_CHAR){
			if(ev.keyboard.keycode == ALLEGRO_KEY_PAD_PLUS){
				subject_scale += 0.1;
			}
			else if(ev.keyboard.keycode == ALLEGRO_KEY_PAD_MINUS){
				subject_scale -= 0.1;
			}
			else if(ev.keyboard.keycode == ALLEGRO_KEY_BACKSPACE){
				for(int i = sizeof(letters)-1; i >= 0; i--){
					if(letters[i] != 0){
						letters[i] = 0;
						break;
					}
				}
				handycam.setMessage(letters);
			}
			else if(ev.keyboard.unichar >= 32 && ev.keyboard.unichar <= 126){
				for(int i = 0; i < sizeof(letters)-1; i++){
					if(letters[i] == 0) {
						letters[i] = ev.keyboard.unichar;
						if(letters[i] >= 97 && letters[i] <= 122) {
							// small caps, sonyfont doesn't have it
							letters[i] -= 32;
						} else if(letters[i] >= 65 && letters[i] <= 90) {
							// type with big caps = symbols
							letters[i] += 32;
						}
						break;
					}
				}
				handycam.setMessage(letters);
			}
		}
		if(ev.type == ALLEGRO_EVENT_KEY_DOWN){
			if(ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE){
				running = false;
			}
		}
		if(ev.type == ALLEGRO_EVENT_MOUSE_AXES){
			if(ev.mouse.dz){
				//handycam.setTimecode(handycam.timecode()+ev.mouse.dz);
				//handycam.setBatteryLevel(handycam.batteryLevel() + ev.mouse.dz);
				subject_scale += (ev.mouse.dz/100);
			}
		}
		if(draw && al_is_event_queue_empty(queue)){
			draw = false;
			
			int w = al_get_display_width(display), h = al_get_display_height(display);
			
			al_clear_to_color(al_map_rgb(0, 0, 0));
			
			al_lock_bitmap(al_get_target_bitmap(), al_get_bitmap_format(al_get_target_bitmap()), ALLEGRO_LOCK_WRITEONLY);
			for(int i = 0; i < (w*h)/4; i++){
				
				int x = rand() % pal_width,
					y = rand() % pal_height;
				
				unsigned char c = 30 + (rand() % 50);
				al_put_pixel(x, y, al_map_rgb(c, c, c));
			}
			al_unlock_bitmap(al_get_target_bitmap());
			
			if(subject_bitmap){
				al_draw_tinted_scaled_rotated_bitmap(subject_bitmap,
					al_map_rgb(255, 255, 255),
					al_get_bitmap_width(subject_bitmap)/2, al_get_bitmap_height(subject_bitmap)/2,
					w/2, h/2, subject_scale, subject_scale, 0, 0);
			}
			
			// curve the bottom pixels
			al_lock_bitmap(al_get_target_bitmap(), al_get_bitmap_format(al_get_target_bitmap()), ALLEGRO_LOCK_WRITEONLY);
			const int dh = h-8;
			double tape_dist = sin((al_get_time()/2.5)*ALLEGRO_PI)+1;
			if(!handycam.isRecording()) tape_dist = 2;
			for(int x = w-8; x >= 4; x--){
				for(int i = 0; i < 6; i++){
					const int shift = ((6-(i+1))/2) * (3 + 8*tape_dist);
					ALLEGRO_COLOR col = al_map_rgb(0, 0, 0);
					if(x-4 > shift) col = al_get_pixel(al_get_target_bitmap(), x-shift, dh+i);
					al_put_pixel(x, dh+i, col);
				}
				
			}
			al_unlock_bitmap(al_get_target_bitmap());
			
			// black border
			if(subject_bitmap){
				al_draw_line(0, 4, w, 4, al_map_rgb(0, 0, 0), 8);
				al_draw_line(w-4, 0, w-4, h, al_map_rgb(0, 0, 0), 8);
				al_draw_line(0, h-2, w, h-2, al_map_rgb(0, 0, 0), 4);
				al_draw_line(2, 0, 2, h, al_map_rgb(0, 0, 0), 4);
			}
			
			// white filter
			al_draw_filled_rectangle(0, 0, w, h, al_map_rgba(20, 20, 20, 20));
			
			if(lens_countdown > 0) al_clear_to_color(al_map_rgb(0, 0, 255));
			
			
			handycam.draw(0, 0, w, h);
			al_flip_display();
		}
	}
	
	al_destroy_bitmap(noise_bitmap);
	if(subject_bitmap) al_destroy_bitmap(subject_bitmap);
	
	
	al_destroy_user_event_source(&file_load_source);
	al_destroy_display(display);
	
	return 0;
}
