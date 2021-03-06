/*
 * Copyright (c) Andre 'Espyo' Silva 2013.
 * The following source file belongs to the open-source project Pikifen.
 * Please read the included README and LICENSE files for more information.
 * Pikmin is copyright (c) Nintendo.
 *
 * === FILE DESCRIPTION ===
 * Header for the menu widgets.
 */

#ifndef MENU_WIDGETS_INCLUDED
#define MENU_WIDGETS_INCLUDED

#include <functional>
#include <string>
#include <vector>

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>

#include "utils/geometry_utils.h"

using namespace std;

class menu_widget {
protected:
    virtual void on_click() = 0;
    
    //Juice.
    float juicy_grow_time_left;
    static const float ICON_SWAY_TIME_SCALE;
    static const float ICON_SWAY_DELTA;
    static const float JUICY_GROW_DURATION;
    static const float JUICY_GROW_DELTA;
    
    
public:
    point center;
    point size;
    function<void()> click_handler;
    bool selected;
    bool enabled;
    
    menu_widget(
        const point &center = point(), const point &size = point(),
        const function<void()> &click_handler = nullptr
    );
    virtual ~menu_widget();
    virtual void tick(const float time);
    virtual void draw(const float time_spent) = 0;
    virtual bool is_clickable() = 0;
    virtual void start_juicy_grow();
    
    void click();
    
    bool mouse_on(const point &mc);
};

class menu_button : public menu_widget {
private:
    virtual void on_click();
    
public:
    string text;
    ALLEGRO_FONT* font;
    ALLEGRO_COLOR text_color;
    int text_align;
    
    menu_button(
        const point &center = point(), const point &size = point(),
        const function<void()> &click_handler = nullptr,
        const string &text = "", ALLEGRO_FONT* font = NULL,
        const ALLEGRO_COLOR &color = al_map_rgb(255, 255, 255),
        const int align = ALLEGRO_ALIGN_CENTER
    );
    ~menu_button();
    virtual void draw(const float time_spent);
    virtual bool is_clickable();
};

class menu_checkbox : public menu_widget {
private:
    virtual void on_click();
    
public:
    string text;
    ALLEGRO_FONT* font;
    ALLEGRO_COLOR text_color;
    bool checked;
    int text_align;
    
    menu_checkbox(
        const point &center = point(), const point &size = point(),
        const function<void()> &click_handler = nullptr,
        const string &text = "", ALLEGRO_FONT* font = NULL,
        const ALLEGRO_COLOR &color = al_map_rgb(255, 255, 255),
        const int align = ALLEGRO_ALIGN_LEFT
    );
    ~menu_checkbox();
    virtual void draw(const float time_spent);
    virtual bool is_clickable();
};

class menu_text : public menu_widget {
private:
    virtual void on_click();
    
public:
    string text;
    ALLEGRO_FONT* font;
    ALLEGRO_COLOR text_color;
    int text_align;
    
    menu_text(
        const point &center = point(), const point &size = point(),
        const string &text = "", ALLEGRO_FONT* font = NULL,
        const ALLEGRO_COLOR &color = al_map_rgb(255, 255, 255),
        const int align = ALLEGRO_ALIGN_CENTER
    );
    ~menu_text();
    virtual void draw(const float time_spent);
    virtual bool is_clickable();
};

#endif //ifndef MENU_WIDGETS_INCLUDED
