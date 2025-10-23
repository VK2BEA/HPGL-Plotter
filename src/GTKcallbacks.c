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
#include <messageEvent.h>
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
	clearHPGL( pGlobal );
	gtk_widget_queue_draw ( WLOOKUP ( pGlobal, "drawing_Plot") );
}

static gchar *sSuggestedHPGLfilename = NULL;

// Call back when file is selected
static void
CB_HPGLsave( GObject *source_object, GAsyncResult *res, gpointer gpGlobal ) {
	GtkFileDialog *dialog = GTK_FILE_DIALOG (source_object);
	tGlobal *pGlobal = (tGlobal *)gpGlobal;
	FILE *fHPGL = NULL;

	GFile *file;
	GError *err = NULL;
	GtkAlertDialog *alert_dialog;

	if (((file = gtk_file_dialog_save_finish (dialog, res, &err)) != NULL) ) {
		gchar *sChosenFilename = g_file_get_path( file );
		gchar *selectedFileBasename =  g_file_get_basename( file );

		if( (fHPGL = fopen( sChosenFilename, "w" )) != NULL ) {
			fwrite( pGlobal->verbatimHPGLplot->str, pGlobal->verbatimHPGLplot->len, 1, fHPGL);
			fclose( fHPGL );
		} else {
			alert_dialog = gtk_alert_dialog_new ("Cannot open file for writing:\n%s", sChosenFilename);
			gtk_alert_dialog_show (alert_dialog, NULL);
			g_object_unref (alert_dialog);
		}

		// If the user chose a specific filename .. then remember it for the next time
		if( strcmp( selectedFileBasename, sSuggestedHPGLfilename ) ) {
			g_free( pGlobal->sUsersHPGLfilename );
			pGlobal->sUsersHPGLfilename = selectedFileBasename;
		} else {
			g_free( selectedFileBasename );
		}
		g_free( sSuggestedHPGLfilename );

		GFile *dir = g_file_get_parent( file );
		gchar *sChosenDirectory = g_file_get_path( dir );
		g_free( pGlobal->sLastDirectory );
		pGlobal->sLastDirectory = sChosenDirectory;

		g_object_unref( dir );
		g_object_unref( file );
		g_free( sChosenFilename );
	} else {
		/*
		alert_dialog = gtk_alert_dialog_new ("%s", err->message);	// Dismissed by user
		gtk_alert_dialog_show (alert_dialog, GTK_WINDOW (win));
		g_object_unref (alert_dialog);
		g_clear_error (&err);
		*/
   }
}

void
CB_btn_SaveHPGL ( GtkButton* wBtnSaveHPGL, gpointer user_data ) {
	tGlobal *pGlobal = (tGlobal *)g_object_get_data(G_OBJECT( wBtnSaveHPGL ), "data");

	GtkFileDialog *fileDialogSave = gtk_file_dialog_new ();
	GtkWidget *win = gtk_widget_get_ancestor (GTK_WIDGET (wBtnSaveHPGL), GTK_TYPE_WINDOW);
	GDateTime *now = g_date_time_new_now_local ();

	g_autoptr (GListModel) filters = (GListModel *)g_list_store_new (GTK_TYPE_FILE_FILTER);
	g_autoptr (GtkFileFilter) filter = NULL;
	filter = gtk_file_filter_new ();
	gtk_file_filter_add_pattern (filter, "*.[Hh][Pp][Gg][Ll]");
	gtk_file_filter_set_name (filter, "HPGL");

	sSuggestedHPGLfilename = g_date_time_format( now, "HPGL.%d%b%y.%H%M%S.hpgl");

	g_list_store_append ( (GListStore*)filters, filter);

	// All files
	filter = gtk_file_filter_new ();
	gtk_file_filter_add_pattern (filter, "*");
	gtk_file_filter_set_name (filter, "All Files");
	g_list_store_append ( (GListStore*) filters, filter);

	gtk_file_dialog_set_filters (fileDialogSave, G_LIST_MODEL (filters));

	GFile *fPath =  g_file_new_for_path( pGlobal->sLastDirectory );
	gtk_file_dialog_set_initial_folder( fileDialogSave, fPath );
	gtk_file_dialog_set_initial_name( fileDialogSave, pGlobal->sUsersHPGLfilename != NULL ? pGlobal->sUsersHPGLfilename : sSuggestedHPGLfilename );

	gtk_file_dialog_save ( fileDialogSave, GTK_WINDOW (win), NULL, CB_HPGLsave, pGlobal);

	g_object_unref (fileDialogSave);
	g_object_unref( fPath );
	g_date_time_unref( now );

}

// Call back when open/recall file is selected
void
HPGLopenFile( GFile *file, gboolean bCommandLineFile, tGlobal *pGlobal ) {
    FILE *fHPGL = NULL;
    GtkAlertDialog *alert_dialog;

    gchar *sChosenFilename = g_file_get_path( file );

#define TBUF_SIZE   10000
    if( (fHPGL = fopen( sChosenFilename, "r" )) != NULL ) {
        gchar tbuf[ TBUF_SIZE+1 ];
        gint n = 0;

        pGlobal->flags.bMuteGPIBreply = TRUE;

        if( pGlobal->flags.bAutoClear )
            clearHPGL( pGlobal );

        do {
            n = fread( tbuf, sizeof( gchar ), TBUF_SIZE, fHPGL);
            tbuf[ n ] = 0;  // null terminate
            deserializeHPGL( tbuf, pGlobal );
        } while ( n == TBUF_SIZE );
        postMessageToMainLoop(TM_REFRESH_PLOT, NULL);
        pGlobal->flags.bMuteGPIBreply = FALSE;
        fclose( fHPGL );

        GFile *dir = g_file_get_parent( file );
        gchar *sChosenDirectory = g_file_get_path( dir );
        g_free( pGlobal->sLastDirectory );
        pGlobal->sLastDirectory = sChosenDirectory;
        g_object_unref( dir );
    } else if ( !bCommandLineFile ){
        alert_dialog = gtk_alert_dialog_new ("Cannot open file for reading:\n%s", sChosenFilename);
        gtk_alert_dialog_show (alert_dialog, NULL);
        g_object_unref (alert_dialog);
    }

    g_free( sChosenFilename );
}

// Call back when open/recall file is selected
static void
CB_HPGLopen( GObject *source_object, GAsyncResult *res, gpointer gpGlobal ) {
	GtkFileDialog *dialog = GTK_FILE_DIALOG (source_object);
	tGlobal *pGlobal = (tGlobal *)gpGlobal;
	FILE *fHPGL = NULL;

	GFile *file;
	GError *err = NULL;
	GtkAlertDialog *alert_dialog;

	if (((file = gtk_file_dialog_open_finish (dialog, res, &err)) != NULL) ) {
	    HPGLopenFile( file, FALSE, pGlobal );
	} else {
		alert_dialog = gtk_alert_dialog_new ("%s", err->message);
		 // gtk_alert_dialog_show (alert_dialog, GTK_WINDOW (win));
		g_object_unref (alert_dialog);
		g_clear_error (&err);
   }
}

void
CB_btn_RecallHPGL ( GtkButton* wBtnRecallHPGL, gpointer user_data ) {
	tGlobal *pGlobal = (tGlobal *)g_object_get_data(G_OBJECT( wBtnRecallHPGL ), "data");

	GtkFileDialog *fileDialogRecall = gtk_file_dialog_new ();
	GtkWidget *win = gtk_widget_get_ancestor (GTK_WIDGET (wBtnRecallHPGL), GTK_TYPE_WINDOW);
	gchar *sSuggestedFilename = NULL;
	GDateTime *now = g_date_time_new_now_local ();

	g_autoptr (GListModel) filters = (GListModel *)g_list_store_new (GTK_TYPE_FILE_FILTER);
	g_autoptr (GtkFileFilter) filter = NULL;
	filter = gtk_file_filter_new ();
	gtk_file_filter_add_pattern (filter, "*.[Hh][Pp][Gg][Ll]");
	gtk_file_filter_set_name (filter, "HPGL");
	sSuggestedFilename = g_date_time_format( now, "HPGL.%d%b%y.%H%M%S.hpgl");
	g_list_store_append ( (GListStore*)filters, filter);

	// All files
	filter = gtk_file_filter_new ();
	gtk_file_filter_add_pattern (filter, "*");
	gtk_file_filter_set_name (filter, "All Files");
	g_list_store_append ( (GListStore*) filters, filter);

	gtk_file_dialog_set_filters (fileDialogRecall, G_LIST_MODEL (filters));

	GFile *fPath =  g_file_new_for_path( pGlobal->sLastDirectory );
	gtk_file_dialog_set_initial_folder( fileDialogRecall, fPath );

	gtk_file_dialog_open ( fileDialogRecall, GTK_WINDOW (win), NULL, CB_HPGLopen, pGlobal);

	g_object_unref (fileDialogRecall);
	g_object_unref( fPath );
	g_free( sSuggestedFilename );
	g_date_time_unref( now );
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

