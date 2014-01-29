void f1() {
	if(TRUE || strcmp(strdup(a), b))
	if(!strcmp(strdup(a), b) == 0)
	42, strcmp(strdup(a), b);
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
