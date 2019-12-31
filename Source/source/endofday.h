#ifndef ENDOFDAY_INCLUDED
#define ENDOFDAY_INCLUDED

#include "game_state.h"

/* ----------------------------------------------------------------------------
 * Standard gameplay state. This is where the action happens.
 */
class endo: public game_state {
private:
	bool bim;
	void do_nightime_drawing();
	void handle_button(
		const size_t button, const float pos, const size_t player
	);
public:
	endo();
	~endo();

	void leave();

	virtual void load();
	virtual void unload();
	virtual void do_drawing();
	virtual void update_transformations();
	virtual void handle_controls(const ALLEGRO_EVENT &ev);
	virtual void do_logic();
};

#endif //ifndef GAMEPLAY_INCLUDED
