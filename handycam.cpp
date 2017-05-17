#include "handycam.h"
#include "handycam_font.h"

#include <stdio.h>
#include <string>
#include <vector>
#include <algorithm>
#include <math.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_memfile.h>

using std::string;
using std::vector;

Handycam::Handycam() :
	seconds_timer(al_create_timer(1)),
	show_interface(true), show_date(true), show_time(true),
	interface_font(NULL),
	hpadding(0), vpadding(0), tpadding(10),
	usermsg(""),
	
	error_code(0),
	battery_level(4), battery_capacity(90),
	light_on(false), light_auto(false),
	recording(false), nightshot(false), tape(true),
	
	timepoint(0),
	timestr("-0:00:00")
{
	// Load font
	ALLEGRO_FILE * memfont = al_open_memfile(handycam_font_png, handycam_font_png_len, "r");
	if(memfont){
		ALLEGRO_BITMAP * bmpfont = al_load_bitmap_f(memfont, ".png");
		if(bmpfont){
			int ranges[] = {32, 126};
			interface_font = al_grab_font_from_bitmap(bmpfont, 1, ranges);
		}
	}
}

void Handycam::drawSystemMessage(string msg, int x, int y, int w, int h)
{
	int font_height = 32;
	
	int ww = w/2, hh = h/2;
	ALLEGRO_BITMAP * upscale_bitmap = al_create_bitmap(ww, hh);
	ALLEGRO_BITMAP * target_bitmap = al_get_target_bitmap();
	al_set_target_bitmap(upscale_bitmap);
	al_clear_to_color(al_map_rgb(0, 0, 255));
	
	size_t lastnewline(0), newlinepos(0);
	vector<string> lines;
	while((newlinepos = msg.find('\n', lastnewline)) != string::npos){
		lines.push_back(msg.substr(lastnewline, (newlinepos-lastnewline)));
		lastnewline = newlinepos+1;
	}
	if(lastnewline < msg.length()){
		lines.push_back(msg.substr(lastnewline, -1));
	}
	int cur_line = lines.size();
	while(cur_line > 0){
		al_draw_text(interface_font, al_map_rgb(255, 255, 0), ww/2, hh/2+((lines.size()/2.0)*(font_height+16)+font_height/2)-cur_line*(font_height+16),
						ALLEGRO_ALIGN_CENTRE, lines.at(lines.size()-cur_line).c_str());
		cur_line--;
	}
	
	al_set_target_bitmap(target_bitmap);
	al_draw_scaled_bitmap(upscale_bitmap, 0, 0, al_get_bitmap_width(upscale_bitmap), al_get_bitmap_height(upscale_bitmap), 0, 0, w, h, 0);
}

void Handycam::draw(int x, int y, int w, int h)
{
	//draws interface, video, etc
	const ALLEGRO_COLOR white = al_map_rgb(255,255,255);
	
	if(show_interface && interface_font){	
		// print left/right aligned info
		al_draw_textf(interface_font, white, 0+hpadding, vpadding, ALLEGRO_ALIGN_LEFT, 
						"%c%01de", this->batteryIcon(), (int)(battery_capacity * ((double)battery_level/255)));
						
		al_draw_text(interface_font, white, w-hpadding, vpadding, ALLEGRO_ALIGN_RIGHT, this->timecodeString().c_str());
		// print middle
		al_draw_text(interface_font, white, w/2-10, vpadding, ALLEGRO_ALIGN_RIGHT, "mn");
		
		if(recording){
			al_draw_text(interface_font, al_map_rgb(255, 0, 0), w/2+10, vpadding, ALLEGRO_ALIGN_LEFT, " REC");
		} else {
			al_draw_text(interface_font, al_map_rgb(0, 255, 0), w/2+10, vpadding, ALLEGRO_ALIGN_LEFT, "STBY");
		}
		
		if(usermsg.length()){
			al_draw_text(interface_font, al_map_rgb(  0,255,  0), w/2, h/2, ALLEGRO_ALIGN_CENTRE, usermsg.c_str());
		}
		if(nightshot){
			al_draw_textf(interface_font, al_map_rgb(255,255,255), w/2, h/3, ALLEGRO_ALIGN_CENTRE, "~");
			al_draw_textf(interface_font, al_map_rgb(255,255,255), w/2, h/3*2, ALLEGRO_ALIGN_CENTRE, "\"NIGHTSHOT\"");
		}
		if(light_on){
			al_draw_textf(interface_font, al_map_rgb(255,255,255), 0+hpadding+w/6, h-vpadding-32, ALLEGRO_ALIGN_LEFT, "%c", (light_auto ? 'k' : 'l'));
		}
		if((!tape || !battery_level) && (fmod(al_get_time()*0.75, 1) <= 0.5)){
			// low battery priority over no tape
			al_draw_textf(interface_font, al_map_rgb(255,255,  0), w/2-32, h/3+32, ALLEGRO_ALIGN_RIGHT, (!battery_level ? "|" : "}"));
		}
		
		if(show_time || show_date){
			
			char timestr[12] = {0};
			time_t curtime = time(0);
			struct tm * now = localtime(&curtime);

			if(show_time){
				strftime(timestr, sizeof(timestr), "%H:%M:%S", now);
				al_draw_text(interface_font, white, w-hpadding, h-vpadding-16, ALLEGRO_ALIGN_RIGHT, timestr);
			}
			if(show_date){
				strftime(timestr, sizeof(timestr), "%d %m %Y", now);
				al_draw_text(interface_font, white, w-hpadding, h-vpadding-16+(show_time ? -32 : 0)-tpadding, ALLEGRO_ALIGN_RIGHT, timestr);
			}
			if(show_time != show_date){
				al_draw_text(interface_font, white, w-hpadding, h-vpadding-16-32-tpadding, ALLEGRO_ALIGN_RIGHT, "AUTO DATE");
			}
		}
	}
}

void Handycam::record()
{
	if(!tape) return;
	recording = true;
	al_start_timer(seconds_timer);
}
void Handycam::stopRecording()
{
	recording = false;
	al_stop_timer(seconds_timer);
}

void Handycam::toggleRecording()
{
	if(!recording) this->record();
	else this->stopRecording();
}
bool Handycam::isRecording() const
{
	return recording;
}

void Handycam::tickCounter()
{
	timepoint++;
}
void Handycam::resetCounter()
{
	timepoint = 0;
}

int Handycam::horizontalPadding() const
{
	return this->hpadding;
}
int Handycam::verticalPadding() const
{
	return this->vpadding;
}

void Handycam::setHorizontalPadding(int p)
{
	this->hpadding = p;
}
void Handycam::setVerticalPadding(int p)
{
	this->vpadding = p;
}

int Handycam::timecode() const
{
	return this->timepoint;
}

void Handycam::setTimecode(int t)
{
	this->timepoint = t;
}

string & Handycam::timecodeString()
{
	if(error_code) return timestr; //TODO errorcodeString()
	// formats timepoint offset to a string (-0:00:00)
	timestr[0] = (timepoint < 0 ? '-' : ' ');
	timestr[1] = '0' + abs(timepoint / 3600);
	timestr[2] = ':';
	timestr[3] = '0' + abs(timepoint / 60 / 10) % 6;
	timestr[4] = '0' + abs(timepoint / 60) % 10;
	timestr[5] = ':';
	timestr[6] = '0' + abs(timepoint / 10) % 6;
	timestr[7] = '0' + abs(timepoint) % 10;
	return timestr;
}
void Handycam::setLightOn()
{
	light_on = true;
	light_auto = false;
}
void Handycam::setLightAuto()
{
	light_on = light_auto = true;
}
void Handycam::setLightOff()
{
	light_on = light_auto = false;
}

void Handycam::cycleLight()
{
	// Off -> Auto -> On
	if(light_on && !light_auto) this->setLightOff();
	else if(light_on && light_auto) this->setLightOn();
	else if(!light_on && !light_auto) this->setLightAuto();
}

bool Handycam::hasNightshot() const
{
	return this->nightshot;
}
void Handycam::setNightshot(bool n)
{
	this->nightshot = n;
}
void Handycam::toggleNightshot()
{
	this->setNightshot(!this->hasNightshot());
}

unsigned char Handycam::batteryLevel() const
{
	return this->battery_level;
}
void Handycam::setBatteryLevel(unsigned char l)
{
	this->battery_level = l;
}
char Handycam::batteryIcon()
{
	if(!battery_level) return 'f'+4;
	return 'f' + (3-(int)(battery_level/84.0));
}

void Handycam::setMessage(string msg)
{
	usermsg = msg.substr(0, 20);
}

void Handycam::event(ALLEGRO_EVENT * ev)
{
	if(ev->type == ALLEGRO_EVENT_TIMER){
		if(ev->timer.source == seconds_timer){
			if(recording)
				tickCounter();
		}
	}
}

void Handycam::registerEventSources(ALLEGRO_EVENT_QUEUE * queue)
{
	al_register_event_source(queue, al_get_timer_event_source(seconds_timer));
}
