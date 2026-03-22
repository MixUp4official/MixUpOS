## Name

mixapp - portable MixUpOS application file format (`.mixapp`)

## Description

A `.mixapp` file is a portable application launcher for MixUpOS.
It uses an INI-style format similar to [`af`(5)](help://man/5/af), but can live anywhere on disk and is meant to be opened directly.

The file supports the `[App]` group:

| Key            | Description |
| -------------- | ----------- |
| `Name`         | Display name |
| `Executable`   | Path to the program to run |
| `Arguments`    | Optional command-line arguments |
| `Argument1`, `Argument2`, ... | Optional explicit arguments, one per key |
| `WorkingDirectory` | Optional working directory |
| `RunInTerminal` | Optional terminal flag |
| `RequiresRoot` | Optional privilege escalation flag |

Relative paths are resolved against the directory containing the `.mixapp` file.

## Example

```ini
[App]
Name=My Tool
Executable=bin/my-tool
WorkingDirectory=.
Arguments=--demo
RunInTerminal=false
RequiresRoot=false
```

## See also

- [`af`(5)](help://man/5/af)
- [`MixAppRunner`(1)](help://man/1/Applications/MixAppRunner)
