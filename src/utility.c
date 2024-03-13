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


#include <cairo/cairo.h>
#include <glib-2.0/glib.h>
#include <gtk/gtk.h>
#include <HPGLplotter.h>
#include <math.h>
#include <sys/utsname.h>

/*! \brief Log version to journal
 *
 */

void
logVersion(void) {

    struct utsname UTSbuffer;

    errno = 0;
    if ( uname(&UTSbuffer) != 0) {
    	LOG( G_LOG_LEVEL_CRITICAL, "%s", strerror (errno) );
        return;
    }
    LOG( G_LOG_LEVEL_INFO, "%s %s %s %s %s",
            UTSbuffer.sysname, UTSbuffer.nodename,
            UTSbuffer.release, UTSbuffer.version,
            UTSbuffer.machine);
    LOG( G_LOG_LEVEL_INFO, "HPGLplotter version: %s", VERSION );

    return;
}
