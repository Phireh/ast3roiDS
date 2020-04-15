#ifndef AST3ROIDS_DEBUG_H
#define AST3ROIDS_DEBUG_H

// Debug macros
// TODO: make macros print to submenu
#if defined(DEBUG_RENDER) || defined(DEBUG_INPUT) || defined(DEBUG_LOGIC) || defined(DEBUG_INIT) || defined(DEBUG_COLLISION) || defined(DEBUG_BULLETS)
#define CHECKDEBUGMODE _debug_log = fopen("debug.log", "w");
#define DEBUG_MODE 1
#define ENDDEBUGMODE   fclose(_debug_log);
#else
#define CHECKDEBUGMODE
#define ENDDEBUGMODE
#endif

#if defined(DEBUG_RENDER) || defined(DEBUG_INPUT) || defined(DEBUG_LOGIC) || defined(DEBUG_COLLISION) || defined(DEBUG_BULLETS) && !defined(DEBUG_INIT)
#define PRINTFRAME                 if(_writing_log && !(_writing_log = false)) fprintf(_debug_log, "-- FRAME %d --\n", framecount)
#else
#define PRINTFRAME
#endif

#ifdef DEBUG_INPUT
#define PRINTDINPUT(fmt, ...)      if((_writing_log = true)) fprintf(_debug_log, fmt, ##__VA_ARGS__)
#else
#define PRINTDINPUT(fmt, ...) 
#endif

#ifdef DEBUG_LOGIC
#define PRINTDLOGIC(fmt, ...)      if((_writing_log = true)) fprintf(_debug_log, fmt, ##__VA_ARGS__)
#else
#define PRINTDLOGIC(fmt, ...)
#endif

#ifdef DEBUG_RENDER
#define PRINTDRENDER(fmt, ...)     if((_writing_log = true)) fprintf(_debug_log, fmt, ##__VA_ARGS__)
#else
#define PRINTDRENDER(fmt, ...)
#endif

#ifdef DEBUG_INIT
#define PRINTDINIT(fmt, ...)       if((_writing_log = true)) fprintf(_debug_log, fmt, ##__VA_ARGS__)
#else
#define PRINTDINIT(fmt, ...)
#endif

#ifdef DEBUG_COLLISION
#define PRINTDCOLLISION(fmt, ...)  if((_writing_log = true)) fprintf(_debug_log, fmt, ##__VA_ARGS__)
#else
#define PRINTDCOLLISION(fmt, ...) 
#endif

#ifdef DEBUG_BULLETS
#define PRINTDBULLETS(fmt, ...)    if((_writing_log = true)) fprintf(_debug_log, fmt, ##__VA_ARGS__)
#else
#define PRINTDBULLETS(fmt, ...) 
#endif

#endif
