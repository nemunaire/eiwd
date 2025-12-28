# Contributing to eiwd

Thank you for your interest in contributing to eiwd! This document provides guidelines and instructions for contributing.

## Code of Conduct

Be respectful, constructive, and professional in all interactions. We value:
- Clear communication
- Constructive criticism
- Collaborative problem-solving
- Quality over quantity

## Ways to Contribute

### Report Bugs

Before submitting a bug report:
1. Check existing issues to avoid duplicates
2. Verify you're using the latest version
3. Test with a clean configuration

Include in your report:
- **System Information**:
  - Distribution and version
  - Enlightenment version: `enlightenment -version`
  - EFL version: `pkg-config --modversion elementary`
  - iwd version: `iwd --version`
  - Kernel version: `uname -r`
  - Wireless chipset: `lspci | grep -i network`

- **Steps to Reproduce**: Detailed, numbered steps
- **Expected Behavior**: What should happen
- **Actual Behavior**: What actually happens
- **Logs**: Relevant excerpts from:
  - `~/.cache/enlightenment/enlightenment.log`
  - `sudo journalctl -u iwd --since "30 minutes ago"`

- **Screenshots**: If UI-related

### Suggest Features

Feature requests should include:
- Clear use case and motivation
- Expected behavior and UI mockups (if applicable)
- Potential implementation approach
- Why it benefits eiwd users

Note: Features must align with the core goal of lightweight, fast Wi-Fi management via iwd.

### Improve Documentation

Documentation contributions are highly valued:
- Fix typos or unclear sections
- Add missing information
- Improve examples
- Translate to other languages

Documentation files:
- `README.md` - Overview and quick start
- `INSTALL.md` - Detailed installation and troubleshooting
- `TESTING.md` - Testing procedures
- Code comments - Explain complex logic

### Submit Code Changes

Follow the development workflow below.

## Development Workflow

### 1. Set Up Development Environment

```bash
# Install dependencies (Arch Linux example)
sudo pacman -S base-devel meson ninja enlightenment efl iwd git

# Clone repository
git clone <repository-url> eiwd
cd eiwd

# Create development branch
git checkout -b feature/your-feature-name
```

### 2. Make Changes

Follow the coding standards below. Key principles:
- Keep changes focused and atomic
- Test thoroughly before committing
- Write clear commit messages
- Update documentation as needed

### 3. Build and Test

```bash
# Clean build
rm -rf build
meson setup build
ninja -C build

# Run manual tests (see TESTING.md)
# At minimum:
# - Module loads without errors
# - Can scan and connect to networks
# - No crashes or memory leaks

# Check for warnings
ninja -C build 2>&1 | grep -i warning
```

### 4. Commit Changes

```bash
# Stage changes
git add src/your-changed-file.c

# Commit with descriptive message
git commit -m "Add feature: brief description

Detailed explanation of what changed and why.
Mention any related issues (#123).

Tested on: [your system]"
```

### 5. Submit Pull Request

```bash
# Push to your fork
git push origin feature/your-feature-name
```

Then create a pull request via GitHub/GitLab with:
- Clear title summarizing the change
- Description explaining motivation and implementation
- Reference to related issues
- Test results and system information

## Coding Standards

### C Code Style

Follow Enlightenment/EFL conventions:

```c
/* Function naming: module_subsystem_action */
void iwd_network_connect(IWD_Network *net);

/* Struct naming: Module prefix + descriptive name */
typedef struct _IWD_Network
{
   const char *path;
   const char *name;
   Eina_Bool known;
} IWD_Network;

/* Indentation: 3 spaces (no tabs) */
void
function_name(int param)
{
   if (condition)
   {
      do_something();
   }
}

/* Braces: Always use braces, even for single-line blocks */
if (test)
{
   single_statement();
}

/* Line length: Aim for < 80 characters, max 100 */

/* Comments: Clear and concise */
/* Check if device is powered on */
if (dev->powered)
{
   /* Device is active, proceed with scan */
   iwd_device_scan(dev);
}
```

### File Organization

```
src/
├── e_mod_*.c           # Enlightenment module interface
├── iwd/                # iwd D-Bus backend
│   └── iwd_*.c         # Backend implementation
└── ui/                 # UI dialogs
    └── wifi_*.c        # Dialog implementations
```

Each file should:
- Include copyright/license header
- Include necessary headers (avoid unnecessary includes)
- Declare static functions before use (or use forward declarations)
- Group related functions together
- Use clear section comments

### Header Files

```c
#ifndef E_IWD_NETWORK_H
#define E_IWD_NETWORK_H

#include <Eina.h>
#include <Eldbus.h>

/* Public structures */
typedef struct _IWD_Network IWD_Network;

/* Public functions */
IWD_Network *iwd_network_new(const char *path);
void iwd_network_free(IWD_Network *net);
void iwd_network_connect(IWD_Network *net);

#endif
```

### Naming Conventions

- **Functions**: `module_subsystem_action()` - e.g., `iwd_device_scan()`
- **Structures**: `Module_Descriptive_Name` - e.g., `IWD_Network`
- **Variables**: `descriptive_name` (lowercase, underscores)
- **Constants**: `MODULE_CONSTANT_NAME` - e.g., `IWD_SERVICE`
- **Static functions**: `_local_function_name()` - prefix with underscore

### Memory Management

```c
/* Use EFL macros for allocation */
thing = E_NEW(Thing, 1);
things = E_NEW(Thing, count);
E_FREE(thing);

/* Use eina_stringshare for strings */
const char *str = eina_stringshare_add("text");
eina_stringshare_del(str);

/* For replaceable strings */
eina_stringshare_replace(&existing, "new value");

/* Always check allocations */
obj = E_NEW(Object, 1);
if (!obj)
{
   ERR("Failed to allocate Object");
   return NULL;
}
```

### Error Handling

```c
/* Use logging macros */
DBG("Debug info: %s", info);
INF("Informational message");
WRN("Warning: potential issue");
ERR("Error occurred: %s", error);

/* Check return values */
if (!iwd_dbus_init())
{
   ERR("Failed to initialize D-Bus");
   return EINA_FALSE;
}

/* Validate parameters */
void iwd_device_scan(IWD_Device *dev)
{
   if (!dev || !dev->station_proxy)
   {
      ERR("Invalid device for scan");
      return;
   }
   /* ... */
}
```

### D-Bus Operations

```c
/* Always use async calls */
eldbus_proxy_call(proxy, "MethodName",
                  callback_function,
                  callback_data,
                  -1,  /* Timeout (-1 = default) */
                  "");  /* Signature */

/* Handle errors in callbacks */
static void
_callback(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending)
{
   const char *err_name, *err_msg;

   if (eldbus_message_error_get(msg, &err_name, &err_msg))
   {
      ERR("D-Bus error: %s: %s", err_name, err_msg);
      return;
   }
   /* Process response */
}
```

### Security Considerations

**CRITICAL**: Never log sensitive data

```c
/* WRONG - Don't do this */
DBG("Connecting with password: %s", password);

/* CORRECT - Log actions without sensitive data */
DBG("Connecting to network: %s", ssid);

/* Clear sensitive data after use */
if (passphrase)
{
   memset(passphrase, 0, strlen(passphrase));
   free(passphrase);
}
```

## Testing Requirements

All code changes must:

1. **Compile without warnings**:
   ```bash
   ninja -C build 2>&1 | grep warning
   # Should return empty
   ```

2. **Pass manual tests** (see TESTING.md):
   - Module loads successfully
   - Core functionality works (scan, connect, disconnect)
   - No crashes during basic operations

3. **No memory leaks** (for significant changes):
   ```bash
   valgrind --leak-check=full enlightenment_start
   # Perform operations, check for leaks
   ```

4. **Update documentation** if:
   - Adding new features
   - Changing behavior
   - Modifying configuration
   - Adding dependencies

## Pull Request Checklist

Before submitting:

- [ ] Code follows style guidelines
- [ ] Compiles without warnings
- [ ] Tested on at least one distribution
- [ ] Documentation updated if needed
- [ ] Commit messages are clear and descriptive
- [ ] No debugging code left in (printfs, commented blocks)
- [ ] No unnecessary whitespace changes
- [ ] Sensitive data not logged

## Review Process

1. **Submission**: Create pull request with description
2. **Automated Checks**: CI runs (if configured)
3. **Code Review**: Maintainers review code
4. **Feedback**: Requested changes or approval
5. **Revision**: Address feedback and update PR
6. **Merge**: Approved changes merged to main

Expect:
- Initial response within 7 days
- Constructive feedback
- Potential requests for changes
- Testing on maintainers' systems

## Commit Message Guidelines

Format:
```
Component: Short summary (50 chars or less)

Detailed explanation of changes (wrap at 72 chars).
Explain WHAT changed and WHY, not just HOW.

If this fixes an issue:
Fixes #123

If this is related to an issue:
Related to #456

Tested on: Arch Linux, E 0.27.1, iwd 2.14
```

Examples:
```
iwd_network: Fix crash when connecting to hidden networks

The connect_hidden function didn't validate the device pointer,
causing a segfault when called before device initialization.

Added null check and error logging.

Fixes #42
Tested on: Gentoo, E 0.27.0
```

## Feature Development Guidelines

When adding features:

1. **Discuss first**: Open an issue to discuss the feature
2. **Keep it focused**: One feature per PR
3. **Follow architecture**: Maintain separation between D-Bus layer, module interface, and UI
4. **Match existing patterns**: Study similar existing code
5. **Think about edge cases**: Handle errors, missing devices, permission issues
6. **Consider performance**: Avoid blocking operations, minimize polling
7. **Document**: Add comments, update user documentation

## Getting Help

- **Questions**: Open a GitHub discussion or issue
- **Stuck**: Ask in the issue/PR, provide context
- **Enlightenment API**: See [E API docs](https://docs.enlightenment.org/)
- **EFL API**: See [EFL API reference](https://docs.enlightenment.org/api/efl/start)
- **iwd D-Bus**: See [iwd documentation](https://iwd.wiki.kernel.org/)

## License

By contributing, you agree that your contributions will be licensed under the same license as the project.

## Recognition

Contributors are recognized in:
- Git commit history
- `AUTHORS` file (if created)
- Release notes for significant contributions

Thank you for contributing to eiwd!
