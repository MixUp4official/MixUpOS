## Name

![Icon](/res/icons/16x16/app-browser.png) ExperimentalMix - MixUpOS web browser

[Open](launch:///bin/Browser)

## Synopsis

```**sh
$ Browser [options] [urls]
```

## Description

ExperimentalMix is the built-in web browser for MixUpOS. It is built on top of the system's open-source `LibWeb` and `LibJS` engines (Ladybird stack), allowing it to render HTML, CSS, and JavaScript.

## Options

-   `--help`: Display help message and exit
-   `--version`: Display version number and exit

## Arguments

-   `urls`: A list of urls to open, one per tab. If none are specified, then the homepage will be opened instead.

## Examples

```**sh
$ Browser
$ Browser --help
$ Browser https://serenityos.org/
$ Browser https://serenityos.org/ /res/html/misc/welcome.html github.com/serenityos/serenity
```
