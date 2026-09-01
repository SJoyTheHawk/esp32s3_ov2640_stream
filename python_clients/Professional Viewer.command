#!/bin/zsh

SCRIPT_DIR="${0:A:h}"
VIEWER_DIR="$SCRIPT_DIR/professional_viewer"

cd "$VIEWER_DIR" || exit 1

if ! command -v python3 >/dev/null 2>&1; then
    print -u2 "Python 3 was not found."
    print -u2 "Install Python 3, then run: python3 -m pip install -r \"$VIEWER_DIR/requirements.txt\""
    STATUS=127
else
    python3 "$VIEWER_DIR/camera_viewer_pro.py"
    STATUS=$?
fi

if (( STATUS != 0 )); then
    print
    print -u2 "Professional Viewer stopped with error code $STATUS."
    print "Make sure Python 3 and the required packages are installed:"
    print "  python3 -m pip install -r \"$VIEWER_DIR/requirements.txt\""
    print
    print "Press Enter to close."
    read
fi

exit $STATUS
