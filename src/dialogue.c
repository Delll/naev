/*
 * See Licensing and Copyright notice in naev.h
 */

/**
 * @file dialogue.c
 *
 * @brief Is a high level api around toolkit.c for easy window creation.
 *
 * Only one dialogue may be open at once or behaviour is unspecified.
 *
 * All these dialogues use what I call the secondary main loop hack.
 *  Basically they spawn another main loop identical to the primary whose only
 *  difference is that it breaks on loop_done.  Therefore this loop hijacks
 *  the main loop until it's over, making these functions seem to be blocking
 *  without really being blocking.
 *
 * @todo Make dialogue system more flexible.
 *
 * @sa toolkit.c
 */


#include "dialogue.h"

#include <stdarg.h>

#include "naev.h"
#include "log.h"
#include "toolkit.h"
#include "pause.h"
#include "opengl.h"
#include "input.h"


/*
 * Prototypes.
 */
/* extern */
extern void main_loop (void); /* from naev.c */
/* dialogues */
static glFont* dialogue_getSize( char* msg, int* w, int* h );
static void dialogue_alertClose( char* str );
static void dialogue_msgClose( char* str );
static void dialogue_YesNoClose( char* str );
static void dialogue_inputClose( char* str );
static void dialogue_inputCancel( char* str );
/* secondary loop hack */
static int loop_done; /**< Used to indicate the secondary loop is finished. */
static int toolkit_loop (void);


/**
 * @fn void dialogue_alert( const char *fmt, ... )
 *
 * @brief Displays an alert popup with only an ok button and a message.
 *
 *    @param fmt Printf style message to display.
 */
void dialogue_alert( const char *fmt, ... )
{
   char msg[512];
   va_list ap;
   unsigned int wdw;
   int h;

   if (window_exists( "Warning" )) return;

   if (fmt == NULL) return;
   else { /* get the message */
      va_start(ap, fmt);
      vsprintf(msg, fmt, ap);
      va_end(ap);
   }

   h = gl_printHeight( &gl_smallFont, 260, msg );

   /* create the window */
   wdw = window_create( "Warning", -1, -1, 300, 90 + h );
   window_addText( wdw, 20, -30, 260, h,  0, "txtAlert",
         &gl_smallFont, &cBlack, msg );
   window_addButton( wdw, 135, 20, 50, 30, "btnOK", "OK",
         dialogue_alertClose );
}
/**
 * @fn static void dialogue_alertClose( char* str )
 * @brief Closes the alert dialogue.
 *    @param str Unused.
 */
static void dialogue_alertClose( char* str )
{
   (void)str;
   if (window_exists( "Warning" ))
      window_destroy( window_get( "Warning" ));
}


/**
 * @fn static glFont* dialogue_getSize( char* msg, int* w, int* h )
 *
 * @brief Gets the size needed for the dialogue.
 * 
 *    @param msg Message of the dialogue.
 *    @param[out] w Gets the width needed.
 *    @param[out] h Gets the height needed.
 */
static glFont* dialogue_getSize( char* msg, int* w, int* h )
{
   glFont* font;

   font = &gl_smallFont; /* try to use smallfont */
   (*h) = gl_printHeight( font, (*w)-40, msg );
   if (strlen(msg) > 100) { /* make font bigger for large texts */
      font = &gl_defFont;
      (*h) = gl_printHeight( font, (*w)-40, msg );
      if ((*h) > 200) (*w) += MIN((*h)-200,600); /* too big, so we make it wider */
      (*h) = gl_printHeight( font, (*w)-40, msg );
   }

   return font;
}



static unsigned int msg_wid = 0; /**< Stores the message window id. */
/**
 * @fn void dialogue_msg( char* caption, const char *fmt, ... )
 *
 * @brief Opens a dialogue window with an ok button and a message.
 *
 *    @param caption Window title.
 *    @param fmt Printf style message to display.
 */
void dialogue_msg( char* caption, const char *fmt, ... )
{
   char msg[4096];
   va_list ap;
   int w,h;
   glFont* font;

   if (msg_wid) return;

   if (fmt == NULL) return;
   else { /* get the message */
      va_start(ap, fmt);
      vsprintf(msg, fmt, ap);
      va_end(ap);
   }

   w = 300; /* default width */
   font =dialogue_getSize( msg, &w, &h );

   /* create the window */
   msg_wid = window_create( caption, -1, -1, w, 110 + h );
   window_addText( msg_wid, 20, -40, w-40, h,  0, "txtMsg",
         font, &cBlack, msg );
   window_addButton( msg_wid, (w-50)/2, 20, 50, 30, "btnOK", "OK",
         dialogue_msgClose );

   toolkit_loop();
}
/**
 * @fn static void dialogue_msgClose( char* str )
 * @brief Closes a message dialogue.
 *    @param str Unused.
 */
static void dialogue_msgClose( char* str )
{
   (void)str;
   window_destroy( msg_wid );
   msg_wid = 0;
   loop_done = 1;
}


static int yesno_result; /**< Stores the yesno dialogue result. */
static unsigned int yesno_wid = 0; /**< Stores the yesno window id. */
/**
 * @fn int dialogue_YesNo( char* caption, const char *fmt, ... )
 *
 * @brief Runs a dialogue with both yes and no options.
 *
 *    @param caption Caption to use for the dialogue.
 *    @param fmt Printf style message.
 *    @return 1 if yes is clicked or 0 if no is clicked.
 */
int dialogue_YesNo( char* caption, const char *fmt, ... )
{
   char msg[4096];
   va_list ap;
   int w,h;
   glFont* font;

   if (yesno_wid) return -1;

   if (fmt == NULL) return -1;
   else { /* get the message */
      va_start(ap, fmt);
      vsprintf(msg, fmt, ap);
      va_end(ap);
   }

   w = 300;
   font = dialogue_getSize( msg, &w, &h );

   /* create window */
   yesno_wid = window_create( caption, -1, -1, w, h+110 );
   /* text */
   window_addText( yesno_wid, 20, -40, w-40, h,  0, "txtYesNo",
         font, &cBlack, msg );
   /* buttons */
   window_addButton( yesno_wid, w/2-50-10, 20, 50, 30, "btnYes", "Yes",
         dialogue_YesNoClose );
   window_addButton( yesno_wid, w/2+10, 20, 50, 30, "btnNo", "No",
         dialogue_YesNoClose );

   /* tricky secondary loop */
   toolkit_loop();

   /* return the result */
   return yesno_result;
}
/**
 * @fn static void dialogue_YesNoClose( char* str )
 * @brief Closes a yesno dialogue.
 *    @param str Unused.
 */
static void dialogue_YesNoClose( char* str )
{
   /* store the result */
   if (strcmp(str,"btnYes")==0) yesno_result = 1;
   else if (strcmp(str,"btnNo")==0) yesno_result = 0;

   /* destroy the window */
   window_destroy( yesno_wid );
   yesno_wid = 0;

   loop_done = 1;
}


static unsigned int input_wid = 0; /**< Stores the input window id. */
static int input_cancelled = 0; /**< Stores whether or not the input was cancelled. */
/**
 * @fn char* dialogue_input( char* title, int min, int max, const char *fmt, ... )
 *
 * @brief Creates a dialogue that allows the player to write a message.
 *
 * You must free the result if it's not null.
 *
 *    @param title Title of the dialogue window.
 *    @param min Minimum length of the message (must be non-zero).
 *    @param max Maximum length of the message (must be non-zero).
 *    @param fmt Printf style message to display on the dialogue.
 *    @return The message the player typed or NULL if it was cancelled.
 */
char* dialogue_input( char* title, int min, int max, const char *fmt, ... )
{
   char msg[512], *input;
   va_list ap;
   int h;

   if (input_wid) return NULL;

   if (fmt == NULL) return NULL;
   else { /* get the message */
      va_start(ap, fmt);
      vsprintf(msg, fmt, ap);
      va_end(ap);
   }

   /* Start out not cancelled. */
   input_cancelled = 0;

   /* get text height */
   h = gl_printHeight( &gl_smallFont, 200, msg );

   /* create window */
   input_wid = window_create( title, -1, -1, 240, h+140 );
   window_setAccept( input_wid, dialogue_inputClose );
   window_setCancel( input_wid, dialogue_inputCancel );
   /* text */
   window_addText( input_wid, 30, -30, 200, h,  0, "txtInput",
         &gl_smallFont, &cDConsole, msg );
   /* input */
   window_addInput( input_wid, 20, -50-h, 200, 20, "inpInput", max, 1 );
   /* button */
   window_addButton( input_wid, -20, 20, 80, 30,
         "btnClose", "Done", dialogue_inputClose );

   /* tricky secondary loop */
   input = NULL;
   while (!input_cancelled && (!input ||
         ((int)strlen(input) < min))) { /* must be longer then min */

      if (input) {
         dialogue_alert( "Input must be at least %d characters long!", min );
         free(input);
         input = NULL;
      }

      if (toolkit_loop() != 0) /* error in loop -> quit */
         return NULL;

      /* save the input */
      if (input_cancelled != 0)
         input = NULL;
      else
         input = strdup( window_getInput( input_wid, "inpInput" ) );
   }

   /* cleanup */
   window_destroy( input_wid );
   input_wid = 0;

   /* return the result */
   return input;
}
/**
 * @fn static void dialogue_inputClose( char* str )
 * @brief Closes an input dialogue.
 *    @param str Unused.
 */
static void dialogue_inputClose( char* str )
{
   (void)str;

   /* break the loop */
   loop_done = 1;
}
/**
 * @fn static void dialogue_inputCancel( char* str )
 * @brief Cancels an input dialogue.
 *    @param str Unused.
 */
static void dialogue_inputCancel( char* str )
{
   input_cancelled = 1;
   dialogue_inputClose(str);
}


/**
 * @fn static int toolkit_loop (void)
 *
 * @brief Creates a secondary loop until loop_done is set to 1 or the toolkit closes.
 *
 * Almost identical to the main loop in naev.c.
 *
 *    @return 0 on success.
 */
static int toolkit_loop (void)
{
   SDL_Event event, quit = { .type = SDL_QUIT };

   loop_done = 0;
   while (!loop_done && toolkit) {
      while (SDL_PollEvent(&event)) { /* event loop */
         if (event.type == SDL_QUIT) { /* pass quit event to main engine */
            loop_done = 1;
            SDL_PushEvent(&quit);
            return -1;
         }

         input_handle(&event); /* handles all the events and player keybinds */
      }

      main_loop();
   }

   return 0;
}
