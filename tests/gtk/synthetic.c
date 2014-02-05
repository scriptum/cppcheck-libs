void f1() {
	if(TRUE || g_strcmp0(g_strdup(a), b))
	if(!g_strlcpy(g_strdup(a), b) == 0)
	42, g_strcmp0(g_strdup(a), b);
	if(g_malloc(42));
}

void f2(gchar *_a, gchar *_b) {
	g_return_if_fail(_a);
	gchar *a = g_strdup(_a);
	g_return_if_fail(_b);
	gchar *b = g_strdup(_b);
	g_free(a);
	g_free(b);
}

void f3(gchar *_a) {
	gchar *a = g_strdup(_a);
	free(a);
}
