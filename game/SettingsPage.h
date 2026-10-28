#ifndef OPENCMR2_SETTINGS_PAGE_H
#define OPENCMR2_SETTINGS_PAGE_H
// Seven unchanged36px rows fit between the breadcrumb and384px carousel.
constexpr int SETTINGS_PAGE_ROWS = 7;
inline int SettingsPage_First(int cursor) {
    return cursor < 0 ? 0 : cursor / SETTINGS_PAGE_ROWS * SETTINGS_PAGE_ROWS;
}
inline int SettingsPage_End(int count,int cursor) {
    int end=SettingsPage_First(cursor)+SETTINGS_PAGE_ROWS;
    return end<count?end:count;
}
inline int SettingsPage_ClampPercent(int value) {return value<0?0:value>100?100:value;}
#endif
