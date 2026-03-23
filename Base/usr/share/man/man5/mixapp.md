## Name

mixapp - portable MixUpOS application file format (`.mixapp`)

## Description

A `.mixapp` file is a portable application launcher for MixUpOS.
It uses an INI-style format similar to [`af`(5)](help://man/5/af), but can live anywhere on disk and is meant to be opened directly.

The file supports the `[App]` group:

| Key            | Description |
| -------------- | ----------- |
| `Name`         | Display name |
| `Type`         | Optional app type (`PythonScript`, `HtmlDocument`, etc.) |
| `Executable`   | Path to the program to run |
| `Source`       | Optional source or entry file for typed apps |
| `EntryPoint`   | Optional entry file inside extracted content (for zip web apps) |
| `Arguments`    | Optional command-line arguments |
| `Argument1`, `Argument2`, ... | Optional explicit arguments, one per key |
| `WorkingDirectory` | Optional working directory |
| `RunInTerminal` | Optional terminal flag |
| `RequiresRoot` | Optional privilege escalation flag |

Relative paths are resolved against the directory containing the `.mixapp` file.
If `Executable` is omitted, `MixAppRunner` can infer it for known `Type` values:

- `Type=PythonScript` -> `/bin/python3` (requires `Source`)
- `Type=HtmlDocument` -> `/bin/Browser` (requires `Source`)
- `Type=PythonZipPackage` -> `/bin/python3` (requires `Source`, runs zip app directly)
- `Type=HtmlZipPackage` -> `/bin/Browser` (requires `Source`, zip is extracted before launch)
- `Type=JavaJar` -> `/bin/java` (requires `Source`, starts with `-jar`)

When opening a `.mixapp`, MixAppRunner can show an installer prompt to copy the package into `/home/anon/Applications/MixApps` and create launcher `.af` entries (for example on the desktop).

## Example

```ini
[App]
Name=My Tool
Type=PythonScript
Executable=bin/my-tool
WorkingDirectory=.
Arguments=--demo
RunInTerminal=false
RequiresRoot=false
```

## See also

- [`af`(5)](help://man/5/af)
- [`MixAppRunner`(1)](help://man/1/Applications/MixAppRunner)
