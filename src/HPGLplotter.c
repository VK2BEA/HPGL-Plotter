/*
 * Copyright (c) 2022 Michael G. Katzmann
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <glib-2.0/glib.h>
#include <gpib/ib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <locale.h>
#include <time.h>
#include <errno.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "messageEvent.h"
#include "HPGLplotter.h"

tGlobal globalData = {0};

static gint     optDebug = 0;
static gboolean bOptQuiet = 0;
static gint     optDeviceID = INVALID;
static gint     optControllerIndex = INVALID;
static gchar    *sOptControllerName = NULL;
static gchar    **argsRemainder = NULL;

static const GOptionEntry optionEntries[] =
{
  { "debug",           'b', 0, G_OPTION_ARG_INT,
          &optDebug, "Print diagnostic messages in journal (0-7)", NULL },
  { "quiet",           'q', 0, G_OPTION_ARG_NONE,
          &bOptQuiet, "No GUI sounds", NULL },
  { "GPIBdeviceID",      'd', 0, G_OPTION_ARG_INT,
          &optDeviceID, "GPIB device ID for HPGL plotter", NULL },
  { "GPIBcontrollerIndex",  'c', 0, G_OPTION_ARG_INT,
          &optControllerIndex, "GPIB controller board index", NULL },
  { "GPIBcontrollerName",  'C', 0, G_OPTION_ARG_STRING,
		          &sOptControllerName, "GPIB controller name (in /etc/gpib.conf)", NULL },
  { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, &argsRemainder, "", NULL },
  { NULL }
};


static gboolean
CB_KeyPressed (GObject             *dataObject,
				  guint                  keyval,
				  guint                  keycode,
				  GdkModifierType        state,
				  GtkEventControllerKey *event_controller)
{

	tGlobal *pGlobal = (tGlobal *)g_object_get_data( dataObject, "globalData");
	GtkWidget *wAspectFrame, *wDrawingArea;

//	if (state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_ALT_MASK))
//      return FALSE;

	switch ( keyval ) {
	case GDK_KEY_F2:
		switch (state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_ALT_MASK | GDK_SUPER_MASK) ) {
		case GDK_SHIFT_MASK:
			break;
		case GDK_CONTROL_MASK:
			break;
		case GDK_ALT_MASK:
			break;
		case GDK_SUPER_MASK:
		  break;
		case 0:
#if 0
			gtk_window_get_default_size( GTK_WINDOW( WLOOKUP( pGlobal, "HPGLplotter_main" )), &width, &height );
			g_print( "Main w: %d, h: %d\n", width, height ); width=0; height=0;
#endif
			break;
		}
		break;
	case GDK_KEY_F11:
		switch (state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_ALT_MASK | GDK_SUPER_MASK) ) {
			case GDK_SHIFT_MASK:
				initializeHPGL( pGlobal, TRUE );
				wAspectFrame = WLOOKUP( pGlobal, "AspectFrame");
				gtk_aspect_frame_set_ratio( GTK_ASPECT_FRAME( wAspectFrame ), sqrt( 2.0 ) );
				wDrawingArea = WLOOKUP( pGlobal, "drawing_Plot");
				gtk_drawing_area_set_content_height ( GTK_DRAWING_AREA( wDrawingArea ), 500 );
				break;
			case GDK_CONTROL_MASK:
				initializeHPGL( pGlobal, FALSE );
				wAspectFrame = WLOOKUP( pGlobal, "AspectFrame");
				gtk_aspect_frame_set_ratio( GTK_ASPECT_FRAME( wAspectFrame ), 1.0/sqrt( 2.0 ) );
				wDrawingArea = WLOOKUP( pGlobal, "drawing_Plot");
				gtk_drawing_area_set_content_height ( GTK_DRAWING_AREA( wDrawingArea ), 1000 );
				break;
			case GDK_ALT_MASK:
				break;
			case GDK_SUPER_MASK:
				break;
			case 0:
				break;
		}
		break;
	case GDK_KEY_F10:
		switch (state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_ALT_MASK | GDK_SUPER_MASK) ) {
			case GDK_SHIFT_MASK:
				break;
			case GDK_CONTROL_MASK:
				gtk_widget_set_visible( WLOOKUP ( pGlobal, "dlg_Debug" ), TRUE );
				break;
			case GDK_ALT_MASK:
				break;
			case GDK_SUPER_MASK:
				break;
			case 0:
				break;
		}
		break;
	default:
		break;
  }

  return TRUE;
}

static gboolean
CB_KeyReleased (GtkWidget *drawing_area)
{
  return FALSE;
}
/*!     \brief  on_activate (activate signal callback)
 *
 * Activate the main window and add it to the application(show or raise the main window)
 *
 *
 * \ingroup callbacks
 *
 * \param  app      : pointer to this GApplication
 * \param  udata    : unused
 */

static void
on_activate (GApplication *app, gpointer udata)
{
    GtkBuilder      *builder;
    GtkWidget       *wApplicationWindow;

	GSList *runner;
	const gchar *name;
	const gchar __attribute__((unused)) *wname;
	GtkWidget *widget;
	GSList *widgetList;
	GtkEventController *event_controller;

	tGlobal *pGlobal= (tGlobal *)udata;

    if ( pGlobal->flags.bRunning ) {
        // gtk_window_set_screen( GTK_WINDOW( MainWindow ),
        //                       unique_message_data_get_screen( message ) );
    	gtk_widget_set_visible(GTK_WIDGET(gtk_widget_get_root(GTK_WIDGET(g_hash_table_lookup ( pGlobal->widgetHashTable,
				(gconstpointer)"WID_HPGLplotter_main")))), TRUE);

        gtk_window_present_with_time( GTK_WINDOW( g_hash_table_lookup ( pGlobal->widgetHashTable,
				(gconstpointer)"WID_HPGLplotter_main")), GDK_CURRENT_TIME /*time_*/ );
        return;
    } else {
    	pGlobal->flags.bRunning = TRUE;
    }

    pGlobal->widgetHashTable = g_hash_table_new( g_str_hash, g_str_equal );

	//  builder = gtk_builder_new();
	//  status = gtk_builder_add_from_file (builder, "hp8505a.glade", NULL);
	// use: 'glib-compile-resources --generate-source HPGLplotter-GTK4.xml' to generate resource.c
    //      'sudo gtk-update-icon-cache /usr/share/icons/hicolor' to update the icon cache after icons copied
    //and   'sudo cp us.heterodyne.hpgl-plotter.gschema.xml /usr/share/glib-2.0/schemas'
    //      'sudo glib-compile-schemas /usr/share/glib-2.0/schemas' for the gsettings schema

	builder = gtk_builder_new ();
	/*
	 * Get the data using
	 * gpointer *pGlobalData = g_object_get_data ( data, "globalData" );
	 */
	GObject *dataObject = g_object_new( G_TYPE_OBJECT, NULL );
	g_object_set_data( dataObject, "globalData", (gpointer)pGlobal );
	gtk_builder_set_current_object (builder, dataObject );
	gtk_builder_add_from_resource ( builder, "/src/HPGLplotter.ui", NULL);
	wApplicationWindow = GTK_WIDGET(gtk_builder_get_object(builder, "WID_HPGLplotter_main"));

	// Get all the widgets in a list
	// Filter for the ones we use (prefixed in glade by WID_)
	// put them in a hash table so we can quickly access them when needed

	widgetList = gtk_builder_get_objects(builder);
 	for (runner = widgetList; runner; runner = g_slist_next(runner))
 	{
 		widget = runner->data;
 		if (GTK_IS_WIDGET(widget))
 		{
 			wname = gtk_widget_get_name(widget);
 			name = gtk_buildable_get_buildable_id(GTK_BUILDABLE(widget));
 			// g_printerr("Widget type is %s and buildable get name is %s\n", wname, name);

#define WIDGET_ID_PREFIX		"WID_"
#define WIDGET_ID_PREXIX_LEN	(sizeof("WID_")-1)

 			if (g_str_has_prefix (name, WIDGET_ID_PREFIX)) {
 				gint sequence = (name[ WIDGET_ID_PREXIX_LEN ] - '1');
 				if( sequence < 0 || sequence > 9 )
 					sequence = INVALID;
 				g_object_set_data(G_OBJECT(widget), "sequence",  (gpointer)(guint64)sequence);
 				g_object_set_data(G_OBJECT(widget), "data",  (gpointer)pGlobal);
 				g_hash_table_insert( pGlobal->widgetHashTable, (gchar *)(name + WIDGET_ID_PREXIX_LEN), widget);
 			}
 		}
 	}
 	g_slist_free(widgetList);

#ifndef DEBUG
	g_timeout_add( 20, (GSourceFunc)splashCreate, pGlobal );
	g_timeout_add( 5000, (GSourceFunc)splashDestroy, pGlobal );
#endif

 	GtkCssProvider * cssProvider = gtk_css_provider_new();
 	gtk_css_provider_load_from_resource(cssProvider, "/src/HPGLplotter.css");
 	gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(cssProvider), GTK_STYLE_PROVIDER_PRIORITY_USER);

	g_object_unref(builder);

	// Define the drawing callback for the GtkDrawingArea
	gtk_drawing_area_set_draw_func ( g_hash_table_lookup ( pGlobal->widgetHashTable, (gconstpointer)"drawing_Plot"),
			CB_DrawingArea_Draw, pGlobal, NULL );

	// Connect signals
/*
	g_signal_connect ( WLOOKUP( pGlobal, "btn_OK" ), "clicked", G_CALLBACK (CB_btn_OK), NULL);
	for( gint pen = 1; pen < NUM_HPGL_PENS; pen++ ) {	// Pen 0 is always white
		gchar sWID[] = "1_Color";
		sWID[0] = '0' + pen;
		GtkWidget *wColorButton =  WLOOKUP( pGlobal, sWID );
		g_signal_connect ( wColorButton, "color-set", G_CALLBACK (CB_color_Pen), (gpointer)(intptr_t)pen);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
		gtk_color_chooser_set_use_alpha( GTK_COLOR_CHOOSER( wColorButton ), TRUE );
		gtk_color_chooser_set_rgba( GTK_COLOR_CHOOSER(wColorButton), &HPGLpens[ pen ] );
#pragma GCC diagnostic pop
	}
	g_signal_connect ( WLOOKUP( pGlobal, "btn_ColorReset" ), "clicked", G_CALLBACK (CB_btn_ColorReset), NULL);
	g_signal_connect_after ( WLOOKUP( pGlobal, "cbutton_ControlerNameNotIdx" ), "toggled", G_CALLBACK (CB_chk_UseControllerName), NULL);


	g_signal_connect ( WLOOKUP( pGlobal, "btn_Options" ), "clicked", G_CALLBACK (CB_btn_Options), NULL);
	g_signal_connect ( WLOOKUP( pGlobal, "btn_Erase" ),   "clicked", G_CALLBACK (CB_btn_Erase),   NULL);

	g_signal_connect ( WLOOKUP( pGlobal, "btn_Print" ),   "clicked", G_CALLBACK (CB_btn_Print), NULL);
	g_signal_connect ( WLOOKUP( pGlobal, "btn_PDF" ),     "clicked", G_CALLBACK (CB_btn_PDF),   NULL);
	g_signal_connect ( WLOOKUP( pGlobal, "btn_PNG" ),     "clicked", G_CALLBACK (CB_btn_PNG),   NULL);
*/

	initializeOptionsDialog( pGlobal );
    gtk_check_button_set_active( WLOOKUP( pGlobal, "chk_AutoErase" ), pGlobal->flags.bAutoClear );

	gtk_window_set_default_icon_name("HPGLplotter");



	// Get callbacks for keypresss and release
	event_controller = gtk_event_controller_key_new ();
	g_signal_connect_object (event_controller, "key-pressed",
	                           G_CALLBACK (CB_KeyPressed), dataObject, G_CONNECT_SWAPPED);
	g_signal_connect_object (event_controller, "key-released",
	                           G_CALLBACK (CB_KeyReleased), dataObject, G_CONNECT_SWAPPED);
	gtk_widget_add_controller (GTK_WIDGET (wApplicationWindow), event_controller);

	gtk_widget_set_visible(wApplicationWindow, TRUE);
	gtk_application_add_window( GTK_APPLICATION(app), GTK_WINDOW(wApplicationWindow) );
	gtk_window_set_icon_name(GTK_WINDOW (wApplicationWindow),"HPGLplotter");

	pGlobal->timeSinceLastHPGLcommand = g_timer_new();
    // Start the GPIB communication thread
    pGlobal->pGThread = g_thread_new( "GPIBthread", threadGPIB, (gpointer)pGlobal );

}

/*!     \brief  on_startup (startup signal callback)
 *
 * Setup application (get configuration and create main window (but do not show it))
 * nb: this occurs before 'activate'
 *
 * \ingroup initialize
 *
 * \param  app      : pointer to this GApplication
 * \param  udata    : unused
 */
static void
on_startup (GApplication *app, gpointer udata)
{
	tGlobal *pGlobal = (tGlobal *)udata;
	gboolean bAbort = FALSE;

    LOG( G_LOG_LEVEL_INFO, "Starting");
	setenv("IB_NO_ERROR", "1", 0);	// no noise
	logVersion();

	pGlobal->GPIBdevicePID       = DEFAULT_GPIB_DEVICE_ID;
	pGlobal->GPIBcontrollerIndex = DEFAULT_GPIB_CONTROLLER_INDEX;
	pGlobal->sGPIBcontrollerName = g_strdup( DEFAULT_GPIB_CONTROLLER_NAME );

	pGlobal->flags.bbDebug = optDebug;
	pGlobal->flags.bGPIB_UseControllerIndex = TRUE;
	pGlobal->flags.bAutoClear = TRUE;

	if( sOptControllerName )  {
		pGlobal->sGPIBcontrollerName = sOptControllerName;
		pGlobal->flags.bGPIB_UseControllerIndex = FALSE;
	}

	if( optControllerIndex != INVALID ) {
		pGlobal->GPIBcontrollerIndex = optControllerIndex;
		pGlobal->sGPIBcontrollerName = NULL;
	}

	if( optDeviceID != INVALID ) {
		pGlobal->GPIBdevicePID = optDeviceID;
	}



    /*! We use a loop source to send data back from the
     *  GPIB threads to indicate status
     */

	pGlobal->messageQueueToMain = g_async_queue_new();
	pGlobal->messageEventSource = g_source_new( &messageEventFunctions, sizeof(GSource) );
	pGlobal->messageQueueToGPIB = g_async_queue_new();

    g_source_attach( globalData.messageEventSource, NULL );

    for( int i=0; i < NUM_HPGL_PENS; i++ ) {
    	pGlobal->HPGLpens[ i ] = HPGLpensFactory[ i ];
    }

    initializeHPGL( pGlobal, TRUE );

    recoverSettings( pGlobal );

	if( bAbort )
		g_application_quit (G_APPLICATION ( app ));

}

/*!     \brief  on_shutdown (shutdown signal callback)
 *
 * Cleanup on shutdown
 *
 * \param  app      : pointer to this GApplication
 * \param  userData : unused
 */
static void
on_shutdown (GApplication *app, gpointer userData)
{
	tGlobal *pGlobal __attribute__((unused)) = (tGlobal *)userData;

	saveSettings( pGlobal );

    // cleanup
    messageEventData *messageData = g_malloc0( sizeof(messageEventData) );
    messageData->command = TG_END;
    g_async_queue_push( pGlobal->messageQueueToGPIB, messageData );

//   saveProgramOptions( pGlobal );


    if( pGlobal->pGThread ) {
        g_thread_join( pGlobal->pGThread );
        g_thread_unref( pGlobal->pGThread );
    }

    // Destroy queue and source
    g_async_queue_unref( pGlobal->messageQueueToMain );
    g_source_destroy( pGlobal->messageEventSource );
    g_source_unref ( pGlobal->messageEventSource );

	g_hash_table_destroy( globalData.widgetHashTable );

	g_timer_destroy ( pGlobal->timeSinceLastHPGLcommand );

    LOG( G_LOG_LEVEL_INFO, "Ending");
}


/*!     \brief  Start of program
 *
 * Start of program
 *
 * \param argc	number of arguments
 * \param argv	pointer to array of arguments
 * \return		success or failure error code
 */
int
main(int argc, char *argv[]) {
    GtkApplication *app;

    GMainLoop __attribute__((unused)) *loop;

    setlocale(LC_ALL, "en_US");
    setenv("IB_NO_ERROR", "1", 0);	// no noise for GPIB library
    g_log_set_writer_func (g_log_writer_journald, NULL, NULL);

    // ensure only one instance of program runs ..
    app = gtk_application_new ("us.heterodyne.HPGLplotter", G_APPLICATION_HANDLES_OPEN);
    g_application_add_main_option_entries (G_APPLICATION ( app ), optionEntries);

    g_signal_connect (app, "activate", G_CALLBACK (on_activate), (gpointer)&globalData);
    g_signal_connect (app, "startup",  G_CALLBACK  (on_startup), (gpointer)&globalData);
    g_signal_connect (app, "shutdown", G_CALLBACK (on_shutdown), (gpointer)&globalData);

    gint __attribute__((unused)) status = g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);

	return EXIT_SUCCESS;
}
