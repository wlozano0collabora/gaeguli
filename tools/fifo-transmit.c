/**
 *  Copyright 2019 SK Telecom, Co., Ltd.
 *    Author: Jeongseok Kim <jeongseok.kim@sk.com>
 *
 */

#include "config.h"

#include "fifo-transmit.h"

#include <gio/gio.h>

typedef struct
{
  gboolean help;
  const gchar *host;
  guint port;
  GaeguliSRTMode mode;
  const gchar *tmpdir;
} FifoTransmitOptions;

static void
activate (GApplication * app, gpointer user_data)
{
  g_autoptr (GError) error = NULL;
  FifoTransmitOptions *options = NULL;
  guint transmit_id;
  GaeguliFifoTransmit *fifo_transmit = user_data;

  options = g_object_get_data (G_OBJECT (app), "options");

  transmit_id =
      gaeguli_fifo_transmit_start (fifo_transmit, options->host, options->port,
      options->mode, &error);

  g_object_set_data (G_OBJECT (app), "transmit-id",
      GINT_TO_POINTER (transmit_id));

  g_application_hold (app);
}

static gboolean
mode_arg_cb (const gchar * option_name, const gchar * value, gpointer data,
    GError ** error)
{
  FifoTransmitOptions *options = (FifoTransmitOptions *) data;

  if (g_str_equal (value, "caller")) {
    options->mode = GAEGULI_SRT_MODE_CALLER;
  } else if (g_str_equal (value, "listener")) {
    options->mode = GAEGULI_SRT_MODE_LISTENER;
  } else {
    *error = g_error_new (GAEGULI_RESOURCE_ERROR,
        GAEGULI_RESOURCE_ERROR_UNSUPPORTED, "Unknown SRT mode '%s'", value);
    return FALSE;
  }

  return TRUE;
}

int
main (int argc, char *argv[])
{
  FifoTransmitOptions options;
  g_autoptr (GError) error = NULL;
  g_autoptr (GApplication) app =
      g_application_new ("org.hwangsaeul.Gaeguli1.FifoTransmitApp", 0);

  g_autoptr (GOptionGroup) group = NULL;
  g_autoptr (GOptionContext) context = NULL;
  GOptionEntry entries[] = {
    {"host", 'h', 0, G_OPTION_ARG_STRING, &options.host, NULL, NULL},
    {"port", 'p', 0, G_OPTION_ARG_INT, &options.port, NULL, NULL},
    {"mode", 'm', 0, G_OPTION_ARG_CALLBACK, mode_arg_cb, NULL, NULL},
    {"tmpdir", 't', 0, G_OPTION_ARG_FILENAME, &options.tmpdir, NULL, NULL},
    {"help", '?', 0, G_OPTION_ARG_NONE, &options.help, NULL, NULL},
    {NULL}
  };

  g_autoptr (GaeguliFifoTransmit) fifo_transmit = NULL;

  options.help = FALSE;
  options.host = NULL;
  options.port = 8888;
  options.mode = GAEGULI_SRT_MODE_LISTENER;
  options.tmpdir = NULL;

  group = g_option_group_new ("FIFO transmit options",
      "Options understood by Gaeguli FIFO transmit", NULL, &options, NULL);
  g_option_group_add_entries (group, entries);

  context = g_option_context_new (NULL);
  g_option_context_set_help_enabled (context, FALSE);
  g_option_context_set_main_group (context, group);

  if (!g_option_context_parse (context, &argc, &argv, &error)) {
    g_printerr ("%s\n", error->message);
    return -1;
  }

  if (options.help) {
    g_autofree gchar *text = g_option_context_get_help (context, FALSE, NULL);
    g_printerr ("%s\n", text);
    return -1;
  }

  if (options.tmpdir) {
    /* TODO: set tmpdir for fifo */
  }

  fifo_transmit = gaeguli_fifo_transmit_new ();
  g_printerr ("Send bytestream to: %s",
      gaeguli_fifo_transmit_get_fifo (fifo_transmit));

  g_signal_connect (app, "activate", G_CALLBACK (activate), fifo_transmit);

  g_object_set_data (G_OBJECT (app), "options", &options);

  return g_application_run (app, argc, argv);
}
