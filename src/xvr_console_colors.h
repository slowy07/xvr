#ifndef XVR_CONSOLE_COLORS_H
#define XVR_CONSOLE_COLORS_H

#if defined(__linux__) || defined(__MINGW32__) || defined(__GNUC__)

// fonts color
#define XVR_CC_FONT_BLACK "\033[30;"
#define XVR_CC_FONT_RED "\033[31;"
#define XVR_CC_FONT_GREEN "\033[32;"
#define XVR_CC_FONT_YELLOW "\033[33;"
#define XVR_CC_FONT_BLUE "\033[34;"
#define XVR_CC_FONT_PURPLE "\033[35;"
#define XVR_CC_FONT_DGREEN "\033[6;"
#define XVR_CC_FONT_WHITE "\033[7;"
#define XVR_CC_FONT_CYAN "\x1b[36m"

// background color
#define XVR_CC_BACK_BLACK "40m"
#define XVR_CC_BACK_RED "41m"
#define XVR_CC_BACK_GREEN "42m"
#define XVR_CC_BACK_YELLOW "43m"
#define XVR_CC_BACK_BLUE "44m"
#define XVR_CC_BACK_PURPLE "45m"
#define XVR_CC_BACK_DGREEN "46m"
#define XVR_CC_BACK_WHITE "47m"

// useful
#define XVR_CC_DEBUG XVR_CC_FONT_BLUE XVR_CC_BACK_BLACK
#define XVR_CC_NOTICE XVR_CC_FONT_GREEN XVR_CC_BACK_BLACK
#define XVR_CC_WARN XVR_CC_FONT_YELLOW XVR_CC_BACK_BLACK
#define XVR_CC_ERROR XVR_CC_FONT_RED XVR_CC_BACK_BLACK
#define XVR_CC_RESET "\033[0m"

// for unsupported platforms, these become no-ops
#else

// fonts color
#define XVR_CC_FONT_BLACK
#define XVR_CC_FONT_RED
#define XVR_CC_FONT_GREEN
#define XVR_CC_FONT_YELLOW
#define XVR_CC_FONT_BLUE
#define XVR_CC_FONT_PURPLE
#define XVR_CC_FONT_DGREEN
#define XVR_CC_FONT_WHITE
#define XVR_CC_FONT_CYAN

// background color
#define XVR_CC_BACK_BLACK
#define XVR_CC_BACK_RED
#define XVR_CC_BACK_GREEN
#define XVR_CC_BACK_YELLOW
#define XVR_CC_BACK_BLUE
#define XVR_CC_BACK_PURPLE
#define XVR_CC_BACK_DGREEN
#define XVR_CC_BACK_WHITE

// useful
#define XVR_CC_DEBUG XVR_CC_FONT_BLUE XVR_CC_BACK_BLACK
#define XVR_CC_NOTICE XVR_CC_FONT_GREEN XVR_CC_BACK_BLACK
#define XVR_CC_WARN XVR_CC_FONT_YELLOW XVR_CC_BACK_BLACK
#define XVR_CC_ERROR XVR_CC_FONT_RED XVR_CC_BACK_BLACK
#define XVR_CC_RESET

#endif // defined(__linux__) || defined(__MINGW32__) || defined(__GNUC__)

#endif // !XVR_CONSOLE_COLORS_H
