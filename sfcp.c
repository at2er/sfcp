#include <dbus/dbus.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ARGBOOL(E) ((E) ? "1" : "0")
#define LENGTH(X) (sizeof(X) / sizeof((X)[0]))
#define LISTEN(con, msg, iface, method, handler) \
	if (dbus_message_is_method_call((msg), (iface), (method))) \
		return (handler)((con), (msg))
#define PATH_MAX 4096
#define RANDTMP(max) (rand() % ((max) + 1))
#define TMPLEN 64
#define URIPREFIX "file://"

static const char *iface = "org.freedesktop.impl.portal.FileChooser";
static const unsigned int tmpidmax = 1000;

static DBusHandlerResult openfile(DBusConnection *con, DBusMessage *msg);
static int readuris(DBusMessageIter *iter, const char *tmp);
static int replyuris(DBusConnection *con, DBusMessage *msg, const char *tmp);
static DBusHandlerResult savefile(DBusConnection *con, DBusMessage *msg);
static void spawnfm(const char *mode, const char *tmp, int argc, const char **args);
static DBusHandlerResult handler(DBusConnection *con, DBusMessage *msg, void *data);

#include "config.h"

DBusHandlerResult
openfile(DBusConnection *con, DBusMessage *msg)
{
	DBusMessageIter iter, options, dict, value;
	dbus_bool_t multiple, directory = 0;
	char tmp[TMPLEN], *key;
	dbus_message_iter_init(msg, &iter);
	// skip 'osss', jump to a{sv}
	for (int i = 0; i < 4; i++)
		dbus_message_iter_next(&iter);
	dbus_message_iter_recurse(&iter, &options);
	while (dbus_message_iter_get_arg_type(&options) != DBUS_TYPE_INVALID) {
		dbus_message_iter_recurse(&options, &dict);
		dbus_message_iter_get_basic(&dict, &key);
		dbus_message_iter_next(&dict);
		dbus_message_iter_recurse(&dict, &value);
		if (strcmp(key, "multiple") == 0) {
			dbus_message_iter_get_basic(&value, &multiple);
		} else if (strcmp(key, "directory") == 0) {
			dbus_message_iter_get_basic(&value, &directory);
		}
		dbus_message_iter_next(&options);
	}
	srand(time(NULL));
	snprintf(tmp, TMPLEN, "/tmp/sfcp-%d", RANDTMP(tmpidmax));
	spawnfm("open", tmp, 2, (const char*[]){
			ARGBOOL(multiple),
			ARGBOOL(directory)});
	if (replyuris(con, msg, tmp))
		goto eopen;
	remove(tmp);
	return DBUS_HANDLER_RESULT_HANDLED;
eopen:
	remove(tmp);
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

int
readuris(DBusMessageIter *iter, const char *tmp)
{
	size_t offset = LENGTH(URIPREFIX) - 1, urilen;
	char *uri = calloc(PATH_MAX + LENGTH(URIPREFIX) + 1, sizeof(char));
	FILE *fp = fopen(tmp, "r");
	if (fp == NULL)
		return 0;
	strncpy(uri, URIPREFIX, LENGTH(URIPREFIX));
	while (fgets(&uri[offset], PATH_MAX, fp) != NULL) {
		urilen = strlen(uri);
		if (uri[urilen - 1] == '\n')
			uri[urilen - 1] = '\0';
		if (!dbus_message_iter_append_basic(iter,
					DBUS_TYPE_STRING, &uri))
			goto ecleanup;
	}
	free(uri);
	fclose(fp);
	return 0;
ecleanup:
	free(uri);
	fclose(fp);
	return 1;
}

int
replyuris(DBusConnection *con, DBusMessage *msg, const char *tmp)
{
	DBusMessageIter results, entry, dict, value, uris;
	DBusMessage *reply = dbus_message_new_method_return(msg);
	dbus_uint32_t response = 0;
	const char *keyuris = "uris";
	dbus_message_iter_init_append(reply, &results);
	dbus_message_iter_append_basic(&results, DBUS_TYPE_UINT32, &response);
	dbus_message_iter_open_container(&results,
			DBUS_TYPE_ARRAY, "{sv}",
			&entry);
	dbus_message_iter_open_container(&entry,
			DBUS_TYPE_DICT_ENTRY, NULL,
			&dict);
	dbus_message_iter_append_basic(&dict,
			DBUS_TYPE_STRING,
			&keyuris);
	dbus_message_iter_open_container(&dict,
			DBUS_TYPE_VARIANT, "as",
			&value);
	dbus_message_iter_open_container(&value,
			DBUS_TYPE_ARRAY, "s",
			&uris);
	if (readuris(&uris, tmp))
		goto ereaduris;
	dbus_message_iter_close_container(&value, &uris);
	dbus_message_iter_close_container(&dict, &value);
	dbus_message_iter_close_container(&entry, &dict);
	dbus_message_iter_close_container(&results, &entry);
	dbus_connection_send(con, reply, NULL);
	dbus_message_unref(reply);
	return 0;
ereaduris:
	dbus_message_iter_close_container(&value, &uris);
	dbus_message_iter_close_container(&dict, &value);
	dbus_message_iter_close_container(&entry, &dict);
	dbus_message_iter_close_container(&results, &entry);
	dbus_message_unref(reply);
	return 1;
}

DBusHandlerResult
savefile(DBusConnection *con, DBusMessage *msg)
{
	DBusMessageIter iter, options, dict, value;
	dbus_bool_t multiple;
	char tmp[TMPLEN], *key, *name;
	dbus_message_iter_init(msg, &iter);
	// skip 'osss', jump to a{sv}
	for (int i = 0; i < 4; i++)
		dbus_message_iter_next(&iter);
	dbus_message_iter_recurse(&iter, &options);
	while (dbus_message_iter_get_arg_type(&options) != DBUS_TYPE_INVALID) {
		dbus_message_iter_recurse(&options, &dict);
		dbus_message_iter_get_basic(&dict, &key);
		dbus_message_iter_next(&dict);
		dbus_message_iter_recurse(&dict, &value);
		if (strcmp(key, "multiple") == 0) {
			dbus_message_iter_get_basic(&value, &multiple);
		} else if (strcmp(key, "current_name") == 0) {
			dbus_message_iter_get_basic(&value, &name);
		}
		dbus_message_iter_next(&options);
	}
	srand(time(NULL));
	snprintf(tmp, TMPLEN, "/tmp/sfcp-%d", RANDTMP(tmpidmax));
	spawnfm("save", tmp, 2, (const char *[]){
			ARGBOOL(multiple),
			name});
	if (replyuris(con, msg, tmp))
		goto eopen;
	remove(tmp);
	return DBUS_HANDLER_RESULT_HANDLED;
eopen:
	remove(tmp);
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

void
spawnfm(const char *mode, const char *tmp, int argc, const char **args)
{
	char **cmd;
	int i, ret = 0;
	if ((ret = fork()) > 0) {
		wait(NULL);
		return;
	}
	if (ret < 0) {
		fprintf(stderr, "sfcp: failed to fork wrap\n");
		exit(1);
	}
	cmd = calloc(LENGTH(wrap) + argc + 1, sizeof(char*));
	cmd[0] = (char*)wrap;
	cmd[1] = (char*)mode;
	cmd[2] = (char*)tmp;
	for (i = 0; i < argc; i++)
		cmd[i + 3] = (char*)args[i];
	cmd[i + 4] = NULL;
	setsid();
	execvp(cmd[0], cmd);
	fprintf(stderr, "sfcp: failed to execvp wrap %s\n", cmd[0]);
	exit(1);
}

DBusHandlerResult
handler(DBusConnection *con, DBusMessage *msg, void *data)
{
	LISTEN(con, msg, iface, "OpenFile", openfile);
	LISTEN(con, msg, iface, "SaveFile", savefile);
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

int
main(int argc, char *argv[])
{
	DBusConnection *con = dbus_bus_get(DBUS_BUS_SESSION, NULL);
	DBusObjectPathVTable vtable = {
		.unregister_function = NULL,
		.message_function = handler,
	};

	if (!con) {
		fprintf(stderr, "Failed to connect to D-Bus session bus\n");
		return 1;
	}

	dbus_bus_request_name(con,
			"org.freedesktop.impl.portal.desktop.sfcp",
			DBUS_NAME_FLAG_REPLACE_EXISTING,
			NULL);
	dbus_connection_register_object_path(con,
			"/org/freedesktop/portal/desktop",
			&vtable,
			NULL);

	while (dbus_connection_read_write_dispatch(con, -1));
}
