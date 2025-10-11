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

#define N_CODE_SETS 10
static const gchar *codeSets7475A[ N_CODE_SETS ][ 127-33 ] = {
        {
// 0     33   34   35   36   37   38   39   40   41   42   43   44   45   46   47   48
         "!", "\"","#", "$", "%", "&", "'", "(", ")", "*", "+", ",", "-", ".", "/", "0",
//       49   50   51   52   53   54   55   56   57   58   59   60   61   62   63   64
         "1", "2", "3", "4", "5", "6", "7", "8", "9", ":", ";", "<", "=", ">", "?", "@",
//       65   66   67   68   69   70   71   72   73   74   75   76   77   78   79   80
         "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P",
//       81   82   83   84   85   86   87   88   89   90   91   92   93   94   95   96
         "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "[", "\\","]", "^", "_", "`",
//       97   98   99   100  101  102  103  104  105  106  107  108  109  110  111  112
         "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p",
//       113  114  115  116  117  118  119  120  121  122  123  124  125  126
         "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "{", "|", "}", "~"
        },
        {
// 1     33   34   35   36   37   38   39   40   41   42   43   44   45   46   47   48
         "!", "\"","#", "$", "%", "&", "'", "(", ")", "*", "+", ",", "-", ".", "/", "0",
//       49   50   51   52   53   54   55   56   57   58   59   60   61   62   63   64
         "1", "2", "3", "4", "5", "6", "7", "8", "9", ":", ";", "<", "=", ">", "?", "@",
//       65   66   67   68   69   70   71   72   73   74   75   76   77   78   79   80
         "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P",
//       81   82   83   84   85   86   87   88   89   90   91   92   93   94   95   96
         "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "[", "√", "]", "↑", "_\b", "`\b",
//       97   98   99   100  101  102  103  104  105  106  107  108  109  110  111  112
         "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p",
//       113  114  115  116  117  118  119  120  121  122  123  124  125  126
         "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "π", "Ⱶ", "→", "~\b"
        },
        {
// 2     33   34   35   36   37   38   39   40   41   42   43   44   45   46   47   48
         "!", "\"","£", "$", "%", "&", "´\b", "(", ")", "*", "+", ",", "-", ".", "/", "0",
//       49   50   51   52   53   54   55   56   57   58   59   60   61   62   63   64
         "1", "2", "3", "4", "5", "6", "7", "8", "9", ":", ";", "<", "=", ">", "?", "@",
//       65   66   67   68   69   70   71   72   73   74   75   76   77   78   79   80
         "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P",
//       81   82   83   84   85   86   87   88   89   90   91   92   93   94   95   96
         "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "[", "ç", "]", "ˆ\b", "_\b", "`\b",
//       97   98   99   100  101  102  103  104  105  106  107  108  109  110  111  112
         "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p",
//       113  114  115  116  117  118  119  120  121  122  123  124  125  126
         "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "¨\b", "˙\b", "˙\b", "'"
        },
        {
// 3     33   34   35   36   37   38   39   40   41   42   43   44   45   46   47   48
         "!", "\"","£", "$", "%", "&", "'", "(", ")", "*", "+", ",", "-", ".", "/", "0",
//       49   50   51   52   53   54   55   56   57   58   59   60   61   62   63   64
         "1", "2", "3", "4", "5", "6", "7", "8", "9", ":", ";", "<", "=", ">", "?", "@",
//       65   66   67   68   69   70   71   72   73   74   75   76   77   78   79   80
         "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P",
//       81   82   83   84   85   86   87   88   89   90   91   92   93   94   95   96
         "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "0", "Æ", "𝟶", "æ", "_\b", "`",
//       97   98   99   100  101  102  103  104  105  106  107  108  109  110  111  112
         "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p",
//       113  114  115  116  117  118  119  120  121  122  123  124  125  126
         "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "\b¨", "\b˙", "\b˙", "\b'"
        },
        {
// 4     33   34   35   36   37   38   39   40   41   42   43   44   45   46   47   48
         "!", "\"","¿", "$", "%", "&", "´\b", "(", ")", "*", "+", ",", "-", ".", "/", "0",
//       49   50   51   52   53   54   55   56   57   58   59   60   61   62   63   64
         "1", "2", "3", "4", "5", "6", "7", "8", "9", ":", ";", "<", "=", ">", "?", "@",
//       65   66   67   68   69   70   71   72   73   74   75   76   77   78   79   80
         "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P",
//       81   82   83   84   85   86   87   88   89   90   91   92   93   94   95   96
         "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "0", "Æ", "𝟶", "ˆ\b", "_\b", "`",
//       97   98   99   100  101  102  103  104  105  106  107  108  109  110  111  112
         "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p",
//       113  114  115  116  117  118  119  120  121  122  123  124  125  126
         "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "\b~", "\b~", "\b~", "\b~"
        },
        // Place-holders .... the following have yet to be properly defined
        // Not Sans does not have APL character code points!
        {
// 5     33   34   35   36   37   38   39   40   41   42   43   44   45   46   47   48
         "!", "\"","#", "$", "%", "&", "'", "(", ")", "*", "+", ",", "-", ".", "/", "0",
//       49   50   51   52   53   54   55   56   57   58   59   60   61   62   63   64
         "1", "2", "3", "4", "5", "6", "7", "8", "9", ":", ";", "<", "=", ">", "?", "@",
//       65   66   67   68   69   70   71   72   73   74   75   76   77   78   79   80
         "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P",
//       81   82   83   84   85   86   87   88   89   90   91   92   93   94   95   96
         "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "[", "\\","]", "^", "_", "`",
//       97   98   99   100  101  102  103  104  105  106  107  108  109  110  111  112
         "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p",
//       113  114  115  116  117  118  119  120  121  122  123  124  125  126
         "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "{", "|", "}", "~"
        },
        {
// 6     33   34   35   36   37   38   39   40   41   42   43   44   45   46   47   48
         "!", "\"","#", "$", "%", "&", "'", "(", ")", "*", "+", ",", "-", ".", "/", "0",
//       49   50   51   52   53   54   55   56   57   58   59   60   61   62   63   64
         "1", "2", "3", "4", "5", "6", "7", "8", "9", ":", ";", "<", "=", ">", "?", "@",
//       65   66   67   68   69   70   71   72   73   74   75   76   77   78   79   80
         "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P",
//       81   82   83   84   85   86   87   88   89   90   91   92   93   94   95   96
         "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "[", "\\","]", "^", "_", "`",
//       97   98   99   100  101  102  103  104  105  106  107  108  109  110  111  112
         "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p",
//       113  114  115  116  117  118  119  120  121  122  123  124  125  126
         "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "{", "|", "}", "~"
        },
        {
// 7     33   34   35   36   37   38   39   40   41   42   43   44   45   46   47   48
         "!", "\"","#", "$", "%", "&", "'", "(", ")", "*", "+", ",", "-", ".", "/", "0",
//       49   50   51   52   53   54   55   56   57   58   59   60   61   62   63   64
         "1", "2", "3", "4", "5", "6", "7", "8", "9", ":", ";", "<", "=", ">", "?", "@",
//       65   66   67   68   69   70   71   72   73   74   75   76   77   78   79   80
         "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P",
//       81   82   83   84   85   86   87   88   89   90   91   92   93   94   95   96
         "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "[", "\\","]", "^", "_", "`",
//       97   98   99   100  101  102  103  104  105  106  107  108  109  110  111  112
         "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p",
//       113  114  115  116  117  118  119  120  121  122  123  124  125  126
         "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "{", "|", "}", "~"
        },
        {
// 8     33   34   35   36   37   38   39   40   41   42   43   44   45   46   47   48
         "!", "\"","#", "$", "%", "&", "'", "(", ")", "*", "+", ",", "-", ".", "/", "0",
//       49   50   51   52   53   54   55   56   57   58   59   60   61   62   63   64
         "1", "2", "3", "4", "5", "6", "7", "8", "9", ":", ";", "<", "=", ">", "?", "@",
//       65   66   67   68   69   70   71   72   73   74   75   76   77   78   79   80
         "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P",
//       81   82   83   84   85   86   87   88   89   90   91   92   93   94   95   96
         "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "[", "\\","]", "^", "_", "`",
//       97   98   99   100  101  102  103  104  105  106  107  108  109  110  111  112
         "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p",
//       113  114  115  116  117  118  119  120  121  122  123  124  125  126
         "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "{", "|", "}", "~"
        },
        {
// 9     33   34   35   36   37   38   39   40   41   42   43   44   45   46   47   48
         "!", "\"","#", "$", "%", "&", "'", "(", ")", "*", "+", ",", "-", ".", "/", "0",
//       49   50   51   52   53   54   55   56   57   58   59   60   61   62   63   64
         "1", "2", "3", "4", "5", "6", "7", "8", "9", ":", ";", "<", "=", ">", "?", "@",
//       65   66   67   68   69   70   71   72   73   74   75   76   77   78   79   80
         "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P",
//       81   82   83   84   85   86   87   88   89   90   91   92   93   94   95   96
         "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "[", "\\","]", "^", "_", "`",
//       97   98   99   100  101  102  103  104  105  106  107  108  109  110  111  112
         "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p",
//       113  114  115  116  117  118  119  120  121  122  123  124  125  126
         "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "{", "|", "}", "~"
        }
};

static  gboolean    bAbsolutePoint = TRUE;

void
initializeHPGL( tGlobal *pGlobal, gboolean bLandscape ) {

	pGlobal->HPGLplotterP1P2[ P1 ].x = HPGL_A3_LL_X;
	pGlobal->HPGLplotterP1P2[ P1 ].y = HPGL_A3_LL_Y;
	if( bLandscape ) {
		pGlobal->HPGLplotterP1P2[ P2 ].x = HPGL_A3_LONG;
		pGlobal->HPGLplotterP1P2[ P2 ].y = HPGL_A3_SHORT;
		pGlobal->aspectRatio = sqrt( 2.0 );
	} else {
		pGlobal->HPGLplotterP1P2[ P2 ].x = HPGL_A3_SHORT;
		pGlobal->HPGLplotterP1P2[ P2 ].y = HPGL_A3_LONG;
		pGlobal->aspectRatio = 1.0 / sqrt( 2.0 );
	}

	bAbsolutePoint = TRUE;
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

void
clearHPGL( tGlobal *pGlobal ) {
	g_free( pGlobal->plotHPGL );
	pGlobal->plotHPGL = 0;
	if ( pGlobal->verbatimHPGLplot )
	    g_string_free( pGlobal->verbatimHPGLplot, TRUE );
	pGlobal->verbatimHPGLplot = NULL;
	gtk_widget_set_sensitive( WLOOKUP( pGlobal, "btn_SaveHPGL" ), FALSE );
}

/*!     \brief  Add points listed as arguments to cammands
 *
 * Commands (PA, PR, UP, DN) may be followed by line points
 * This indicates to move the pen to these points. We capture these movements.
 *
 * \param pGlobal          : pointer to global data
 * \param sXYpoints        : pointer to the command arguments (points)
 * \param pHPGLserialCount : pointer to count of characters in parsed HPGL data
 * \param bAbsolute        : is the line absolute or relative
 * \return number of points added
 */
gint
addLinePoints( tGlobal *pGlobal, gchar *sXYpoints, guint *pHPGLserialCount, gboolean bAbsolute ) {
    gboolean bMorePoints;
    gchar  *pNextChar;
    gint nPoints = 0;
    tCoord p;

    pNextChar = sXYpoints;
    bMorePoints = *pNextChar != 0;

    while ( bMorePoints ) {
        gint64 x = g_ascii_strtoll( pNextChar, &pNextChar, 10 );
        while( g_ascii_isspace(*pNextChar) || *pNextChar == ',' )
            pNextChar++;
        gint64 y = g_ascii_strtoll( pNextChar, &pNextChar, 10 );
        while( g_ascii_isspace(*pNextChar) || *pNextChar == ',' )
            pNextChar++;

        p.x = (gint32)x;
        p.y = (gint32)y;

        append( &pGlobal->plotHPGL, pHPGLserialCount,
                bAbsolute ? CHPGL_MOVE : CHPGL_RMOVE,  &p, sizeof(tCoord)  );

        if( !(g_ascii_isdigit( *pNextChar ) || *pNextChar == '-' || *pNextChar == '.' ))
            bMorePoints = FALSE;

        nPoints++;
    }
    return nPoints;

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
	static gfloat charSizeX = 0.0, charSizeY = 0.0;
	static guint8 colour = 0;
	static guint8 lineType = 0;
	static gint characterSet = 0;
	gboolean bMorePoints;
	gchar  *pNextChar;
	// If a line is started .. we add to it

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
        addLinePoints( pGlobal, sHPGLargs, &HPGLserialCount, bAbsolutePoint = TRUE );
	    break;

	case HPGL_POSN_REL:	// PR
		// We accumulate points in the currentLine malloced array while the pen is down.
		// When the pen is lifted (PU), we add this sequence of points to the compiled plot

        addLinePoints( pGlobal, sHPGLargs, &HPGLserialCount, bAbsolutePoint = FALSE );
        break;

    case HPGL_INITIALIZE: // PR
        bAbsolutePoint = TRUE;
        break;

	case HPGL_DEF_TERMINATOR:
		if( *sHPGLargs == 0 )
			break;
		labelTerminator = *sHPGLargs;
		break;
	case HPGL_LABEL:	// LB
		strLength = strlen( sHPGLargs );
		if( strLength == 0 )
			break;	// don't bother adding null labels ("LB;")
		{
		    GString *unicodeString = g_string_new( NULL );
		    guint16 unicodeSize;
            for( int i=0; i < strLength; i++ ) {
                gchar c = sHPGLargs[i];
                if( c >= 33 && c <= 126 )
                    g_string_append( unicodeString, codeSets7475A[ characterSet ][ (gint)c - 33 ] );
                else
                    g_string_append_c( unicodeString, c );
            }
            unicodeSize = unicodeString->len;
            append( &pGlobal->plotHPGL, &HPGLserialCount, CHPGL_LABEL, NULL, 0  );
            // Length of string
            append( &pGlobal->plotHPGL, &HPGLserialCount, PAYLOAD_ONLY, &unicodeSize, sizeof(guint16)  );
            // The string (and the trailing null)
            append( &pGlobal->plotHPGL, &HPGLserialCount, PAYLOAD_ONLY, unicodeString->str, unicodeString->len+1  );
            g_string_free( unicodeString, TRUE );
		}

		break;

	case HPGL_PEN_UP:	// PU
		append( &pGlobal->plotHPGL, &HPGLserialCount, CHPGL_PEN_UP,  NULL, 0  );
		addLinePoints( pGlobal, sHPGLargs, &HPGLserialCount, bAbsolutePoint );
		break;

	case HPGL_PEN_DOWN:	// PD
		append( &pGlobal->plotHPGL, &HPGLserialCount, CHPGL_PEN_DOWN,  NULL, 0  );
		addLinePoints( pGlobal, sHPGLargs, &HPGLserialCount, bAbsolutePoint );
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

    case HPGL_CHARACTER_SET:
        nargs = sscanf(sHPGLargs, "%d", &characterSet);
        if( nargs <= 0 || characterSet >= N_CODE_SETS )
            characterSet = 0;
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

    case HPGL_USER_CHARACTER:
        // We accumulate points in the currentLine malloced array while the pen is down.
        // When the pen is lifted (PU), we add this sequence of points to the compiled plot
        pNextChar = sHPGLargs;
        bMorePoints = *pNextChar != 0;
        if( bMorePoints ){
            gboolean bPenDown = FALSE;
            guint16     nDummy = 0;
            guint16    *pnPoints = &nDummy;
            tCoordFloat pf;
            // nPoints is a place holder that we will update
            append( &pGlobal->plotHPGL, &HPGLserialCount, CHPGL_UCHAR,  &nDummy, sizeof(guint16)  );
            pnPoints = pGlobal->plotHPGL + HPGLserialCount - sizeof( guint16 );

            while ( bMorePoints ) {
                gdouble x = g_ascii_strtod( pNextChar, &pNextChar );
                while( g_ascii_isspace(*pNextChar) || *pNextChar == ',' )
                    pNextChar++;
                if ( x >= 99.0 ) {
                    bPenDown = TRUE;
                } else if ( x <= -99.0 ){
                    bPenDown = FALSE;
                } else {
                    gdouble y = g_ascii_strtod( pNextChar, &pNextChar );
                    while( g_ascii_isspace(*pNextChar) || *pNextChar == ',' )
                        pNextChar++;

                    pf.x = (float)x + (bPenDown ? UCPENDOWN_INDICATOR : 0.0);    // this will also indicate if the pen is up/down
                    pf.y = (float)y;

                    append( &pGlobal->plotHPGL, &HPGLserialCount, PAYLOAD_ONLY,  &pf, sizeof(tCoordFloat)  );
                    (*pnPoints)++;
                }
                if( !(g_ascii_isdigit( *pNextChar ) || *pNextChar == '-' || *pNextChar == '.' ))
                    bMorePoints = FALSE;
            }
        }
        break;

	case HPGL_OUTPUT_ERROR:
		sendGPIBreply( "0\n", pGlobal );	// nothing to see here
		break;

	case HPGL_OUTPUT_POINTS:
		postInfo("New plot");
		sReply = g_strdup_printf( "%d,%d,%d,%d;\n",
			    				pGlobal->HPGLplotterP1P2[ P1 ].x, pGlobal->HPGLplotterP1P2[ P1 ].y,
								pGlobal->HPGLplotterP1P2[ P2 ].x, pGlobal->HPGLplotterP1P2[ P2 ].y );
		sendGPIBreply( sReply, pGlobal );
#if 0
		// Erase if we had previously seen a pen park (SR;) command
		if( pGlobal->flags.bAutoClear  &&
				pGlobal->flags.bErasePrimed ) {
			g_free( pGlobal->plotHPGL );
			pGlobal->plotHPGL = NULL;
			HPGLserialCount = sizeof( gint );
		}
#endif
		append( &pGlobal->plotHPGL, &HPGLserialCount, CHPGL_OP, &pGlobal->HPGLplotterP1P2[ P1 ], sizeof( tCoord)  );
		append( &pGlobal->plotHPGL, &HPGLserialCount, PAYLOAD_ONLY, &pGlobal->HPGLplotterP1P2[ P2 ], sizeof( tCoord)  );

		g_free( sReply );
		break;

	case HPGL_OUTPUT_STATUS:
		sendGPIBreply( "26;\n", pGlobal );	// all OK
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

	// It may not be able to tell when the start of a new plot begins; therefore,
	//       if no HPGL is received in 250ms, we reset the plot (if the option is set)
	if( g_timer_elapsed( pGlobal->timeSinceLastHPGLcommand, NULL )
	               > pGlobal->HPGLperiodEnd && pGlobal->flags.bAutoClear ) {
		clearHPGL( pGlobal );
	}

	// initialize HPGLcmdArgs if needed
	if( HPGLcmdArgs == 0 )
		HPGLcmdArgs = g_string_new(0);

	if( pGlobal->verbatimHPGLplot == NULL ) {
		pGlobal->verbatimHPGLplot = g_string_new(0);
	    // If we have cleared the plot (either above or explicitly by pressing the button)
	    // We also reset the accumulated command (in case there was some snippet partially accumulated)
        HPGLcmd = 0;
        g_string_truncate( HPGLcmdArgs, 0 );
	}
	g_string_append( pGlobal->verbatimHPGLplot, sHPGLserial );

	g_timer_start( pGlobal->timeSinceLastHPGLcommand );

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

	gtk_widget_set_sensitive( WLOOKUP( pGlobal, "btn_SaveHPGL" ), pGlobal->verbatimHPGLplot->len > 0 );
	return bPenParked;
}
