/*------------------------------------------------------------------
 * Wrapper for the gnutls modules.
 *
 * Compile the tls modules into one file, but only if USE_TLS is defined.
 *------------------------------------------------------------------
 */

#include "driver.h"

#ifdef USE_TLS

#include <gnutls/gnutls.h>

#include "pkg-tls.h"

#include "actions.h"
#include "array.h"
#include "comm.h"
#include "main.h"
#include "mstrings.h"
#include "object.h"
#include "svalue.h"
#include "xalloc.h"

#include "../mudlib/sys/tls.h"

/*-------------------------------------------------------------------------*/

#define DH_BITS 1024

#define KEYFILE "key.pem"
#define CERTFILE "cert.pem"

/*-------------------------------------------------------------------------*/
/* Variables */

static gnutls_certificate_server_credentials x509_cred;
  /* The x509 credentials. */

static gnutls_dh_params dh_params;
  /* The Diffie-Hellmann parameters */

/*-------------------------------------------------------------------------*/
static void
initialize_tls_session (gnutls_session *session)

/* Initialise a TLS <session>.
 */

{
    gnutls_init(session, GNUTLS_SERVER);

    /* avoid calling all the priority functions, since the defaults
     * are adequate.
     */
    gnutls_set_default_priority( *session);   
	    
    gnutls_credentials_set( *session, GNUTLS_CRD_CERTIFICATE, x509_cred);

    /* request client certificate if any.
     */
    gnutls_certificate_server_set_request( *session, GNUTLS_CERT_REQUEST);

    gnutls_dh_set_prime_bits( *session, DH_BITS);
} /* initialize_tls_session() */

/*-------------------------------------------------------------------------*/
static int
generate_dh_params (void)

/* Generate Diffie Hellman parameters and store them in the global
 * <dh_params>.  They are for use with DHE kx algorithms. These should be
 * discarded and regenerated once a day, once a week or once a month. Depends
 * on the security requirements.
 */

{
    gnutls_datum prime, generator;

    gnutls_dh_params_init( &dh_params);
    gnutls_dh_params_generate( &prime, &generator, DH_BITS);
    gnutls_dh_params_set( dh_params, prime, generator, DH_BITS);

    free( prime.data);
    free( generator.data);

    return 0;
} /* generate_dh_params() */

/*-------------------------------------------------------------------------*/
void tls_global_init (void)

/* Initialise the TLS package; to be called once at program startup.
 */

{
    int f;

    gnutls_global_init();

    gnutls_certificate_allocate_credentials(&x509_cred);

    f = gnutls_certificate_set_x509_key_file(x509_cred, CERTFILE, KEYFILE, GNUTLS_X509_FMT_PEM);
    if (f < 0)
    {
	printf("%s TLS: Error setting x509 keyfile: %s\n"
              , time_stamp(), gnutls_strerror(f));
    }

    generate_dh_params();

    gnutls_certificate_set_dh_params( x509_cred, dh_params);
} /* tls_global_init() */

/*-------------------------------------------------------------------------*/
void
tls_global_deinit (void)

/* Clean up the TLS package on program termination.
 */

{
    gnutls_certificate_free_credentials(x509_cred);

    gnutls_global_deinit();
} /* tls_global_deinit() */

/*-------------------------------------------------------------------------*/
int
tls_read (interactive_t *ip, char *buffer, int length)

/* Read up to <length> bytes data for the TLS connection of <ip>
 * and store it in <buffer>.
 * Return then number of bytes read, or a negative number if an error
 * occured.
 */

{
    int ret;

    ret = gnutls_record_recv( ip->tls_session, buffer, length);

    if (ret == 0)
    {
	gnutls_bye(ip->tls_session, GNUTLS_SHUT_WR);
	gnutls_deinit(ip->tls_session);
	ip->tls_inited = MY_FALSE;
    }
    else if (ret < 0)
    {
	debug_message("%s TLS: Received corrupted data (%d). "
                      "Closing the connection.\n"
                     , time_stamp(), ret);
	gnutls_bye(ip->tls_session, GNUTLS_SHUT_WR);
	gnutls_deinit(ip->tls_session);
	ip->tls_inited = MY_FALSE;
    }
    return ret;
} /* tls_read() */

/*-------------------------------------------------------------------------*/
int
tls_write (interactive_t *ip, char *buffer, int length)

/* Write <length> bytes from <buffer> to the TLS connection of <ip>
 * Return the number of bytes written, or a negative number if an error
 * occured.
 */

{
    int ret;

    ret = gnutls_record_send( ip->tls_session, buffer, length );
    if (ret < 0)
    {
	gnutls_bye(ip->tls_session, GNUTLS_SHUT_WR);
	gnutls_deinit(ip->tls_session);
	ip->tls_inited = MY_FALSE;
    }
    return ret;
} /* tls_write() */

/*-------------------------------------------------------------------------*/
svalue_t *
f_tls_init_connection (svalue_t *sp)

/* EFUN tls_init_connection()
 *
 *      int tls_init_connection(object ob)
 *
 * tls_init_connection() tries to start a TLS secured connection to the
 * interactive object <ob> (or this_object() if <ob> is not given).  Returns
 * an error (< 0) if not successful. Try tls_error() to get an useful
 * description of the error.
 */

{
    int ret;
    interactive_t *ip;

    if (!O_SET_INTERACTIVE(ip, sp->u.ob))
        error("Bad arg 1 to tls_init_connection(): "
              "object not interactive.\n");

    free_svalue(sp);

    {
        object_t * save_c_g = command_giver;
        command_giver = sp->u.ob;
        add_message(message_flush);
        command_giver = save_c_g;
    }

    initialize_tls_session(&ip->tls_session);
    gnutls_transport_set_ptr(ip->tls_session, (gnutls_transport_ptr)(ip->socket));
    ret = gnutls_handshake(ip->tls_session);
    if (ret < 0)
    {
	gnutls_deinit(ip->tls_session);
	ip->tls_inited = MY_FALSE;
	put_number(sp, ret);
    }
    else
    {
	ip->tls_inited = MY_TRUE;
	put_number(sp, 1);
    }
    return sp;
} /* f_tls_init_connection() */

/*-------------------------------------------------------------------------*/
svalue_t *
f_tls_deinit_connection(svalue_t *sp)

/* EFUN tls_deinit_connection()
 *
 *      void tls_deinit_connection(object ob)
 *
 * tls_deinit_connection() shuts down a TLS connection to the interactive
 * object <ob> (or this_object() if <ob> is not given) but the connection is
 * not closed.
 */

{
    interactive_t *ip;

    if (!O_SET_INTERACTIVE(ip, sp->u.ob))
        error("Bad arg 1 to tls_deinit_connection(): "
              "object not interactive.\n");

    gnutls_bye( ip->tls_session, GNUTLS_SHUT_WR);
    gnutls_deinit(ip->tls_session);

    ip->tls_inited = MY_FALSE;

    free_svalue(sp);
    put_number(sp, 1);
    return sp;
} /* f_tls_deinit_connection() */

/*-------------------------------------------------------------------------*/
svalue_t *
f_tls_error(svalue_t *sp)

/* EFUN tls_error()
 *
 *     string tls_error(int errorno)
 *
 * tls_error() returns a string describing the error behind the
 * error number <errorno>.
 */

{
    string_t *s;
    const char *text;
    int err = sp->u.number;

    text = gnutls_strerror(err);

    if (text)
    {
        memsafe(s = new_mstring(text), strlen(text), "tls_error()");
        free_svalue(sp);
        put_string(sp, s);
    }
    else
    {
        free_svalue(sp);
        put_number(sp, 0);
    }

    return sp;
} /* f_tls_error() */

/*-------------------------------------------------------------------------*/
svalue_t *
f_tls_query_connection_state (svalue_t *sp)

/* EFUN tls_query_connection_state()
 *
 *      int tls_query_connection_state(object ob)
 *
 * tls_query_connection_state() returns TRUE if <ob>'s connection
 * is TLS secured, FALSE otherwise.
 */

{
    interactive_t *ip;

    if (!O_SET_INTERACTIVE(ip, sp->u.ob))
        error("Bad arg 1 to tls_deinit_connection_state(): "
              "object not interactive.\n");
    free_svalue(sp);
    put_number(sp, ip->tls_inited);
    return sp;
} /* f_tls_query_connection_state() */

/*-------------------------------------------------------------------------*/
svalue_t *
f_tls_query_connection_info (svalue_t *sp)

/* EFUN tls_query_connection_info()
 *
 *
 *       #include <sys/ tls.h>
 *       int *tls_query_connection_info (object ob)
 *
 * If <ob> does not have a TLS connection, the efun returns 0.
 *
 * If <ob> has a TLS connection, tls_query_connection_info() returns an array
 * that contains some parameters of <ob>'s connection:
 *
 *    int [TLS_CIPHER]: the cipher used
 *    int [TLS_COMP]:   the compression used
 *    int [TLS_KX]:     the key-exchange used
 *    int [TLS_MAC]:    the digest algorithm used
 *    int [TLS_PROT]:   the protocol used
 *
 * To translate these numbers into strings, <tls.h> offers a number of macros:
 *
 *    TLS_xxx_TABLE: a literal array of strings describing the value in
 *        question.
 *    TLS_xxx_NAME(x): a macro translating the numeric result value into a
 *        string.
 *
 *    xxx: CIPHER, COMP, KX, MAC, PROT
 */

{
    interactive_t *ip;

    if (!O_SET_INTERACTIVE(ip, sp->u.ob))
        error("Bad arg 1 to tls_query_connection_info(): "
              "object not interactive.\n");

    free_svalue(sp);

    if (ip->tls_inited)
    {
        vector_t * rc;
        rc = allocate_array(TLS_INFO_MAX);
        put_number(&(rc->item[TLS_CIPHER])
                  , gnutls_cipher_get(ip->tls_session));
	put_number(&(rc->item[TLS_COMP])
                  , gnutls_compression_get(ip->tls_session));
	put_number(&(rc->item[TLS_KX])
                  , gnutls_kx_get(ip->tls_session));
	put_number(&(rc->item[TLS_MAC])
                  , gnutls_mac_get(ip->tls_session));
	put_number(&(rc->item[TLS_PROT])
                  , gnutls_protocol_get_version(ip->tls_session));
        put_array(sp, rc);
    }
    else
    {
        put_number(sp, 0);
    }

    return sp;
} /* tls_query_connection_info() */

#endif /* USE_TLS */

/***************************************************************************/
