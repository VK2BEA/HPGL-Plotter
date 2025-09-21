
#ifndef VERSION
   #define VERSION "1.28-1"
#endif

#define INVALID	(-1)

#include <glib-2.0/glib.h>
#include <gtk/gtk.h>
#include <cairo/cairo.h>
#include <gpib/ib.h>
#include <math.h>

#define LOG( level, message, ...) \
    g_log_structured (G_LOG_DOMAIN, level, \
		  "SYSLOG_IDENTIFIER", "HPGLplotter", \
		  "CODE_FUNC", __FUNCTION__, \
		  "CODE_LINE", G_STRINGIFY(__LINE__), \
		  "MESSAGE", message, ## __VA_ARGS__)

/*!     \brief Debugging levels
 */
enum _debug
{
        eDEBUG_NONE      = 0,
        eDEBUG_INFO      = 1,
        eDEBUG_MINOR     = 3,
        eDEBUG_EXTENSIVE = 5,
        eDEBUG_MAXIMUM   = 7
};

typedef enum { eA4 = 0, eLetter = 1, eA3 = 2, eTabloid = 3, eNumPaperSizes = 4 } tPaperSize;
typedef struct {
    gint   height, width;
    gdouble margin;
} tPaperDimensions;

extern tPaperDimensions paperDimensions[];


typedef struct {
        Addr4882_t GPIBaddress;
        const gchar *name;
        struct {
                guint bActive   :1;
        } flags;

} GPIBdevice;

typedef struct {
		gint16 x, y;
	} tCoord;

typedef struct {
        gfloat x, y;
    } tCoordFloat;

#define UCPENDOWN_INDICATOR 10000

#define A4882( pad, sad )       ( (pad & 0xFF) | ((sad & 0xFF) << 8) )

extern GPIBdevice devices[];
extern GHashTable *widgetHashTable;
extern gint       optInitializeGPIBasListener;

#define ERROR   (-1)
#define OK      ( 0)
#define CLEAR   ( 0)

gpointer        threadGPIB (gpointer);

#define NUM_HPGL_PENS   9      // white (pen 0) + 8

typedef struct {
	struct {
		gushort bHPGLscaled				 : 1;
		gushort bbHPGLscaleType			 : 2;
		gushort bHPGLscaleLBposition	 : 1;
	} flags;

	tCoord		HPGLplotterP1P2[2];			// The plotter sheet
	gint		widthTransformed, heightTransformed;
	tCoord		HPGLinputP1P2  [2];			// The part of the plotter sheet we are using
	tCoord		HPGLscaledP1P2 [2];			// The user coordinates mapped to the input P1, P2
	tCoord		HPGLscaleIsotropicOffset;	// left bottom position of isotropic area
	gint		HPGLrotation;
    gfloat 		charSizeX, charSizeY;

    cairo_matrix_t intitalMatrix;
} tPlotterState;

typedef struct {

	struct {
		guint32 bRunning         		 : 1;
		guint32 bbDebug					 : 3;
		guint32 bGPIBcommsActive		 : 1;
		guint32 bGPIB_UseControllerIndex : 1;
		guint32 bGPIB_ControllerOpenedWithIndex : 1;
        guint32 bGPIB_InitialListener    : 1;
		guint32 bInitialGPIB_ATN         : 1;
		guint32 bInitialActiveController : 1;
		guint32 bErasePrimed			 : 1;
		guint32 bAutoClear				 : 1;
		guint32 bPortrait				 : 1;
		guint32 bDoNotEnableSystemController : 1;
		guint32 bMuteGPIBreply           : 1;
	} flags;
	gint		PDFpaperSize;
#define P1	0
#define P2	1
	tCoord			HPGLplotterP1P2[2];			// The plotter sheet
	gdouble			aspectRatio;

	GdkRGBA HPGLpens[ NUM_HPGL_PENS ];

	gint	 		GPIBcontrollerIndex;
	gint			GPIBdevicePID;
	gchar 			*sGPIBcontrollerName;
	gint			GPIBcontrollerDevice;		// from ibfind (or copied from controllerIndex) when opened

	gdouble         HPGLperiodEnd;              // period after last HPGL command to assume plot has ended

    GtkPrintSettings *printSettings;
    GtkPageSetup     *pageSetup;
    gchar            *sLastDirectory;

	GHashTable 		*widgetHashTable;

	GSource 		*messageEventSource;
	GAsyncQueue 	*messageQueueToMain;
	GAsyncQueue 	*messageQueueToGPIB;

	void 			*plotHPGL;				// Optimized HPGL - potentially better for redrawing plot on the screen
	GString  		*verbatimHPGLplot;		// The HPGL as received

	gchar			*sUsersHPGLfilename;	// filename chose by user for saving HPGL file
	gchar			*sUsersPDFImageFilename;	// filename chosen by user for PDF file
	gchar			*sUsersPNGImageFilename;	// filename chosen by user for PNG file
	gchar			*sUsersSVGImageFilename;	// filename chosen by user for SVG file
	GTimer   		*timeSinceLastHPGLcommand;
	GThread 		*pGThread;

} tGlobal;

extern GdkRGBA HPGLpensFactory[ NUM_HPGL_PENS ];

#define HPGL_FONT "Noto Sans Mono Light"   // OR "Noto Sans Mono ExtraLight"
#define COLOR_HPGL_DEFAULT              eColorBlack     // HPGL from 8753 always sets the color .. so not useful

// Lines of HPGL commands are compiled into a serial sream that can be quickly drawn and stored
// Each variable length command is prceeded by a CHPGL (compiled HPGL) byte
typedef enum { PAYLOAD_ONLY=0,   CHPGL_MOVE=1,      CHPGL_RMOVE=2,
               CHPGL_PEN_UP=3,   CHPGL_PEN_DOWN=4,  CHPGL_PEN=5,
               CHPGL_LINETYPE=6, CHPGL_TEXT_SIZE=7, CHPGL_LABEL=8,
               CHPGL_OP=9,       CHPGL_IP=10,       CHPGL_SCALING=12,
               CHPGL_ROTATION=13, CHPGL_UCHAR=14 } eHPGL;
typedef enum { SCALING_NONE=0, SCALING_ANISOTROPIC=1, SCALING_ISOTROPIC=2, SCALING_POINT=3, SCALING_ISOTROLIC_LB=4 } eHPGLscalingType;

// The subset of HPGL commands that the 8753 provides are the following
#define HPGL_CHARACTER_SET  ('C'<<8|'S')    // CS (Character Set)
#define HPGL_DEF_TERMINATOR ('D'<<8|'T')    // DT (Define Terminator (label))
#define HPGL_INPUT_MASK     ('I'<<8|'M')    // IM (Input Mask)
#define HPGL_INITIALIZE     ('I'<<8|'N')    // IN (Initialize)
#define HPGL_INPUT_POINTS   ('I'<<8|'P')    // IP (Input P1 & P2)
#define HPGL_INPUT_WINDOW	('I'<<8|'W')    // IW (Input Window)
#define HPGL_LABEL    		('L'<<8|'B')	// LB (label)
#define HPGL_LINE_TYPE		('L'<<8|'T')	// LT (line type)

#define HPGL_OUTPUT_ERROR   ('O'<<8|'E')    // OE (Output Error)
#define HPGL_OUTPUT_POINTS  ('O'<<8|'P')    // OP (Output P1 & P2)
#define HPGL_OUTPUT_STATUS  ('O'<<8|'S')    // OS (Output Status)

#define HPGL_POSN_ABS		('P'<<8|'A')	// PA (position absolute)
#define HPGL_PEN_DOWN		('P'<<8|'D')	// PD (pen down)
#define HPGL_POSN_REL		('P'<<8|'R')	// PA (position relative)
#define HPGL_PEN_UP			('P'<<8|'U')	// PU (pen up)
#define HPGL_ROTATION		('R'<<8|'O')	// RO (rotation)
#define HPGL_SCALING        ('S'<<8|'C')    // SC (Scaling)
#define HPGL_SELECT_PEN     ('S'<<8|'P')    // SP (Select Pen)
#define HPGL_CHAR_SIZE_REL	('S'<<8|'R')	// SR (character size relative)
#define HPGL_USER_CHARACTER ('U'<<8|'C')    // UC (User Character)
#define HPGL_VELOCITY		('V'<<8|'S')	// VS (Velocity Select)

#define HPGL_DEFAULT        ('D'<<8|'F')    // DF (Default)
#define HPGL_PAGE_FEED      ('P'<<8|'G')    // PG (Page Feed)

#define HPGL_LINE_TERMINATOR_CHARACTER '\003'

#define QUANTIZE(x, y) ((gint)(((gdouble)(x)/(y))+1) * y)

gboolean parseHPGLcmd( guint16 HPGLcmd, gchar *sHPGLargs, tGlobal *pGlobal );
gboolean deserializeHPGL( gchar *sHPGL, tGlobal *pGlobal );
void initializeHPGL( tGlobal *pGlobal, gboolean bLandscape );
void CB_DrawingArea_Draw (GtkDrawingArea *widget, cairo_t *cr, gint areaWidth, gint areaHeight, gpointer pGlobal);

gboolean sendGPIBreply( gchar *sHPGLreply, tGlobal *pGlobal );
#define DEFAULT_GPIB_DEVICE_ID		  23
#define DEFAULT_GPIB_CONTROLLER_INDEX 0
#define DEFAULT_GPIB_CONTROLLER_NAME  "NI_USBHS"

// HPGL plotter units are 0.025mm / ~0.00098in
// A3 is 420mm x 297mm (16.5354" x 11.6929") 16800pu x 11880pu
// A3 is 420mm x 297mm (16.5354" x 11.6929") 19842pt x 14032pt (@ 1200 ppi)
// With 5mm margin, P1 is 200pu,200pu & P2 is 14800pu,11680pu

#define HPGL_A3_LL_X	0
#define HPGL_A3_LL_Y	0
#define HPGL_A3_LONG	16800
#define HPGL_A3_SHORT	11880

#define SQU_ROOT_2			(1.41421356237)

#define HPGL_MARGIN		200

#define LOCAL_DELAYms   50              // Delay after going to local from remote

extern tGlobal globalData;

#define ms(x)           ((x) * 1000)
#define ms2us(x)        ((x) * 1000)

#define DBG( level, message, ... ) \
	if( globalData.flags.bbDebug >= level ) \
		LOG( G_LOG_LEVEL_DEBUG, message, ## __VA_ARGS__)

#if !GLIB_CHECK_VERSION(2, 67, 3)
static inline gpointer
g_memdup2(gconstpointer mem, gsize byte_size) {
        gpointer new_mem = NULL;
                if(mem && byte_size != 0) {
                        new_mem = g_malloc (byte_size);
                        memcpy (new_mem, mem, byte_size);
                }
                return new_mem;
}
#endif /* !GLIB_CHECK_VERSION(2, 67, 3) */
#ifndef GPIB_CHECK_VERSION
 #define GPIB_CHECK_VERSION(ma,mi,mic) 0
 #pragma message "You should update to a newer version of linux-gpib"
#endif

enum _Widgets {
	eW_drawingArea
};

#define WLOOKUP(p,n) g_hash_table_lookup ( (p)->widgetHashTable, (gconstpointer)(n))
#define COMMA2SPACE(x) { for( gchar *p=(x); *p != 0; p++ ) if( *p == ',' ) *p = ' '; }

void logVersion(void);

void CB_btn_Options    ( GtkButton* wBtnOptions, gpointer user_data );
void CB_btn_Erase      ( GtkButton* wBtnOptions, gpointer user_data );
void CB_chk_AutoErase  ( GtkCheckButton* wBtnOptions, gpointer user_data );
void CB_btn_Print      ( GtkButton* wBtnOptions, gpointer user_data );
void CB_btn_PDF        ( GtkButton* wBtnOptions, gpointer user_data );
void CB_btn_PNG        ( GtkButton* wBtnOptions, gpointer user_data );
void CB_btn_SVG        ( GtkButton* wBtnOptions, gpointer user_data );
void CB_btn_OK         ( GtkButton* wBtnOK, gpointer user_data );
void CB_color_Pen      ( GtkColorButton* wColorBtn, gpointer user_data );
void CB_btn_ColorReset ( GtkButton* wBtnColorReset, gpointer user_data );
void CB_chk_UseControllerName ( GtkCheckButton* wBtnUseControllerName, gpointer user_data );
void initializeOptionsDialog( tGlobal *pGlobal );
gint saveSettings( tGlobal *pGlobal );
gint recoverSettings( tGlobal *pGlobal );
gint splashCreate (tGlobal *pGlobal);
gint splashDestroy (tGlobal *pGlobal);

void drawHPlogo (cairo_t *cr, gdouble centreX, gdouble lowerLeftY, gdouble scale);

gboolean plotCompiledHPGL (cairo_t *cr, gdouble areaWidth, gdouble areaHeight, tGlobal *pGlobal);
void clearHPGL( tGlobal *pGlobal );

#define GSETTINGS_SCHEMA	"us.heterodyne.HPGLplotter"
