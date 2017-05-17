#ifndef HANDYCAM_H
#define HANDYCAM_H

#include <string>
using std::string;

#include <allegro5/allegro.h>

struct ALLEGRO_FONT;

class Handycam
{
	private:
	ALLEGRO_TIMER * seconds_timer;
	
	// INTERFACE
	bool show_interface, show_date, show_time;
	ALLEGRO_FONT * interface_font;
	int hpadding, vpadding, tpadding;
	string usermsg;
	
	// STATES
	int error_code;
	unsigned char battery_level;
	int battery_capacity;
	bool light_on, light_auto;
	bool recording, nightshot, tape;
	
	int timepoint;
	string timestr;
	
	public:
	Handycam();
	
	void draw(int x, int y, int w, int h);
	void drawSystemMessage(string msg, int x, int y, int w, int h);
	
	void record();
	void toggleRecording();
	void stopRecording();
	bool isRecording() const;
	
	void tickCounter();
	void resetCounter();
	
	int horizontalPadding() const;
	int verticalPadding() const;
	
	void setHorizontalPadding(int p);
	void setVerticalPadding(int p);
	
	int timecode() const;
	string & timecodeString();
	void setTimecode(int t);
	
	void setLightOn();
	void setLightAuto();
	void setLightOff();
	void cycleLight();
	
	bool hasNightshot() const;
	void setNightshot(bool n);
	void toggleNightshot();
	
	unsigned char batteryLevel() const;
	void setBatteryLevel(unsigned char l);
	char batteryIcon();
	
	void setMessage(string);
	
	void event(ALLEGRO_EVENT * ev);
	void registerEventSources(ALLEGRO_EVENT_QUEUE * queue);
};

#endif