/**
 *  Copyright 2019 SK Telecom, Co., Ltd.
 *    Author: Jeongseok Kim <jeongseok.kim@sk.com>
 *
 */

#include "config.h"

#include "pipeline.h"

#include <glib-unix.h>
#include <gio/gio.h>
#include <gst/gst.h>

static guint signal_watch_intr_id;
static guint target_id;

static void
activate (GApplication * app, gpointer user_data)
{
  g_autoptr (GError) error = NULL;
  const gchar *fifo = NULL;

  GaeguliPipeline *pipeline = user_data;

  g_application_hold (app);

  fifo = g_object_get_data (G_OBJECT (app), "fifo");
  target_id = gaeguli_pipeline_add_fifo_target (pipeline, fifo, &error);
}

static gboolean
intr_handler (gpointer user_data)
{
  GaeguliPipeline *pipeline = user_data;
  g_autoptr (GError) error = NULL;

  gaeguli_pipeline_remove_target (pipeline, target_id, &error);

  g_debug ("target removed");

  return G_SOURCE_REMOVE;
}

static void
stream_stopped_cb (GaeguliPipeline * pipeline, guint target_id,
    gpointer user_data)
{
  GApplication *app = user_data;

  g_debug ("stream stopped");
  g_application_release (app);
}

int
main (int argc, char *argv[])
{
  gboolean help = FALSE;
  const gchar *fifo = NULL;
  int result;

  g_autoptr (GError) error = NULL;
  g_autoptr (GApplication) app =
      g_application_new ("org.hwangsaeul.Gaeguli1.PipelineApp", 0);

  g_autoptr (GOptionContext) context = NULL;
  GOptionEntry entries[] = {
    {"fifo", 'f', 0, G_OPTION_ARG_FILENAME, &fifo, NULL, NULL},
    {"help", '?', 0, G_OPTION_ARG_NONE, &help, NULL, NULL},
    {NULL}
  };

  g_autoptr (GaeguliPipeline) pipeline = gaeguli_pipeline_new ();

  gst_init (&argc, &argv);

  context = g_option_context_new (NULL);
  g_option_context_set_help_enabled (context, FALSE);
  g_option_context_add_main_entries (context, entries, NULL);

  if (!g_option_context_parse (context, &argc, &argv, &error)) {
    g_printerr ("%s\n", error->message);
    return -1;
  }

  if (help) {
    g_autofree gchar *text = g_option_context_get_help (context, FALSE, NULL);
    g_printerr ("%s\n", text);
    return -1;
  }

  signal_watch_intr_id =
      g_unix_signal_add (SIGINT, (GSourceFunc) intr_handler, pipeline);

  g_signal_connect (pipeline, "stream-stopped", G_CALLBACK (stream_stopped_cb),
      app);

  g_signal_connect (app, "activate", G_CALLBACK (activate), pipeline);
  g_object_set_data_full (G_OBJECT (app), "fifo", g_strdup (fifo), g_free);

  result = g_application_run (app, argc, argv);

  gaeguli_pipeline_stop (pipeline);

  if (signal_watch_intr_id > 0)
    g_source_remove (signal_watch_intr_id);

  return result;
}
