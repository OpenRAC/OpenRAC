# Sourced by the build scripts: where the SN toolchain lives and how to run it.
#
# The toolchain is 32-bit Windows programs. On Windows (Git Bash) they run
# directly and the build uses SN's own make.exe. Anywhere else they run
# through Wine and the build uses the host's GNU make, in parallel;
# tools/docker/ provides a ready-made 32-bit Wine environment for that.
# Override either choice with WINE=... or MAKE_SN=... in the environment.
TC=toolchain/sn-prodg-3.01/usr/local/sce/ee/gcc/bin
case "$(uname -s)" in
  MINGW*|MSYS*|CYGWIN*)
    : "${WINE=}"
    : "${MAKE_SN=$TC/make.exe}" ;;
  *)
    : "${WINE=wine}"
    : "${MAKE_SN=make -j$(nproc 2>/dev/null || echo 4)}" ;;
esac

# Parallel make starts many Wine processes at once. With no wineserver running
# yet they race to start one and some fail ("wine: chdir to
# /tmp/wine-.../server-...: No such file or directory"), so start it up front
# and keep it alive for a minute between waves. It exits 2 when a server is
# already running, which is fine (and must not trip `set -e` in a caller).
if [ -n "$WINE" ] && command -v wineserver >/dev/null 2>&1; then
  wineserver -p60 2>/dev/null || true
fi

# sn PROGRAM.exe ARGS...   run one toolchain program
sn() { $WINE "$@"; }

# make_sn ARGS...          run Makefile.sn with the right make and Wine
make_sn() { $MAKE_SN -f Makefile.sn WINE="$WINE" "$@"; }
