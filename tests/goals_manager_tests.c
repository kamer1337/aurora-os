/**
 * Aurora OS - Goals Manager Tests
 * 
 * Tests for the Long-Term Goals Manager application
 */

#include "goals_manager_tests.h"
#include "../kernel/gui/goals_manager.h"
#include "../kernel/drivers/vga.h"

// Test statistics
static uint32_t tests_passed = 0;
static uint32_t tests_failed = 0;

// Helper function to print test result
static void print_test_result(const char* test_name, int passed) {
    vga_write("[");
    if (passed) {
        vga_write("PASS");
        tests_passed++;
    } else {
        vga_write("FAIL");
        tests_failed++;
    }
    vga_write("] ");
    vga_write(test_name);
    vga_write("\n");
}

/**
 * Test goals manager initialization
 */
static int test_goals_manager_init(void) {
    goals_manager_init();
    
    // Check that goals were loaded
    uint32_t count = goals_manager_get_count();
    if (count == 0) return 0;
    
    // Check that some goals are marked as completed
    uint32_t completed = goals_manager_get_completed_count();
    if (completed == 0) return 0;
    
    return 1;
}

/**
 * Test adding a new goal
 */
static int test_add_goal(void) {
    uint32_t initial_count = goals_manager_get_count();
    
    int result = goals_manager_add_goal("Test Goal", "Test Category", GOAL_STATUS_PENDING);
    if (result != 0) return 0;
    
    uint32_t new_count = goals_manager_get_count();
    if (new_count != initial_count + 1) return 0;
    
    return 1;
}

/**
 * Test toggling goal status
 */
static int test_toggle_goal(void) {
    goals_manager_init();
    
    // Get initial completed count
    uint32_t initial_completed = goals_manager_get_completed_count();
    
    // Find a pending goal and toggle it
    uint32_t count = goals_manager_get_count();
    for (uint32_t i = 0; i < count; i++) {
        goal_t* goal = goals_manager_get_goal(i);
        if (goal && goal->status == GOAL_STATUS_PENDING) {
            goals_manager_toggle_goal(i);
            
            // Check if it was marked as completed
            goal = goals_manager_get_goal(i);
            if (goal->status != GOAL_STATUS_COMPLETED) return 0;
            
            // Check completed count increased
            if (goals_manager_get_completed_count() != initial_completed + 1) return 0;
            
            // Toggle back
            goals_manager_toggle_goal(i);
            if (goal->status != GOAL_STATUS_PENDING) return 0;
            
            return 1;
        }
    }
    
    return 0;
}

/**
 * Test getting goal by index
 */
static int test_get_goal(void) {
    goals_manager_init();
    
    uint32_t count = goals_manager_get_count();
    if (count == 0) return 0;
    
    // Get first goal
    goal_t* goal = goals_manager_get_goal(0);
    if (!goal) return 0;
    
    // Check that goal has valid data
    if (goal->name[0] == '\0') return 0;
    if (goal->category[0] == '\0') return 0;
    
    // Test invalid index
    goal = goals_manager_get_goal(count + 10);
    if (goal != 0) return 0;  // Should return NULL for invalid index
    
    return 1;
}

/**
 * Test goals manager scroll functionality
 */
static int test_scroll(void) {
    goals_manager_init();
    
    // Scroll down
    goals_manager_scroll(5);
    
    // Scroll up
    goals_manager_scroll(-3);
    
    // Test boundary conditions
    goals_manager_scroll(-1000);  // Should clamp to 0
    goals_manager_scroll(1000);   // Should clamp to max
    
    return 1;
}

/**
 * Test goals manager filter toggle
 */
static int test_filter_toggle(void) {
    goals_manager_init();
    
    // Toggle filter
    goals_manager_toggle_filter();
    
    // Toggle back
    goals_manager_toggle_filter();
    
    return 1;
}

/**
 * Run all goals manager tests
 */
void goals_manager_run_tests(void) {
    vga_write("\n========================================\n");
    vga_write("Goals Manager Tests\n");
    vga_write("========================================\n\n");
    
    tests_passed = 0;
    tests_failed = 0;
    
    // Run tests
    print_test_result("Goals Manager Initialization", test_goals_manager_init());
    print_test_result("Add Goal", test_add_goal());
    print_test_result("Toggle Goal Status", test_toggle_goal());
    print_test_result("Get Goal by Index", test_get_goal());
    print_test_result("Scroll Functionality", test_scroll());
    print_test_result("Filter Toggle", test_filter_toggle());
    
    // Print summary
    vga_write("\n========================================\n");
    vga_write("Test Summary:\n");
    
    char buffer[64];
    vga_write("  Total Tests: ");
    uint32_t total = tests_passed + tests_failed;
    // Simple number to string
    int pos = 0;
    if (total == 0) {
        buffer[pos++] = '0';
    } else {
        char rev[32];
        int rev_pos = 0;
        uint32_t temp = total;
        while (temp > 0) {
            rev[rev_pos++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = rev_pos - 1; i >= 0; i--) {
            buffer[pos++] = rev[i];
        }
    }
    buffer[pos] = '\0';
    vga_write(buffer);
    vga_write("\n");
    
    vga_write("  Passed: ");
    pos = 0;
    if (tests_passed == 0) {
        buffer[pos++] = '0';
    } else {
        char rev[32];
        int rev_pos = 0;
        uint32_t temp = tests_passed;
        while (temp > 0) {
            rev[rev_pos++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = rev_pos - 1; i >= 0; i--) {
            buffer[pos++] = rev[i];
        }
    }
    buffer[pos] = '\0';
    vga_write(buffer);
    vga_write("\n");
    
    vga_write("  Failed: ");
    pos = 0;
    if (tests_failed == 0) {
        buffer[pos++] = '0';
    } else {
        char rev[32];
        int rev_pos = 0;
        uint32_t temp = tests_failed;
        while (temp > 0) {
            rev[rev_pos++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = rev_pos - 1; i >= 0; i--) {
            buffer[pos++] = rev[i];
        }
    }
    buffer[pos] = '\0';
    vga_write(buffer);
    vga_write("\n");
    
    vga_write("========================================\n\n");
}
