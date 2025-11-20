# Aurora OS - Beta Testing Guide

**Version**: 1.0-RC1  
**Last Updated**: November 20, 2025  
**Status**: Ready for Beta Testing

---

## Welcome Beta Testers!

Thank you for participating in the Aurora OS beta testing program. Your feedback is crucial for making Aurora OS production-ready. This guide will help you get started with testing and provide effective feedback.

---

## Getting Started

### System Requirements

**Minimum Requirements:**
- CPU: x86 32-bit processor (Intel Core 2 Duo or equivalent)
- RAM: 128 MB
- Graphics: VESA/VBE compatible framebuffer
- Storage: 100 MB free space

**Recommended Requirements:**
- CPU: x86 64-bit processor (compatibility mode)
- RAM: 512 MB or more
- Graphics: Hardware framebuffer with VESA 2.0+
- Storage: 1 GB free space

### Installation

1. **Download the ISO**:
   ```bash
   wget https://github.com/kamer1337/aurora-os/releases/download/v1.0-rc1/aurora-os-1.0-rc1.iso
   ```

2. **Verify checksum**:
   ```bash
   sha256sum aurora-os-1.0-rc1.iso
   # Compare with official checksum
   ```

3. **Test in QEMU** (Recommended for initial testing):
   ```bash
   qemu-system-i386 -cdrom aurora-os-1.0-rc1.iso -m 512M -enable-kvm
   ```

4. **Install on Physical Hardware** (Advanced):
   - Create bootable USB: `dd if=aurora-os-1.0-rc1.iso of=/dev/sdX bs=4M`
   - Boot from USB and follow installation prompts

---

## What to Test

### Priority Areas

#### 1. System Boot and Stability
- [ ] System boots successfully
- [ ] Boot time (target: < 10 seconds)
- [ ] No kernel panics or crashes
- [ ] System runs stable for extended periods (1+ hours)

#### 2. GUI and Desktop Environment
- [ ] Desktop renders correctly at 1920x1080 resolution
- [ ] GUI performance (should maintain ~125 FPS)
- [ ] Window management (create, drag, resize, minimize, maximize, close)
- [ ] Start menu functionality
- [ ] Taskbar window list
- [ ] Desktop icons
- [ ] Visual effects (3D/4D/5D effects)

#### 3. Applications
Test each bundled application:
- [ ] **Calculator**: Perform basic calculations
- [ ] **Text Editor**: Create, edit, save files
- [ ] **File Explorer**: Browse files and directories
- [ ] **Terminal**: Execute basic commands
- [ ] **Clock**: Display correct time
- [ ] **Music Player**: (if media files available)
- [ ] **Video Player**: (if media files available)
- [ ] **Goals Manager**: Track project goals
- [ ] **System Settings**: Access and modify settings
- [ ] **Application Store**: Browse and install packages

#### 4. New Advanced Features
- [ ] **Virtual Desktops**: Switch between workspaces (Ctrl+Alt+1-4)
- [ ] **Window Snapping**: Drag windows to edges for tiling
- [ ] **Package Manager**: Install/remove applications
- [ ] **System Settings**: Configure display, theme, user preferences

#### 5. File System
- [ ] Create files and directories
- [ ] Read and write data
- [ ] Delete files and directories
- [ ] File system integrity after crash recovery

#### 6. Input Devices
- [ ] Keyboard input (typing, special keys)
- [ ] Mouse movement and clicking
- [ ] Right-click context menus

#### 7. Security
- [ ] Login screen authentication
- [ ] User session management
- [ ] Quantum cryptography features (if applicable)

---

## How to Report Issues

### Using GitHub Issues

1. **Go to**: https://github.com/kamer1337/aurora-os/issues
2. **Click**: "New Issue"
3. **Select**: "Bug Report" template
4. **Fill in**:
   - Clear, descriptive title
   - Steps to reproduce
   - Expected behavior
   - Actual behavior
   - System configuration
   - Screenshots/logs if applicable

### Bug Report Template

```markdown
**Summary**: [Brief description of the issue]

**Steps to Reproduce**:
1. [First step]
2. [Second step]
3. [...]

**Expected Result**: [What should happen]

**Actual Result**: [What actually happens]

**System Configuration**:
- Aurora OS Version: [e.g., 1.0-RC1]
- Hardware: [CPU, RAM, Graphics]
- Installation Method: [QEMU/Physical hardware]

**Additional Information**:
- Screenshots: [Attach if relevant]
- Logs: [Include error messages]
- Frequency: [Always/Sometimes/Once]
```

### Issue Severity

Please label issues appropriately:
- **Critical**: System crash, data loss, security vulnerability
- **High**: Major feature broken, severe performance issue
- **Medium**: Feature partially works, minor performance issue
- **Low**: Cosmetic issue, minor inconvenience

---

## Feedback Categories

### Performance Feedback

- Boot time
- GUI rendering speed (FPS)
- Application launch time
- File system operations speed
- Memory usage
- System responsiveness

### Usability Feedback

- Ease of navigation
- Clarity of UI elements
- Intuitiveness of features
- Accessibility
- Documentation quality

### Feature Requests

- Missing features you'd like to see
- Improvements to existing features
- New application ideas
- Workflow enhancements

---

## Testing Scenarios

### Scenario 1: Daily Desktop Use
**Duration**: 2 hours  
**Goal**: Use Aurora OS as your primary desktop

1. Boot the system
2. Launch multiple applications
3. Create and edit documents
4. Browse files
5. Switch between applications
6. Test window management features
7. Use system settings
8. Shut down properly

**Report**: Overall experience, any issues encountered

### Scenario 2: Stress Testing
**Duration**: 1 hour  
**Goal**: Test system stability under load

1. Open 10+ windows simultaneously
2. Perform rapid window switching
3. Create large files
4. Test with animated visual effects enabled
5. Leave system idle for 30 minutes
6. Return and test responsiveness

**Report**: Stability, performance degradation, memory leaks

### Scenario 3: New Features Testing
**Duration**: 1 hour  
**Goal**: Test advanced features

1. Test virtual desktops (create, switch, move windows)
2. Test window snapping (all edge positions)
3. Install/uninstall packages via App Store
4. Customize settings in System Settings
5. Test keyboard shortcuts

**Report**: Feature functionality, bugs, suggestions

---

## Communication Channels

### Primary Channels

1. **GitHub Issues**: For bug reports and feature requests
   - https://github.com/kamer1337/aurora-os/issues

2. **GitHub Discussions**: For general questions and feedback
   - https://github.com/kamer1337/aurora-os/discussions

3. **Beta Tester Survey**: Weekly feedback form
   - [Survey link to be provided]

### Response Time

- Critical issues: Within 24 hours
- High priority: Within 48 hours
- Medium/Low priority: Within 1 week

---

## Beta Testing Schedule

### Phase 1: Internal Alpha (Completed)
- **Duration**: 2 weeks (Nov 1-15, 2025)
- **Status**: ✅ Complete
- **Result**: 100% test pass rate, zero memory leaks

### Phase 2: Closed Beta (Current)
- **Duration**: 4 weeks (Nov 20 - Dec 18, 2025)
- **Participants**: 10-20 early adopters
- **Focus**: Core functionality, stability, usability

### Phase 3: Open Beta
- **Duration**: 4 weeks (Jan 2026)
- **Participants**: Open to public
- **Focus**: Broad compatibility, edge cases, polish

### Phase 4: Release Candidate
- **Duration**: 2 weeks (Feb 2026)
- **Goal**: Final validation before v1.0 release

---

## Rewards and Recognition

### Beta Tester Benefits

1. **Early Access**: First to try new features
2. **Credit**: Listed in CREDITS.md
3. **Influence**: Direct impact on product direction
4. **Support**: Priority technical support
5. **Exclusive**: Beta tester badge on community forums

### Top Contributors

Special recognition for testers who:
- Report the most valid bugs
- Provide highest quality feedback
- Contribute documentation improvements
- Help other beta testers

---

## Tips for Effective Testing

### Do's

✅ Test in different scenarios  
✅ Document steps to reproduce issues  
✅ Include screenshots and logs  
✅ Test both normal and edge cases  
✅ Provide constructive feedback  
✅ Be patient with known issues  
✅ Communicate clearly and respectfully

### Don'ts

❌ Don't test on production systems with important data  
❌ Don't assume bugs are known without checking  
❌ Don't provide vague feedback ("it doesn't work")  
❌ Don't test only what you like  
❌ Don't ignore documentation  
❌ Don't duplicate existing bug reports

---

## Frequently Asked Questions

### Q: Can I use Aurora OS as my primary OS?
**A**: Not yet. This is beta software and may have bugs. Test in a VM or on non-critical hardware.

### Q: Will my data be safe?
**A**: While we've tested extensively, this is beta software. Always backup important data.

### Q: How often should I update?
**A**: We recommend checking for updates weekly during beta.

### Q: Can I share my feedback publicly?
**A**: Yes! You can blog, tweet, or make videos about your experience.

### Q: What if I find a security vulnerability?
**A**: Please report security issues privately to security@aurora-os.org

### Q: Can I contribute code?
**A**: Yes! See CONTRIBUTING.md for guidelines.

---

## Getting Help

### Documentation

- **README.md**: Project overview
- **FEATURES.md**: Complete feature list
- **INSTALLATION_GUIDE.md**: Detailed installation
- **FAQ.md**: Frequently asked questions

### Support

- **GitHub Discussions**: Community help
- **Email**: beta@aurora-os.org
- **Discord**: [Coming soon]

---

## Thank You!

Your participation in the Aurora OS beta program is invaluable. Together, we're building the future of operating systems!

**Happy Testing!**

---

**Document Version**: 1.0  
**Last Updated**: November 20, 2025  
**Next Update**: Weekly during beta period

For questions about this guide, contact: beta@aurora-os.org
