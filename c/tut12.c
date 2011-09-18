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

/*  This file implements the high-level 'sumclient' client, which
 *  asynchronously interacts with the high level 'sumservice'
 *  (implemented in tut11.c).
 */

#include <stdlib.h>
#include <glib.h>
#include <xmmsclient/xmmsclient.h>
#include <xmmsclient/xmmsclient-glib.h>

/* Callback for the "odd sum" broadcast.
 */
gint
on_odd_sum (xmmsv_t *sumv, void *udata)
{
	int sum;

	if (xmmsv_is_error (sumv)) {
		printf ("Hey, an error just occurred!\n");
		return false;
	}

	xmmsv_get_int (sumv, &sum);
	printf ("Hey, an odd sum just occurred: %d\n", sum);
	return true;
}

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

/* Print the description dict of a namespace. */
void
print_desc_dict (xmmsv_t *desc, int indent)
{
	int i;
	const char *key;
	xmmsv_t *value;
	xmmsv_dict_iter_t *it;

	xmmsv_get_dict_iter (desc, &it);
	while (xmmsv_dict_iter_valid (it)) {
		for (i = 0; i < indent; i++) printf (" ");

		xmmsv_dict_iter_pair (it, &key, &value);

		printf ("%s", key);
		switch (xmmsv_get_type (value)) {
			case XMMSV_TYPE_STRING:
				xmmsv_get_string (value, &key);
				printf (": %s", key);

				break;

			case XMMSV_TYPE_INT32:
				xmmsv_get_int (value, &i);
				printf (": %d", i);

				break;

			case XMMSV_TYPE_DICT:
				printf ("\n");
				print_desc_dict (value, indent + 2);

				break;

			default:
				break;
		}

		printf ("\n");
		xmmsv_dict_iter_next (it);
	}

	return;
}

int
main (int argc, char **argv)
{
	GMainLoop *ml;
	int service_id, op1, op2;
	xmmsc_result_t *result;
	xmmsv_t *path, *pargs;
	xmmsv_t *namespace_desc;
	xmmsc_connection_t *connection;

	if (argc != 4) {
		fprintf (stderr, "Usage: %s service-id op1 op2\n", argv[0]);
		return EXIT_FAILURE;
	}

	service_id = atoi (argv[1]);
	op1 = atoi (argv[2]);
	op2 = atoi (argv[3]);

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

	/* First, introspect into the "Arithmetic" namespace. */
	path = xmmsv_build_list (XMMSV_LIST_ENTRY_STR ("Arithmetic"),
	                         XMMSV_LIST_END);
	result = xmmsc_sc_namespace_introspect (connection, service_id, path);
	xmmsv_unref (path);

	xmmsc_result_wait (result);
	namespace_desc = xmmsc_result_get_value (result);
	print_desc_dict (xmmsv_c2c_message_get_payload (namespace_desc), 0);
	xmmsc_result_unref (result);

	ml = g_main_loop_new (NULL, FALSE);

	/* Now let's subscribe to the "odd sum" broadcast.
	 * First, create a list with the path to the broadcast...
	 */
	path = xmmsv_build_list (XMMSV_LIST_ENTRY_STR ("odd sum"),
	                         XMMSV_LIST_END);

	/* ...then subscribe to it and add a callback as usual. */
	result = xmmsc_sc_broadcast_subscribe (connection, service_id, path);
	xmmsv_unref (path);

	xmmsc_result_notifier_set_default (result, on_odd_sum, NULL);
	xmmsc_result_unref (result);

	/* Let's now call the "sum" method!
	 * Start by building a list specifying its path...
	 */
	path = xmmsv_build_list (xmmsv_new_string ("Arithmetic"),
	                         xmmsv_new_string ("sum"),
	                         XMMSV_LIST_END);

	/* ...then a list of arguments... */
	pargs = xmmsv_build_list (xmmsv_new_int (op1),
	                          xmmsv_new_int (op2),
	                          XMMSV_LIST_END);

	/* ...and we're ready to make the call. */
	result = xmmsc_sc_call (connection, service_id,
	                        path, pargs, NULL);

	xmmsc_result_notifier_set (result, print_sum, ml);
	xmmsc_result_unref (result);

	xmmsc_mainloop_gmain_init (connection);
	g_main_loop_run (ml);

	xmmsv_unref (path);
	xmmsv_unref (pargs);
	xmmsc_unref (connection);
	return EXIT_SUCCESS;
}
