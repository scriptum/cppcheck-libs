/* handle g_return* */
#define g_return_if_fail(expr) do{if(!(expr)){return;}}while(0)
#define g_return_val_if_fail(expr, val) do{if(!(expr)){return val;}}while(0)
#define g_return_if_reached() do{return;}while(0)
#define g_return_val_if_reached(val) do{return val;}while(0)
/* replace g_list_sort coz it just returns first pointer */
#define g_list_sort(a, b) a
#define g_list_sort_with_data(a, b, c) a