/**
 * Aurora OS - Theme and Wallpaper System Tests Header
 */

#ifndef THEME_WALLPAPER_TESTS_H
#define THEME_WALLPAPER_TESTS_H

/**
 * Run theme manager tests
 */
void run_theme_manager_tests(void);

/**
 * Run wallpaper manager tests
 */
void run_wallpaper_manager_tests(void);

/**
 * Run integration tests
 */
void run_integration_tests(void);

/**
 * Run all theme and wallpaper tests
 * @return Number of failed tests
 */
int run_all_theme_wallpaper_tests(void);

/**
 * Get number of passed tests
 * @return Number of passed tests
 */
int get_theme_wallpaper_tests_passed(void);

/**
 * Get number of failed tests
 * @return Number of failed tests
 */
int get_theme_wallpaper_tests_failed(void);

#endif // THEME_WALLPAPER_TESTS_H
