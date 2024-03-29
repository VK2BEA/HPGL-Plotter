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


#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <glib-2.0/glib.h>
#include <HPGLplotter.h>
#include <math.h>

//  WID_entry_ControllerName
//  WID_frm_ControllerID

void
CB_btn_Options ( GtkButton* wBtnOptions, gpointer user_data ) {
	tGlobal *pGlobal = (tGlobal *)g_object_get_data(G_OBJECT(wBtnOptions), "data");

	gtk_window_unminimize( GTK_WINDOW( WLOOKUP ( pGlobal, "dlg_Options") ));
	gtk_widget_set_visible( WLOOKUP ( pGlobal, "dlg_Options" ), TRUE );
}


void
CB_chk_UseControllerName ( GtkCheckButton* wBtnUseControllerName, gpointer user_data ) {
	tGlobal *pGlobal = (tGlobal *)g_object_get_data(G_OBJECT(wBtnUseControllerName), "data");

	gboolean bUseControllerName = gtk_check_button_get_active( wBtnUseControllerName );

	gtk_widget_set_sensitive( WLOOKUP( pGlobal, "spin_ControllerIndex"), !bUseControllerName );
	gtk_widget_set_sensitive( WLOOKUP( pGlobal, "entry_ControllerName"), bUseControllerName );

}

void
CB_chk_AutoErase ( GtkCheckButton* wBtnAutoErase, gpointer user_data ) {
	tGlobal *pGlobal = (tGlobal *)g_object_get_data(G_OBJECT(wBtnAutoErase), "data");

	pGlobal->flags.bAutoClear = gtk_check_button_get_active( wBtnAutoErase );
}

void
CB_btn_Erase ( GtkButton* wBtnErase, gpointer user_data ) {
	tGlobal *pGlobal = (tGlobal *)g_object_get_data(G_OBJECT( wBtnErase ), "data");
	gtk_widget_queue_draw ( WLOOKUP ( pGlobal, "drawing_Plot") );
}


void
CB_btn_DbgPlot ( GtkButton* wBtnDbgPlot, gpointer user_data ) {
	tGlobal *pGlobal = (tGlobal *)g_object_get_data(G_OBJECT(wBtnDbgPlot), "data");
	GtkTextIter iterStart, iterEnd;
	GtkTextView *wTextView = GTK_TEXT_VIEW( WLOOKUP( pGlobal, "txtview_Debug") );
	GtkTextBuffer *wTextBuffer = gtk_text_view_get_buffer(wTextView);
	gtk_text_buffer_get_start_iter( wTextBuffer, &iterStart );
	gtk_text_buffer_get_end_iter( wTextBuffer, &iterEnd );
	gchar *sHPGL = gtk_text_buffer_get_text ( wTextBuffer, &iterStart, &iterEnd, FALSE );

	g_free( pGlobal->plotHPGL );
	pGlobal->plotHPGL = NULL;
	deserializeHPGL( sHPGL, pGlobal );
	gtk_widget_queue_draw ( WLOOKUP ( pGlobal, "drawing_Plot") );

	g_free( sHPGL );
}

void
CB_btn_DbgClose ( GtkButton* wBtnDbgClose, gpointer user_data ) {
	tGlobal *pGlobal = (tGlobal *)g_object_get_data(G_OBJECT(wBtnDbgClose), "data");

	gtk_widget_set_visible( WLOOKUP ( pGlobal, "dlg_Debug" ), FALSE );
}

void
CB_btn_DbgClear ( GtkButton* wBtnDbgClose, gpointer user_data ) {
	tGlobal *pGlobal = (tGlobal *)g_object_get_data(G_OBJECT(wBtnDbgClose), "data");
	GtkTextIter iterStart, iterEnd;
	GtkTextView *wTextView = GTK_TEXT_VIEW( WLOOKUP( pGlobal, "txtview_Debug") );
	GtkTextBuffer *wTextBuffer = gtk_text_view_get_buffer(wTextView);
	gtk_text_buffer_get_start_iter( wTextBuffer, &iterStart );
	gtk_text_buffer_get_end_iter( wTextBuffer, &iterEnd );
	gtk_text_buffer_delete ( wTextBuffer, &iterStart, &iterEnd );
}

