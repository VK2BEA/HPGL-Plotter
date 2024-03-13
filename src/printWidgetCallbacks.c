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


/*!     \brief  Callback to draw plots for printing
 *
 *
 * \param  operation  pointer to GTK print operation structure
 * \param  context	  pointer to GTK print context structure
 * \param  pageNo	  page number
 * \param  pGlobal    pointer to data
 */
#define HEADER_HEIGHT 10
static void
CB_PrintDrawPage (GtkPrintOperation *operation,
           GtkPrintContext   *context,
           gint               pageNo,
		   tGlobal *          pGlobal)
{
	cairo_t *cr;
	gdouble height, width;

	cr     = gtk_print_context_get_cairo_context (context);
	height = gtk_print_context_get_height( context );
	width  = gtk_print_context_get_width (context);

	// Letter and Tabloid size are not in the ratio of our data ( height = width / sqrt( 2 ) )
	// we need to adjust
	if( height / (width / SQU_ROOT_2) > 1.01 ) {// this should leave A4 and A3 untouched
	  cairo_translate( cr, 0.0, (height - width / SQU_ROOT_2) / 2.0  );
	  height = width / SQU_ROOT_2;
	} else if( height / (width / SQU_ROOT_2) < 0.99 ) {
	  cairo_translate( cr, (height - width / SQU_ROOT_2) / 2.0, 0.0  );
	  width = height * SQU_ROOT_2;
	}

	plotCompiledHPGL (cr, width, height, pGlobal);
}

/*!     \brief  Callback when printing commences
 *
 * Increment page number if it is the second plot
 *
 * \param  printOp  pointer to GTK print operation structure
 * \param  context	pointer to GTK print context structure
 * \param  pGlobal  pointer to data
 */
static void
CB_PrintBegin (GtkPrintOperation *printOp,
           GtkPrintContext   *context, tGlobal *pGlobal)
{
	gint pageNos = 1;
	gtk_print_operation_set_n_pages( printOp, pageNos );
}

/*!     \brief  Callback when printing completes
 *
 *
 * \param  printOp  pointer to GTK print operation structure
 * \param  context	pointer to GTK print context structure
 * \param  pGlobal  pointer to data
 */
static void
CB_PrintDone (GtkPrintOperation *printOp,
        GtkPrintContext   *context, tGlobal *pGlobal)
{

}

static void
CB_PrintRequestPageSetup(GtkPrintOperation* printOp,
                            GtkPrintContext* context, gint page_number, GtkPageSetup* setup,
							tGlobal *pGlobal)
{
#if 0
	GtkPageOrientation orientation;
	orientation = gtk_page_setup_get_orientation(setup);

	GtkPaperSize *paperSize = gtk_paper_size_new ( GTK_PAPER_NAME_LETTER );
	gtk_page_setup_set_paper_size( setup, paperSize );
#endif
}

void
CB_btn_Print ( GtkButton* wBtnOptions, gpointer user_data ) {

	GtkPrintOperation *printOp;
	GtkPrintOperationResult res;

	tGlobal *pGlobal = (tGlobal *)g_object_get_data(G_OBJECT(wBtnOptions), "data");

	printOp = gtk_print_operation_new ();

	if (pGlobal->printSettings != NULL)
		  gtk_print_operation_set_print_settings (printOp, pGlobal->printSettings);

	if (pGlobal->pageSetup != NULL)
		  gtk_print_operation_set_default_page_setup (printOp, pGlobal->pageSetup);


	g_signal_connect(printOp, "begin_print", G_CALLBACK (CB_PrintBegin), pGlobal);
	g_signal_connect(printOp, "draw_page", G_CALLBACK (CB_PrintDrawPage), pGlobal);
	g_signal_connect(printOp, "request-page-setup", G_CALLBACK(CB_PrintRequestPageSetup), pGlobal);
	g_signal_connect(printOp, "done", G_CALLBACK(CB_PrintDone), pGlobal);

	gtk_print_operation_set_embed_page_setup ( printOp, TRUE );
	gtk_print_operation_set_use_full_page ( printOp, FALSE );

	gtk_print_operation_set_n_pages ( printOp, 1 );

	res = gtk_print_operation_run (printOp, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
			  GTK_WINDOW(g_hash_table_lookup ( pGlobal->widgetHashTable, (gconstpointer)"HPGLplotter_main") ), NULL);

	if (res == GTK_PRINT_OPERATION_RESULT_APPLY) {
		if (pGlobal->printSettings != NULL)
			g_object_unref (pGlobal->printSettings);
		pGlobal->printSettings = g_object_ref (gtk_print_operation_get_print_settings (printOp));

		if (pGlobal->pageSetup != NULL)
			g_object_unref (pGlobal->pageSetup);
		pGlobal->pageSetup = g_object_ref (gtk_print_operation_get_default_page_setup (printOp));
	}

	g_object_unref (printOp);
}
