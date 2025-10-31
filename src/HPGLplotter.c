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
static gint     bOptDoNotEnableSystemController = 0;
static gint     bOptEnableSystemController = 0;
       gint     optInitializeGPIBasListener = INVALID;
static gint     optDeviceID = INVALID;
static gint     optControllerIndex = INVALID;
static gchar    *sOptControllerName = NULL;
static gchar    **argsRemainder = NULL;

GDBusConnection *conSystemBus = NULL;

#define OPTION_ERROR           g_quark_from_static_string ("Option error")
enum option_errors
{
    OPTION_OK = 0,
    OPTION_LISTENER
};


gboolean
argumentGPIBlistener (
  const gchar* option_name,
  const gchar* value,
  gpointer data,
  GError** error
) {

    if( error )
        *error = (GError *)NULL;

    if( value == NULL || !g_strcmp0( value, "1" ) || !g_strcmp0( value, "true" )) {
        optInitializeGPIBasListener = 1;
    } else if( !g_strcmp0( value, "0" ) || !g_strcmp0( value, "false" ) ) {
        optInitializeGPIBasListener = 0;
    } else {
        g_set_error (error, OPTION_ERROR, OPTION_LISTENER,
                "%s option argument: '%s'; Must be '1', 'true', '0' or 'false' or no value",
                option_name, value);
        return FALSE;
    }

    return TRUE;
}

static const GOptionEntry optionEntries[] =
{
  { "debug",           'b', G_OPTION_FLAG_NONE, G_OPTION_ARG_INT,
          &optDebug, "Print diagnostic messages in journal (0-7)", NULL },
  { "quiet",           'q', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE,
          &bOptQuiet, "No GUI sounds", NULL },
  { "GPIBnoSystemController",  'n', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE,
          &bOptDoNotEnableSystemController, "Do not enable GPIB interface as a system controller", NULL },
  { "GPIBuseSystemController",  'N', G_OPTION_FLAG_NONE, G_OPTION_ARG_NONE,
                  &bOptEnableSystemController, "Enable GPIB interface as a system controller when needed", NULL },
  { "GPIBinitialListener",  'l', G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK,
          argumentGPIBlistener, "Force GPIB interface as a listener ('1', 'true' or no argument) or not ('0' or 'false')", NULL },
  { "GPIBdeviceID",      'd', G_OPTION_FLAG_NONE, G_OPTION_ARG_INT,
          &optDeviceID, "GPIB device ID for HPGL plotter", NULL },
  { "GPIBcontrollerIndex",  'c', G_OPTION_FLAG_NONE, G_OPTION_ARG_INT,
          &optControllerIndex, "GPIB controller board index", NULL },
  { "GPIBcontrollerName",  'C', G_OPTION_FLAG_NONE, G_OPTION_ARG_STRING,
		          &sOptControllerName, "GPIB controller name (in /etc/gpib.conf)", NULL },
  { G_OPTION_REMAINING, 0, G_OPTION_FLAG_NONE, G_OPTION_ARG_STRING_ARRAY, &argsRemainder, "", NULL },
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

	GtkTextBuffer *wTextBuffer;

//	if (state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_ALT_MASK))
//      return FALSE;

	switch ( keyval ) {
	case GDK_KEY_Escape:
		pGlobal->flags.bInitialActiveController = TRUE;
	    messageEventData *messageData = g_malloc0( sizeof(messageEventData) );
	    messageData->command = TG_REINITIALIZE_GPIB;
	    g_async_queue_push( pGlobal->messageQueueToGPIB, messageData );
		break;
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
		gtk_window_set_default_size( GTK_WINDOW( WLOOKUP( pGlobal, "HPGLplotter_main" )), 1500, 1000 );
		gtk_widget_queue_draw ( WLOOKUP ( pGlobal, "HPGLplotter_main") );
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
				gtk_drawing_area_set_content_width  ( GTK_DRAWING_AREA( wDrawingArea ), 707 );
				pGlobal->flags.bPortrait = FALSE;
				break;
			case GDK_CONTROL_MASK:
				initializeHPGL( pGlobal, FALSE );
				wAspectFrame = WLOOKUP( pGlobal, "AspectFrame");
				gtk_aspect_frame_set_ratio( GTK_ASPECT_FRAME( wAspectFrame ), 1.0/sqrt( 2.0 ) );
				wDrawingArea = WLOOKUP( pGlobal, "drawing_Plot");
				gtk_drawing_area_set_content_height ( GTK_DRAWING_AREA( wDrawingArea ), 707 );
				gtk_drawing_area_set_content_width  ( GTK_DRAWING_AREA( wDrawingArea ), 500 );
				pGlobal->flags.bPortrait = TRUE;
				break;
			case GDK_ALT_MASK:
				break;
			case GDK_SUPER_MASK:
				break;
			case 0:
				break;
		}
		break;
	case GDK_KEY_F12:
		switch (state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_ALT_MASK | GDK_SUPER_MASK) ) {
			case GDK_SHIFT_MASK:
				break;
			case GDK_CONTROL_MASK:
				break;
			case GDK_ALT_MASK:
				break;
			case GDK_SUPER_MASK:
				wTextBuffer
						= gtk_text_view_get_buffer(GTK_TEXT_VIEW( WLOOKUP( pGlobal, "txtview_Debug") ));
				if( pGlobal->verbatimHPGLplot )
					gtk_text_buffer_set_text ( wTextBuffer, pGlobal->verbatimHPGLplot->str, pGlobal->verbatimHPGLplot->len );
				gtk_widget_set_visible( WLOOKUP ( pGlobal, "dlg_Debug" ), TRUE );
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



/* It may be necessary to reconfigure the GPIB driver on waking from system suspend
 * (especially if it is a USB GPIB device that requires firmware).
 * This is an example.

	#!/bin/sh
	#
	# /lib/systemd/system-sleep/20-linux-gpib

	PATH=/sbin:/usr/sbin:/bin:/usr/bin

	case "$1" in
		pre)
				#code execution BEFORE sleeping/hibernating/suspending
		;;
		post)
			# Re-trigger udev on suspend wakeup so that firmware can be reloaded
			# AGILENT_BUS_DEV=($(lsusb -d 0957:0718 | sed -nE 's/^Bus ([0-9]+) Device ([0-9]+):.*$/\1 \2/p'))
			udevadm trigger -a idVendor=0957 -a idProduct=0718
			udevadm trigger -a idVendor=3923 -a idProduct=709b
		;;
	esac

	exit 0

*/

/*!     \brief DBUS signal callback
 *
 * \param connection    : connection data
 * \param sender_name   : sender of signal
 * \param object_path   :
 * \param interface_name: interface
 * \param signal_name   : name
 * \param parameters    : signal paramateres
 * \param udata         : unused
 *
 * \ingroup callbacks
 */
static void
on_DBUSresume(GDBusConnection *connection,
        const gchar *sender_name, const gchar *object_path,
        const gchar *interface_name, const gchar *signal_name,
        GVariant *parameters, gpointer udata)
{
	tGlobal *pGlobal= (tGlobal *)udata;
	gboolean bState;

	g_variant_get (parameters, "(b)", &bState);

	if( !bState ) {
		messageEventData *messageData = g_malloc0( sizeof(messageEventData) );
		messageData->command = TG_REINITIALIZE_GPIB;
		g_async_queue_push( pGlobal->messageQueueToGPIB, messageData );
	}
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

    g_free( pGlobal->sUsersHPGLfilename );
    g_free( pGlobal->sUsersPDFImageFilename );
    g_free( pGlobal->sUsersPNGImageFilename );
    g_free( pGlobal->sUsersSVGImageFilename );

    LOG( G_LOG_LEVEL_INFO, "HPGLplotter ending");
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

	tGlobal *pGlobal = (tGlobal *)udata;

    if ( pGlobal->flags.bRunning ) {
        // gtk_window_set_screen( GTK_WINDOW( MainWindow ),
        //                       unique_message_data_get_screen( message ) );
    	gtk_widget_set_visible(GTK_WIDGET(gtk_widget_get_root(GTK_WIDGET(g_hash_table_lookup ( pGlobal->widgetHashTable,
				(gconstpointer)"WID_HPGLplotter_main")))), TRUE);

        gtk_window_present( GTK_WINDOW( g_hash_table_lookup ( pGlobal->widgetHashTable,
				(gconstpointer)"WID_HPGLplotter_main")) );
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

#ifndef DEBUG
        // Splash screen when main window is created
        // This assures that the splash window is anchored to the main window and thus
        // it appears in the middle of the screen in wayland
    g_idle_add((GSourceFunc)splashCreate, pGlobal);
#endif
	pGlobal->timeSinceLastHPGLcommand = g_timer_new();
    // Start the GPIB communication thread
    pGlobal->pGThread = g_thread_new( "GPIBthread", threadGPIB, (gpointer)pGlobal );

    if( conSystemBus ) {
    	g_dbus_connection_signal_subscribe( conSystemBus,
				   "org.freedesktop.login1",
				   "org.freedesktop.login1.Manager",
				   "PrepareForSleep",
				   "/org/freedesktop/login1", NULL,
				   G_DBUS_SIGNAL_FLAGS_NONE, on_DBUSresume, (gpointer) pGlobal, NULL);
    }
    if( argsRemainder ) {
        LOG( G_LOG_LEVEL_INFO, "Open initial HPGL file: %s\n", argsRemainder[0] );
        GFile *file = g_file_new_for_path( argsRemainder[0] );
        HPGLopenFile ( file, TRUE, pGlobal );
        g_object_unref( file );
    }
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

    LOG( G_LOG_LEVEL_INFO, "HPGLplotter starting");
	setenv("IB_NO_ERROR", "1", 0);	// no noise
	logVersion();

    if( bOptDoNotEnableSystemController && bOptEnableSystemController ) {
        LOG( G_LOG_LEVEL_WARNING, "command line options -n and -N are mutually exclusive" );
        g_printerr( "command line options -n and -N are mutually exclusive\n" );
        g_signal_handlers_disconnect_by_func (app, G_CALLBACK (on_activate), (gpointer)&globalData);
        g_signal_handlers_disconnect_by_func (app, G_CALLBACK (on_shutdown), (gpointer)&globalData);
        g_application_quit (G_APPLICATION ( app ));
        return;
    }

	pGlobal->GPIBdevicePID       = DEFAULT_GPIB_DEVICE_ID;
	pGlobal->GPIBcontrollerIndex = DEFAULT_GPIB_CONTROLLER_INDEX;
	pGlobal->sGPIBcontrollerName = g_strdup( DEFAULT_GPIB_CONTROLLER_NAME );

	pGlobal->flags.bbDebug = optDebug;
	pGlobal->flags.bGPIB_UseControllerIndex = TRUE;
	pGlobal->flags.bAutoClear = TRUE;

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

    // The command line switches should override the recovered settings
    if( bOptDoNotEnableSystemController ) {
    	pGlobal->flags.bDoNotEnableSystemController = TRUE;
    }

    // The command line switches should override the recovered settings
    if( bOptEnableSystemController ) {
        pGlobal->flags.bDoNotEnableSystemController = FALSE;
    }

    if( optInitializeGPIBasListener != INVALID ) {
        if( optInitializeGPIBasListener == 0 || optInitializeGPIBasListener == 1 )
            pGlobal->flags.bGPIB_InitialListener = optInitializeGPIBasListener;
        else
            optInitializeGPIBasListener = INVALID;
    }

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

    if( !(conSystemBus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, NULL)) ) {
    	LOG( G_LOG_LEVEL_WARNING, "Cannot get system dbus bus" );
    }

}

/*!     \brief  Filter the log messages
 *
 * Filter the log messages, including those debug messages from libraries (gnome and gtk4)
 *
 * \param log_level  log level
 * \param fields     pointer to message fields
 * \param n_fields   number of fields
 * \param gpGlobal   pointer to global data
 * \return           G_LOG_WRITER_HANDLED or G_LOG_WRITER_UNHANDLED
 */
static GLogWriterOutput
filtered_log_writer_journald (GLogLevelFlags log_level,
                        const GLogField *fields,
                        gsize n_fields,
                        gpointer gpGlobal)
{
    __attribute__((unused)) tGlobal *pGlobal = (tGlobal *)gpGlobal;

    // Messages generated by HPGLplotter have first field as MESSAGE
    // glib / gtk+ messages do not
    if( strncmp( fields[0].key, "MESSAGE", 7 ) )
        return G_LOG_WRITER_UNHANDLED;

    switch (log_level) {
    case G_LOG_LEVEL_ERROR:
    case G_LOG_LEVEL_CRITICAL:
    case G_LOG_LEVEL_WARNING:
    case G_LOG_LEVEL_MESSAGE:
    case G_LOG_LEVEL_INFO:
        return g_log_writer_journald (log_level, fields, n_fields, gpGlobal);
        break;
    case G_LOG_LEVEL_DEBUG:
        return g_log_writer_journald (log_level, fields, n_fields, gpGlobal);
        break;
    default:
        break;
    }

    return G_LOG_WRITER_HANDLED;
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
    g_log_set_writer_func (filtered_log_writer_journald, (gpointer)&globalData, NULL);
    // g_log_set_handler(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, g_log_default_handler, NULL);
    // gtk_set_debug_flags( 0 );

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
