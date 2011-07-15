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
 *
 *  This file implements the low-level 'sumserver' service client.
 *  It accepts messages containing a list of two operands, and replies
 *  with the sum of these operands.
 *  Should be run/studied along with tut10 aka sumclient.
 */

#include <stdlib.h>
#include <xmmsclient/xmmsclient.h>
#include <xmmsclient/xmmsclient-glib.h>

#include <glib.h>

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
	        "Pass this id in the command line for tut10.\n", id);

	xmmsc_result_unref (result);
	return;
}

/* Client-to-client messages come with a special broadcast, so
 * in order to receive them we must set up a callback to that broadcast.
 * This is the callback for this client. It parses two operands out of
 * the client-to-client message and replies with their sum.
 */
int
sum (xmmsv_t *c2c_msg, void *userdata)
{
	int op1, op2;
	xmmsc_result_t *res;
	xmmsv_t *operands, *sum;
	xmmsc_connection_t *connection;

	connection = (xmmsc_connection_t *) userdata;

	/* Get the payload list of operands from the message.
	 * It is the payload sent by sumclient.
	 */
	operands = xmmsv_c2c_message_payload_get (c2c_msg);
	if (!operands) {
		fprintf (stderr, "Can't get operands from c2c message!\n");
		return TRUE;
	}

	/* Get the two operands from the list */
	if (!xmmsv_list_get_int (operands, 0, &op1)) {
		fprintf (stderr, "Failed to get first operand.\n");
		return TRUE;
	}

	if (!xmmsv_list_get_int (operands, 1, &op2)) {
		fprintf (stderr, "Failed to get second operand.\n");
		return TRUE;
	}

	/* Build a new xmmsv with the sum of the operands
	 * and send it back to sumclient as a reply.
	 * We specify that we are answering to this particular
	 * message by passing its id (which we retrieve using
	 * xmmsv_c2c_message_id_get), and use a reply policy
	 * of not expecting counter-replies. This means that the
	 * other client will get an error from the server if it
	 * attempts to reply to our reply.
	 */
	sum = xmmsv_new_int (op1 + op2);
	res = xmmsc_c2c_reply (connection,
	                       xmmsv_c2c_message_id_get (c2c_msg),
	                       XMMS_C2C_REPLY_POLICY_NO_REPLY,
	                       sum);

	/* Replies, being themselves client-to-client messages,
	 * are also associated with results (so you can set up a
	 * callback for the counter-reply).
	 * Since there will be no more replies, just unref the result.
	 */
	xmmsc_result_unref (res);

	return TRUE;
}

int
main (int argc, char **argv)
{
	GMainLoop *ml;
	xmmsc_result_t *result;
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

	/* Set up the c2c message broadcast.
	 * This allows us to receive messages sent by other clients to our id.
	 * We are going to need the connection to the server in order to reply,
	 * so pass it as user data.
	 */
	result = xmmsc_broadcast_c2c_message (connection);
	xmmsc_result_notifier_set_c2c (result, sum, connection);
	xmmsc_result_unref (result);

	xmmsc_mainloop_gmain_init (connection);
	g_main_loop_run (ml);

	xmmsc_unref (connection);
	return EXIT_SUCCESS;
}
