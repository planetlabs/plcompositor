Pixel Lapse Compositor
======================

The Pixel Lapse Compositor is intended to build seamless and cloudless image 
mosaics from deep stacks of satellite imagery.

It's implemented in C++ and depends on the [GDAL
library](http://www.gdal.org/) and
[WJElement](https://github.com/netmail-open/wjelement).

The lead developer is Frank Warmerdam (warmerdam@pobox.com) of Planet Labs
(http://planet.com).

Building plcompositor
---------------------

If a new developer has the dependencies listed above installed on the
local system, building the project can be accomplished in very few
steps using [GNU
Autoconf](http://www.gnu.org/software/autoconf/autoconf.html). One
needs only to run the `autoreconf` script (which in turn runs
`aclocal`, `autoheader` and `autoconf` if necessary) to generate a
fully functioning `configure` script. One can then use the `configure`
script to point the build system to the directories containing
dependencies.

For example, if the developer had `WJElement` installed in his/her
home directory, the full build process can be accomplished with:

```bash
$ autoreconf
$ LDFLAGS=-L$HOME/lib/ CPPFLAGS=-I$HOME/include/ ./configure
$ make
```

In general, GDAL will be found by the configure script using the
`gdal-config` binary installed in the user's path. However, this can
also be explicitly specified using the configure script (see `$
./configure --help` for details).
