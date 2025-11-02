/*
 * Copyright (c) 2024 Michael G. Katzmann
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

/*! \file GTKplot.c
 *  \brief Plot trace onto GtkDrawingArea widget
 *
 * HPGL plot data obtained from the GPIB instrument (i.e. network or spectrum analyzer)
 * transformed to cairo objects for display on the GtkDrawingArea.
 * The drawing contexts of the PNG, PDF Cairo objects are substituted for the widget's in
 * order to print or save to image.
 *
 */

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <cairo/cairo.h>
#include <glib-2.0/glib.h>
#include <HPGLplotter.h>
#include "messageEvent.h"
#include <math.h>

void
initializeOptionsDialog( tGlobal *pGlobal ) {
    gtk_spin_button_set_value( WLOOKUP( pGlobal, "spin_ControllerIndex"), pGlobal->GPIBcontrollerIndex );
    gtk_spin_button_set_value( WLOOKUP( pGlobal, "spin_DevicePID"), pGlobal->GPIBdevicePID );
    gtk_spin_button_set_value( WLOOKUP( pGlobal, "spin_EndOfPlotPeriod" ), pGlobal->HPGLperiodEnd );
    gtk_entry_buffer_set_text(
            gtk_entry_get_buffer( WLOOKUP( pGlobal, "entry_ControllerName") ),
            pGlobal->sGPIBcontrollerName, -1 );
    // gboolean bUseGPIBcontrollerName = gtk_check_button_get_active ( WLOOKUP( pGlobal, "cbutton_ControlerNameNotIdx" ) );
    gtk_check_button_set_active ( WLOOKUP( pGlobal, "cbutton_ControlerNameNotIdx" ), !pGlobal->flags.bGPIB_UseControllerIndex );
    gtk_check_button_set_active ( WLOOKUP( pGlobal, "cbutton_NoGPIBSystemCtrlr" ), pGlobal->flags.bDoNotEnableSystemController );
    gtk_check_button_set_active ( WLOOKUP( pGlobal, "cbutton_Listener" ), pGlobal->flags.bGPIB_InitialListener );

    for( gint pen = 1; pen < NUM_HPGL_PENS; pen++ ) {	// Pen 0 is always white
        gchar sWID[] = "1_Color";
        sWID[0] = '0' + pen;
        GtkWidget *wColorButton =  WLOOKUP( pGlobal, sWID );
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        gtk_color_chooser_set_use_alpha( GTK_COLOR_CHOOSER( wColorButton ), TRUE );
        gtk_color_chooser_set_rgba( GTK_COLOR_CHOOSER(wColorButton), &pGlobal->HPGLpens[ pen ] );
#pragma GCC diagnostic pop
    }

    gchar *sOId = g_strdup_printf( "%d_chk_PaperSize", pGlobal->PDFpaperSize+1 );
    gtk_check_button_set_active( WLOOKUP( pGlobal, sOId ), TRUE );
    g_free( sOId );

    gchar *sPID = g_strdup_printf( "GPIB %d", pGlobal->GPIBdevicePID );
    gtk_label_set_label( WLOOKUP( pGlobal, "label_PID" ), sPID );
    g_free( sPID );

}

void
CB_btn_OK ( GtkButton* wBtnOK, gpointer user_data ) {
    tGlobal *pGlobal = (tGlobal *)g_object_get_data(G_OBJECT(wBtnOK), "data");

    gboolean bPreviousGPIB_InitialListener = pGlobal->flags.bGPIB_InitialListener;
    //note: Pen colors and PDF/SVF page sizes are collected when they change


    gdouble GPIBcontrollerIndex = gtk_spin_button_get_value( WLOOKUP( pGlobal, "spin_ControllerIndex") );
    gdouble GPIBplotterID = gtk_spin_button_get_value( WLOOKUP( pGlobal, "spin_DevicePID") );
    const gchar *sGPIBcontrollerName = gtk_entry_buffer_get_text(
            gtk_entry_get_buffer( WLOOKUP( pGlobal, "entry_ControllerName") ) );
    gboolean bUseGPIBcontrollerName = gtk_check_button_get_active ( WLOOKUP( pGlobal, "cbutton_ControlerNameNotIdx" ) );

    pGlobal->flags.bDoNotEnableSystemController = gtk_check_button_get_active ( WLOOKUP( pGlobal, "cbutton_NoGPIBSystemCtrlr" ) );
    pGlobal->flags.bGPIB_InitialListener = gtk_check_button_get_active ( WLOOKUP( pGlobal, "cbutton_Listener" ) );


    gboolean bGPIBchanged = FALSE;

    bGPIBchanged = (pGlobal->GPIBcontrollerIndex != (gint)GPIBcontrollerIndex)
			        || (pGlobal->GPIBdevicePID != (gint)GPIBplotterID)
			        || (g_strcmp0( pGlobal->sGPIBcontrollerName, sGPIBcontrollerName ) != 0)
			        || (pGlobal->flags.bGPIB_UseControllerIndex != !bUseGPIBcontrollerName );

    pGlobal->GPIBcontrollerIndex = (gint)GPIBcontrollerIndex;
    pGlobal->GPIBdevicePID = (gint)GPIBplotterID;
    g_free( pGlobal->sGPIBcontrollerName );
    pGlobal->sGPIBcontrollerName = g_strdup( sGPIBcontrollerName );
    pGlobal->flags.bGPIB_UseControllerIndex = !bUseGPIBcontrollerName;

    gchar *sPID = g_strdup_printf( "GPIB %d", pGlobal->GPIBdevicePID );
    gtk_label_set_label( WLOOKUP( pGlobal, "label_PID" ), sPID );
    g_free( sPID );

    pGlobal->HPGLperiodEnd = gtk_spin_button_get_value( WLOOKUP( pGlobal, "spin_EndOfPlotPeriod" )  );

    if( bGPIBchanged || bPreviousGPIB_InitialListener != pGlobal->flags.bGPIB_InitialListener ) {
        messageEventData *messageData = g_malloc0( sizeof(messageEventData) );
        messageData->command = TG_REINITIALIZE_GPIB;
        g_async_queue_push( pGlobal->messageQueueToGPIB, messageData );
    }

    gtk_widget_set_visible( WLOOKUP ( pGlobal, "dlg_Options" ), FALSE );
}

void
CB_color_Pen( GtkColorButton* wColorBtn, gpointer not_used ){
    tGlobal *pGlobal = (tGlobal *)g_object_get_data(G_OBJECT(wColorBtn), "data");
    gint sequence = (intptr_t)g_object_get_data(G_OBJECT(wColorBtn), "sequence");

    GdkRGBA penColor = {0};
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    gtk_color_chooser_get_rgba ( GTK_COLOR_CHOOSER( wColorBtn ), &penColor );
#pragma GCC diagnostic pop
    if( sequence >= 0 && sequence < NUM_HPGL_PENS-1 )
        pGlobal->HPGLpens[ sequence + 1 ] = penColor;


    gtk_widget_queue_draw ( WLOOKUP ( pGlobal, "drawing_Plot") );
}

void
CB_btn_ColorReset ( GtkButton* wBtnColorReset, gpointer user_data ) {
    gchar sWID[] = "1_Color";

    tGlobal *pGlobal = (tGlobal *)g_object_get_data(G_OBJECT(wBtnColorReset), "data");

    for( gint pen = 1; pen < NUM_HPGL_PENS; pen++ ) {	// Pen 0 is always white
        pGlobal->HPGLpens[ pen ] = HPGLpensFactory[ pen ];
        sWID[0] = '0' + pen;
        GtkWidget *wColorButton =  WLOOKUP( pGlobal, sWID );
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        gtk_color_chooser_set_rgba( GTK_COLOR_CHOOSER(wColorButton), &pGlobal->HPGLpens[ pen ] );
#pragma GCC diagnostic pop
    }
    gtk_widget_queue_draw ( WLOOKUP ( pGlobal, "drawing_Plot") );
}

void
CB_chk_PaperSize ( GtkCheckButton* wBtnPaperSize, gpointer user_data ) {
    tGlobal *pGlobal = (tGlobal *)g_object_get_data(G_OBJECT(wBtnPaperSize), "data");
    gint sequence = (intptr_t)g_object_get_data(G_OBJECT(wBtnPaperSize), "sequence");

    if( gtk_check_button_get_active( wBtnPaperSize ) ) {
        pGlobal->PDFpaperSize = sequence;
    }
}

