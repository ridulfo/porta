#define _POSIX_C_SOURCE 200112L
#include "ds.h"

/**
 * Takes a markdown text and return a new heap allocated string with kitty
 * control characters to format it in the kitty terminal.
 */
pt_str *pt_format_string(const pt_str *input);

/** Replaces all the alpha-num characters up to the last one with an asterisk */
void censor_text(pt_str *text);
