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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <inttypes.h>

#include <glib-2.0/glib.h>
#include <gmodule.h>
#include <errno.h>
#include <HPGLplotter.h>

#include "messageEvent.h"

void
initializeHPGL( tGlobal *pGlobal ) {
	pGlobal->HPGLplotterP1P2[ P1 ].x = HPGL_A3_LL_X;
	pGlobal->HPGLplotterP1P2[ P1 ].y = HPGL_A3_LL_Y;
	pGlobal->HPGLplotterP1P2[ P2 ].x = HPGL_A3_UR_X;
	pGlobal->HPGLplotterP1P2[ P2 ].y = HPGL_A3_UR_Y;
}

static gchar labelTerminator = '\003';

void
append( void **pCompiledHPGL, guint *countOfBytes, eHPGL HPGLfn, void *pObject, size_t size ){

	// We may not need all this space, but ... just in case
	// We quantize so as to minimize the number of reallocs (glib probably does this too)
	*pCompiledHPGL = g_realloc( *pCompiledHPGL,
				QUANTIZE( *countOfBytes + size + sizeof( eHPGL ) + sizeof( tCoord ) + sizeof(gchar), 1000 ) );

	if( HPGLfn != PAYLOAD_ONLY ) {
		*(eHPGL *)(*pCompiledHPGL + *countOfBytes) = HPGLfn;
		*countOfBytes  += sizeof( eHPGL );
	}
	memcpy( *pCompiledHPGL + *countOfBytes, pObject, size );
	*countOfBytes += size;
}

/*!     \brief  Parse an HPGL command
 *
 * Parse an HPGL command and prepare data for plotting.
 * We create a compiled serial data set for (a little more) efficient plotting....
 * (we replot every time the window is enlarged or reduced, so making a little faster is helpful)
 * NNNNNNNN - byte count (total count of bytes for the data)
 * Line         - CHPGL_LINE2PT - identifier byte
 *                NN          - 16 bit count of bytes in line (n)
 *                NN          - 16 bit x1 position
 *                NN          - 16 bit y1 position
 *                x & y repeated for coordinate 2 to n (min of three points)
 * 2 point Line - CHPGL_LINE2PT - identifier byte
 *                NN          - 16 bit x1 position
 *                NN          - 16 bit y1 position
 *                NN          - 16 bit x2 position
 *                NN          - 16 bit y2 position
 * label        - CHPGL_LABEL or CHPGL_LABEL_REL - identifier byte
 *                NN          - 16 bit x position
 *                NN          - 16 bit y position
 *                NN          - 16 bit byte count of string (including trailling null)
 *                SSSSSSS.... - variable length string (including terminating by NULL)
 * lable size   - HPGL_CHAR_SIZE_REL - identifier byte
 *                NNNN        - float character size scaling in x (percentage of P2-P1 x)
 *                NNNN        - float character size scaling in y (percentahge of P2-P1 y)
 * pen select   - HPGL_SELECT_PEN - identifier byte
 *                N           - 8 bits identifying the pen (colour)
 * line type    - HPGL_LINE_TYPE - identifier byte
 *                N           - 8 bits identifying the line type (AFAICT this does not change)
 *
 *
 * \param  sHPGL	pointer to HPGL snippet
 * \return 0 (OK) or -1 (ERROR)
 */
gboolean
parseHPGLcmd( guint16 HPGLcmd, gchar *sHPGLargs, tGlobal *pGlobal ) {
	static gboolean bPenDown = FALSE;
	static tCoord posn = {0};
	static gfloat charSizeX = 0.0, charSizeY = 0.0;
	static guint8 colour = 0;
	static guint8 lineType = 0;
	gboolean bMorePoints;
	gchar  *pNextChar;
	// If a line is started .. we add to it
	static tCoord *currentLine = 0;
	static guint16	nPointsInLine = 0;
	static gboolean	bNewPosition = FALSE;

	gint   nargs, arg1, arg2, arg3, arg4, arg5, arg6, arg7;

	tCoord pointP1, pointP2, pointLeftBottom;
	eHPGLscalingType scalingType;

    static gboolean bPenParked = FALSE;			// this often indicate the end of a plot

	gchar *sReply = 0;

	// number of bytes used in the malloced memory
	guint HPGLserialCount;
	guint16 strLength;


	if( pGlobal->plotHPGL )
		HPGLserialCount = *(guint *)(pGlobal->plotHPGL);
	else
		HPGLserialCount = sizeof( guint );	// byte count at the beginning of malloced string

	switch( HPGLcmd ) {
	case HPGL_POSN_ABS:	// PA
	case HPGL_POSN_REL:	// PR
		// We accumulate points in the currentLine malloced array while the pen is down.
		// When the pen is lifted (PU), we add this sequence of points to the compiled plot
		bMorePoints = TRUE; pNextChar = sHPGLargs;
		while ( bMorePoints ) {
			gint64 x=0, y=0;
			if( HPGLcmd == HPGL_POSN_REL ) {
				x = posn.x;
				y = posn.y;
			}
			x += g_ascii_strtoll( pNextChar, &pNextChar, 10 );
			while( g_ascii_isspace(*pNextChar) || *pNextChar == ',' )
				pNextChar++;
			y += g_ascii_strtoll( pNextChar, &pNextChar, 10 );
			while( g_ascii_isspace(*pNextChar) || *pNextChar == ',' )
				pNextChar++;

			posn.x = (guint16)x;
			posn.y = (guint16)y;

			if( bPenDown ) {
				// make sure we have enough space .. quantized by 100 points
				currentLine = g_realloc_n( currentLine, QUANTIZE(nPointsInLine + 1, 100) + sizeof(guint16), sizeof( tCoord ) );
				currentLine[ nPointsInLine ] = posn;
				nPointsInLine++;
			}

			if( !(g_ascii_isdigit( *pNextChar ) || *pNextChar == '-' || *pNextChar == '.' ))
				bMorePoints = FALSE;
		}
		bNewPosition = TRUE;
		break;

	case HPGL_LABEL:	// LB
		strLength = strlen( sHPGLargs );
		if( strLength == 0 )
			break;	// don't bother adding null labels ("LB;")

		// Compiled HPGL Function, and label position
		append( &pGlobal->plotHPGL, &HPGLserialCount, bNewPosition ? CHPGL_LABEL:CHPGL_LABEL_REL,  &posn, sizeof(tCoord)  );
		// Length of string
		append( &pGlobal->plotHPGL, &HPGLserialCount, PAYLOAD_ONLY, &strLength, sizeof(guint16)  );
		// The string (and the trailing null)
		append( &pGlobal->plotHPGL, &HPGLserialCount, PAYLOAD_ONLY, sHPGLargs, strLength+1  );

		bNewPosition = FALSE;
		break;

	case HPGL_PEN_UP:	// PU
		if( bPenDown ) {
			// End of a line ...
			// this concludes the line..  Add the accumulated line points to the compiled HPGL serial store
			// Insert the compiled HPGL command (either CHPGL_LINE2PT or CHPGL_LINE)
			if( nPointsInLine == 1) {
				append( &pGlobal->plotHPGL, &HPGLserialCount, CHPGL_DOT,  NULL, 0  );
			} else if( nPointsInLine == 2 ) {
				append( &pGlobal->plotHPGL, &HPGLserialCount, CHPGL_LINE2PT,  NULL, 0  );
			} else {
				append( &pGlobal->plotHPGL, &HPGLserialCount, CHPGL_LINE,  &nPointsInLine, sizeof(guint16)  );
			}
			append( &pGlobal->plotHPGL, &HPGLserialCount, PAYLOAD_ONLY,  currentLine, nPointsInLine * sizeof(tCoord) );
		}

		// We have completed the line, so dispose of the malloced memory
		g_free( currentLine );
		nPointsInLine = 0;
		currentLine = NULL;
		// Remember that the pen is up
		bPenDown = FALSE;
		break;

	case HPGL_PEN_DOWN:	// PD
		// assume we are starting a new line and save the start point
		if( !bPenDown ) {
			currentLine = g_realloc_n( currentLine, QUANTIZE(1, 100), sizeof( tCoord ) );
			currentLine[0] = posn;
			nPointsInLine = 1;
			bPenDown = TRUE;
		}
		break;

	case HPGL_CHAR_SIZE_REL:	// SR
		COMMA2SPACE( sHPGLargs );
		sscanf(sHPGLargs, "%f %f", &charSizeX, &charSizeY);
		// add the text size change to the compiled HPGL serialized string
		append( &pGlobal->plotHPGL, &HPGLserialCount, CHPGL_TEXT_SIZE,  &charSizeX, sizeof( gfloat)  );
		append( &pGlobal->plotHPGL, &HPGLserialCount, PAYLOAD_ONLY,     &charSizeY, sizeof( gfloat)  );
		break;

	case HPGL_LINE_TYPE:	// LT
		sscanf(sHPGLargs, "%"SCNu8, &lineType);
		append( &pGlobal->plotHPGL, &HPGLserialCount, CHPGL_LINETYPE,  &lineType, sizeof( guint8 )  );
		break;

	case HPGL_SELECT_PEN:	// SP
		colour = 0;		// if we have "SP;" without arguments, it equals SP0;
		sscanf(sHPGLargs, "%"SCNu8, &colour);
		// bizarrely there is, occasionally, a pen change while 4.0the pen is down..
		// so close the current line (so the old color will be used when it is stroked)
		// and start a new line from the current point
		if( bPenDown ) {
            parseHPGLcmd( HPGL_PEN_UP, NULL, pGlobal );
            HPGLserialCount = *(guint *)(pGlobal->plotHPGL);
            parseHPGLcmd( HPGL_PEN_DOWN, NULL, pGlobal );
		}

		append( &pGlobal->plotHPGL, &HPGLserialCount, CHPGL_PEN,  &colour, sizeof( guint8 )  );

		if( colour == 0 ) {
			bPenParked = TRUE;
			pGlobal->flags.bErasePrimed = TRUE;
			postInfo("");
		} else {
			bPenParked = FALSE;
		}
		break;

	case HPGL_INPUT_POINTS:	// IP
		COMMA2SPACE( sHPGLargs );
	    nargs = sscanf(sHPGLargs, "%d %d %d %d", &arg1, &arg2, &arg3, &arg4);
	    if( nargs == 2 || nargs == 4 ) {
	    	gint xdiff = pGlobal->HPGLplotterP1P2[ P2 ].x - pGlobal->HPGLplotterP1P2[ P1 ].x;
	    	gint ydiff = pGlobal->HPGLplotterP1P2[ P2 ].y - pGlobal->HPGLplotterP1P2[ P1 ].y;
	    	pointP1.x = arg1;
	    	pointP1.y = arg2;
			if( nargs == 2 ) {
		    	pointP2.x = arg1 + xdiff;
		    	pointP2.y = arg2 + ydiff;
			} else {
				pointP2.x = arg3;
				pointP2.y = arg4;
			}
	    } else {
	    	pointP1 = pGlobal->HPGLplotterP1P2[ P1 ];
	    	pointP2 = pGlobal->HPGLplotterP1P2[ P2 ];
	    }
		append( &pGlobal->plotHPGL, &HPGLserialCount, CHPGL_IP,  &pointP1, sizeof( tCoord)  );
		append( &pGlobal->plotHPGL, &HPGLserialCount, PAYLOAD_ONLY, &pointP2, sizeof( tCoord)  );
	    break;

	case HPGL_ROTATION:
	    nargs = sscanf(sHPGLargs, "%d", &arg1);
	    if( nargs <= 0 )
	    	arg1 = 0;
	    append( &pGlobal->plotHPGL, &HPGLserialCount, CHPGL_ROTATION,  &arg1, sizeof( gint )  );
	    break;

	case HPGL_SCALING:		// SC
		COMMA2SPACE( sHPGLargs );
	    nargs = sscanf(sHPGLargs, "%d %d %d %d %d %d %d", &arg1, &arg2, &arg3, &arg4, &arg5, &arg6, &arg7);

		if( nargs >= 4 ) {
			pointP1.x = arg1;
			pointP2.x = arg2;
			pointP1.y = arg3;
			pointP2.y = arg4;
		}

		switch( nargs ) {
		case 0:
		default:
			scalingType = SCALING_NONE;
			append( &pGlobal->plotHPGL, &HPGLserialCount, CHPGL_SCALING, &scalingType, sizeof( eHPGLscalingType )  );
			break;
		case 4:
			scalingType = SCALING_ANISOTROPIC;
			append( &pGlobal->plotHPGL, &HPGLserialCount, CHPGL_SCALING,  &scalingType, sizeof( eHPGLscalingType )  );
			append( &pGlobal->plotHPGL, &HPGLserialCount, PAYLOAD_ONLY,  &pointP1, sizeof( tCoord) );
			append( &pGlobal->plotHPGL, &HPGLserialCount, PAYLOAD_ONLY,  &pointP2, sizeof( tCoord) );
			break;
		case 5:
			scalingType = arg5;
			append( &pGlobal->plotHPGL, &HPGLserialCount, CHPGL_SCALING,  &scalingType, sizeof( eHPGLscalingType )  );
			append( &pGlobal->plotHPGL, &HPGLserialCount, PAYLOAD_ONLY,  &pointP1, sizeof( tCoord) );
			append( &pGlobal->plotHPGL, &HPGLserialCount, PAYLOAD_ONLY,  &pointP2, sizeof( tCoord) );
			break;
		case 7:
			scalingType = SCALING_ISOTROLIC_LB;	// LB is specified only for isotropic
			pointLeftBottom.x   = arg6;
			pointLeftBottom.y   = arg7;
			append( &pGlobal->plotHPGL, &HPGLserialCount, CHPGL_SCALING,  &scalingType, sizeof( eHPGLscalingType )  );
			append( &pGlobal->plotHPGL, &HPGLserialCount, PAYLOAD_ONLY,  &pointP1, sizeof( tCoord) );
			append( &pGlobal->plotHPGL, &HPGLserialCount, PAYLOAD_ONLY,  &pointP2, sizeof( tCoord) );
			append( &pGlobal->plotHPGL, &HPGLserialCount, PAYLOAD_ONLY,  &pointLeftBottom, sizeof( tCoord) );
			break;
		}

	    break;

	case HPGL_OUTPUT_ERROR:
		sendGPIBreply( "0", pGlobal );	// nothing to see here
		break;

	case HPGL_OUTPUT_POINTS:
		postInfo("New plot");
		sReply = g_strdup_printf( "%d,%d,%d,%d;",
			    				pGlobal->HPGLplotterP1P2[ P1 ].x, pGlobal->HPGLplotterP1P2[ P1 ].y,
								pGlobal->HPGLplotterP1P2[ P2 ].x, pGlobal->HPGLplotterP1P2[ P2 ].y );
		sendGPIBreply( sReply, pGlobal );

		// Erase if we had previously seen a pen park (SR;) command
		if( pGlobal->flags.bAutoClear  &&
				pGlobal->flags.bErasePrimed ) {
			g_free( pGlobal->plotHPGL );
			pGlobal->plotHPGL = NULL;
			HPGLserialCount = sizeof( gint );
		}

		append( &pGlobal->plotHPGL, &HPGLserialCount, CHPGL_OP, &pGlobal->HPGLplotterP1P2[ P1 ], sizeof( tCoord)  );
		append( &pGlobal->plotHPGL, &HPGLserialCount, PAYLOAD_ONLY, &pGlobal->HPGLplotterP1P2[ P2 ], sizeof( tCoord)  );

		g_free( sReply );
		break;

	case HPGL_OUTPUT_STATUS:
		sendGPIBreply( "26", pGlobal );	// all OK
		break;

    case HPGL_VELOCITY:		// VS
    case HPGL_INPUT_MASK:	// IM
	case HPGL_DEFAULT:		// DF
	case HPGL_PAGE_FEED:	// PG
	default:
		break;
	}

	// update the count
	if( pGlobal->plotHPGL )
		*(guint *)(pGlobal->plotHPGL) = HPGLserialCount;

	return bPenParked;
}

/*
 * HPGL can have several forms.. eg:
 * PDPU10,20
 * PDlPU10,20;
 * PD PU 10 20;
 *
 * Parse the input data and break into individual commands.
 * The HPGL commands may be terminated with a semicolon or the next command (2 upper case characters)
 */
gboolean
deserializeHPGL( gchar *sHPGLserial, tGlobal *pGlobal ) {
	static guint16  HPGLcmd = 0;
	static GString	*HPGLcmdArgs = 0;
	gboolean bPenParked = FALSE;

	gchar *ptrHPGL = sHPGLserial;
#define FIRSTcmdBYTE	0xFF00
#define SECONDcmdBYTE	0x00FF

	// initialize HPGLcmdArgs if needed
	if( HPGLcmdArgs == 0 )
		HPGLcmdArgs = g_string_new(0);

	while( *ptrHPGL != 0 ) {
		// are wee looking for the command
		if( (HPGLcmd & FIRSTcmdBYTE) == 0 ) {
			// looking for first byte of two byte command
			if( g_ascii_isupper( *ptrHPGL ) ) {
					HPGLcmd = *ptrHPGL << 8;
			}
			ptrHPGL++;
		} else if( (HPGLcmd & SECONDcmdBYTE) == 0 ) {
			if( g_ascii_isupper( *ptrHPGL ) ) {
					HPGLcmd |= *ptrHPGL;
			} else {
				HPGLcmd = 0;	// reset.. we need two character in a row
			}
			ptrHPGL++;
		} else if( HPGLcmd == HPGL_LABEL ) {	// special terminator for label
			if( *ptrHPGL != labelTerminator ) {
				HPGLcmdArgs = g_string_append_c( HPGLcmdArgs, *ptrHPGL );
			} else {
				// digest the HPGL command
				bPenParked = parseHPGLcmd( HPGLcmd, HPGLcmdArgs->str, pGlobal );
				g_string_truncate( HPGLcmdArgs, 0 );
				HPGLcmd = 0;
			}
			ptrHPGL++;
		} else {
			// We have a command and are accumulating argument digits (or separator) until a terminator.
			// the terminator could be the first character of the next command!
			if( g_ascii_isupper( *ptrHPGL ) || *ptrHPGL == ';' ) {
				// this is the terminator .. now process this command
				bPenParked = parseHPGLcmd( HPGLcmd, HPGLcmdArgs->str, pGlobal );
				g_string_truncate( HPGLcmdArgs, 0 );
				// We need to keep the next command byte (if that is what is the terminator)
				if( !g_ascii_isupper( *ptrHPGL ) )
					ptrHPGL++;
				HPGLcmd = 0;	// We will now look for the next command
			} else {
				HPGLcmdArgs = g_string_append_c( HPGLcmdArgs, *ptrHPGL );
				ptrHPGL++;
			}
		}
	}

	return bPenParked;
}
