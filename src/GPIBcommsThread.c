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
#include <math.h>

#include <glib-2.0/glib.h>
#include <gpib/ib.h>
#include <errno.h>
#include <HPGLplotter.h>
#include <GPIBcomms.h>
#include <locale.h>

#include "messageEvent.h"

/*!     \brief  See if there are messages on the asynchronous queue
 *
 * If the argument contains a pointer to a queue, set the default queue to it
 *
 * Check the queue and report the number of messages
 *
 * \param asyncQueue  pointer to async queue (or NULL)
 * \return            number of messages in queue
 */
static gint
GPIB_checkQueue( GAsyncQueue *asyncQueue )
{
    static GAsyncQueue *queueToCheck = NULL;

    if( asyncQueue )
        queueToCheck = asyncQueue;

    if( !asyncQueue && queueToCheck ) {
        return g_async_queue_length( queueToCheck );
    } else {
        return 0;
    }
}

#define THIRTY_MS 0.030
#define ONE_SECOND 1.0
#define FIVE_SECONDS 5.0
/*!     \brief  Write data from the GPIB device asynchronously
 *
 * Read data from the GPIB device asynchronously while checking for exceptions
 * This is needed when it is anticipated that the response will take some time.
 *
 * \param GPIBdescriptor GPIB device descriptor
 * \param readBuffer     pointer to data to save read data
 * \param maxBytes       maxium number of bytes to read
 * \param pGPIBstatus    pointer to GPIB status
 * \param timeout        the maximum time to wait before abandoning
 * \param queue          message queue from main thread
 * \return               read status result
 */
tGPIBReadWriteStatus
GPIBasyncWriteBinary( gint GPIBdescriptor, const void *sData, glong length,
        glong *pBytesWritten, gint *pGPIBstatus, gdouble timeoutSecs )
{
    gint currentTimeout;
    gdouble waitTime = 0.0;
    tGPIBReadWriteStatus rtn = eRDWT_CONTINUE;
    gint waitStatus;
    gboolean bDCAS = TRUE;

    *pBytesWritten = 0;

    if( GPIBfailed( *pGPIBstatus ) ) {
        return eRDWT_PREVIOUS_ERROR;
    }
    // Save timeout to restore later
    ibask(GPIBdescriptor, IbaTMO, &currentTimeout);
    // No timeout on the actual read command ( we have a short timeout on the wait)
    ibtmo( GPIBdescriptor, TNONE );
    *pGPIBstatus = ibwrta( GPIBdescriptor, sData, length );

    if( GPIBfailed( *pGPIBstatus ) )
        return eRDWT_ERROR;
    //todo - remove when Linux GPIB driver fixed
    // a bug in the drive means that the timeout used for the ibrda command is not accessed immediately
    // we delay, so that the timeout used is TNONE before changing to T30ms
#if !GPIB_CHECK_VERSION(4,3,6)
    usleep( 20 * 1000 );
#endif

    // set the timout for the ibwait to 30ms
    ibtmo( GPIBdescriptor, T30ms );
    do {
        waitStatus = ibwait(GPIBdescriptor,  bDCAS ? (TIMO | CMPL) : (TIMO | CMPL | DCAS) );
        // The wait may return with the DCAS flag before the driver has set the CMPL flag.
        // Take note of the flag and wait for the CMPL flag (which will follow).
        if( (waitStatus & DCAS) == DCAS )
            bDCAS = TRUE;

        if( (waitStatus & TIMO) == TIMO ){
            // Timeout
            rtn = eRDWT_CONTINUE;
            waitTime += THIRTY_MS;
            if (waitTime > FIVE_SECONDS && fmod(waitTime, 1.0) < THIRTY_MS) {
                gchar *sMessage = g_strdup_printf("✍🏻 Waiting for GPIB instrument: %ds", (gint) (waitTime));
                postInfo(sMessage);
                g_free(sMessage);
            }
        } else {
            // An artifact of the GPIB driver is that the ibwait() may complete
            // but none of the conditions in the mask are set. We need to check
            // that at least one of the conditions are met (here CMPL)
            // Did we complete the read or get an error?
            if((waitStatus & ERR) == ERR ) { // or did we have a read error
                if( (waitStatus & CMPL) == CMPL && AsyncIberr() == EABO )
                    rtn = eRDWT_OK;
                else
                    rtn = eRDWT_ERROR;
            } else {
                // We must be here because either CMPL or END is set
                if ((waitStatus & CMPL) == CMPL )
                    rtn = eRDWT_OK;
            }
        }
        // If we get a message on the queue, it is assumed to be an abort
        if( GPIB_checkQueue( NULL ) )
            rtn = eRDWT_ABORT;
    } while( rtn == eRDWT_CONTINUE
            && (timeoutSecs != TIMEOUT_NONE ? (waitTime < timeoutSecs) : TRUE)  );

    // Stop the I/O operation if it is not complete
    if( ( waitStatus & CMPL ) != CMPL) {
        //  On successfully aborting an asynchronous operation,
        //  the ERR bit is set in ibsta, and iberr is set to EABO.
        //  If the ERR bit is not set in ibsta, then there was no asynchronous i/o operation in progress.
        ibstop( GPIBdescriptor );
    }
    // Only the status bits END | ERR | TIMO | CMPL are valid. (all others are 0)
    // Also flag that device clear was seen
    *pGPIBstatus = AsyncIbsta() | (bDCAS ? DCAS : 0);

    if( pBytesWritten )
        *pBytesWritten = AsyncIbcnt();

    DBG( eDEBUG_EXTENSIVE, "🖊: %d / %ld bytes", AsyncIbcnt(), length );

    if( rtn == eRDWT_ABORT )
        LOG( G_LOG_LEVEL_WARNING, "GPIB async write abort status/error: %04X/%d", *pGPIBstatus, AsyncIberr() );

    if( rtn == eRDWT_CONTINUE && rtn != eRDWT_ABORT ) {
        if( timeoutSecs != TIMEOUT_NONE && waitTime >= timeoutSecs )
            LOG( G_LOG_LEVEL_WARNING, "GPIB async write timeout after %.2f sec. status %04X", timeoutSecs, waitStatus);
        else
            LOG( G_LOG_LEVEL_WARNING, "GPIB async write status/error: %04X/%d", waitStatus, AsyncIberr() );
    }
    ibtmo( GPIBdescriptor, currentTimeout);

    if( rtn == eRDWT_CONTINUE ) {
        *pGPIBstatus |= ERR_TIMEOUT;
        return (eRDWT_TIMEOUT);
    } else {
        return (rtn);
    }

}

/*!     \brief  Write (async) string to the GPIB device
 *
 * Send NULL terminated string to the GPIB device (asynchronously)
 *
 * \param GPIBdescriptor GPIB device descriptor
 * \param sData          data to send
 * \param pGPIBstatus    pointer to GPIB status
 * \return               count or ERROR
 */
tGPIBReadWriteStatus
GPIBasyncWriteString( gint GPIBdescriptor, const void *sData, gint *pGPIBstatus, gdouble timeoutSecs ) {
    glong nBytesWritten;
    DBG( eDEBUG_EXTENSIVE, "🖊: %s", (char *)sData );
    return GPIBasyncWriteBinary( GPIBdescriptor, sData, strlen( (gchar *)sData ),
            &nBytesWritten, pGPIBstatus, timeoutSecs );
}

/*!     \brief  Read data from the GPIB device asynchronously
 *
 * Read data from the GPIB device asynchronously while checking for exceptions
 * This is needed when it is anticipated that the response will take some time.
 *
 * \param GPIBdescriptor GPIB device descriptor
 * \param readBuffer     pointer to data to save read data
 * \param maxBytes       maxium number of bytes to read
 * \param pGPIBstatus    pointer to GPIB status
 * \param timeout        the maximum time to wait before abandoning
 * \return               read status result
 */
tGPIBReadWriteStatus
GPIBasyncRead( gint GPIBdescriptor, void *readBuffer, glong maxBytes,
        glong *pNbytesRead, gint *pGPIBstatus, gdouble timeoutSecs )
{
    gint currentTimeout;
    gdouble waitTime = 0.0;
    tGPIBReadWriteStatus rtn = eRDWT_CONTINUE;
    gint waitStatus;
    __attribute__((unused)) gboolean bDCAS = FALSE;

    *pNbytesRead = 0;

    if( GPIBfailed( *pGPIBstatus ) ) {
        return eRDWT_PREVIOUS_ERROR;
    }

    ibask(GPIBdescriptor, IbaTMO, &currentTimeout);
    // for the read itself we have no timeout .. we loop using ibwait with short timeout
    ibtmo( GPIBdescriptor, TNONE );
    *pGPIBstatus = ibrda( GPIBdescriptor, readBuffer, maxBytes );

    if( GPIBfailed( *pGPIBstatus ) )
        return eRDWT_ERROR;

    //todo - remove when linux GPIB driver fixed
    // a bug in the driver means that the timeout used for the ibrda command is not accessed immediately
    // we delay, so that the timeout used is TNONE before changing to T30ms
#if !GPIB_CHECK_VERSION(4,3,6)
    usleep( 20 * 1000 );
#endif

    // set the timout for the ibwait to 30ms
    ibtmo( GPIBdescriptor, T30ms );
    do {
        // Wait for read completion or timeout or being set as a talker
        // We may also receive a device clear
        waitStatus = ibwait(GPIBdescriptor,  bDCAS ? (TIMO | CMPL) : (TIMO | CMPL | DCAS) );
        // The wait may return with the DCAS flag before the driver has set the CMPL flag.
        // Take note of the flag and wait for the CMPL flag (which will follow).
        if( (waitStatus & DCAS) == DCAS )
            bDCAS = TRUE;

        if( (waitStatus & TIMO) == TIMO ){
            // Timeout
            rtn = eRDWT_CONTINUE;
            waitTime += THIRTY_MS;
            if( waitTime > FIVE_SECONDS && fmod( waitTime, 1.0 ) < THIRTY_MS ) {
                gchar *sMessage =  g_strdup_printf( "🕐 Waiting for HPGL" );
                postInfo( sMessage );
                g_free( sMessage );
            }
        } else {
            // An artifact of the GPIB driver is that the ibwait() may complete
            // but none of the conditions in the mask are set. We need to check
            // that at least one of the conditions are met (here CMPL)
            // Did we complete the read or get an error?
            if((waitStatus & ERR) == ERR ) { // or did we have a read error
                if( (waitStatus & CMPL) == CMPL && AsyncIberr() == EABO )
                    rtn = eRDWT_OK;
                else
                    rtn = eRDWT_ERROR;
            } else {
                // We must be here because either CMPL or END is set
                if ((waitStatus & CMPL) == CMPL )
                    rtn = eRDWT_OK;
            }
        }

        // If we get a message on the queue, it is assumed to be an abort
        if( GPIB_checkQueue( NULL ) )
            rtn = eRDWT_ABORT;

    } while( rtn == eRDWT_CONTINUE
            && (timeoutSecs != TIMEOUT_NONE ? (waitTime < timeoutSecs) : TRUE)  );

    // Stop the I/O operation if it is not complete
    if( ( waitStatus & CMPL ) != CMPL) {
        //  On successfully aborting an asynchronous operation,
        //  the ERR bit is set in ibsta, and iberr is set to EABO.
        //  If the ERR bit is not set in ibsta, then there was no asynchronous i/o operation in progress.
        ibstop( GPIBdescriptor );
    }

    // Only the status bits END | ERR | TIMO | CMPL are valid. (all others are 0)
    // Also flag that device clear was seen
    *pGPIBstatus = AsyncIbsta() | (bDCAS ? DCAS : 0);

    if( pNbytesRead ) {
        *pNbytesRead = AsyncIbcnt();
    }

    /* A change of state from listener to talker may occur  before a terminating
     * condition (EOI or eos character).
     * Don't treat an abort as an error. It can come from an ibclr()
     */

    if ( (*pGPIBstatus & ERR) == ERR && AsyncIberr() == EABO  )
        *pGPIBstatus &=  ~ERR;

    if( bDCAS )
        DBG( eDEBUG_EXTENSIVE, "👓 Device Clear received\n" );

    DBG( eDEBUG_EXTENSIVE, "👓 %d bytes (%ld max)", *pNbytesRead, maxBytes );

    if( rtn == eRDWT_CONTINUE && (waitStatus & TACS) != TACS && rtn != eRDWT_ABORT ) {
        // Log if we are not a talker and we did not complete the read
        if( timeoutSecs != TIMEOUT_NONE && waitTime >= timeoutSecs )
            LOG( G_LOG_LEVEL_WARNING, "GPIB async read timeout after %.2f sec. status %04X", timeoutSecs, waitStatus);
        else
            LOG( G_LOG_LEVEL_WARNING, "GPIB async read status/error: %04X/%d", waitStatus, AsyncIberr() );
    }

    ibtmo( GPIBdescriptor, currentTimeout);

    if( rtn == eRDWT_CONTINUE ) {
        *pGPIBstatus |= ERR_TIMEOUT;
        return (eRDWT_TIMEOUT);
    } else {
        return (rtn);
    }
}

/*!     \brief  Read configuration value from the GPIB device
 *
 * Read configuration value for GPIB device
 *
 * \param GPIBdescriptor GPIB device descriptor
 * \param option         the paramater to read
 * \param result         pointer to the integer receiving the value
 * \param pGPIBstatus    pointer to GPIB status
 * \return               OK or ERROR
 */
int
GPIBreadConfiguration( gint GPIBdescriptor, gint option, gint *result, gint *pGPIBstatus ) {
    *pGPIBstatus = ibask( GPIBdescriptor, option, result );

    if( GPIBfailed( *pGPIBstatus ) )
        return ERROR;
    else
        return OK;
}


/*!     \brief  close the GPIB devices
 *
 * Close the controller if it was opened and close the device
 *
 * \param pDescGPIBcontroller pointer to GPIB controller descriptor
 * \param pDescGPIB_HP662X    pointer to GPIB device descriptor
 */
gint
closeGPIBcontroller( tGlobal *pGlobal ) {
#define FIRST_ALLOCATED_CONTROLLER_DESCRIPTOR 16
    // If we have another system controller on the bus, the commands here will cause a problem
    // Do not change to controller mode unless the setting is checked.

    if( pGlobal->GPIBcontrollerDevice == INVALID || !pGlobal->flags.bGPIBcommsActive ) {
        return 0;
    }

    if( !pGlobal->flags.bDoNotEnableSystemController && pGlobal->flags.bInitialActiveController) {
        // Request system control
        // i.e. make board system controller
        if( (ibrsc( pGlobal->GPIBcontrollerDevice, TRUE ) & ERR ) == ERR )
            LOG( G_LOG_LEVEL_WARNING, "ibrsc (TRUE) error: %s / status: 0x%04x", gpib_error_string(ThreadIberr()), ThreadIbsta());
        // perform interface clear (board)
        // this is to get the instrument (e.g. HP8595E) to release the GPIB
        // without this we get "not controller in charge" errors

        if( ibsic( pGlobal->GPIBcontrollerDevice ) & ERR )  {
            LOG( G_LOG_LEVEL_WARNING, "ibsic (0) (TRUE) error: %s / status: 0x%04x", gpib_error_string(ThreadIberr()), ThreadIbsta());
        }

        if( ibsre( pGlobal->GPIBcontrollerDevice, TRUE ) & ERR )  {
            LOG( G_LOG_LEVEL_WARNING, "ibsre (TRUE) error: %s / status: 0x%04x", gpib_error_string(ThreadIberr()), ThreadIbsta());
        }
        // assert ATN (board)
        // i.e. become active controller (if it was set when we started )
        if( pGlobal->flags.bInitialGPIB_ATN &&
                (ibcac( pGlobal->GPIBcontrollerDevice, 0 ) & ERR ) == ERR )
            LOG( G_LOG_LEVEL_WARNING, "ibcac (true) error: %s / status: 0x%04x", gpib_error_string(ThreadIberr()), ThreadIbsta());
    }
    // reinitialize controller (parameters in /etc/gpib.conf)
    // if we opened the device with ibfind(), release the resources
    if(  !pGlobal->flags.bGPIB_ControllerOpenedWithIndex
            && ( ibonl( pGlobal->GPIBcontrollerDevice, 0 ) & ERR ) )
        LOG( G_LOG_LEVEL_WARNING, "ibonl error: %s / status: 0x%04x", gpib_error_string(ThreadIberr()), ThreadIbsta());

    pGlobal->flags.bGPIB_ControllerOpenedWithIndex = FALSE;
    pGlobal->flags.bGPIBcommsActive = FALSE;
    return 0;
}

#define GPIB_EOI		TRUE
#define	GPIB_EOS_NONE	0

/*!     \brief  open the GPIB device
 *
 * Get the device descriptors of the controller and GPIB device
 * based on the parameters set (whether to use descriptors or GPIB addresses)
 *
 * \param pGlobal             pointer to global data structure
 * \return                    0 on success or ERROR on failure
 */
gint
openGPIBcontroller( tGlobal *pGlobal, gboolean bResetInterface ) {
    gshort	lineStatus;
    gint	ibaskResult = 0;
    gboolean bNoListeners = FALSE;
    static gboolean bFirst = TRUE;

    /* The board index can be used as a device descriptor; however,
     * if a device descriptor was returned from the ibfind, it must be freed
     * with ibonl().
     * The board index corresponds to the minor number /dev/
     * $ ls -l /dev/gpib0
     * crw-rw----+ 1 root root 160, 0 May 12 09:24 /dev/gpib0
     */
    // Close if we are already open
    if( pGlobal->flags.bGPIBcommsActive )
        closeGPIBcontroller( pGlobal );

    if( pGlobal->flags.bGPIB_UseControllerIndex ) {
        pGlobal->GPIBcontrollerDevice = pGlobal->GPIBcontrollerIndex;
    } else {
        pGlobal->GPIBcontrollerDevice = ibfind( pGlobal->sGPIBcontrollerName );
    }
    pGlobal->flags.bGPIB_ControllerOpenedWithIndex = pGlobal->flags.bGPIB_UseControllerIndex;

    if( pGlobal->GPIBcontrollerDevice == INVALID )
        goto err;

    // ask if we are the system controller
    if( bFirst ) {
        if( ibask( pGlobal->GPIBcontrollerDevice, IbaSC, &ibaskResult ) & ERR ) {
            LOG( G_LOG_LEVEL_WARNING, "ibask (IbaSC) error: %s / status: 0x%04x", gpib_error_string(ThreadIberr()), ThreadIbsta());
            goto err;
        }
        // Remember if we are the system controller or not
        pGlobal->flags.bInitialActiveController = (ibaskResult != 0);
    }
    bFirst = FALSE;

    // Set the EOT (assert EOI with last data byte)
    if( ibeot( pGlobal->GPIBcontrollerDevice, GPIB_EOI ) & ERR ) {
        LOG( G_LOG_LEVEL_WARNING, "ibeot error: %s / status: 0x%04x", gpib_error_string(ThreadIberr()), ThreadIbsta());
        goto err;
    }

    // Disable read termination on character
    if( ibeos( pGlobal->GPIBcontrollerDevice,
            pGlobal->flags.bEOIonLF ? BIN | REOS | 0x0a : BIN | 0x0a ) & ERR ) {	// no | REOS  (Enable termination of reads when eos character is received.)
        LOG( G_LOG_LEVEL_WARNING, "ibeos error: %s / status: 0x%04x", gpib_error_string(ThreadIberr()), ThreadIbsta());
        goto err;
    }

    // Check that there is a controller there (USB device may have been removed)
    if( iblines( pGlobal->GPIBcontrollerDevice, &lineStatus )  & ERR ) {
        goto err;
    }

    // If we have another system controller on the bus, the commands here will cause a problem
    // Do not change to controller mode unless the setting is checked.
    if( !pGlobal->flags.bDoNotEnableSystemController && ( pGlobal->flags.bInitialActiveController || bResetInterface ) ) {

        if( bResetInterface ) {
            // Request system control (board
            // i.e. make board system controller
            if( !pGlobal->flags.bInitialActiveController )	// We only have to make system controller if it not already
                if( (ibrsc( pGlobal->GPIBcontrollerDevice, TRUE ) & ERR ) == ERR )
                    LOG( G_LOG_LEVEL_WARNING, "ibrsc (TRUE) error: %s / status: 0x%04x", gpib_error_string(ThreadIberr()), ThreadIbsta());

            // perform interface clear (board)
            // this is to get the instrument (e.g. HP8595E) to release the GPIB
            // without this we get "not controller in charge" errors
            if( ibsic( pGlobal->GPIBcontrollerDevice ) & ERR )  {
                LOG( G_LOG_LEVEL_WARNING, "ibsic (0) (TRUE) error: %s / status: 0x%04x", gpib_error_string(ThreadIberr()), ThreadIbsta());
            }
        }

        pGlobal->flags.bInitialGPIB_ATN = ( ( lineStatus  & ( ValidATN | BusATN ) ) == ( ValidATN | BusATN ) ? 1 : 0 );
        // Release the ATN line (if it's set)

        if( pGlobal->flags.bInitialGPIB_ATN &&
                (ibgts( pGlobal->GPIBcontrollerDevice, 0 ) & ERR) == ERR ) {
            LOG( G_LOG_LEVEL_WARNING, "ibgts error: %s / status: 0x%04x", gpib_error_string(ThreadIberr()), ThreadIbsta());
            goto err;
        }
    }

    if( pGlobal->flags.bGPIB_InitialListener ) {
        guchar listenGPIBcmds[] = { UNT, UNL, LAD | pGlobal->GPIBdevicePID };
        if( ibcmd( pGlobal->GPIBcontrollerDevice, listenGPIBcmds, sizeof( listenGPIBcmds ) ) & ERR ) {
            if( ThreadIberr() != ENOL ) {
                LOG( G_LOG_LEVEL_WARNING, "ibcmd error: %s / status: 0x%04x", gpib_error_string(ThreadIberr()), ThreadIbsta());
                goto err;
            } else if( ThreadIberr() == ENOL ) {
                bNoListeners = TRUE;
            }
        }

        if( ibgts( pGlobal->GPIBcontrollerDevice, 0 ) & ERR ) {
            LOG( G_LOG_LEVEL_WARNING, "ibgts error: %s / status: 0x%04x", gpib_error_string(ThreadIberr()), ThreadIbsta());
            goto err;
        }
    }

    // Relinquish system control (argument is FALSE)
    if( ibrsc( pGlobal->GPIBcontrollerDevice, FALSE ) & ERR ) {
        LOG( G_LOG_LEVEL_WARNING, "ibrsc error: %s / status: 0x%04x", gpib_error_string(ThreadIberr()), ThreadIbsta());
        goto err;
    }

    // Set the primary GPIB device ID of the plotter we are simulating
    if( ibpad( pGlobal->GPIBcontrollerDevice, pGlobal->GPIBdevicePID ) & ERR ) {
        LOG( G_LOG_LEVEL_WARNING, "ibpad error: %s / status: 0x%04x", gpib_error_string(ThreadIberr()), ThreadIbsta());
        goto err;
    }
    // raise(SIGSEGV);
    if( !bNoListeners )
        pGlobal->flags.bGPIBcommsActive = TRUE;

    return bNoListeners;

    err:
    if( pGlobal->GPIBcontrollerDevice >= FIRST_ALLOCATED_CONTROLLER_DESCRIPTOR )
        ibonl( pGlobal->flags.bGPIB_UseControllerIndex, 0 );
    pGlobal->flags.bGPIB_ControllerOpenedWithIndex = FALSE;
    pGlobal->flags.bGPIBcommsActive = FALSE;
    return ERROR;
}



/*!     \brief  Get real time in ms
 *
 * Get the time in milliseconds
 *
 * \return       the current time in milliseconds
 */
gulong
now_milliSeconds() {
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    return now.tv_sec * 1.0e3 + now.tv_nsec / 1.0e6;
}

gboolean
sendGPIBreply( gchar *sHPGLreply, tGlobal *pGlobal ) {

    gint GPIBstatus;
    gboolean bWaitingForTalker = TRUE;

    if( pGlobal->flags.bMuteGPIBreply )
        return TRUE;

    while ( bWaitingForTalker ) {
        GPIBstatus = 0;

        if( GPIB_checkQueue( NULL ) )
            return FALSE;

        // Short timeout
        ibconfig( pGlobal->GPIBcontrollerDevice, IbcTMO, T100ms );

        // Wait for GPIB line to toggle (or timeout)
        // LACS - Board is currently addressed as a listener (IEEE listener state machine is in LACS or LADS).
        GPIBstatus = ibwait( pGlobal->GPIBcontrollerDevice, TIMO | TACS);
        if ( GPIBstatus & TIMO )
            return FALSE;	// we shouldn't be waiting this line...
#if 0
        if ( GPIBstatus & ATN ) {
            // only for 100ms max
            if( timeouts++ > 10 )
                return FALSE;
            usleep( ms(10) );
        } else {
#else
            {
#endif
                ibconfig( pGlobal->GPIBcontrollerDevice, IbcTMO, T1s );

                if ( GPIBasyncWriteString( pGlobal->GPIBcontrollerDevice, sHPGLreply, &GPIBstatus, 0.2 ) & eRDWT_OK ) {
                    LOG( G_LOG_LEVEL_WARNING, "ibwrt error: %s / status: 0x%04x", gpib_error_string(ThreadIberr()), ThreadIbsta());
                }
                bWaitingForTalker = FALSE;
            }
        }
        return TRUE;	// return TRUE
    }

    /*!     \brief  Post a refresh message if we have received data but no SP;
     *
     * Ensure we draw the plot when the data is over
     *
     *
     * \param  tGlobal Pointer to global data
     * \return false (do not restart)
     */
    gint
    postRefreshOnTimeout (tGlobal *pGlobal)
    {
        postMessageToMainLoop(TM_REFRESH_PLOT, NULL);
        pGlobal->refreshTimer = 0;
        return G_SOURCE_REMOVE;
    }

    /*!     \brief  Thread to communicate with GPIB
     *
     * Start thread to perform asynchronous GPIB communication
     *
     * \param _pGlobal : pointer to structure holding global variables
     * \return       0 for success and ERROR on problem
     */
    gpointer
    threadGPIB(gpointer _pGlobal) {
        tGlobal *pGlobal = (tGlobal*) _pGlobal;

        gchar *sGPIBversion;
        gint GPIBstatus;
        gboolean bInitialAddressedAsListener = FALSE;

        messageEventData *message;
        gboolean bRunning = TRUE;
        gboolean bDCAS = FALSE;

#define MAX_HPGL_PLOT_CHUNK    2000
        gchar sHPGL[ MAX_HPGL_PLOT_CHUNK + 1 ] = "";

        gulong __attribute__((unused)) datum = 0;
        glong  nBytesRead;
        tGPIBReadWriteStatus readResult;

        // The HP662X formats numbers like 3.141 not, the continental European way 3,14159
        setlocale(LC_NUMERIC, "C");
        ibvers(&sGPIBversion);
        LOG( G_LOG_LEVEL_WARNING, "Linux GPIB version: %s", sGPIBversion);

        // g_print( "Linux GPIB version: %s\n", sGPIBversion );

        // look for the hp82357b GBIB controller
        // it is defined in /usr/local/etc/gpib.conf which can be overridden with the IB_CONFIG environment variable
        //
        //	interface {
        //    	minor = 0                       /* board index, minor = 0 uses /dev/gpib0, minor = 1 uses /dev/gpib1, etc.	*/
        //    	board_type = "agilent_82357b"   /* type of interface board being used 										*/
        //    	name = "hp82357b"               /* optional name, allows you to get a board descriptor using ibfind() 		*/
        //    	pad = 0                         /* primary address of interface             								*/
        //		sad = 0                         /* secondary address of interface          								    */
        //		timeout = T100ms                /* timeout for commands 												    */
        //		master = yes                    /* interface board is system controller 								    */
        //	}

        // loop waiting for messages from the main loop

        // Set the default queue to check for interruptions to async GPIB reads
        GPIB_checkQueue( pGlobal->messageQueueToGPIB );

        while ( bRunning ) {
            GPIBstatus = 0;

            message = g_async_queue_try_pop(pGlobal->messageQueueToGPIB );
            if( message ) {
                switch (message->command) {
                case TG_END:
                    closeGPIBcontroller( pGlobal );
                    bRunning = FALSE;
                    break;
                case TG_OFFLINE:
                    closeGPIBcontroller( pGlobal );
                    postMessageToMainLoop(TM_OFFLINE, NULL);
                    break;
                case TG_REINITIALIZE_GPIB:
                    if( openGPIBcontroller( pGlobal, TRUE ) == ERROR ) {
                        postError("GPIB controller no connection");
                    } else {
                        if( pGlobal->flags.bDoNotEnableSystemController )
                            postInfo("GPIB controller configured");
                        else
                            postInfo("GPIB interface cleared and controller configured");
                    }
                    break;
                default:
                    break;
                }
                g_free(message->sMessage);
                g_free(message->data);
                g_free(message);
                continue;
            }

            if( !pGlobal->flags.bOnline ) {
                continue;
            }

            // Only get here if we are on-line
            // Open the GPIB controller as a device if not already opened
            if( !pGlobal->flags.bGPIBcommsActive ) {
                bInitialAddressedAsListener = TRUE;
                static gint loops = 0;
                if( (loops++ % 50) == 0 ) {
                    loops = 1;	// don't do this every time
                    switch( openGPIBcontroller( pGlobal, FALSE ) ) {
                    case ERROR:
                        postInfo("GPIB controller no connection");
                        usleep( ms(100) );
                        continue; // can't do anything without GPIB
                        break;
                    case 1:
                        postInfo("GPIB ⚠️ no listeners on bus");
                        usleep( ms(100) );
                        continue; // can't do anything without GPIB
                    default:
                        postInfo("GPIB controller configured");
                        break;
                    }
                } else {
                    usleep( ms(100) );
                    continue; // can't do anything without GPIB
                }
            }

            // Short timeout
            ibconfig( pGlobal->GPIBcontrollerDevice, IbcTMO, T100ms );

            // Wait for GPIB line to toggle (or timeout)
            // LACS - Board is currently addressed as a listener (IEEE listener state machine is in LACS or LADS).
            GPIBstatus = ibwait( pGlobal->GPIBcontrollerDevice, TIMO | LACS | (bDCAS ? 0 : DCAS) );
            // Check to see if we received a device clear
            if( GPIBstatus & DCAS ) {
                bDCAS = TRUE;
                DBG( eDEBUG_EXTENSIVE, "⟳ Device Clear received\n" );
            }
            // If we have not yet been addressed as a listener (LACS)
            // or if we are still receiving commands (ATN), loop and wait
            if ( !(GPIBstatus & LACS) || (GPIBstatus & ATN) ) {
                usleep(1000);
                continue;
            }
            // For now, we don't do anything further if there is a device clear
            bDCAS = FALSE;

            // We must be a listener if we are here
            if( bInitialAddressedAsListener ) {
                postInfo("GPIB addressed as a listener");
                bInitialAddressedAsListener = FALSE;
            }

            // We cannot timeout, but will return if there is a message to abort
            // or we detect that we are no longer addressed as a listener
            readResult = GPIBasyncRead( pGlobal->GPIBcontrollerDevice, sHPGL,
                    MAX_HPGL_PLOT_CHUNK,  &nBytesRead,
                    &GPIBstatus, TIMEOUT_NONE);
            // If we were interrupted by a message... it's not an error.. see what the message is
            if( readResult == eRDWT_ABORT )
                continue;

            if( readResult != eRDWT_OK ) {
                if( readResult == eRDWT_CLEAR )
                    LOG( G_LOG_LEVEL_WARNING, "clear received during ibrd / status: 0x%04x", ThreadIbsta());
                else
                    LOG( G_LOG_LEVEL_WARNING, "ibrd error: %s / status: 0x%04x", gpib_error_string(ThreadIberr()), ThreadIbsta());

                sHPGL[0] = 0;
                continue;
            }

            sHPGL[ nBytesRead ] = 0;	// Null terminate

            if( pGlobal->flags.bbDebug == 6 ) {
                g_printerr( "%.*s", (gint)nBytesRead, sHPGL );
            }

            // As a precaution, we will refresh the plot after 100ms
            // of the last data received
            if ( pGlobal->refreshTimer ) {
                g_source_remove( pGlobal->refreshTimer );
                pGlobal->refreshTimer = 0;
            }

            if( deserializeHPGL( sHPGL, pGlobal ) == TRUE ) {
                postMessageToMainLoop(TM_REFRESH_PLOT, NULL);
            } else {
                pGlobal->refreshTimer = g_timeout_add( 250, (GSourceFunc)postRefreshOnTimeout, pGlobal );
            }

        }
        LOG( G_LOG_LEVEL_INFO, "🪡 threadGPIB ending");
        return NULL;
    }

