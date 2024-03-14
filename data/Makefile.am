HPGLplotterdir = $(prefix)/share/icons/hicolor/256x256/apps/
HPGLplotter_DATA = application-icon/hicolor/256x256/apps/HPGLplotter.png

HPGLplotter2dir = $(prefix)/share/icons/hicolor/512x512/apps/
HPGLplotter2_DATA = application-icon/hicolor/512x512/apps/HPGLplotter.png

HPGLplotter3dir = $(prefix)/share/icons/hicolor/48x48/apps/
HPGLplotter3_DATA = application-icon/hicolor/48x48/apps/HPGLplotter.png

HPGLplotter4dir = $(prefix)/share/icons/hicolor/scalable/apps/
HPGLplotter4_DATA = application-icon/hicolor/scalable/apps/HPGLplotter.svg

HPGLplotter5dir = $(prefix)/share/glib-2.0/schemas/
HPGLplotter5_DATA = gsettings/us.heterodyne.hpgl-plotter.gschema.xml

HPGLplotter6dir = $(prefix)/share/applications
HPGLplotter6_DATA = applications/HPGLplotter.desktop

HPGLplotter7dir = $(prefix)/usr/local/share/fonts/HPGLplotter/
HPGLplotter_DATA = fonts/NotoSansMono-Light.ttf

install-data-hook:
	gtk-update-icon-cache --ignore-theme-index $(prefix)/share/icons/hicolor/
	glib-compile-schemas $(prefix)/share/glib-2.0/schemas
	fc-cache $(prefix)/share/fonts/HPGLplotter

