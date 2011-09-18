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

/*  This file implements the high-level 'sumserver' service client.
 */

#include <stdlib.h>
#include <xmmsclient/xmmsclient.h>
#include <xmmsclient/xmmsclient-glib.h>

#include <glib.h>

GMainLoop *ml;

/* Each connected client is uniquely identified by a numeric
 * id in the server. A client's id should be used in order to
 * send messages to it.
 * The server provides a command for a client to consult its own id.
 * This is a simple command given to the server, no client-to-client
 * messages are involved just yet.
 * Here we fetch and print the id assigned by the server to this client.
 * This should be passed in the command line for tut10.
 */
void
print_id (xmmsc_connection_t *connection)
{
	gint32 id;
	xmmsv_t *idv;
	xmmsc_result_t *result;

	result = xmmsc_c2c_get_own_id (connection);
	xmmsc_result_wait (result);

	idv = xmmsc_result_get_value (result);
	xmmsv_get_int (idv, &id);

	printf ("Connected as id %d.\n"
	        "Pass this id in the command line for tut12.\n", id);

	xmmsc_result_unref (result);
	return;
}

/* A sum method.
 * This method expects two integers as positional arguments, sums them and
 * replies with the result.
 * If the sum is odd, the "odd sum" broadcast is emitted, also carrying the
 * result.
 */
xmmsv_t *
sum (xmmsv_t *positional_args, xmmsv_t *named_args, void *userdata)
{
	xmmsv_t *sumval;
	int op1, op2, sum;
	xmmsc_connection_t *connection;

	connection = (xmmsc_connection_t *) userdata;

	/* Get the two operands from the list */
	xmmsv_list_get_int (positional_args, 0, &op1);
	xmmsv_list_get_int (positional_args, 1, &op2);

	sum = op1 + op2;
	sumval = xmmsv_new_int (sum);

	if (sum % 2) {
		xmmsv_t *broadcast_name;

		broadcast_name = xmmsv_build_list (XMMSV_LIST_ENTRY_STR ("odd sum"),
		                                   XMMSV_LIST_END);
		xmmsc_sc_broadcast_emit (connection, broadcast_name, sumval);
		xmmsv_unref (broadcast_name);
	}

	return sumval;
}

int
main (int argc, char **argv)
{
	xmmsv_t *pargs, *pi;
	xmmsc_sc_namespace_t *arithmetic;
	xmmsc_connection_t *connection;

	/* Connect as usual */
	connection = xmmsc_init ("sumservice");
	if (!connection) {
		fprintf (stderr, "OOM!\n");
		exit (EXIT_FAILURE);
	}

	if (!xmmsc_connect (connection, getenv ("XMMS_PATH"))) {
		fprintf (stderr, "Connection failed: %s\n",
		         xmmsc_get_last_error (connection));

		exit (EXIT_FAILURE);
	}

	/* Output the id assigned to this client. */
	print_id(connection);

	ml = g_main_loop_new (NULL, FALSE);

	/* Create a namespace to hold our "sum" method.
	 * The new namespace will be called "Arithmetic" and will be nested under
	 * the root namespace in the connection structure.
	 */
	arithmetic = xmmsc_sc_namespace_new (connection,
	                                     NULL,
	                                     "Arithmetic",
	                                     "A namespace for "
	                                     "arithmetic methods.");

	/* Add a weird constant to the "Arithmetic" namespace. */
	pi = xmmsv_new_int (3);
	xmmsc_sc_namespace_add_constant (arithmetic, "pi", pi);
	xmmsv_unref (pi);

	/* Create the list of positional arguments for "sum".
	 * Each argument is expected to be an integer with no default value.
	 */
	pargs = xmmsv_build_list (xmmsv_sc_argument_new ("op1",
	                                                 "the first operand",
	                                                 XMMSV_TYPE_INT32,
	                                                 NULL),
	                          xmmsv_sc_argument_new ("op2",
	                                                 "the second operand",
	                                                 XMMSV_TYPE_INT32,
	                                                 NULL),
	                          XMMSV_LIST_END);

	/* And expose "sum" for remote calls. */
	xmmsc_sc_method_new_positional (connection,
	                                arithmetic, sum,
	                                "sum", "sum(op1, op2) -> op1 + op2",
	                                pargs, false, false, connection);

	/* Finally, define the "odd sum" broadcast. */
	xmmsc_sc_broadcast_new (connection,
	                        NULL, "odd sum",
	                        "This broadcast gets triggered whenever an odd "
	                        "number results from a sum.");

	xmmsc_sc_setup (connection);

	xmmsc_mainloop_gmain_init (connection);
	g_main_loop_run (ml);

	xmmsv_unref (pi);
	xmmsv_unref (pargs);
	xmmsc_unref (connection);
	return EXIT_SUCCESS;
}
