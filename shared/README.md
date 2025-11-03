# Shared Directory

Contains shared modules, global variables, and the Rubolt SDK for cross-project functionality.

## Structure

- `modules/` — Shared Rubolt modules (.rbo) that can be imported across projects
- `globals/` — Global variable registries and configuration
- `sdk/` — Rubolt SDK with utilities, bindings, and development tools

## Usage

### Shared Modules

Place reusable `.rbo` modules in `shared/modules/`. Import them from any Rubolt project:

```rubolt
import shared.modules.common
import shared.modules.helpers
```

### Global Variables

Define global variables in `shared/globals/config.rbo`:

```rubolt
global APP_VERSION: str = "1.0.0"
global DEBUG_MODE: bool = true
```

### SDK

The SDK (`shared/sdk/`) provides:
- Native C extensions API
- FFI bindings
- Development utilities
- Project templates
- Standard library extensions
