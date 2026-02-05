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
#include <gtk/gtk.h>
#include <locale.h>
#include <time.h>
#include <errno.h>

#include "HPGLplotter.h"


static gboolean g_settings_schema_exist (const char * id)
{
    gboolean bReturn = FALSE;
    GSettingsSchema *gss;

    if( (gss = g_settings_schema_source_lookup (
            g_settings_schema_source_get_default(), id, TRUE)) != NULL ) {
        bReturn = TRUE;
        g_settings_schema_unref ( gss );
    }

    return bReturn;
}

/*
 * The schema is defined in us.heterodyne.hpgl-plotter.gschema.xml
 * It is copied to /usr/share/glib-2.0/schemas and then glib-compile-schemas is run to compile.
 * (to test the schema use: glib-compile-schemas --dry-run /path/to/us.heterodyne.hpgl-plotter.gschema.xml
 * $ sudo cp us.heterodyne.hpgl-plotter.gschema.xml /usr/share/glib-2.0/schemas
 * $ sudo glib-compile-schemas /usr/share/glib-2.0/schemas
 */
gint
saveSettings( tGlobal *pGlobal ) {

    GVariant *gvPrintSettings = NULL, *gvPageSetup = NULL, *gvPenColors;
    GVariantBuilder *builder;

    GSettings *gs;

    if( !g_settings_schema_exist( GSETTINGS_SCHEMA ) )
        return FALSE;

    gs = g_settings_new( GSETTINGS_SCHEMA );
    if( pGlobal->printSettings )
        gvPrintSettings = gtk_print_settings_to_gvariant( pGlobal->printSettings );
    if( pGlobal->pageSetup )
        gvPageSetup  = gtk_page_setup_to_gvariant( pGlobal->pageSetup );

    //    g_print( "Print Settings is of type %s\n", g_variant_get_type_string( gvPrintSettings ));
    if( gvPrintSettings )
        g_settings_set_value( gs, "print-settings", gvPrintSettings ); // this consumes the GVariant

    //    g_print( "Print Setup is of type %s\n", g_variant_get_type_string( gvPageSetup ));
    if( gvPageSetup )
        g_settings_set_value( gs, "page-setup", gvPageSetup );	// this consumes the GVariant

    builder = g_variant_builder_new (G_VARIANT_TYPE ("a(dddd)"));
    for (int i = 0; i < NUM_HPGL_PENS; i++)
    {
        g_variant_builder_add (builder, "(dddd)", pGlobal->HPGLpens[i].red,
                pGlobal->HPGLpens[i].green, pGlobal->HPGLpens[i].blue, pGlobal->HPGLpens[i].alpha);
    }
    gvPenColors = g_variant_new ("a(dddd)", builder);
    g_variant_builder_unref (builder);

    g_settings_set_value( gs, "pen-colors", gvPenColors ); // this consumes the GVariant

    g_settings_set_string( gs, "last-directory", pGlobal->sLastDirectory );
    g_settings_set_int( gs, "gpib-controller-index", pGlobal->GPIBcontrollerIndex );
    g_settings_set_int( gs, "gpib-device-pid", pGlobal->GPIBdevicePID );
    g_settings_set_double( gs, "hpgl-end-period", pGlobal->HPGLperiodEnd );
    g_settings_set_string( gs, "gpib-controller-name", pGlobal->sGPIBcontrollerName );
    g_settings_set_boolean( gs, "gpib-use-controller-index", pGlobal->flags.bGPIB_UseControllerIndex);
    g_settings_set_boolean( gs, "gpib-do-not-enable-system", pGlobal->flags.bDoNotEnableSystemController);
    g_settings_set_boolean( gs, "auto-clear", pGlobal->flags.bAutoClear );
    g_settings_set_boolean( gs, "gpib-initial-listener", pGlobal->flags.bGPIB_InitialListener );
    g_settings_set_int( gs, "pdf-paper-size", pGlobal->PDFpaperSize );

    g_settings_set_boolean( gs, "online", pGlobal->flags.bOnline );
    //    g_variant_unref (gvPenColors);

    g_object_unref( gs );

    return TRUE;
}

gint
recoverSettings( tGlobal *pGlobal ) {
    GSettings *gs;
    GVariant *gvPenColors, *gvPrintSettings, *gvPageSetup;;
    GVariantIter iter;
    gdouble RGBAcolor[4] = {0};
    gint	pen = 0;

    if( !g_settings_schema_exist( GSETTINGS_SCHEMA ) )
        return FALSE;

    gs = g_settings_new( GSETTINGS_SCHEMA );

    gvPrintSettings = g_settings_get_value ( gs, "print-settings" );
    // This will have been NULL .. so no need to free any old settings
    pGlobal->printSettings = gtk_print_settings_new_from_gvariant( gvPrintSettings );
    g_variant_unref (gvPrintSettings);
    // ditto
    gvPageSetup     = g_settings_get_value ( gs, "page-setup" );
    pGlobal->pageSetup = gtk_page_setup_new_from_gvariant( gvPageSetup );
    g_variant_unref (gvPageSetup);

    gvPenColors = g_settings_get_value ( gs, "pen-colors" );
    g_variant_iter_init ( &iter, gvPenColors);
    while (g_variant_iter_next (&iter, "(dddd)", &RGBAcolor[0], &RGBAcolor[1], &RGBAcolor[2], &RGBAcolor[3])) {
        pGlobal->HPGLpens[ pen ].red   = RGBAcolor[0];
        pGlobal->HPGLpens[ pen ].green = RGBAcolor[1];
        pGlobal->HPGLpens[ pen ].blue  = RGBAcolor[2];
        pGlobal->HPGLpens[ pen ].alpha = RGBAcolor[3];
        pen++;
    }
    g_variant_unref (gvPenColors);

    pGlobal->GPIBcontrollerIndex  = g_settings_get_int( gs, "gpib-controller-index" );
    pGlobal->GPIBdevicePID = g_settings_get_int( gs, "gpib-device-pid" );
    pGlobal->HPGLperiodEnd = g_settings_get_double( gs, "hpgl-end-period" );
    pGlobal->sGPIBcontrollerName = g_settings_get_string( gs, "gpib-controller-name" );
    pGlobal->sLastDirectory = g_settings_get_string( gs, "last-directory" );
    pGlobal->flags.bGPIB_UseControllerIndex = g_settings_get_boolean( gs, "gpib-use-controller-index" );
    if( optInitializeGPIBasListener == INVALID )
        pGlobal->flags.bGPIB_InitialListener = g_settings_get_boolean( gs, "gpib-initial-listener" );
    pGlobal->flags.bDoNotEnableSystemController = g_settings_get_boolean( gs, "gpib-do-not-enable-system" );
    pGlobal->flags.bAutoClear = g_settings_get_boolean( gs, "auto-clear" );
    pGlobal->PDFpaperSize = g_settings_get_int( gs, "pdf-paper-size" );

    if( bOptOffline == INVALID )
        pGlobal->flags.bOnline = g_settings_get_boolean( gs, "online" );

    g_object_unref( gs );

    return TRUE;
}
