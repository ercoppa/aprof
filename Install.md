### Install aprof ###

Currently, aprof does not ship with Valgrind, so you have to download it from our [download section](http://code.google.com/p/aprof/downloads/list).

Alternatively, if you want to try the latest version under development, you can download it from SVN:
```
svn checkout http://aprof.googlecode.com/svn/trunk/valgrind aprof
```

Uncompress the tgz package (if stable version) and compile it (**automake tools are needed**):
```
cd aprof-X.Y.Z
./build.sh
```
last stable version includes Valgrind 3.9.0 - svn rev. 13392.

aprof will be installed locally under the `aprof-X.Y.Z/inst` directory. See [Basic usage](BasicUsage.md).

**Please note** that the `README` and `INSTALL` files in the source directory are the ones provided by Valgrind developers and they can give you different instructions (goal: system installation).

If you are on an ARM/PPC architecture please read [this](AprofOnARM_PPC.md) first.

### Install aprof-plot ###

You can download the latest release of aprof-plot from the [download section](http://code.google.com/p/aprof/downloads/list).

Alternatively, if you want to try the latest version under development, you can download it from SVN:
```
svn checkout http://aprof.googlecode.com/svn/trunk/aprofplot aprofplot
```

JCommon, JFreeChart and RSyntaxTextArea are required by aprofplot; they are included in the aprofplot code repository (under `aprofplot/lib`).

In order to use the source code browsing function, you need to have installed on your system a recent version of [ctags](http://ctags.sourceforge.net/). On Mac OS X, you can download a recent binary (dmg) of ctags [here](http://code.google.com/p/rudix/downloads/detail?name=ctags-5.8-1.dmg&can=2&q=label%3ARudix-2011). Then you need to set in aprof-plot `/usr/local/bin/exuberant-ctags` as ctags binary (see below).

To make the jar file, run the following commands:

```
cd aprofplot
ant jar
```

#### Set ctags binary path ####

aprof-plot needs to know where ctags is installed on your system. By default, it will search ctags as `ctags-exuberant`. If your ctags has another name, you can set the right name (or the right path) in _aprof-plot > edit > settings > Tools > ctags binary_.