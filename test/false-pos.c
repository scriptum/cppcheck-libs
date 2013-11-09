static void
load_config (GtkImContextMultipress *self)
{
  GKeyFile *key_file;
  GError   *error = NULL;
  gchar   **keys;
  gsize     n_keys = 0;
  gsize     i;

  key_file = g_key_file_new ();

  if (!g_key_file_load_from_file (key_file, CONFIGURATION_FILENAME,
                                  G_KEY_FILE_NONE, &error))
    {
      g_warning ("Error while trying to open the %s configuration file: %s",
                 CONFIGURATION_FILENAME, error->message);
      g_error_free (error);
      g_key_file_free (key_file);
      return;
    }

  keys = g_key_file_get_keys (key_file, "keys", &n_keys, &error);

  if (error != NULL)
    {
      g_warning ("Error while trying to read the %s configuration file: %s",
                 CONFIGURATION_FILENAME, error->message);
      g_error_free (error);
      g_key_file_free (key_file);
      return;
    }

  for (i = 0; i < n_keys; ++i)
    {
      KeySequence *seq;
      guint        keyval;

      keyval = gdk_keyval_from_name (keys[i]);

      if (keyval == GDK_VoidSymbol)
        {
          g_warning ("Error while trying to read the %s configuration file: "
                     "invalid key name \"%s\"",
                     CONFIGURATION_FILENAME, keys[i]);
          continue;
        }

      seq = g_slice_new (KeySequence);
      seq->characters = g_key_file_get_string_list (key_file, "keys", keys[i],
                                                    &seq->n_characters, &error);
      if (error != NULL)
        {
          g_warning ("Error while trying to read the %s configuration file: %s",
                     CONFIGURATION_FILENAME, error->message);
          g_error_free (error);
          error = NULL;
          g_slice_free (KeySequence, seq);
          continue;
        }

      /* Ownership of the KeySequence is taken over by the hash table */
      g_hash_table_insert (self->key_sequences, GUINT_TO_POINTER (keyval), seq);
    }

  g_strfreev (keys);
  g_key_file_free (key_file);
}

