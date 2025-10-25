#ifndef PT_EDITOR_H
#define PT_EDITOR_H
#include "ds.h"
#include <stdbool.h>

#define CTRL_KEY(k) ((k) & 0x1F)

typedef struct {
        unsigned short rows;
        unsigned short cols;
        pt_str *content;
        pt_str *filename;
        bool is_censored;
} PTState;

PTState *pt_new_glob_state(pt_str *filename);
void pt_add_char(PTState *state, char c);
void pt_delete_char(PTState *state);
void pt_render_state(PTState *state);
// pt_str pt_format_string(const pt_str *input);
void pt_process_key_press(PTState *state);
void pt_splash_screen(PTState *state);

void pt_move_cursor(unsigned short row, unsigned short col);

void pt_save_to_file(PTState *state, const pt_str *filename);
void pt_load_from_file(PTState *state, const pt_str *filename);

#endif
