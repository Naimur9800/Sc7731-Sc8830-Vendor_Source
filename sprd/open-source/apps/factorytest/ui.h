#ifndef __UI_H__
#define __UI_H__

#define NO_ACTION           -1
#define HIGHLIGHT_UP        -2
#define HIGHLIGHT_DOWN      -3
#define SELECT_ITEM         -4
#define GO_BACK             -5

enum UI_COLOR {
	CL_WHITE,
	CL_BLACK,
	CL_RED,
	CL_BLUE,
	CL_GREEN,
	CL_SCREEN_BG,
	CL_SCREEN_FG,
	CL_TITLE_BG,
	CL_TITLE_FG,
	CL_MENU_HL_BG,
	CL_MENU_HL_FG,
};

void ui_fill_locked(void);
void ui_show_title(const char* title);
int ui_show_text(int row, int col, const char* text);
int ui_handle_button(const char* left,  const char* right);//const char* centor,
void ui_show_button(const char* left,  const char* right);//const char* centor,
void ui_fill_screen(unsigned char r, unsigned char g, unsigned char b);
void ui_clear_rows(int start, int n);
void ui_clear_rows_cols(int row_start, int n1,int col_start,int n2);
int ui_show_menu(char* title, char** items, int is_root, int sel,int itemnum);

void ui_set_color(int cl);
void ui_draw_line(int x1,int y1,int x2,int y2);
void ui_draw_line_mid(int x1,int y1,int x2,int y2);
void ui_push_result(int result);

#endif

