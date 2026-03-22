## Name

mixappify - create a `.mixapp` wrapper for a foreign executable

## Synopsis

```sh
$ mixappify [-f] <INPUT> [OUTPUT]
```

## Description

`mixappify` creates a `.mixapp` file that points to `CompatibilityAssistant` and wraps the given executable.
This is useful for `.exe` files you want to treat like a native MixUpOS launcher entry.

If `OUTPUT` is omitted, the tool creates a `.mixapp` file next to the input file.

## See also

- [`mixapp`(5)](help://man/5/mixapp)
- [`MixAppRunner`(1)](help://man/1/Applications/MixAppRunner)
