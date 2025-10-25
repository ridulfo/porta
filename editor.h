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

void pt_process_key_press(PTState *state);

void pt_splash_screen(PTState *state);

void pt_save_to_file(PTState *state, const pt_str *filename);
void pt_load_from_file(PTState *state, const pt_str *filename);

#endif
