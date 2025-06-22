#pragma once

#ifdef DEBUG
#define printd(...) printf(__VA_ARGS__)
#else
#define printd(...)
#endif