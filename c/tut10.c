/*  XMMS2 - X Music Multiplexer System
 *  Copyright (C) 2003-2011 XMMS2 Team
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 */

/*  This file implements the low-level 'sumclient' client, which
 *  asynchronously interacts with 'sumservice' (implemented in tut9.c).
 *  It sends a message containing two operands in a list to another client
 *  and expects their sum as a reply. The destination client's id and the
 *  two operands are specified in the command line.
 *  Should be run/studied along with tut9 aka sumservice.
 */

#include <stdlib.h>
#include <glib.h>
#include <xmmsclient/xmmsclient.h>
#include <xmmsclient/xmmsclient-glib.h>

/* Callback for the reply from the other client.
 * This receives a client-to-client reply, which we expect to
 * contain an integer (the sum of the operands we sent), parses
 * this integer and prints it out.
 */
gint
print_sum (xmmsv_t *c2c_reply, void *udata)
{
	gint sum;
	const gchar *err;

	if (xmmsv_get_error (c2c_reply, &err)) {
		fprintf (stderr, "ERROR: %s\n", err);
		goto out;
	}

	/* Since we used xmmsc_result_notifier_set when setting up
	 * this callback in the main funcion, it only receives the payload
	 * of the reply, with no metadata. This means 'c2c_reply' is simply a
	 * xmmsv_t integer, which we parse as such.
	 * In contrast, we could have used xmmsc_result_notifier_set_raw
	 * or xmmsc_result_notifier_set_c2c, in which case the contents of
	 * 'c2c_reply' would be different.
	 * Please consult the documentation for those functions for more
	 * information.
	 */
	if (!xmmsv_get_int (c2c_reply, &sum)) {
		fprintf (stderr, "Can't get an integer out of the reply.\n");
		goto out;
	}

	printf ("Sum is %d\n", sum);

out:
	/* Return value for this callback doesn't matter, it's not
	 * a broadcast nor a signal.
	 * We leave the mainloop and exit the client.
	 */
	g_main_loop_quit ((GMainLoop *) udata);
	return TRUE;
}

int
main (int argc, char **argv)
{
	GMainLoop *ml;
	xmmsv_t *operands;
	xmmsc_result_t *result;
	xmmsc_connection_t *connection;

	if (argc != 4) {
		fprintf (stderr, "Usage: %s service-id op1 op2\n", argv[0]);
		return EXIT_FAILURE;
	}

	/* Connect as usual. */
	connection = xmmsc_init ("sumclient");
	if (!connection) {
		fprintf (stderr, "OOM!\n");
		exit (EXIT_FAILURE);
	}

	if (!xmmsc_connect (connection, getenv ("XMMS_PATH"))) {
		fprintf (stderr, "Connection failed: %s\n",
		         xmmsc_get_last_error (connection));

		exit (EXIT_FAILURE);
	}

	ml = g_main_loop_new (NULL, FALSE);

	/* Build a list of two operands for sumservice (tut9). */
	operands = xmmsv_build_list (xmmsv_new_int (atoi(argv[2])),
	                             xmmsv_new_int (atoi(argv[3])),
	                             XMMSV_LIST_END);

	/* And send the list to the sumservice.
	 * We use the reply policy of only expecting a single reply,
	 * which means the receiving client will get an error from
	 * the server if it attempts to reply to this message more
	 * than once.
	 */
	result = xmmsc_c2c_send (connection,
	                         atoi(argv[1]),
	                         XMMS_C2C_REPLY_POLICY_SINGLE_REPLY,
	                         operands);

	xmmsv_unref (operands);

	xmmsc_result_notifier_set (result, print_sum, ml);
	xmmsc_result_unref (result);

	xmmsc_mainloop_gmain_init (connection);
	g_main_loop_run (ml);

	xmmsc_unref (connection);
	return EXIT_SUCCESS;

}
