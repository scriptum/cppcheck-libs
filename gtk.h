/* handle g_return* */
#define g_return_if_fail(expr) do{if(!(expr)){return;}}while(0)
#define g_return_val_if_fail(expr, val) do{if(!(expr)){return val;}}while(0)
#define g_return_if_reached() do{return;}while(0)
#define g_return_val_if_reached(val) do{return val;}while(0)
