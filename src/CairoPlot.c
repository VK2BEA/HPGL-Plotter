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
#include <math.h>

GdkRGBA HPGLpensFactory[ NUM_HPGL_PENS ] = {
        { 1.00, 1.00, 1.00, 1.0 }, // 0 - White
		{ 0.00, 0.00, 0.00, 1.0 }, // 1 - Black
		{ 0.75, 0.00, 0.00, 1.0 }, // 2 - Red
		{ 0.00, 0.75, 0.00, 1.0 }, // 3 - Green
		{ 0.00, 0.00, 0.75, 1.0 }, // 4 - Blue
		{ 0.00, 0.75, 0.75, 1.0 }, // 5 - Cyan
		{ 0.75, 0.00, 0.75, 1.0 }, // 6 - Magenta
		{ 0.75, 0.75, 0.00, 1.0 }, // 7 - Yellow
		{ 0.75, 0.75, 0.75, 1.0 }, // 8 - Grey
};

static void
showLabel ( cairo_t *cr, gchar *pLabel) {
	gdouble startX, startY;
	cairo_matrix_t fontMatrix;
	cairo_text_extents_t fontExtents;

	GString *strLabel = g_string_new ( pLabel );

	// replace composite zero stroke with zero
	g_string_replace ( strLabel, "0\b/", "0", 0 );
	g_string_replace ( strLabel, "\x1E", "", 0 );
	g_string_replace ( strLabel, "\x1D", "", 0 );

	cairo_text_extents ( cr, "0", &fontExtents);

	cairo_get_font_matrix ( cr, &fontMatrix );
	gchar **lines = g_strsplit( strLabel->str, "\n", -1 );

	cairo_get_current_point ( cr, &startX, &startY );
	for( gint thisLine = 0; lines[ thisLine ] != NULL; thisLine++ ) {
		cairo_move_to( cr, startX, startY + thisLine * fontMatrix.yy );
		gchar **fragments = g_strsplit( lines[ thisLine ], "\b", -1 );
		for( gint thisFragment = 0; fragments[ thisFragment ] != NULL; thisFragment++ ) {
			if( thisFragment != 0 )
				cairo_rel_move_to( cr, -fontExtents.x_advance, 0 );
			cairo_show_text ( cr, fragments[ thisFragment ] );
		}
		g_strfreev( fragments );
	}
	g_strfreev( lines );
	g_string_free( strLabel, TRUE );
}

static void
setSurfaceRotation( cairo_t *cr, tPlotterState *plotterState, gdouble areaWidth, gdouble areaHeight,
		gdouble *areaWidthModified, gdouble *areaHeightModified) {

	gdouble t;

	cairo_set_matrix (cr, &plotterState->intitalMatrix );

	// flip Y axis
	// first make 0, 0 the bottom left corner and sane direction
	cairo_translate( cr, 0.0, areaHeight);
	cairo_scale( cr, 1.0, -1.0);

	switch( plotterState->HPGLrotation ) {
	default:
		break;
	case 0:
		plotterState->widthTransformed =
				plotterState->HPGLplotterP1P2[ P2 ].x - plotterState->HPGLplotterP1P2[ P1 ].x;
		plotterState->heightTransformed =
				plotterState->HPGLplotterP1P2[ P2 ].y - plotterState->HPGLplotterP1P2[ P1 ].y;
    	break;
	case 90:
		cairo_rotate( cr, M_PI / 2.0 );	// 90 degrees
    	cairo_translate( cr, 0.0, -areaWidth);
    	t = areaWidth;
    	areaWidth  = areaHeight;
    	areaHeight = t;
		plotterState->widthTransformed =
				plotterState->HPGLplotterP1P2[ P2 ].y - plotterState->HPGLplotterP1P2[ P1 ].y;
		plotterState->heightTransformed =
				plotterState->HPGLplotterP1P2[ P2 ].x - plotterState->HPGLplotterP1P2[ P1 ].x;
		break;
	case 180:
		cairo_rotate( cr, M_PI  );	// 180 degrees
    	cairo_translate( cr, -areaHeight, -areaWidth);
		plotterState->widthTransformed =
				plotterState->HPGLplotterP1P2[ P2 ].x - plotterState->HPGLplotterP1P2[ P1 ].x;
		plotterState->heightTransformed =
				plotterState->HPGLplotterP1P2[ P2 ].y - plotterState->HPGLplotterP1P2[ P1 ].y;
		break;
	case 270:
		cairo_rotate( cr, -M_PI / 2.0 );	// 90 degrees
    	cairo_translate( cr, -areaHeight, 0.0 );
    	t = areaWidth;
    	areaWidth  = areaHeight;
    	areaHeight = t;
		plotterState->widthTransformed =
				plotterState->HPGLplotterP1P2[ P2 ].y - plotterState->HPGLplotterP1P2[ P1 ].y;
		plotterState->heightTransformed =
				plotterState->HPGLplotterP1P2[ P2 ].x - plotterState->HPGLplotterP1P2[ P1 ].x;
		break;
	}
	// Add a border
#define MARGIN_FRACTION	0.015
	cairo_translate( cr, areaWidth * MARGIN_FRACTION, areaHeight * MARGIN_FRACTION);
	*areaWidthModified  = areaWidth * (1.0 - 2.0 * MARGIN_FRACTION);
	*areaHeightModified = areaHeight * (1.0 - 2.0 * MARGIN_FRACTION);
	cairo_set_line_width (cr, *areaWidthModified / 1000.0 * 0.75);
}


#define SCALE_ONLY			TRUE
#define SCALEandTRANSLATE	FALSE
static void
translateHPGLpointToCairo( tCoord *HPGLpoint,
					 gdouble cairoWidth, gdouble cairoHeight,
					 gdouble *pCairoX, gdouble *pCairoY,
					 tPlotterState *plotterState, gdouble bScaleOnly ) {
	gdouble fractionX, fractionY;
	gdouble cairoScaleFactorX, cairoScaleFactorY;

	cairoScaleFactorX = cairoWidth  / (gdouble)(plotterState->widthTransformed);
	cairoScaleFactorY = cairoHeight / (gdouble)(plotterState->heightTransformed);

	if( plotterState->flags.bHPGLscaled ) {
		// HPGL coords may be scaled to the input coords (which may be a part of the plotter coords)
		fractionX = (gdouble)(HPGLpoint->x - (bScaleOnly ? 0 : plotterState->HPGLscaledP1P2[ P1 ].x)) /
						(gdouble)(plotterState->HPGLscaledP1P2[ P2 ].x - plotterState->HPGLscaledP1P2[ P1 ].x);
		fractionY = (gdouble)(HPGLpoint->y - (bScaleOnly ? 0 : plotterState->HPGLscaledP1P2[ P1 ].y)) /
				(gdouble)(plotterState->HPGLscaledP1P2[ P2 ].y - plotterState->HPGLscaledP1P2[ P1 ].y);

		// We want to maintain the margin we setup with the plotter units...
		//   ... so do not adjust for plotterState->HPGLplotterP1P2[ P1 ] position
		*pCairoX = ((fractionX * (plotterState->HPGLinputP1P2[ P2 ].x - plotterState->HPGLinputP1P2[ P1 ].x))
				+ plotterState->HPGLinputP1P2[ P1 ].x) * cairoScaleFactorX;
		*pCairoY = ((fractionY * (plotterState->HPGLinputP1P2[ P2 ].y - plotterState->HPGLinputP1P2[ P1 ].y))
				+ plotterState->HPGLinputP1P2[ P1 ].y) * cairoScaleFactorY;
	} else {
		*pCairoX = HPGLpoint->x * cairoScaleFactorX;
		*pCairoY = HPGLpoint->y * cairoScaleFactorY;
	}
}

static void
translateHPGLfontSizeToCairo( gdouble HPGLcharSizeX, gdouble HPGLcharSizeY,
					 gdouble cairoWidth, gdouble cairoHeight,
					 gdouble *pCairoX, gdouble *pCairoY,
					 tPlotterState *plotterState ) {

	gdouble cairoScaleFactorX, cairoScaleFactorY;
	gdouble widthInPlotterUnits, heightInPlotterUnits;

	widthInPlotterUnits  = (gdouble)HPGLcharSizeX *
			(gdouble)(plotterState->HPGLinputP1P2[ P2 ].x - plotterState->HPGLinputP1P2[ P1 ].x) / 100.0;
	heightInPlotterUnits = (gdouble)HPGLcharSizeY *
			(gdouble)(plotterState->HPGLinputP1P2[ P2 ].y - plotterState->HPGLinputP1P2[ P1 ].y) / 100.0;

	cairoScaleFactorX = cairoWidth  / (gdouble)(plotterState->widthTransformed);
	cairoScaleFactorY = cairoHeight / (gdouble)(plotterState->heightTransformed);


#define X_HPGL_TO_CAIRO_EM	2.4 // 2.4
#define Y_HPGL_TO_CAIRO_EM	2.0 // 2.0
	*pCairoX =    widthInPlotterUnits * cairoScaleFactorX * X_HPGL_TO_CAIRO_EM;
	*pCairoY =  -heightInPlotterUnits * cairoScaleFactorY * Y_HPGL_TO_CAIRO_EM;
}


#define EXTRACT( d, p, c, t ) { d = (*(t *)(p + c)); c += sizeof( t ); }
#define EXTRACT_ARRAY( d, p, c, n, t ) { d = (t *)(p + c); c += (n * sizeof( t )); }


/*!     \brief  Display the 8753 screen image
 *
 * If the plot is polar, draw the grid and legends.
 * If there is an overlay of two polar grids of the same scale show both,
 * otherwise actual grid is only drawn once.
 *
 * \ingroup drawing
 *
 * \param cr		pointer to cairo context
 * \param pGrid		pointer to grid parameters
 * \param pGlobal	pointer to global data
 * \return			TRUE
 *
 */
gboolean
plotCompiledHPGL (cairo_t *cr, gdouble imageWidth, gdouble imageHeight, tGlobal *pGlobal)
{
	if( pGlobal->plotHPGL ) {
		guint HPGLserialCount = 0;
		gfloat charSizeX = 1.0, charSizeY = 1.0;
		tPlotterState plotterState = {0};
		guint length = *((guint *)pGlobal->plotHPGL);
		gint HPGLpen = 0;
		gboolean bFirstPoint = FALSE;
		gchar *pLabel;
		guint labelLength;
		gboolean bPenDown = FALSE;
		cairo_matrix_t matrix;
		__attribute__((unused)) gint HPGLlineType = 0;

		gdouble cairoX, cairoY;
		gdouble areaWidth = imageWidth, areaHeight = imageHeight;

		tCoord *pPoint;
		eHPGLscalingType scaleType;

		plotterState.HPGLplotterP1P2[ P1 ] = pGlobal->HPGLplotterP1P2[P1];
		plotterState.HPGLplotterP1P2[ P2 ] = pGlobal->HPGLplotterP1P2[P2];
		plotterState.HPGLinputP1P2[ P1 ] = plotterState.HPGLplotterP1P2[ P1 ];
		plotterState.HPGLinputP1P2[ P2 ] = plotterState.HPGLplotterP1P2[ P2 ];
		// The surface may have been adjusted (i.e. for printing, PDF & SVG
		// to position correctly on a page that is not 1.414:1 aspect ratio
		// We need to save the transformation matrix, so that is can be recovered
		// if we get one or more rotation commands
		cairo_get_matrix (cr, &plotterState.intitalMatrix );

		// There is always a character count at the head of the data
		HPGLserialCount += sizeof(guint);

		// Use a font that is monospaced (like the HP vector plotter)

	    cairo_save(cr);
	    {
	    	setSurfaceRotation( cr, &plotterState, imageWidth, imageHeight,
	    			&areaWidth, &areaHeight );

			// Noto Sans Mono Light
			cairo_select_font_face(cr, HPGL_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
			// The default character size is 0.19 cm wide by 0.27 cm high..
			// Our A4 page is 297 wide x 210 high i.e 156.3 characters along the wide side
			//
			cairo_set_font_size (cr, imageWidth / 85 );

	    	// flip Y axis
	    	cairo_matrix_t font_matrix;
	    	// we need to flip the font back (otherwise it will be upside down)
	    	cairo_get_font_matrix( cr, &font_matrix );
	    	font_matrix.yy = -font_matrix.yy ;
	    	cairo_set_font_matrix( cr, &font_matrix );

	    	// Better center the plot in the screen
			gdouble dot = (gdouble)areaWidth / 300.0;
			gdouble dashes[] = { dot * 5, dot * 2 };

			// If we don't set the color its black ... but the HP8753 does
			gdk_cairo_set_source_rgba (cr, &pGlobal->HPGLpens[1] );      // black pen by default
			cairo_set_line_width( cr, areaWidth/1000.0 );
			cairo_move_to(cr, 0, 0 );

			do {
				// get compiled HPGL command byte
				eHPGL cmd = *((guchar *)(pGlobal->plotHPGL+HPGLserialCount)) ;
				HPGLserialCount += sizeof( eHPGL );

				switch ( cmd ) {
				case CHPGL_PEN_UP:
					bPenDown = FALSE;
					if( bFirstPoint ) {
						// just a dot
						if( cairo_has_current_point( cr ) ) {
							cairo_get_current_point( cr, &cairoX, &cairoY );
						} else {
							cairoX = 0.0;
							cairoY = 0.0;
						}
						cairo_new_path( cr );
	#define DOT_SIZE areaWidth/1250
						cairo_arc( cr, cairoX, cairoY, DOT_SIZE, 0.0, 2.0 * M_PI );
						cairo_fill( cr );
					} else {
						cairo_get_current_point( cr, &cairoX, &cairoY );
					}
					cairo_stroke( cr );
					cairo_move_to( cr, cairoX, cairoY );
					bFirstPoint = FALSE;
					break;

				case CHPGL_PEN_DOWN:
					bPenDown = TRUE;
					bFirstPoint = TRUE;
					break;

				case CHPGL_RMOVE:
					EXTRACT_ARRAY( pPoint, pGlobal->plotHPGL, HPGLserialCount, 1, tCoord );
					translateHPGLpointToCairo( pPoint, areaWidth, areaHeight,
										 &cairoX, &cairoY, &plotterState, SCALE_ONLY );

					if( bPenDown ) {
						bFirstPoint = FALSE;
						if( cairo_has_current_point( cr ) )
							cairo_rel_line_to(cr, cairoX, cairoY );
						else
							cairo_line_to(cr, cairoX, cairoY );
					} else {
						if( cairo_has_current_point( cr ) )
							cairo_rel_move_to(cr, cairoX, cairoY );
						else
							cairo_move_to(cr, cairoX, cairoY );
					}
					break;

				case CHPGL_MOVE:
					EXTRACT_ARRAY( pPoint, pGlobal->plotHPGL, HPGLserialCount, 1, tCoord );
					translateHPGLpointToCairo( pPoint, areaWidth, areaHeight,
										 &cairoX, &cairoY, &plotterState, SCALEandTRANSLATE );
					if( bPenDown ) {
						bFirstPoint = FALSE;
						cairo_line_to(cr, cairoX, cairoY );
					} else {
						cairo_move_to(cr, cairoX, cairoY );
					}
					break;

				case CHPGL_PEN:
					if( bPenDown ) {
						cairo_stroke_preserve( cr );
					}
					EXTRACT( HPGLpen, pGlobal->plotHPGL, HPGLserialCount, guint8 );
					gdk_cairo_set_source_rgba (cr, &pGlobal->HPGLpens[ HPGLpen < NUM_HPGL_PENS ? HPGLpen : 1 ] );
					break;

				case CHPGL_LINETYPE:
					EXTRACT( HPGLlineType, pGlobal->plotHPGL, HPGLserialCount, guint8 );
					switch ( HPGLlineType ) {
					case 0:
					default:
						cairo_set_dash( cr, NULL, 0, 0.0 );
						break;
					case 2:
						cairo_set_dash( cr, dashes, sizeof(dashes)/sizeof(gdouble), 0.0 );
						break;
					case 1:
						cairo_set_dash( cr, &dot, 1, 0.0 );
						break;
					}
					break;

				case CHPGL_LABEL:
					EXTRACT( labelLength, pGlobal->plotHPGL, HPGLserialCount, guint16 );
					// label is null terminated
					EXTRACT_ARRAY( pLabel, pGlobal->plotHPGL, HPGLserialCount, labelLength+1, gchar );
					showLabel (cr, pLabel);
					// display the label
					break;

				case CHPGL_TEXT_SIZE:
				    cairo_matrix_init_identity( &matrix );
				    EXTRACT( charSizeX, pGlobal->plotHPGL, HPGLserialCount, gfloat );
				    EXTRACT( charSizeY, pGlobal->plotHPGL, HPGLserialCount, gfloat );
					translateHPGLfontSizeToCairo( charSizeX, charSizeY, areaWidth, areaHeight,
										 &cairoX, &cairoY, &plotterState );
					matrix.xx = cairoX;
					matrix.yy = cairoY;
					// matrix.y0 = -matrix.yy * 0.15;
					cairo_set_font_matrix (cr, &matrix);
					break;

				case CHPGL_OP:
					EXTRACT( plotterState.HPGLplotterP1P2[ P1 ], pGlobal->plotHPGL, HPGLserialCount, tCoord );
					EXTRACT( plotterState.HPGLplotterP1P2[ P2 ], pGlobal->plotHPGL, HPGLserialCount, tCoord );

					// Remove scaling and rotation
					plotterState.HPGLrotation = 0;
					plotterState.flags.bHPGLscaled = 0;
					plotterState.HPGLinputP1P2[ P1 ] = plotterState.HPGLplotterP1P2[ P1 ];
					plotterState.HPGLinputP1P2[ P2 ] = plotterState.HPGLplotterP1P2[ P2 ];
			    	setSurfaceRotation( cr, &plotterState, imageWidth, imageHeight,
			    			&areaWidth, &areaHeight );
					break;

				case CHPGL_IP:
					EXTRACT( plotterState.HPGLinputP1P2[ P1 ], pGlobal->plotHPGL, HPGLserialCount, tCoord );
					EXTRACT( plotterState.HPGLinputP1P2[ P2 ], pGlobal->plotHPGL, HPGLserialCount, tCoord );
					break;

				case CHPGL_SCALING:
					EXTRACT( scaleType, pGlobal->plotHPGL, HPGLserialCount, eHPGLscalingType );
					if( scaleType == SCALING_NONE ) {
						plotterState.flags.bHPGLscaled = 0;
					} else {
						plotterState.flags.bHPGLscaled = 1;
						plotterState.flags.bbHPGLscaleType = scaleType;
						plotterState.HPGLscaledP1P2[ P1 ] = *(tCoord *)(pGlobal->plotHPGL + HPGLserialCount);
						HPGLserialCount += sizeof( tCoord );
						plotterState.HPGLscaledP1P2[ P2 ] = *(tCoord *)(pGlobal->plotHPGL + HPGLserialCount);
						HPGLserialCount += sizeof( tCoord );
						if( scaleType == SCALING_ISOTROLIC_LB ) {
							plotterState.HPGLscaleIsotropicOffset = *(tCoord *)(pGlobal->plotHPGL + HPGLserialCount);
							HPGLserialCount += sizeof( tCoord );
						}
					}
					break;

				case CHPGL_ROTATION:
					EXTRACT( plotterState.HPGLrotation, pGlobal->plotHPGL, HPGLserialCount, gint );
			    	setSurfaceRotation( cr, &plotterState, imageWidth, imageHeight,
			    			&areaWidth, &areaHeight );
					break;

				default:
					break;
				}
			} while (HPGLserialCount < length);
		} cairo_restore( cr );
	} else {
		drawHPlogo ( cr, imageWidth / 2.0, imageHeight * 0.8, imageWidth / 1000.0 );
	}
	return TRUE;
}

/*!     \brief  Signal received to draw the first drawing area
 *
 * Draw the plot for area A
 *
 * \param widget        pointer to GtkDrawingArea widget
 * \param cr            pointer to cairo structure
 * \param areaWidth     width
 * \param areaHeight    height
 * \param pGlobal       pointer to the global data structure
 */
void
CB_DrawingArea_Draw (GtkDrawingArea *widget, cairo_t *cr,
				gint areaWidth, gint areaHeight, gpointer gpGlobal)
{
	tGlobal *pGlobal = (tGlobal *)gpGlobal;
    // clear the screen
	if( pGlobal->flags.bAutoClear || pGlobal->plotHPGL == NULL) {
		cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 1.0 );
		cairo_paint( cr );
	}

    plotCompiledHPGL ( cr, areaWidth, areaHeight, pGlobal);
}

