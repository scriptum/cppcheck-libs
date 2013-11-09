#include <glib.h>

int main(int argc, char **argv)
{
	gchar *a = g_strdup("Hello world");
	GString *s = g_string_new(a);
	g_string_free(s, TRUE);
	g_free(a);
}