#ifndef AST3ROIDS_DEBUG_H
#define AST3ROIDS_DEBUG_H

// Debug macros
// TODO: make TODO macros print to submenu
#if defined(DEBUG_RENDER) || defined(DEBUG_INPUT) || defined(DEBUG_LOGIC) || defined(DEBUG_INIT) || defined(DEBUG_COLLISION) || defined(DEBUG_BULLETS)
#define CHECKDEBUGMODE
#define DEBUG_MODE 1
#else
#define CHECKDEBUGMODE
#endif

#if defined(DEBUG_RENDER) || defined(DEBUG_INPUT) || defined(DEBUG_LOGIC) || defined(DEBUG_COLLISION) || defined(DEBUG_BULLETS) && !defined(DEBUG_INIT)
#define PRINTFRAME          
#else
#define PRINTFRAME
#endif

#ifdef DEBUG_INPUT
#define PRINTDINPUT(fmt, ...) 
#else
#define PRINTDINPUT(fmt, ...) 
#endif

#ifdef DEBUG_LOGIC
#define PRINTDLOGIC(fmt, ...) 
#else
#define PRINTDLOGIC(fmt, ...)
#endif

#ifdef DEBUG_RENDER
#define PRINTDRENDER(fmt, ...)
#else
#define PRINTDRENDER(fmt, ...)
#endif

#ifdef DEBUG_INIT
#define PRINTDINIT(fmt, ...) 
#else
#define PRINTDINIT(fmt, ...)
#endif

#ifdef DEBUG_COLLISION
#define PRINTDCOLLISION(fmt, ...)
#else
#define PRINTDCOLLISION(fmt, ...) 
#endif

#ifdef DEBUG_BULLETS
#define PRINTDBULLETS(fmt, ...)
#else
#define PRINTDBULLETS(fmt, ...) 
#endif

#endif
