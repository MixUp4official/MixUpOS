## Name

AppStore - Browse and launch MixUpOS ports installs

## Synopsis

```**sh
$ AppStore
$ AppStore --updates-only
```

## Description

App Store is a graphical front-end for the ports index used by MixUpOS and SerenityOS.
It can:

- browse available packages
- search and sort the package list
- filter to all packages, installed packages, or updates only
- show whether a package is already installed
- update all installable packages at once
- open the package website
- open the local `/usr/Ports/<package>` directory
- copy the exact install/update command
- start the existing `package.sh` install flow in Terminal

If `/usr/Ports/AvailablePorts.md` is not present, App Store can fetch a cached copy of the upstream ports index for browsing.

## Notes

App Store currently delegates installation to the existing ports system instead of performing package installation itself.
The install button requires the relevant `/usr/Ports/<package>` directory to exist in the running image.

## See also

- [`pkg`(1)](help://man/1/pkg)
- [`pls`(8)](help://man/8/pls)
